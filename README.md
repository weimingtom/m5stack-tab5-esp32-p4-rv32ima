# m5stack-tab5-esp32-p4-rv32ima
[Very WIP] My fork of rv32ima linux emulator on M5Stack Tab5 (M5Tab5) ESP32-P4, based on Epiczhul/esp32p4-rv32ima and cnlohr/mini-rv32ima

## About M5Stack Tab5
* https://docs.m5stack.com/zh_CN/core/Tab5  
* 主控制器 SoC	ESP32-P4NRW32@RISC-V 32 位双核 360MHz + LP 单核 40MHz
* 无线模块 SoC	ESP32-C6-MINI-1U
* Flash	16MB
* PSRAM	32MB Octal
* 5 英寸 IPS TFT 显示屏，分辨率 1280×720（720P）
* 采用 5 英寸 IPS TFT 显示屏，分辨率 1280×720（720P），搭载显示触控一体化驱动 IC：ST7123 / ST7121
* 在已接入 USB 数据线或电池供电的情况下，长按复位按键（约 2 秒），直至内部绿色 LED 指示灯开始快速闪烁，松开按键后，设备即进入下载模式，等待固件烧录。
* https://docs.m5stack.com/zh_CN/tab5/Tab5_Keyboard
* https://github.com/waveshareteam/ESP32-P4-Platform
* https://docs.waveshare.com/ESP32-P4

## TODO
* https://github.com/search?q=ReadKBByte+esp&type=code  
* 简化sdkconfig并写入sdkconfig.defaults
* m5stack tab5 (esp32-p4) command input successfully, but no echo (echo after pressing enter)    
* https://github.com/weimingtom/m5stack-tab5-esp32-p4-rv32ima/blob/master/rv32ima-mingw/esp32p4-rv32ima/todo.txt  
* https://github.com/weimingtom/m5stack-tab5-esp32-p4-rv32ima/blob/master/mini-rv32ima_mod/_TODO_add_burn_Image_with_flash_download_tool.txt  
* 记录如何用flash_download_tool写入Image/DownloadedImage, 如何烧录运行，包括两个工程
* wasm
* Android version, How to emulate a console ?  
* Refer to openrisc jor1k, how to display (write to /dev/fb0) ?  
https://s-macke.github.io/jor1k/demos/main.html
```
$ help
(choose Graphic Demos)  
(choose fbdemo)  
``` 
https://github.com/weimingtom/wmt_ai_study/blob/master/fpga_cpld_001.md  
```
转，《MIPS构架之：我和龙芯有个约会》
这篇文章解释为什么龙芯选择MIPS而不选择ARM，主要是因为MIPS允许改ISA架构指令。
不过我的理解ARM应该也可以，不过要给钱罢了。现在有了RISC-V，应该也是一个出路，
类似于很久以前的OpenRISC（不过OpenRISC据说是GPL的） ​​​

mips的另一个心累的地方是，你怎么瞎折腾mips它也跑不起来linux，
但openrisc却是有完整的一套方案可以跑linux，甚至有个很好看的web模拟器——
所以有时候openrisc和risc-v会更好一些。不过也有可能openrisc和risc-v和
mips都是同样的心累，只是表面上来看mips会更麻烦一些。当然对于喜欢折腾的人来说，
这样不更好吗

转，《Linux 6.1.14-rv32ima On Scratch》，这个的确看上去很酷炫，
用Scratch去模拟RV32IMA和RISC-V Linux，但不如OpenRISC的浏览器linux酷炫，
那个可以跑buildroot和命令行提示符（甚至可以图形输出），而这个虽然也是
buildroot但我看不到命令行（可能太卡了），我看过这个Scratch有10M左右，
解压成json有100多M

写指令级别模拟器有多难呢？我觉得我可以什么都不做写这个写一年——不过我实际没做到。
反正我现在还只是观望和用别人写好的模拟器代码跑，自己写写不出。我最想要的是
m68k（16位机）和mips（32位机）的指令模拟器，能跑linux和uclinux，但只是梦，
梦里什么都有。具体效果可以参考OpenRISC的网页版Linux 5，它就是长得我想要的模样

其实不要说risc-v，其他架构要跑baremetal和linux都非常艰难。
目前比较有希望的是mips, arm, risc-v和m68k，或者再加上x86和openrisc，
但这些随便一种要跑通baremetal和linux都很难，
也许需要一些时间去研究哪个切入点更容易一些 ​​​
```
* refer to uclinux fbcon, /dev/fb0  
https://github.com/weimingtom/wmt_uclinux_study  

