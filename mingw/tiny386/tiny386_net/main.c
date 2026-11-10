// "headless" tiny386 - OpenWrt MT7621 MIPS 锚点基准 + TCP 1234远程键盘
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "pc.h"
#include "vga.h"
#include "i8042.h"

// 标准键盘注入接口前置声明
typedef struct PS2KbdState PS2KbdState;
void ps2_put_keycode(PS2KbdState *s, int is_down, int keycode);

// 获取微秒时间戳
uint32_t get_uticks()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((uint32_t) ts.tv_sec * 1000000 +
		(uint32_t) ts.tv_nsec / 1000);
}

// 平台内存分配，修复mmap参数顺序
#ifndef _WIN32
#include <sys/mman.h>
void *bigmalloc(size_t size)
{
	void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (p == MAP_FAILED) {
		fprintf(stderr, "bigmalloc mmap fail size=%zu: %s\n", size, strerror(errno));
		abort();
	}
	return p;
}
#else
void *bigmalloc(size_t size)
{
	void *p = malloc(size);
	if (!p) {
		fprintf(stderr, "bigmalloc malloc fail size=%zu\n", size);
		abort();
	}
	return p;
}
#endif

// 加载ROM镜像通用接口
int load_rom(void *phys_mem, const char *file, uword addr, int backward)
{
	FILE *fp = fopen(file, "rb");
	if (fp == NULL) {
		fprintf(stderr, "load_rom open fail %s: %s\n", file, strerror(errno));
		abort();
	}
	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	if (len <= 0) {
		fprintf(stderr, "load_rom empty file %s\n", file);
		fclose(fp);
		abort();
	}
	fprintf(stderr, "load_rom: %s, len %ld\n", file, len);
	rewind(fp);
	if (backward)
		fread(phys_mem + addr - len, 1, len, fp);
	else
		fread(phys_mem + addr, 1, len, fp);
	fclose(fp);
	return (int)len;
}

// VGA文本渲染全局常量
#define VGA_COLS         80
#define VGA_ROWS         25
#define TERM_BUF_SZ      (VGA_COLS * (VGA_ROWS + 2))
#define MIN_REFRESH_US   100000
#define VGA_TEXT_OFFSET   0x18000 // 0xB8000 - 0xA0000 标准文本显存偏移

static char vga_term_buf[TERM_BUF_SZ];
static char vga_term_last[TERM_BUF_SZ];
static uint32_t last_refresh_tick = 0;

// 安全读取VGA文本字符，使用官方vga_mem_read，不直接访问VGAState内部
static uint16_t vga_text_read_char(VGAState *vga, int row, int col)
{
	uint32_t offset = VGA_TEXT_OFFSET + (row * VGA_COLS + col) * 2;
	uint8_t ch  = vga_mem_read(vga, offset);
	uint8_t attr= vga_mem_read(vga, offset + 1);
	return ((uint16_t)attr << 8) | ch;
}

// 终端刷新回调，输出VGA画面到stdout
static void redraw_terminal(void *opaque, int x, int y, int w, int h)
{
	PC *pc = (PC*)opaque;
	uint32_t now = get_uticks();

	// 刷新限流，降低串口负载
	if ((now - last_refresh_tick) < MIN_REFRESH_US)
		return;

	memset(vga_term_buf, 0, TERM_BUF_SZ);
	int buf_off = 0;

	// 遍历标准80x25文本显存
	for (int r = 0; r < VGA_ROWS; r++) {
		for (int c = 0; c < VGA_COLS; c++) {
			uint16_t charattr = vga_text_read_char(pc->vga, r, c);
			uint8_t ch = charattr & 0xFF;
			// 只打印可打印ASCII，其余替换空格
			if (ch >= 0x20 && ch < 0x7F)
				vga_term_buf[buf_off++] = ch;
			else
				vga_term_buf[buf_off++] = ' ';
			if (buf_off >= TERM_BUF_SZ - 2)
				goto buf_end;
		}
		vga_term_buf[buf_off++] = '\n';
		if (buf_off >= TERM_BUF_SZ - 2)
			goto buf_end;
	}
buf_end:
	vga_term_buf[buf_off] = '\0';

	// 裁剪尾部多余空行
	while (buf_off > 0 && (vga_term_buf[buf_off-1] == '\n' || vga_term_buf[buf_off-1] == ' '))
		buf_off--;
	vga_term_buf[buf_off] = '\0';

	// 仅画面变化时输出，避免重复刷屏
	if (strcmp(vga_term_buf, vga_term_last) != 0) {
	
		fputs("\033[H\033[2J", stdout);
		fputs(vga_term_buf, stdout);
		fflush(stdout);
		strncpy(vga_term_last, vga_term_buf, TERM_BUF_SZ - 1);
		vga_term_last[TERM_BUF_SZ - 1] = '\0';
		last_refresh_tick = now;
	}
}

