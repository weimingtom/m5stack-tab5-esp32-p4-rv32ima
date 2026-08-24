// Copyright 2022 Charles Lohr, you may use this file or any portions herein under any of the BSD, MIT, or CC0 licenses.

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h>

#include "default64mbdtc.h"
#include "port.h"
//#include "cache.h"
#include "psram.h"

// Just default RAM amount is 64MB.
//uint32_t ram_amt = 64*1024*1024;
uint32_t ram_amt = 8 * 1024 * 1024;
int fail_on_all_faults = 0;

//static int64_t SimpleReadNumberInt( const char * number, int64_t defaultNumber );
uint64_t GetTimeMicroseconds();
//static void ResetKeyboardInput();
//static void CaptureKeyboardInput();
static uint32_t HandleException( uint32_t ir, uint32_t retval );
static uint32_t HandleControlStore( uint32_t addy, uint32_t val );
static uint32_t HandleControlLoad( uint32_t addy );
static void HandleOtherCSRWrite( uint8_t * image, uint16_t csrno, uint32_t value );
static int32_t HandleOtherCSRRead( uint8_t * image, uint16_t csrno );
static void MiniSleep();
int IsKBHit();
int ReadKBByte();

// This is the functionality we want to override in the emulator.
//  think of this as the way the emulator's processor is connected to the outside world.
#define MINIRV32WARN( x... ) printf( x );
#define MINIRV32_DECORATE  static
#define MINI_RV32_RAM_SIZE ram_amt
#define MINIRV32_IMPLEMENTATION
#define MINIRV32_POSTEXEC( pc, ir, retval ) { if( retval > 0 ) { if( fail_on_all_faults ) { printf( "FAULT\n" ); return 3; } else retval = HandleException( ir, retval ); } }
#define MINIRV32_HANDLE_MEM_STORE_CONTROL( addy, val ) if( HandleControlStore( addy, val ) ) return val;
#define MINIRV32_HANDLE_MEM_LOAD_CONTROL( addy, rval ) rval = HandleControlLoad( addy );
#define MINIRV32_OTHERCSR_WRITE( csrno, value ) HandleOtherCSRWrite( image, csrno, value );
#define MINIRV32_OTHERCSR_READ( csrno, value ) value = HandleOtherCSRRead( image, csrno );

#include "mini-rv32ima.h"

extern uint8_t *psram_base;
extern size_t psram_size;
//uint8_t * ram_image = 0;
struct MiniRV32IMAState * core;
const char * kernel_command_line = 0;

static void DumpState( struct MiniRV32IMAState * core, uint8_t * ram_image );



//--------------------------------------



