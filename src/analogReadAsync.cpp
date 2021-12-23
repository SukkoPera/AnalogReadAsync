// Asynchronous Analog Read
// Copyright (C) 2021  Joshua Booth

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "analogReadAsync.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdint.h>

static volatile analogReadCompleteCallback_t analogReadCompleteCallback = nullptr;
static const void * volatile analogReadCompleteCallbackData = nullptr;

void analogReadAsync(uint8_t pin, analogReadCompleteCallback_t cb, const void *data)
{
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	if (pin >= 54) pin -= 54; // allow for channel or pin numbers
#elif defined(__AVR_ATmega32U4__)
	if (pin >= 18) pin -= 18; // allow for channel or pin numbers
#elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
	if (pin >= 24) pin -= 24; // allow for channel or pin numbers
#else
	if (pin >= 14) pin -= 14; // allow for channel or pin numbers
#endif

#if defined(ADCSRB) && defined(MUX5)
	// the MUX5 bit of ADCSRB selects whether we're reading from channels
	// 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
	ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
#endif
  
	// set the analog reference (high two bits of ADMUX) and select the
	// channel (low 4 bits).  this also sets ADLAR (left-adjust result)
	// to 0 (the default).
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
	ADMUX = (1 << REFS0) | (pin & 0x07);
#else
	ADMUX = (1 << REFS0) | (pin & 0x07);
#endif

  // Set the callback
  analogReadCompleteCallback = cb;
	analogReadCompleteCallbackData = data;

	// start the conversion
	ADCSRA |= (1 << ADSC);

	// enable the interrupt
	if (cb)
	{
		ADCSRA |= (1 << ADIE);
	}
}

ISR(ADC_vect)
{
  analogReadCompleteCallback_t cb = analogReadCompleteCallback;
	const void *data = analogReadCompleteCallbackData;

  // Disable interrupt and clear global callback and data before calling to allow for another call from the callback
	ADCSRA &= ~(1 << ADIE);
  analogReadCompleteCallback = nullptr;
	analogReadCompleteCallbackData = nullptr;
	
  if (cb)
  {
    cb(ADC, const_cast<void *>(data));
  }
}

bool analogReadComplete()
{
	return (ADCSRA & _BV(ADSC)) == 0;
}

uint16_t getAnalogReadValue()
{
	uint16_t tmp;
	
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		tmp = ADC;
	}

	return tmp;
}