all : mini-rv32ima

mini-rv32ima : mini-rv32ima.c port-posix.c
	gcc -std=c99 -o $@ $^ -g3 -O0 -Wall

uc.dtb : uc.dts
	dtc -I dts -O dtb -o $@ $^ -S 1536	
	
test : mini-rv32ima
	./mini-rv32ima

debug:
	gdb32.exe ./mini-rv32ima
	
clean :
	rm -rf mini-rv32ima *.exe ram

default8mbdtc.h: uc.dtb
	xxd -i -c 16 uc.dtb > default8mbdtc.h
