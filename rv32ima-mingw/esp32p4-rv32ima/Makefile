all : mini-rv32ima

mini-rv32ima : uc-rv32ima.c port-posix.c cache.c
	gcc -o $@ $^ -g -O2 -Wall

uc.dtb : uc.dts
	dtc -I dts -O dtb -o $@ $^ -S 1536	
	
test : mini-rv32ima
	./mini-rv32ima

clean :
	rm -rf mini-rv32ima *.exe ram

