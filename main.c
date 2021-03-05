//#define __AVR_ATmega324A__ // todo uncomment during development (only needed by CMake, not avr-gcc)
#define F_CPU 8000000UL // 8MHz
#define ONE_MIN 60
#define THREE_MINS 180
#define CYCLES 12

// required libraries
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

// protocol types
typedef enum Protocol {
    OFF = '0',
    TEN_HZ = '1',
    FORTY_HZ = '2',
    RANDOM = '3'
} Protocol;

// volatile to avoid getting optimised out by compiler
volatile int inputReceived = 0;


// Each loop causes a delay of 10ms. Looping seconds * 100 times essentially
// allows us to treat the input parameter (int seconds) as actual seconds.
void delay(int seconds) {

    for (int i = 0; i < seconds * 100; i++) {
        _delay_ms(10);
    }
}


// blink led at given frequency (freq) & set the on_duration and off_duration
void blink_led(int freq, int on_duration, int off_duration) {

    // TODO handle bug (case when freq = 0)
    int length = 1000 / freq;

    // toggle led on for on_duration amount of time
    for (int i = 0; i < on_duration * freq; ++i) {
        PINB |= (1 << DDB3); // toggle led
        _delay_ms(length);
    }

    // turn LED off for off_duration amount of time
    //todo may have to make this PINB = (1 << DDB3) to avoid interfering with 2p signal (test it)
    PORTB = 0b00000000;
    delay(off_duration);
}


// interrupt handler for UART Receive Complete - a character has arrived in
// the UART Data Register (UDR)
ISR(USART0_RX_vect) {

    // toggle on inputReceived variable
    inputReceived = 1;
}

// print the given frequency (freq) to terminal over serial port.
void print_random_freq(int freq) {

//    UDR0 = '\n';
//    char* report = "Random frequency selected: ";
//    for (int i = 0; report[i] != '\0'; ++i) {
//        UDR0 = report[i];
//        _delay_ms(5);
//    }
    UDR0 = '\r';
    UDR0 = '\n';

    UDR0 = (freq / 10) + '0';  // tens place
    UDR0 = (freq % 10) + '0';  // ones place
    UDR0 = '\n';
}

// return true if the absolute difference between a and b is less
// than 5, else return false.
int plus_minus_5(int a, int b) {
    return (abs(a - b) < 5);
}

// given the protocol, run the experiment cycles
void run_cycle(Protocol protocol) {

    int freq;
    int prevFreq = 10;

    // set freq value based on given protocol parameter
    switch (protocol) {
        case OFF:
            // off for 240 seconds, CYCLES number of times
            delay(240 * CYCLES);
            return;
        case RANDOM:
            // blank - drop thru to next case
        case TEN_HZ:
            freq = 10;
            break;
        case FORTY_HZ:
            freq = 40;
            break;
        default: return;
    }

    // run CYCLES number of cycles
    for (int i = 0; i < CYCLES; i++) {

        if (protocol == RANDOM) {

            // if freq is 10, 40 or same as previous loop, randomise it
            while (plus_minus_5(freq, 10) || plus_minus_5(freq, 40) || plus_minus_5(freq, prevFreq)) {

                // mod80 to ensure 0 <= freq >= 80
                freq = rand() % 80;
            }

            prevFreq = freq;
            print_random_freq(freq);
        }
//        blink_led(freq, ONE_MIN, THREE_MINS);
        blink_led(freq, 2, 2);  //todo delete me (for testing only)
    }
}

//void clear_screen() {
//
//    UDR0 = 27;
//    UDR0 = '[';
//    UDR0 = '2';
//    UDR0 = 'j';
//    UDR0 = '\n';
//}

// setup hardware including a random seed, baud rate for serial io, enable
// interrupts, and set data direction on pins
void init_hardware() {

    // TODO feed random param into srand so it's non-deterministic
    srand(93);

    // set baud rate to 500,000 (page 200 datasheet)
    UBRR0 = 0;

    // enable transmission and receiving via UART and enable the Receive
    // Complete Interrupt (page 194-195 datasheet)
    UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);

    // enable global interrupts
    sei();

//    clear_screen();

    // set pins 2 and 3 on port B to be outputs.
    // pin 3 for led, pin 2 for signal to 2p system.
    DDRB = (1 << 3) | (1 << 2);
}

// Send visual indicator that experiment cycles have ended
void end_cycle() {

    // TODO signal experiment end to 2 photon system

    // serial io indicator for end of experiment
    UDR0 = '\r';
    UDR0 = '\n';
    _delay_ms(2);
    UDR0 = '-';

    // toggle off inputReceived variable
    inputReceived = 0;
}

// program entry //TODO address compiler warning!
int main() {

    init_hardware();

    // loop until receiving interrupt
    while (1) {

        if (inputReceived) {

            // extract character from UART Data register
            char protocol = UDR0;

            // TODO signal experiment start to 2 photon system PINB = (1 << DDB2)
            run_cycle(protocol);
            end_cycle();
        }
    }
}
