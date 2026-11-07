CC := gcc
LD := ld
RM := rm -rf

CFLAGS :=
CFLAGS += -O0 -g3
#CFLAGS += -std=c99
CFLAGS += -std=gnu99

OBJS := 
OBJS += adlib.o
OBJS += amd64.o
OBJS += fmopl.o
OBJS += fpu.o
OBJS += i386.o
OBJS += i8042.o
OBJS += i8254.o
OBJS += i8257.o
OBJS += i8259.o
OBJS += ide.o
OBJS += ini.o
OBJS += kvm.o
OBJS += main.o
OBJS += misc.o
OBJS += ne2000.o
OBJS += pc.o
OBJS += pci.o
OBJS += pcspk.o
OBJS += sb16.o
OBJS += term.o
OBJS += vga.o
OBJS += vgafont.o
OBJS += win32.o

all : tiny386_headless

tiny386_headless: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	
#test0: tiny386_headless
#	(cd test;../tiny386_headless -term ./tiny386.ini;)

#use this, make test1
#test_data/rootfs.img
test: tiny386_headless
	(cd test_data;../tiny386_headless ./tiny386.ini;)

test_data/rootfs.img:
	unzip test_data/rootfs.zip -d test_data

clean:
	$(RM) ./tiny386_headless *.exe $(OBJS) 
#test_data/rootfs.img
