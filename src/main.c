#define F_CPU 8000000UL

#include <stdbool.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "ds1307.h"

volatile uint8_t hour = 9,                         // current hour
                 minute = 34;                       // current minute

typedef enum { SHOW_TIME } State;         // current clock state
volatile State  state = SHOW_TIME;

volatile uint8_t current_digit = 0;                // current digit being displayed (changes at 1000 Hz)
volatile uint8_t digit[4] = { 0, 0, 0, 0 };        // current number for each one of the 4 digits

typedef struct {
    bool update_from_clock : 1;
} Events;
volatile Events events = { 0 };

volatile unsigned long timer1_counter = 0;
volatile bool show_point = false;

uint8_t images[] = {
    //CG.DEBFA
    0b10011111,  // 0
    0b10000100,  // 1
    0b01011101,  // 2
    0b11010101,  // 3
    0b11000110,  // 4
    0b11010011,  // 5
    0b11011011,  // 6
    0b10000101,  // 7
    0b11011111,  // 8
    0b11010111,  // 9

    0b10111111,  // 0.
    0b10100100,  // 1.
    0b01111101,  // 2.
    0b11110101,  // 3.
    0b11100110,  // 4.
    0b11110011,  // 5.
    0b11111011,  // 6.
    0b10100101,  // 7.
    0b11111111,  // 8.
    0b11110111,  // 9.

    0b00000000,  // empty (20)
};

static void iosetup()
{
    _delay_ms(500);

    // setup ports
    DDRB |= _BV(DDB1) | _BV(DDB2) | _BV(DDB3) | _BV(DDB4);  // PORTD - choose digit
    DDRD = 0xFF;  // PORTD - digit image
    PORTD = 0x00;

    cli();
    // setup timer 0 for 7-seg digit rotation (1000 Hz)
    TCCR0A = 0;
    TCCR0B = 0;
    TCNT0 = 0;
    OCR0A = 124; // 1000 Hz
    TCCR0B |= (1 << WGM01);  // turn on CTC mode
    TCCR0B |= (0 << CS02) | (1 << CS01) | (1 << CS00); // prescaler = 64
    TIMSK0 |= (1 << OCIE0A);  // enable timer compare interrupt

    // timer for events (50 Hz)
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;
    OCR1A = 20000;  // 50 Hz
    TCCR1B |= (1 << WGM12);  // turn on CTC mode 
    TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);  // prescaler = 8
    TIMSK1 |= (1 << OCIE1A);  // enable interrupt

    // setup i2c
    ds1307_init();
    ds1307_setdate(12, 12, 31, 23, 59, 35);

    sei();
}

static void update_from_clock() {
}

static void set_digits() {
        uint8_t year = 0;
        uint8_t month = 0;
        uint8_t day = 0;
        uint8_t hh = 0;
        uint8_t mm = 0;
        uint8_t second = 0;

        //check set date

				ds1307_getdate(&year, &month, &day, &hh, &mm, &second);
    hour = mm;
    minute = second;
    digit[0] = (hour / 10);
    digit[1] = (hour % 10);
    digit[2] = (minute / 10);
    digit[3] = (minute % 10);
}

// update 7seg digit - runs at 1000 Hz
ISR(TIMER0_COMPA_vect) {
    // choose digit
    static uint8_t mask = 0b11100001;
    uint8_t pb = PORTB;
    pb &= mask;
    pb |= (~mask & (1 << (current_digit + 1)));

    // prepare digit image
    uint8_t tdigit = digit[current_digit];
    if (current_digit == 1 && show_point)
        tdigit += 10;
    if (current_digit == 0 && tdigit == 0)
        tdigit = 20;

    // draw digit
    PORTB = pb;
    PORTD = ~images[tdigit];

    // advance to next digit
    if (current_digit == 3)
        current_digit = 0;
    else
        ++current_digit;
}

// update events - runs at 50 Hz
ISR(TIMER1_COMPA_vect) {
    if (timer1_counter % 10 == 0)
        events.update_from_clock = true;
    if (timer1_counter % 40 == 0)
        show_point = !show_point;
    ++timer1_counter;
}

int main() {
    iosetup();
    while (1) {
        if (events.update_from_clock) {
            update_from_clock();
            set_digits();
            events.update_from_clock = false;
        }
    }
}

// vim: st=4:sts=4:sw=4:expandtab
