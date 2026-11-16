idf_component_register(SRCS "usb_serial_echo_main.c"
adlib.c
amd64.c
fmopl.c
fpu.c
i386.c
i8042.c
i8254.c
i8257.c
i8259.c
ide.c
ini.c
kvm.c
main.c
misc.c
ne2000.c
pc.c
pci.c
pcspk.c
sb16.c
term.c
vga.c
vgafont.c
win32.c
                       INCLUDE_DIRS ".")

idf_build_set_property(COMPILE_OPTIONS "-Wno-error=maybe-uninitialized" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=format=" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=empty-body" APPEND)


target_add_binary_data(${COMPONENT_TARGET} "${CMAKE_SOURCE_DIR}/main/test_data_gray386linux/tiny386.ini" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "${CMAKE_SOURCE_DIR}/main/test_data_gray386linux/linuxstart.bin" BINARY)
target_add_binary_data(${COMPONENT_TARGET} "${CMAKE_SOURCE_DIR}/main/test_data_gray386linux/bzImage" BINARY)

