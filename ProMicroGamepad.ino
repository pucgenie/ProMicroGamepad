// Joystick library and code skeleton by Matthew Heironimus (2015)
//--------------------------------------------------------------------

//-DJoystick_DISABLE_AUTOSEND
//-DUSBCore_DISABLE_LEDS

//#define TRACE_MEASURE
//#define TRACE_MEASURE_PRINT_ALL
//#define TRACE_MEASURE_SKIP_SMALL
#define SACRIFICE_CYCLES_FOR_RAM // saves about 12 bytes of RAM, adds 12 bytes of Program storage.
//#define DEBUG_MY_TIMER1
#define COMPLETELY_UNTOUCH_TIMER1
//#define MY_KEEP_SERIAL
//#define MY_DISABLE_POWERSAVE

//#define MY_DEBUG_USB


#include <Arduino.h>
//#include <USBCore.h>

#include "Joystick.h"
#include <avr/power.h>
#include <avr/sleep.h>

struct MappedPin {
  // pins 2 to 16: need 4 bits when using offset -2
  uint8_t pin
    #ifdef SACRIFICE_CYCLES_FOR_RAM
    :7
    #endif
  ;
  uint8_t state
    #ifdef SACRIFICE_CYCLES_FOR_RAM
    :1
    #endif
  ;
};

// TODO: Store first column in EEPROM?
static struct MappedPin MAPPED_PINS[] = {
  {2, 0},
  {3, 0},
  {4, 0},
  {5, 0},
  {6, 0},
  {7, 0},
  {8, 0},
  {9, 0},
  {10, 0},
  {16, 0},
  {14, 0},
  {15, 0},
};

#ifdef TRACE_MEASURE
  #include "trace_measure.h"
#endif

#ifndef COMPLETELY_UNTOUCH_TIMER1
  ISR(TIMER1_COMPA_vect) {
    // disable timer1 interrupts
    TIMSK1 &= ~(1 << OCIE1A);
    #ifdef DEBUG_MY_TIMER1
      static uint16_t debugLedState = 0;
      static bool debugLedNext = false;
      if (++debugLedState == 0) {
        digitalWrite(LED_BUILTIN_TX, debugLedNext ? HIGH : LOW);
        debugLedNext = !debugLedNext;
      }
    #endif
  }
#endif

static Joystick_ Joystick(sizeof(MAPPED_PINS));

void setup() {
	#if defined(MY_KEEP_SERIAL) || defined(MY_DEBUG_USB)
		SerialUSB.begin(9600);
		// optimization: Use USB init wait time for reading from Flash
		const __FlashStringHelper* const BOOT_MESSAGE = F("ProMicroGamepad booting...");
		#ifdef MY_DEBUG_USB
			uint_fast8_t waitLoopCount = 0;
		#endif
		while (!SerialUSB) {
			delay(1);
			#ifdef MY_DEBUG_USB
				++waitLoopCount;
			#endif
		}
		SerialUSB.print(BOOT_MESSAGE);
		#ifdef MY_DEBUG_USB
			SerialUSB.print(F("after waiting for "));
			SerialUSB.print(waitLoopCount);
			SerialUSB.print("ms ...");
		#endif
		SerialUSB.flush();
	#endif
	#ifdef DEBUG_MY_TIMER1
		pinMode(LED_BUILTIN_TX, OUTPUT);
	#endif
	#ifndef MY_DISABLE_POWERSAVE
		// try to reach <200 μA
		power_spi_disable();
		power_twi_disable();
		ADCSRA = 0;
		power_adc_disable();
		#ifdef MY_KEEP_SERIAL
			SerialUSB.print(F("power consumption optimized."));
			SerialUSB.flush();
		#endif
	#endif

	#ifndef COMPLETELY_UNTOUCH_TIMER1
		noInterrupts();
		// configure timer1 for waking up from standby
		TCCR1A = 0x00;
		TCCR1B = 0;

		// CTC
		TCCR1B |= (1 << WGM12);
		// Prescaler 64 (^= 64 µs per tick)
		TCCR1B |= (1 << CS11) | (1 << CS10);

		interrupts();
		#ifdef MY_KEEP_SERIAL
			SerialUSB.print(F("timer1 configured."));
			SerialUSB.flush();
		#endif
	#endif

	// Initialize Button Pins
	for (auto &button : MAPPED_PINS) {
		pinMode(button.pin, INPUT_PULLUP);
	}
	#ifdef MY_KEEP_SERIAL
		SerialUSB.print(F("pull-ups configured."));
		SerialUSB.flush();
	#endif

	if (!Joystick.begin(0x03, 0x05)) {
		#ifdef MY_DEBUG_USB
			SerialUSB.println(F("ERROR: Couldn't register HID device!"));
			SerialUSB.flush();
		#endif
	}
	// necessary to show up in device managers etc.
	doJoystickSend(25);
	#ifdef MY_KEEP_SERIAL
		SerialUSB.println(F("...finished booting."));
		SerialUSB.flush();
	#endif
}

void doJoystickSend(u8 timeout) {
	auto usbHidSendState = Joystick.sendState(timeout);
	#ifdef MY_DEBUG_USB
		if (usbHidSendState != 0) {
			if (usbHidSendState & 0b01000000) {
				usbHidSendState ^= 0b01000000;
				if (usbHidSendState == 0) {
					SerialUSB.print(F("Got no USB_SendSpace before timeout!"));
				} else {
					SerialUSB.print(F("Missed too many timeslots: "));
					SerialUSB.print(usbHidSendState ^ 0b01000000, 16);
				}
			} else {
				SerialUSB.print(F("Unknown error code or just some retries: "));
				SerialUSB.print(usbHidSendState, 16);
			}
			SerialUSB.println();
		}
		SerialUSB.flush();
	#endif
}

/**
 Polling and sending
**/
void loop() {

  bool hasChanges = false;
  #ifdef TRACE_MEASURE
    static unsigned long time1;
    time1 = micros();
  #endif
  // Read pin values
  for (uint8_t i = sizeof(MAPPED_PINS); i --> 0 ;) {
    auto &button = MAPPED_PINS[i];
    const auto currentButtonState = !digitalRead(button.pin);
    if (currentButtonState != button.state) {
      Joystick.setButton(i, currentButtonState);
      button.state = currentButtonState;
      hasChanges = true;
    }
  }
  // TODO: reflex measure, (v)sync to USB FS timing (1000 Hz max.)
  // by powersaving (wakeup timer) or sleeping (quick'n'dirty).
  // MEASUREMENT: maximum 92 µs for block A: reading inputs (12 buttons)
  #ifdef TRACE_MEASURE
    static unsigned long time2;
    time2 = micros() + (65536 - time1);
    // measured about 32 µs for 4 buttons in earlier version
  #endif
	if (hasChanges) {
		doJoystickSend(9);
		#ifndef COMPLETELY_UNTOUCH_TIMER1
			OCR1A = 11;
		} else {
			OCR1A = 13;
		#endif
	}
// TODO: sync to USB: sleepMicroseconds(1000 - time it takes to read_interpret - time it takes to send USB packet)

  // MEASUREMENT: maximum 204 µs (single outliers at 1216 µs, wtf?) for block B: sending USB data
  // minimum 84 µs, maybe USB send takes 124 µs (sync error?)
  #ifdef TRACE_MEASURE
    time1 = micros() + (65536 - time1);
    // measured up to 600 µs (or even 640 µs?) in earlier version
    addStatPoint(time2, time1);
  #endif

  //delayMicroseconds(hasChanges ? 350 : 800);
  #ifndef COMPLETELY_UNTOUCH_TIMER1
    // OCR1A: how many scaled ticks to count
    TCNT1 = 0; // reset timer
    // Output Compare Match A Interrupt Enable
    TIMSK1 |= (1 << OCIE1A);

    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_mode();
    sleep_disable();
  #endif
}
