all: main.o
	avr-gcc -mmcu=atmega328p main.o -o main
	avr-objcopy -O ihex -R .eeprom main main.hex
	avr-objdump -S -z -d main > main.lst

flash:
	avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:main.hex

clean:
	rm -f *.o *.lst *.hex main

main.o: main.c
	avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c main.c -o main.o 