// ====================== 本地终端stdin键盘输入（修复阻塞 + Shift锁定） ======================
static uint8_t ansi_seq[8];
static int seq_len = 0;
static bool shift_active = false;    // Tab切换Shift锁定

typedef struct PS2KbdState PS2KbdState;
void ps2_put_keycode(PS2KbdState *s, int is_down, int keycode);

static void send_key(PS2KbdState *kbd, int sdl_key);
static void parse_ansi(PS2KbdState *kbd);
static int char_to_sdl(char c);
static void term_poll(PC *pc);
static int stdin_nonblock_setup(void);

static int stdin_nonblock_setup(void)
{
    int flags = fcntl(STDIN_FILENO, F_GETFL);
    if(flags == -1) return -1;
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    return 0;
}

static void send_key(PS2KbdState *kbd, int sdl_key)
{
    ps2_put_keycode(kbd, 1, sdl_key);
    ps2_put_keycode(kbd, 0, sdl_key);
}

static void parse_ansi(PS2KbdState *kbd)
{
    if (seq_len < 3)
    {
        seq_len = 0;
        memset(ansi_seq, 0, sizeof(ansi_seq));
        return;
    }
    uint8_t cmd = ansi_seq[2];
    switch(cmd)
    {
        case 'A': send_key(kbd, 0x48); break; /* UP */
        case 'B': send_key(kbd, 0x50); break; /* DOWN */
        case 'C': send_key(kbd, 0x4D); break; /* RIGHT */
        case 'D': send_key(kbd, 0x4B); break; /* LEFT */
    }
    seq_len = 0;
    memset(ansi_seq, 0, sizeof(ansi_seq));
}

static int char_to_sdl(char c)
{
    if (!shift_active)
    {
        switch(c)
        {
            case '0': return 11;
            case '1': return 2; case '2': return 3; case '3': return 4;
            case '4': return 5; case '5': return 6; case '6': return 7;
            case '7': return 8; case '8': return 9; case '9': return 10;

            case 'a': return 0x1E; case 'b': return 0x30; case 'c': return 0x2E;
            case 'd': return 0x20; case 'e': return 0x12; case 'f': return 0x21;
            case 'g': return 0x22; case 'h': return 0x23; case 'i': return 0x17;
            case 'j': return 0x24; case 'k': return 0x25; case 'l': return 0x26;
            case 'm': return 0x32; case 'n': return 0x31; case 'o': return 0x18;
            case 'p': return 0x19; case 'q': return 0x10; case 'r': return 0x13;
            case 's': return 0x1F; case 't': return 0x14; case 'u': return 0x16;
            case 'v': return 0x2F; case 'w': return 0x11; case 'x': return 0x2D;
            case 'y': return 0x15; case 'z': return 0x2C;

            case ' ':  return 0x39;
            case '\r': return 0x1C;
            case '\n': return 0x1C;
            case '\b': return 0x0E;

            case '.': return 0x34;
            case '/': return 0x35;
            case '\\':return 0x2B;
            case '-': return 0x0C;
            case '=': return 0x0D;
            case '[': return 0x1A;
            case ']': return 0x1B;
            case ';': return 0x27;
            case '\'':return 0x28;
            case ',': return 0x33;

            default: return 0;
        }
    }
    else
    {
        // Shift开启：大写字母 + 上档符号
        switch(c)
        {
            case '0': return 11;
            case '1': return 2; case '2': return 3; case '3': return 4;
            case '4': return 5; case '5': return 6; case '6': return 7;
            case '7': return 8; case '8': return 9; case '9': return 10;

            case 'a': return 0x1E; case 'b': return 0x30; case 'c': return 0x2E;
            case 'd': return 0x20; case 'e': return 0x12; case 'f': return 0x21;
            case 'g': return 0x22; case 'h': return 0x23; case 'i': return 0x17;
            case 'j': return 0x24; case 'k': return 0x25; case 'l': return 0x26;
            case 'm': return 0x32; case 'n': return 0x31; case 'o': return 0x18;
            case 'p': return 0x19; case 'q': return 0x10; case 'r': return 0x13;
            case 's': return 0x1F; case 't': return 0x14; case 'u': return 0x16;
            case 'v': return 0x2F; case 'w': return 0x11; case 'x': return 0x2D;
            case 'y': return 0x15; case 'z': return 0x2C;

            case '\r': return 0x1C;
            case '\n': return 0x1C;
            case '\b': return 0x0E;

            case '-': return 0x0C;
            case '=': return 0x0D;
            case '[': return 0x1A;
            case ']': return 0x1B;
            case ';': return 0x27;
            case '\'':return 0x28;
            case ',': return 0x33;
            case '.': return 0x34;
            case '/': return 0x35;

            default: return 0;
        }
    }
}

