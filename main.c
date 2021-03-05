#define __AVR_ATmega324A__
#define F_CPU 8000000UL // 8MHz
#define ONE_MIN 60
#define THREE_MINS 180

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

    // todo handle freq = 0 case
    int length = 1000 / freq;

    // toggle led on for on_duration amount of time
    for (int i = 0; i < on_duration * freq; ++i) {
        PINB |= (1 << DDB3); // toggle led
        _delay_ms(length);
    }

    // turn LED off for off_duration amount of time
    PORTB = 0b00000000;
    delay(off_duration);
}


// Define the interrupt handler for UART Receive Complete - i.e. a new
// character has arrived in the UART Data Register (UDR).
ISR(USART0_RX_vect) {
        inputReceived = 1;
}

void print_random_freq(int freq) {

//    UDR0 = '\n';
//    char* report = "Random frequency selected: ";
//    for (int i = 0; report[i] != '\0'; ++i) {
//        UDR0 = report[i];
//        _delay_ms(5);
//    }
    UDR0 = '\r';
    UDR0 = '\n';

    UDR0 = (freq / 10) + '0';    // tens place
    UDR0 = (freq % 10) + '0';    // ones place
    UDR0 = '\n';
}

//
int plus_minus_5(int a, int b) {
    return (abs(a - b) < 5);
}

//
void run_cycle(Protocol input) {

    int freq;
    int prevFreq = 10;

    // assign freq value based on input parameter
    switch (input) {
        case OFF:
            delay(240 * 12); // off for 240 seconds, 12 times
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

    // run the 12 cycles
    for (int i = 0; i < 12; i++) {

        if (input == RANDOM) {
            // TODO do stuff?

            // if freq is 10, 40 or same as previous loop, randomise it
            while (plus_minus_5(freq, 10) || plus_minus_5(freq, 40) || plus_minus_5(freq, prevFreq)) {

                // mod80 to ensure 0 <= freq >= 80
                freq = rand() % 80;
            }

            prevFreq = freq;
            print_random_freq(freq);
        }
//        blink_led(freq, ONE_MIN, THREE_MINS);
        blink_led(freq, 10, 10);  //todo delete me (test only)
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
// interrupts, and make pins 2 and 3 on port b outputs
void init_hardware() {

    // TODO replace 10, feed random param into srand so it's not determinant
    srand(10);

    // set baud rate to 19200 (page 200 datasheet)
//    UBRR0 = 25;
    UBRR0 = 0; // 500,000

    // enable transmission and receiving via UART and enable the Receive
    // Complete Interrupt (page 194-195 datasheet)
    UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);

    // Enable global interrupts
    sei();

//    clear_screen();

    // set pins 2 and 3 on port B to be outputs
    DDRB = (1 << 3) | (1 << 2);
}

// main program
int main() {

    init_hardware();

    // loop until receiving interrupt
    while (1) {

        if (inputReceived) {

            // extract character from UART Data register
            char protocol = UDR0;

            // TODO signal output start bit to 2 photon system.
            run_cycle(protocol);
        }
    }
}
