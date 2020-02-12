# DEVICE = atmega103
DEVICE = attiny85
CLOCK  = 25000000
COMPILE = avr-gcc -I../../../include -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE) -Wl,--defsym=__stack=0x80efff 
all:
	$(COMPILE) main.c -o main.elf
	avr-objcopy -j .text -j .data -O binary main.elf main.bin
	avr-objdump -S main.elf > main.lst
	rm main.elf
	php ../../hexer.php main.bin > ../../avr.hex
	php ../../mifer.php main.bin > ../../flash.mif
	cd ../.. && make
	./../../../av2/avr main.bin

mnt:
	sudo mount sd.img -t vfat -o loop,rw,uid="`whoami`",sync,offset=$[1048576] disk/

clean:
	rm *.o rm *.elf *.lst