static void term_poll(PC *pc)
{
    PS2KbdState *kbd = pc->kbd;
    uint8_t ch;
    ssize_t ret;

    ret = read(STDIN_FILENO, &ch, 1);
    if(ret <= 0)
        return;
    
    if (ch == 0x1B)
    {
        ansi_seq[seq_len++] = ch;
        return;
    }
    if (seq_len > 0)
    {
        ansi_seq[seq_len++] = ch;
        if (seq_len >= 3)
        {
            parse_ansi(kbd);
        }
        return;
    }

    // Tab 切换 Shift锁定开关
    if (ch == '\t')
    {
        shift_active = !shift_active;
        return;
    }

    // 空格独立注入修复
    if (ch == ' ')
    {
        if (shift_active)
            ps2_put_keycode(kbd,1,0x2A);

        ps2_put_keycode(kbd,1,0x39);
        ps2_put_keycode(kbd,0,0x39);

        if (shift_active)
            ps2_put_keycode(kbd,0,0x2A);
        return;
    }

    int sdl_sc = char_to_sdl(ch);
    if (sdl_sc != 0)
    {
        if (shift_active)
            ps2_put_keycode(kbd, 1, 0x2A);

        send_key(kbd, sdl_sc);

        if (shift_active)
            ps2_put_keycode(kbd, 0, 0x2A);
    }
}
// ====================== 终端键盘模块结束 ======================

int main(int argc, char *argv[])
{
	PCConfig conf;
	memset(&conf, 0, sizeof(conf));
	conf.mem_size    = 4 * 1024 * 1024;
	conf.vga_mem_size= 256 * 1024;
	conf.width       = 720;
	conf.height      = 480;
	conf.cpu_gen     = 4;
	conf.fpu         = 0;
	conf.enable_serial=1;

	const char *ini_path = NULL;
	bool enable_kvm = false;

	// 命令行参数解析，MIPS屏蔽KVM
	if (argc == 2) {
		ini_path = argv[1];
	} else if (argc == 3) {
		if (strcmp(argv[1], "-kvm") == 0) {
#if defined(__mips__)
			fprintf(stderr, "WARN: MT7621 MIPS 不支持KVM，忽略参数\n");
#else
			enable_kvm = true;
#endif
			ini_path = argv[2];
		}
	} else {
		fprintf(stderr, "Usage: %s config.ini\n", argv[0]);
#if !defined(__mips__)
		fprintf(stderr, "x86专用: %s -kvm config.ini\n", argv[0]);
#endif
		return 1;
	}

	int parse_err = ini_parse(ini_path, parse_conf_ini, &conf);
	if (parse_err) {
		fprintf(stderr, "INI解析错误 code=%d\n", parse_err);
		return parse_err;
	}

#if !defined(__mips__)
	if (enable_kvm)
		conf.cpu_gen = -1;
#endif

	// headless不分配图形帧缓冲
	void *fb = NULL;
	PC *pc = pc_new(redraw_terminal, NULL, fb, &conf);

	load_bios_and_reset(pc);
	pc->boot_start_time = get_uticks();
    // 初始化stdin非阻塞键盘
    stdin_nonblock_setup();
	fprintf(stdout, "tiny386 headless | VGA stdout | stdin terminal keyboard\n");
	for (; pc->shutdown_state != 8;)
	{
		term_poll(pc);
		pc_step(pc);
		pc_vga_step(pc);
	}

	printf("\n虚拟机退出\n");
	return 0;


}