void app_main(void)
{
//---------------------------
	kernel_command_line = 0;







//	int i;
	long long instct = -1;
//	int show_help = 0;
	int time_divisor = 1;
	int fixed_update = 0;
	int do_sleep = 1;
	int single_step = 0;
	int dtb_ptr = 0;
//	const char * image_file_name = 0;
	const char * dtb_file_name = 0;



//------------------------------

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("psram init\n");

	if (psram_init() < 0) {
		printf("failed to init psram\n");
		return;
	}

	printf("\nLoading kernel from flash...\n");

restart:

	if (load_images(ram_amt, NULL) < 0)
		return;
	
	//FIXME:added
	if (1)
	{
		// Load a default dtb.
		dtb_ptr = ram_amt - sizeof(default64mbdtb) - sizeof( struct MiniRV32IMAState );
		memcpy( psram_base + dtb_ptr, default64mbdtb, sizeof( default64mbdtb ) );
		if( kernel_command_line )
		{
			strncpy( (char*)( psram_base + dtb_ptr + 0xc0 ), kernel_command_line, 54 );
		}
	}

//----------------------

	//CaptureKeyboardInput();

	// The core lives at the end of RAM.
	core = (struct MiniRV32IMAState *)(psram_base + ram_amt - sizeof( struct MiniRV32IMAState ));
	core->pc = MINIRV32_RAM_IMAGE_OFFSET;
	core->regs[10] = 0x00; //hart ID
	core->regs[11] = dtb_ptr?(dtb_ptr+MINIRV32_RAM_IMAGE_OFFSET):0; //dtb_pa (Must be valid pointer) (Should be pointer to dtb)
	core->extraflags |= 3; // Machine-mode.

	if( dtb_file_name == 0 )
	{
		// Update system ram size in DTB (but if and only if we're using the default DTB)
		// Warning - this will need to be updated if the skeleton DTB is ever modified.
		uint32_t * dtb = (uint32_t*)(psram_base + dtb_ptr);
		if( dtb[0x13c/4] == 0x00c0ff03 )
		{
			uint32_t validram = dtb_ptr;
			dtb[0x13c/4] = (validram>>24) | ((( validram >> 16 ) & 0xff) << 8 ) | (((validram>>8) & 0xff ) << 16 ) | ( ( validram & 0xff) << 24 );
		}
	}
	
	printf("RV32IMA starting\n");
	printf("Initial PC: 0x%08" PRIx32 "\n", core->pc);

	// Image is loaded.
	uint64_t rt;
printf("0--PC: 0x%08" PRIx32 ", instct==%lld, sizeof(void*)==%d\n", core->pc, instct, sizeof(void*));
	uint64_t lastTime = (fixed_update)?0:(GetTimeMicroseconds()/time_divisor);
printf("0.5--PC: 0x%08" PRIx32 ", instct==%lld\n", core->pc, instct);
	int instrs_per_flip = single_step?1:1024;
printf("1--PC: 0x%08" PRIx32 ", instct==%lld\n", core->pc, instct);
	for( rt = 0; rt < instct+1 || instct < 0; rt += instrs_per_flip )
	{
//printf("2--PC: 0x%08" PRIx32 "\n", core->pc);
		uint64_t * this_ccount = ((uint64_t*)&core->cyclel);
		uint32_t elapsedUs = 0;
		if( fixed_update )
			elapsedUs = *this_ccount / time_divisor - lastTime;
		else
			elapsedUs = GetTimeMicroseconds()/time_divisor - lastTime;
		lastTime += elapsedUs;

		if( single_step )
			DumpState( core, psram_base);

		int ret = MiniRV32IMAStep( core, psram_base, 0, elapsedUs, instrs_per_flip ); // Execute upto 1024 cycles before breaking out.
		switch( ret )
		{
			case 0: break;
			case 1: if( do_sleep ) MiniSleep(); *this_ccount += instrs_per_flip; break;
			case 3: instct = 0; break;
			case 0x7777: goto restart;	//syscon code for restart
			case 0x5555: printf( "POWEROFF@0x%08x%08x\n", (unsigned int)core->cycleh, (unsigned int)core->cyclel ); return; //syscon code for power-off
			default: printf( "Unknown failure\n" ); break;
		}
	}

	DumpState( core, psram_base);
}


//////////////////////////////////////////////////////////////////////////
// Platform-specific functionality
//////////////////////////////////////////////////////////////////////////

#if defined(WINDOWS) || defined(WIN32) || defined(_WIN32)

#include <windows.h>
#include <conio.h>

static void MiniSleep()
{
	Sleep(1);
}
#else
static void MiniSleep()
{
	usleep(500);
}	
#endif

//////////////////////////////////////////////////////////////////////////
// Rest of functions functionality
//////////////////////////////////////////////////////////////////////////

static uint32_t HandleException( uint32_t ir, uint32_t code )
{
	// Weird opcode emitted by duktape on exit.
	if( code == 3 )
	{
		// Could handle other opcodes here.
	}
	return code;
}

static uint32_t HandleControlStore( uint32_t addy, uint32_t val )
{
	if( addy == 0x10000000 ) //UART 8250 / 16550 Data Buffer
	{
		printf( "%c", (int)val );
		fflush( stdout );
	}
	return 0;
}


static uint32_t HandleControlLoad( uint32_t addy )
{
	// Emulating a 8250 / 16550 UART
	if( addy == 0x10000005 )
		return 0x60 | IsKBHit();
	else if( addy == 0x10000000 && IsKBHit() )
		return ReadKBByte();
	return 0;
}

