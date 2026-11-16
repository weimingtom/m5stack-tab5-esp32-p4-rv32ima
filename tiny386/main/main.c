// "headless" tiny386
// for SDL port, see `sdl/main.c`
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "pc.h"

#if !defined(CLOCK_MONOTONIC) && defined(__MINGW32__)
#include <time.h>
#define CLOCK_MONOTONIC 1

struct timespec {
    long tv_sec;
    long tv_nsec;
};

extern int clock_gettime(int clk_id, struct timespec* ts);
/*clock_gettime see win32.c*/
#endif

// platform HAL implementation
#include <time.h>
uint32_t get_uticks()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((uint32_t) ts.tv_sec * 1000000 +
		(uint32_t) ts.tv_nsec / 1000);
}

#ifndef _WIN32
#if 0
#include <sys/mman.h>
void *bigmalloc(size_t size)
{
	return mmap(NULL, size, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}
#else
void *bigmalloc(size_t size)
{
	return malloc(size);
}
#endif
#else
void *bigmalloc(size_t size)
{
	return malloc(size);
}
#endif

int load_rom(void *phys_mem, const char *file, uword addr, int backward)
{
#if 0
	FILE *fp = fopen(file, "rb");
	if (fp == NULL) {
		fprintf(stderr, "load_rom: open %s failed: %s\n", file, strerror(errno));
		abort();
	}

	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);
	fprintf(stderr, "load_rom: %s, len %d\n", file, len);
	rewind(fp);
	if (backward)
		fread(phys_mem + addr - len, 1, len, fp);
	else
		fread(phys_mem + addr, 1, len, fp);
	fclose(fp);
	return len;
#else
	if (!(file && (0 == strcmp(file, "bzImage") || 0 == strcmp(file, "linuxstart.bin")))) {
		fprintf(stderr, "load_rom embedded: open %s failed\n", file);
		abort();
		return 0;
	}
	extern const unsigned char bzImage_start[] asm("_binary_bzImage_start");
    extern const unsigned char bzImage_end[]   asm("_binary_bzImage_end");
	extern const unsigned char linuxstart_bin_start[] asm("_binary_linuxstart_bin_start");
    extern const unsigned char linuxstart_bin_end[]   asm("_binary_linuxstart_bin_end");
	
	int len = 0;
	if (0 == strcmp(file, "bzImage")) {
		len = bzImage_end - bzImage_start;
		fprintf(stderr, "load_rom: %s, len %d\n", file, len);
		if (backward)
			memcpy(phys_mem + addr - len, bzImage_start, len);
		else
			memcpy(phys_mem + addr, bzImage_start, len);
	} else if (0 == strcmp(file, "linuxstart.bin")) {
		len = linuxstart_bin_end - linuxstart_bin_start;
		fprintf(stderr, "load_rom: %s, len %d\n", file, len);
		if (backward)
			memcpy(phys_mem + addr - len, linuxstart_bin_start, len);
		else
			memcpy(phys_mem + addr, linuxstart_bin_start, len);
	} else {
		fprintf(stderr, "load_rom embedded 2: open %s failed\n", file);
		abort();
	}
	return len;
#endif
}

//
static void redraw(void *opaque,
		   int x, int y, int w, int h)
{
//printf("redraw\n");
}

#include "term.h"
static void put_key(void *o, unsigned char scan_code, int is_pressed)
{
	ps2_put_keycode(o, is_pressed, scan_code);
}

static void usage(const char *argv0)
{
	fprintf(stderr,
		"Usage: %s [-kvm] [-term] inifile\n",
		argv0);
}

#ifndef _WIN32
/*
#include <signal.h>
volatile int g_running = 1;
static void sig_handler(int _)
{
	g_running = 0;
}
static void set_sig_handler()
{
	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
}
*/
#define g_running 1
#define set_sig_handler()
#else
#define g_running 1
#define set_sig_handler()
#endif

#if defined(__MINGW32__)
int g_enable_term = 0;
#endif

#if 1
int main__(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	PCConfig conf;
	memset(&conf, 0, sizeof(conf));
	conf.mem_size = 8 * 1024 * 1024;
	conf.vga_mem_size = 256 * 1024;
	conf.width = 720;
	conf.height = 480;
	conf.cpu_gen = 4;
	conf.fpu = 0;

#if defined(__MINGW32__)
	g_enable_term = 1; //for no -term switch, close all debug output
#endif

	const char *argv1;
	bool enable_kvm = false;
	bool use_term = false;
	if (argc > 1) {
		for (int i = 1; i < argc - 1; i++) {
			if (strcmp(argv[i], "-kvm") == 0) {
				enable_kvm = true;
			} else if (strcmp(argv[i], "-term") == 0) {
				use_term = true;
#if defined(__MINGW32__)
				g_enable_term = 1;
#endif
			} else {
				usage(argv[0]);
				return 1;
			}
		}
		argv1 = argv[argc - 1];
		ne2000_set_config_file(argv1);
	} else {
		usage(argv[0]);
		return 1;
	}

#if 0
	int err = ini_parse(argv1, parse_conf_ini, &conf);
#else
	//int ini_parse(const char* filename, ini_handler handler, void* user)
	//int ini_parse_string(const char* string, ini_handler handler, void* user)
	extern const unsigned char tiny386_ini_start[] asm("_binary_tiny386_ini_start");
	//extern const unsigned char tiny386_ini_end[]   asm("_binary_tiny386_ini_end");
	int err = ini_parse_string((const char *)tiny386_ini_start, parse_conf_ini, &conf);
#endif
	if (err) {
		fprintf(stderr, "error %d\n", err);
		return err;
	}
	if (enable_kvm)
		conf.cpu_gen = -1;

	set_sig_handler();
	
	void *fb = bigmalloc(conf.width * conf.height * 4);
	PC *pc = pc_new(redraw, NULL, fb, &conf);
	Term *term = NULL;
	if (use_term)
		term = term_init(pc->vga, put_key, pc->kbd);
	load_bios_and_reset(pc);

	pc->boot_start_time = get_uticks();
	for (; g_running && pc->shutdown_state != 8;) {
		pc_step(pc);
		pc_vga_step(pc);
		if (use_term)
			term_step(term);
	}
	return 0;
}
