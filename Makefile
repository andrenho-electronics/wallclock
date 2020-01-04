AVRDUDE_FLAGS=-p atmega328p -C ./avrdude_gpio.conf -c pi_1
	
test-connection:
	sudo avrdude ${AVRDUDE_FLAGS}

# vim: set ts=8 sts=8 sw=8 noexpandtab:
