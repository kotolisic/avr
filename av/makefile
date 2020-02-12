CC=g++
SDL=`sdl-config --cflags --libs`
LIBS=-lSDL
WARN=-Wall -Wno-strict-overflow -Wno-unused-result -Wno-unused-variable -Wno-unused-but-set-variable
CFLAGS=-Icore -I. $(SDL) -O3 $(WARN)
OBJ=app.o disasm.o assign.o cpu.o spi.o avr.o

all: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o avr
	./avr

%.o: %.cc avr.h
	$(CC) $(CFLAGS) $(LIBS) $< -c -o $@

clean:
	rm *.o
