#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#if !defined(__MINGW32__)
#include <termios.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#define strtoll _strtoi64
#endif
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#if !defined(__MINGW32__)
#include <sys/ioctl.h>
#endif

extern struct MiniRV32IMAState core;
extern void DumpState(struct MiniRV32IMAState *core);
extern void app_main(void);
#if 0 //!defined(__MINGW32__)
extern char kernel_start[], kernel_end[];
#else
static char *kernel_start, *kernel_end;
#endif

#define USE_RAM_FILE 0
#if USE_RAM_FILE
static int ramfd;
#endif
static int is_eofd;

static void ResetKeyboardInput(void)
{
#if !defined(__MINGW32__)
	// Re-enable echo, etc. on keyboard.
	struct termios term;
	tcgetattr(0, &term);
	term.c_lflag |= ICANON | ECHO;
	tcsetattr(0, TCSANOW, &term);
#endif
}

static void CtrlC(int sig)
{
#if !defined(__MINGW32__)
	//DumpState(&core);
#endif
	ResetKeyboardInput();
	exit(0);
}

// Override keyboard, so we can capture all keyboard input for the VM.
static void CaptureKeyboardInput(void)
{
#if !defined(__MINGW32__)
	struct termios term;

	// Hook exit, because we want to re-enable keyboard.
	signal(SIGINT, CtrlC);

	tcgetattr(0, &term);
	term.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
	tcsetattr(0, TCSANOW, &term);
#else	
	system(""); // Poorly documented tick: Enable VT100 Windows mode.
#endif
}

uint64_t GetTimeMicroseconds()
{
#if !defined(__MINGW32__)
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_usec + ((uint64_t)(tv.tv_sec)) * 1000000LL;
#else
	static LARGE_INTEGER lpf;
	LARGE_INTEGER li;

	if( !lpf.QuadPart )
		QueryPerformanceFrequency( &lpf );

	QueryPerformanceCounter( &li );
	return ((uint64_t)li.QuadPart * 1000000LL) / (uint64_t)lpf.QuadPart;	
#endif
}

int ReadKBByte(void)
{
//FIXME:???
#if !defined(__MINGW32__)
	char rxchar;
	int rread;

	if (is_eofd)
		return 0xffffffff;

	rread = read(0, &rxchar, 1);

	if (rread > 0) // Tricky: getchar can't be used with arrow keys.
		return rxchar;
	else
		return -1;
#else
	// This code is kind of tricky, but used to convert windows arrow keys
	// to VT100 arrow keys.
	static int is_escape_sequence = 0;
	int r;
	if( is_escape_sequence == 1 )
	{
		is_escape_sequence++;
		return '[';
	}

	r = _getch();

	if( is_escape_sequence )
	{
		is_escape_sequence = 0;
		switch( r )
		{
			case 'H': return 'A'; // Up
			case 'P': return 'B'; // Down
			case 'K': return 'D'; // Left
			case 'M': return 'C'; // Right
			case 'G': return 'H'; // Home
			case 'O': return 'F'; // End
			default: return r; // Unknown code.
		}
	}
	else
	{
		switch( r )
		{
			case 13: return 10; //cr->lf
			case 224: is_escape_sequence = 1; return 27; // Escape arrow keys
			default: return r;
		}
	}
#endif
}

int IsKBHit(void)
{
#if !defined(__MINGW32__)
	int byteswaiting;

	if (is_eofd)
		return -1;

	ioctl(0, FIONREAD, &byteswaiting);
	// Is end-of-file for
	if (!byteswaiting && write(0, 0, 0 ) != 0) {
		is_eofd = 1;
		return -1;
	}
	return !!byteswaiting;
#else	
	return _kbhit();
#endif
}

extern uint32_t ram_amt;
/*static*/ uint8_t *psram_base = NULL;
#if 0
/*static*/ size_t psram_size = 0;
#endif

int psram_init(void)
{
#if USE_RAM_FILE
#if !defined(__MINGW32__)
	ramfd = open("/tmp/ram", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
#else
	ramfd = open("./ram", O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);	
#endif
	if (ramfd < 0) {
		perror("open\n");
		return -1;
	}
#else
	psram_base = (uint8_t *)calloc(1, ram_amt);
	if (psram_base == 0) {
		return -1;
	}
#endif
	return 0;
}

int psram_read(uint32_t addr, void *buf, int len)
{
#if USE_RAM_FILE
	lseek(ramfd, addr, SEEK_SET);
	read(ramfd, buf, len);
#else
	memcpy(buf, psram_base + addr, len);
#endif
}

int psram_write(uint32_t addr, void *buf, int len)
{
#if USE_RAM_FILE
	lseek(ramfd, addr, SEEK_SET);
	write(ramfd, buf, len);
#else
	memcpy(psram_base + addr, buf, len);
#endif
}

int load_images(int ram_size, int *kern_len)
{
	long flen;

#if 0 //!defined(__MINGW32__)
	flen = kernel_end - kernel_start;
#else
	{
		FILE *file;
		void *buffer;
		long fileSize;
		file = fopen("DownloadedImage", "rb");
		if (file == NULL) {
			perror("Error opening file 'DownloadedImage'");
			return -1;
		}
		fseek(file, 0, SEEK_END);
		fileSize = ftell(file);
		rewind(file);

		buffer = malloc(fileSize);
		if (buffer == NULL) {
			perror("Memory allocation failed");
			fclose(file);
			return -1;
		}
		size_t bytesRead = fread(buffer, 1, fileSize, file); // read file content
		if (bytesRead != fileSize) {
			perror("Failed to read the whole file");
			free(buffer);
			fclose(file);
			return -1;
		}
		//free(buffer);
		fclose(file);
		
		kernel_start = buffer;
		kernel_end = kernel_start + fileSize;
	}
	flen = kernel_end - kernel_start;
#endif
	if (flen > ram_size) {
		fprintf(stderr, "Error: Could not fit RAM image (%ld bytes) into %d\n", flen, ram_size);
		return -1;
	}
	if (kern_len)
		*kern_len = flen;

#if USE_RAM_FILE
	write(ramfd, kernel_start, flen);
#else
	psram_write(0, kernel_start, flen);
#endif

	return 0;
}

int main(int argc, char **argv)
{
	CaptureKeyboardInput();
	app_main();
}
