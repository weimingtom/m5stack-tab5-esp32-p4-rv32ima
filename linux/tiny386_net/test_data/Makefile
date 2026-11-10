#xubuntu 20.04 64bit

all:
	(cd ..;gcc -O0 -g3 *.c -o tiny386 -lm)

test:
	../tiny386 ./test.ini

debug:
	gdb --args ../tiny386 ./test.ini

clean:
	rm -rf ../tiny386

