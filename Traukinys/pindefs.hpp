/*
 * pindefs.hpp
 *
 *  Created on: Mar 30, 2013
 *      Author: walmis
 */

#ifndef PINDEFS_HPP_
#define PINDEFS_HPP_

#include <xpcc/architecture.hpp>

template <typename Pin>
class OpenDrain
{
public:
	ALWAYS_INLINE static void
	setOutput()
	{
		set();
	}

	ALWAYS_INLINE static void
	setOutput(bool value)
	{
		set(value);
	}

	ALWAYS_INLINE static void
	setInput()
	{
	}

	ALWAYS_INLINE static void
	set()
	{
		Pin::setOutput(0);
	}

	ALWAYS_INLINE static void
	set(bool value)
	{
		if(value)
			set();
		else
			reset();
	}

	ALWAYS_INLINE static void
	reset()
	{
		Pin::setInput(xpcc::lpc::InputType::FLOATING);
	}

	ALWAYS_INLINE static void
	toggle()
	{
		if(Pin::read())
			reset();
		else
			set();
	}

	ALWAYS_INLINE static bool
	read()
	{
		return !Pin::read();
	}
};


GPIO__IO(progPin, 0, 1);


//motor controller definitions
GPIO__IO(_startStop, 0, 0);
GPIO__IO(_rotDirection, 1, 5);
GPIO__IO(_runBrake, 2, 0);

typedef OpenDrain<_startStop> startStop;
typedef OpenDrain<_rotDirection> rotDirection;
typedef OpenDrain<_runBrake> runBrake;

GPIO__INPUT(speed, 0, 2);
GPIO__INPUT(alarm, 0, 3);

GPIO__OUTPUT(_led, 0, 4);
typedef xpcc::gpio::Invert<_led> led;

///radio definitions
GPIO__OUTPUT(rfSel, 0, 7);
GPIO__OUTPUT(rfReset, 0, 11);
GPIO__INPUT(rfIrq, 0, 6);
GPIO__IO(rfSlpTr, 1, 10);


///general purpose IO header
GPIO__IO(gpio1, 0, 5);
GPIO__IO(gpio2, 1, 9);
GPIO__IO(gpio3, 3, 4);
GPIO__IO(gpio4, 3, 5);

inline void gpio_init() {
	progPin::setInput(xpcc::lpc::InputType::PULLUP);

	rfIrq::setInput();

	speed::setInput(xpcc::lpc::InputType::PULLUP);
	alarm::setInput(xpcc::lpc::InputType::PULLUP);

	LPC_IOCON->PIO0_4 |= 1<<8;
	LPC_IOCON->PIO0_5 |= 1<<8;

	startStop::setInput();

	led::setOutput(0);
}

#endif /* PINDEFS_HPP_ */
