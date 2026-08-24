all : mini-rv32ima

mini-rv32ima : uc-rv32ima.c port-posix.c cache.c
	gcc -o $@ $^ -g -O2 -Wall

# sudo apt install device-tree-compiler
uc.dtb : uc.dts
	dtc -I dts -O dtb -o $@ $^ -S 1536	

# $ make uc.dtb
# dtc -I dts -O dtb -o uc.dtb uc.dts -S 1536	
# uc.dts:63.12-68.5: Warning (simple_bus_reg): /soc/poweroff: missing or empty reg/ranges property
# uc.dts:70.10-75.5: Warning (simple_bus_reg): /soc/reboot: missing or empty reg/ranges property
# uc.dts:84.4-48: Warning (interrupts_extended_property): /soc/clint@11000000:interrupts-extended: cell 0 is not a phandle reference
# uc.dts:84.4-48: Warning (interrupts_extended_property): /soc/clint@11000000:interrupts-extended: cell 2 is not a phandle reference


test : mini-rv32ima
	./mini-rv32ima

clean :
	rm -rf mini-rv32ima *.exe ram

