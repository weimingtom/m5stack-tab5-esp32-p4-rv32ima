all : mini-rv32ima

mini-rv32ima : mini-rv32ima.c mini-rv32ima.h default64mbdtc.h
	gcc -std=c99 -o $@ $< -g -O2 -Wall

DownloadedImage :
	wget https://github.com/cnlohr/mini-rv32ima-images/raw/master/images/linux-6.1.14-rv32nommu-cnl-1.zip -O linux-6.1.14-rv32nommu-cnl-1.zip
	unzip linux-6.1.14-rv32nommu-cnl-1.zip
	mv Image DownloadedImage

sixtyfourmb.dtb : sixtyfourmb.dts
	dtc -I dts -O dtb -o $@ $^ -S 1536	
	
test : mini-rv32ima DownloadedImage
	./mini-rv32ima -f DownloadedImage -b sixtyfourmb.dtb

clean :
	rm -rf mini-rv32ima *.exe