## ESP-IDF version for **Win10 and Win11**
* ESP-IDF 5.4.4, for **Win10 and Win11**
* https://dl.espressif.cn/dl/esp-idf/
* https://dl.espressif.com/dl/idf-installer/esp-idf-tools-setup-offline-5.4.4.exe
* https://github.com/espressif/idf-installer/releases/download/offline-5.4.4/esp-idf-tools-setup-offline-5.4.4.exe
* https://github.com/espressif/esp-bsp/tree/master/examples
* https://github.com/m5stack/M5Tab5-UserDemo
* NOTE, not need ESP-IDF v5.4.2 (https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32s3/index.html), ESP-IDF 5.4.4 is also good  
```
我测试过可以运行在m5stack tab5上运行的esp-idf示例代码有：
espressif/esp-bsp的examples和m5stack/M5Tab5-UserDemo。
另外我发现不必装ESP-IDF v5.4.2，只需要装5.4.4，效果是一样的，
只是5.4.4的安装程序没有显示ESP32P4的勾选，但不用管，
默认安装就可以，是可以设置目标板为esp32p4的 ​​​
```
* You should install **ONLY ONE** ESP-IDF instance on your computer at the same time, otherwise when you uninstall one of the ESP-IDF instances, the other instances will be unavailable (the shortcut will be deleted). It is recommended to reinstall the other ESP-IDF instances if you encounter similar situations
```
你应该在电脑上同时安装不多于一个ESP-IDF实例，
否则当你卸载其中一个ESP-IDF实例，
其他实例就不可用（快捷方式被删除），
建议如果遇到类似的情况最好重新安装其他ESP-IDF实例
```

## References
* https://github.com/Epiczhul/esp32p4-rv32ima
* https://github.com/cnlohr/mini-rv32ima
* https://github.com/weimingtom/wmt_riscv_study
* https://github.com/weimingtom/mini-rv32ima_fork
* https://github.com/xhackerustc/uc-rv32ima
* https://github.com/cnlohr/mini-rv32ima-images
* https://github.com/RCSN/hpm_rv32ima
* https://www.cnblogs.com/jeason1997/p/19122455  
在单片机上运行Linux  
* https://github.com/tvlad1234/linux-ch32v003
* https://github.com/tvlad1234/tiny-rv32ima
* https://github.com/GrieferPig/rv32ima-emu
* ESP32 也能跑 Linux 了  
https://mp.weixin.qq.com/s/AGv0u_onAEG6P77Jw-k6qw  
* https://www.cnx-software.com/2026/08/22/espressif-systems-releases-a-linux-bsp-developer-preview-for-esp32-s31-risc-v-microprocessor/  
https://x.com/cnxsoft/status/2091022194814120368  
https://github.com/espressif/esp-linux-bsp  
https://esp32-s31.espressif.com/en  
https://gojimmypi.github.io/ESP32-S3-Linux/  
https://documentation.espressif.com/en/home  
* https://bokuweb.github.io/undefined/articles/20230523.html  
https://github.com/bokuweb/r2  
https://bokuweb.github.io/r2/  

## How to port to mingw
* https://github.com/cnlohr/mini-rv32ima/blob/master/mini-rv32ima/mini-rv32ima.c

## ram_amt, minimal memory, linux kernel memory footprint, >= 16 * 1024 * 1024 (for mini-rv32ima_mod)  
```
做Linux小电脑有门槛且很麻烦，目前最流行用f1c100s/f1c200s，
因为这样不需要外置的ddr 2 sdram（如mt7628），
为什么要人为制造焊接的麻烦呢？
大概是为了做成奇怪接口的核心板或邮票核心板。
另外linux好像有32M的最小内存足迹
（不可考，例如龙芯1c，uclinux则最低可能到8M，但还是需要sdram）。
如果换做我，我把linux 0.11移植到esp32算了（有psram可以代替sdram），
这样可以方便焊接，xv6也算半个linux，不过移植就更难了。
归根到底，就是大部分单片机都没有集成sdram，所以跑linux的门槛比较高

我以前说过，linux好像有32M的最小内存。但今天我试了一下mini-rv32ima的linux模拟器，
似乎可以修改ram_amt的最小内存数量为16M，内核仍然能正常启动，
如果改成8M内核就会内核crash——我猜测这个panic是没办法绕过的，
就是说对于这个内核预编译文件，它的内存footprint足迹是16M，
比32M还要小，但再小就不行了

其实我怀疑dtc里面指定的内存大小不一定要和实际内存大小一样，
就算dtc的内存配置值过大也可能不会导致内核panic——当然我还没实际测试过，
我希望是这样 ​​​​（补注：试了好像也不行，总之dtc设置的数值最好不要过小，
可能要根据内核版本不同而有所不同） ​​​

（想了两个月）我好像把esp32-p4 (M5Stack Tab5)运行Epiczhul/esp32p4-rv32ima和
cnlohr/mini-rv32ima
的rv32ima linux模拟器问题跑通了（虽然回显问题还没解决），
里面有不少问题（我觉得这俩工程都需要魔改才能运行），
其中最棘手的是内存大小的问题，如果模拟器c代码里面的内存大小
（不是dtc/dts设备树里面配置的内存大小）太小，
是有可能会内核panic的，而且无法绕过——只能你设置一个较大的数，
但允许比dtc的内存配置值小——我最开始就卡在这里，
我还以为必须和dtc的内存值一样，但这样会比m5stack tab5的psram大小更大，
导致无法运行，但其实可以小于dtc/dts内存配置的64M
（我用的是16M，实际psram是32MB）
```
