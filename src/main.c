#define F_CPU 8000000UL

#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

uint8_t hour = 0,                   // current hour
        minute = 0;                 // current minute

typedef enum { SHOW_TIME } State;   // current clock state
uint8_t current_digit = 0;          // current digit being displayed (changes at 1000 Hz)

typedef struct {
    unsigned int update_clock : 1;
} Events;

// globals
State  state = SHOW_TIME;
Events events = { .update_clock = 0 };
int    timer1_counter = 0;

static void iosetup()
{
    // setup ports
    DDRB |= _BV(DDB1) | _BV(DDB2) | _BV(DDB3) | _BV(DDB4);  // PORTD - choose digit
    DDRD = 0xFF;  // PORTD - digit image
    PORTD = 0xF0;

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

    sei();
}

static void update_clock() {
}

static void set_digits() {
}

// update 7seg digit - runs at 1000 Hz
ISR(TIMER0_COMPA_vect) {
    // choose digit
    static uint8_t mask = 0b11100001;
    uint8_t pb = PORTB;
    pb &= mask;
    pb |= (~mask & (1 << (current_digit + 1)));

    // advance to next digit
    if (current_digit == 3)
        current_digit = 0;
    else
        ++current_digit;

    // draw digit
    PORTB = pb;
}

// update events - runs at 50 Hz
ISR(TIMER1_COMPA_vect) {
    if (timer1_counter % 10 == 0)
        events.update_clock = 1;
    // TODO - events
}

int main() {
    iosetup();
    while (1) {
        if (events.update_clock) {
            update_clock();
            set_digits();
        }
    }

    events = (Events) { 0 };
}

// vim: st=4:sts=4:sw=4:expandtab
