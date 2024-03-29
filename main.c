#define __AVR_ATmega324A__ // todo uncomment during development (only needed by CMake, not avr-gcc)
#define F_CPU 8000000UL // 8MHz
#define ONE_MIN 60
#define THREE_MINS 180
#define CYCLES 12
#define SEED 9823
#define MAX_FREQ 80

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

    // toggle on
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
// than n, else return false.
int plus_minus_n(int a, int b, int n) {
    return (abs(a - b) < n);
}


// randomise the frequency based on the protocol rules. return the frequency (freq).
int randomise_freq(int freq, int prevFreq) {

    // if freq equals 10, 40 or prevFreq (plus or minus 5), then randomise it
    while (plus_minus_n(freq, 10, 5) ||
           plus_minus_n(freq, 40, 5) ||
           plus_minus_n(freq, prevFreq, 5)) {

        // randomise freq (mod80 to ensure that 0 <= freq <= 80)
        freq = rand() % MAX_FREQ;
    }

    return freq;
}


// given the protocol, run the experiment cycles
void run_experiment(Protocol protocol) {

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

            freq = randomise_freq(freq, prevFreq);
            prevFreq = freq;
            print_random_freq(freq);
        }
//        blink_led(freq, ONE_MIN, THREE_MINS);
        blink_led(freq, 2, 2);  // todo - for testing only
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

    // TODO feed non-constant param so it's non-deterministic
    srand(SEED);

    // set baud rate to 500,000 (page 200 datasheet)
    UBRR0 = 0;

    // enable transmission and receiving via UART and enable the Receive
    // Complete Interrupt (page 194-195 datasheet)
    UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);

    // enable global interrupts
    sei();

//    clear_screen();

    // set pins 2 and 3 on port B to be outputs.
    // pin 3 for led, pin 2 for signal to 2-photon system.
    DDRB = (1 << 3) | (1 << 2);
}


// Send visual indicator that experiment cycles have ended
void end_experiment() {

    // todo - signal experiment end to 2 photon system

    // serial io indicator for end of experiment
    UDR0 = '\r';
    UDR0 = '\n';
    _delay_ms(2);
    UDR0 = '-';

    // toggle off
    inputReceived = 0;
}


// signal experiment start to 2-photon system. High output on pin 2 port B
void signal_start() {
//    todo - implement this after we do the 2p sync training
//    PINB = (1 << DDB2);
}


// program entry
int main() {

    init_hardware();

    // loop until receiving interrupt
    while (1) {

        if (inputReceived) {

            // extract character from UART Data register
            char protocol = UDR0;
            signal_start();

            run_experiment(protocol);
            end_experiment();
        }
    }
}