static void HandleOtherCSRWrite( uint8_t * image, uint16_t csrno, uint32_t value )
{
	if( csrno == 0x136 )
	{
		printf( "%d", (int)value ); fflush( stdout );
	}
	if( csrno == 0x137 )
	{
		printf( "%08x", (unsigned int)value ); fflush( stdout );
	}
	else if( csrno == 0x138 )
	{
		//Print "string"
		uint32_t ptrstart = value - MINIRV32_RAM_IMAGE_OFFSET;
		uint32_t ptrend = ptrstart;
		if( ptrstart >= ram_amt )
			printf( "DEBUG PASSED INVALID PTR (%08x)\n", (unsigned int)value );
		while( ptrend < ram_amt )
		{
			if( image[ptrend] == 0 ) break;
			ptrend++;
		}
		if( ptrend != ptrstart )
			fwrite( image + ptrstart, ptrend - ptrstart, 1, stdout );
	}
	else if( csrno == 0x139 )
	{
		putchar( value ); fflush( stdout );
	}
}

static int32_t HandleOtherCSRRead( uint8_t * image, uint16_t csrno )
{
	if( csrno == 0x140 )
	{
		if( !IsKBHit() ) return -1;
		return ReadKBByte();
	}
	return 0;
}

#if 0
static int64_t SimpleReadNumberInt( const char * number, int64_t defaultNumber )
{
	if( !number || !number[0] ) return defaultNumber;
	int radix = 10;
	if( number[0] == '0' )
	{
		char nc = number[1];
		number+=2;
		if( nc == 0 ) return 0;
		else if( nc == 'x' ) radix = 16;
		else if( nc == 'b' ) radix = 2;
		else { number--; radix = 8; }
	}
	char * endptr;
	uint64_t ret = strtoll( number, &endptr, radix );
	if( endptr == number )
	{
		return defaultNumber;
	}
	else
	{
		return ret;
	}
}
#endif

static void DumpState( struct MiniRV32IMAState * core, uint8_t * ram_image )
{
	uint32_t pc = core->pc;
	uint32_t pc_offset = pc - MINIRV32_RAM_IMAGE_OFFSET;
	uint32_t ir = 0;

	printf( "PC: %08x ", (unsigned int)pc );
	if( /*pc_offset >= 0 &&*/ pc_offset < ram_amt - 3 )
	{
		ir = *((uint32_t*)(&((uint8_t*)ram_image)[pc_offset]));
		printf( "[0x%08x] ", (unsigned int)ir ); 
	}
	else
		printf( "[xxxxxxxxxx] " ); 
	uint32_t * regs = core->regs;
	printf( "Z:%08x ra:%08x sp:%08x gp:%08x tp:%08x t0:%08x t1:%08x t2:%08x s0:%08x s1:%08x a0:%08x a1:%08x a2:%08x a3:%08x a4:%08x a5:%08x ",
		(unsigned int)regs[0], (unsigned int)regs[1], (unsigned int)regs[2], (unsigned int)regs[3], (unsigned int)regs[4], (unsigned int)regs[5], (unsigned int)regs[6], (unsigned int)regs[7],
		(unsigned int)regs[8], (unsigned int)regs[9], (unsigned int)regs[10], (unsigned int)regs[11], (unsigned int)regs[12], (unsigned int)regs[13], (unsigned int)regs[14], (unsigned int)regs[15] );
	printf( "a6:%08x a7:%08x s2:%08x s3:%08x s4:%08x s5:%08x s6:%08x s7:%08x s8:%08x s9:%08x s10:%08x s11:%08x t3:%08x t4:%08x t5:%08x t6:%08x\n",
		(unsigned int)regs[16], (unsigned int)regs[17], (unsigned int)regs[18], (unsigned int)regs[19], (unsigned int)regs[20], (unsigned int)regs[21], (unsigned int)regs[22], (unsigned int)regs[23],
		(unsigned int)regs[24], (unsigned int)regs[25], (unsigned int)regs[26], (unsigned int)regs[27], (unsigned int)regs[28], (unsigned int)regs[29], (unsigned int)regs[30], (unsigned int)regs[31] );
}
