#define F_CPU 8000000UL

#include <stdbool.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "ds1307.h"

// {{{ global variables

volatile uint8_t hour = 0,                         // current hour
                 minute = 0;                       // current minute

volatile uint8_t current_digit = 0;                // current digit being displayed (changes at 1000 Hz)
volatile uint8_t digit[4] = { 0, 0, 0, 0 };        // current number for each one of the 4 digits

typedef struct {
    bool update_from_clock : 1;
    bool check_buttons: 1;
} Events;
volatile Events events = { 0, 0 };

volatile unsigned long timer1_counter = 0;
volatile bool show_point = false;
volatile bool dim = false;

// }}}

// {{{ 7-seg digits

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

// }}}

// {{{ initial setup

static void iosetup()
{
    _delay_ms(500);

    // setup ports
    DDRB |= _BV(DDB1) | _BV(DDB2) | _BV(DDB3) | _BV(DDB4);  // PORTD - choose digit
    DDRD = 0xFF;  // PORTD - digit image
    PORTD = 0x00;
    DDRC &= ~_BV(DDC3);  // PORTC - inputs with pullup
    DDRC &= ~_BV(DDC2);
    DDRC &= ~_BV(DDC1);
    PORTC |= _BV(PC3) | _BV(PC2) | _BV(PC1);

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

    sei();
}

// }}}

// {{{ communication with DS1307

static void update_from_clock() 
{
    uint8_t n = 0;
    uint8_t hh = 0, mm = 0;
    ds1307_getdate(&n, &n, &n, &hh, &mm, &n);
    hour = hh;
    minute = mm;
}

static void set_digits()
{
    digit[0] = (hour / 10);
    digit[1] = (hour % 10);
    digit[2] = (minute / 10);
    digit[3] = (minute % 10);
}

// }}}

// {{{ update 7seg digit - runs at 1000 Hz

ISR(TIMER0_COMPA_vect) 
{
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
    if (dim) {
        _delay_us(2);
        PORTD = 0xFF;
    }

    // advance to next digit
    if (current_digit == 3)
        current_digit = 0;
    else
        ++current_digit;
}

// }}}

// {{{ update events - runs at 50 Hz

ISR(TIMER1_COMPA_vect) 
{
    if (timer1_counter % 10 == 0)
        events.update_from_clock = true;
    if (timer1_counter % 14 == 0)
        events.check_buttons = true;
    if (timer1_counter % 40 == 0)
        show_point = !show_point;
    ++timer1_counter;
}

// }}}

// {{{ inputs

void check_buttons()
{
    bool time_changed = false;

    // hour button
    if ((PINC & (1 << PC3)) == 0) {
        ++hour;
        if (hour > 23)
            hour = 0;
        time_changed = true;
    }

    // minute button
    if ((PINC & (1 << PC2)) == 0) {
        ++minute;
        if (minute > 59)
            minute = 0;
        time_changed = true;
    }

    dim = ((PINC & (1 << PC1)) == 0);

    if (time_changed) {
        ds1307_setdate(1, 1, 1, hour, minute, 0);
        update_from_clock();
        set_digits();
    }
}

// }}}

int main() 
{
    iosetup();
    while (1) {
        if (events.check_buttons) {
            check_buttons();
            events.check_buttons = false;
        }
        if (events.update_from_clock) {
            update_from_clock();
            set_digits();
            events.update_from_clock = false;
        }
    }
}

// vim: st=4:sts=4:sw=4:expandtab:foldmethod=marker
