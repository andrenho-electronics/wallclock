AVRDUDE_FLAGS=-p atmega328p -C ./avrdude_gpio.conf -c pi_1
MCU=atmega328p
CC=avr-gcc
CFLAGS=-Os -mmcu=${MCU}

upload: wallclock.hex
	sudo avrdude ${AVRDUDE_FLAGS} -U flash:w:$<:i

wallclock.hex: wallclock.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

wallclock.elf: src/main.o
	$(CC) -mmcu=${MCU} -o $@ $<

test-connection:
	sudo avrdude ${AVRDUDE_FLAGS}

clean:
	rm -f *.elf src/*.o

# vim: set ts=8 sts=8 sw=8 noexpandtab:
