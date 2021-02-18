// LED is connected to pin B3 (OC0A)

#define __AVR_ATmega32U4__
#include <avr/io.h>
#define F_CPU 8000000UL	// 8MHz
#include <util/delay.h>


void delay(int seconds) {
    // delay for 1 second
    for (int i = 0; i < seconds; i++) {
        _delay_ms(1000);
    }
}

int main() {

    // Make bit 3 of port B output
    DDRB = 0b00001000;

//     loop 4 times
    for (int i = 0; i < 4; ++i) { // TODO make no of loops configurable

        // TODO send signal to 2-photon system indicating start

        //turn on for 1 min
        PORTB = 0b00001000;
        delay(60); // TODO make this parameter configurable

        //turn off for 3 mins
        PORTB = 0b00000000;
        delay(180); // TODO make this parameter configurable
    }
}
