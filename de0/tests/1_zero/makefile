all:
	avr-as -mmcu=avr5 avr.asm -o avr.o
	avr-ld  avr.o -o avr.elf
	avr-objcopy -j .text -j .data -O binary avr.elf avr.bin
	php ../../hexer.php avr.bin > ../../avr.hex
	php ../../mifer.php avr.bin > ../../flash.mif
	avr-objdump -S avr.elf > avr.lst
	rm avr.elf avr.o
	cd ../.. && make
	./../../../av/avr avr.bin

