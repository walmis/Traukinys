/*
 * motor_control.hpp
 *
 *  Created on: Apr 12, 2013
 *      Author: walmis
 */

#ifndef MOTOR_CONTROL_HPP_
#define MOTOR_CONTROL_HPP_

#include <xpcc/architecture.hpp>

using namespace xpcc;

class MotorControl : xpcc::TickerTask {
public:
	static const int pwm_top = 100;
	static const int counter_val = 30*10; //10 motor revolutions
	static const int timeout_val = 500; //200ms

	void init() {
		lpc11::Timer32B1::init(lpc11::TimerMode::TIMER_MODE, 2);

		lpc11::Timer32B1::initPWM(pwm_top, 0);
		lpc11::Timer32B1::initPWMChannel(3);

		lpc11::Timer32B1::activate(true);

		lpc11::Timer16B0::init(lpc11::TimerMode::COUNTER_FALLING_MODE, 1);

		lpc11::Timer16B0::enableCapturePins();
		lpc11::Timer16B0::configureMatch(0, counter_val,
				lpc11::ExtMatchOpt::EXTMATCH_NOTHING, true, false, true);

		NVIC_EnableIRQ(TIMER_16_0_IRQn);
		lpc11::Timer16B0::activate();

		timeout.restart(timeout_val);
	}

	void handleTick() override {
		if(timeout.isExpired()) {
			speed = (lpc11::Timer16B0::getCounterValue()*1000) / timeout_val;

			lpc11::Timer16B0::resetCounter();
			timeout.restart(timeout_val);
		}

		if(stopping && speed == 0) {
			stopped = true;
			stopping = false;
		}

		if(new_direction != direction && stopped) {
			if(new_direction) {
				rotDirection::set(true);
			} else {
				rotDirection::set(false);
			}
			direction = new_direction;
			run();
		}
	}

	void handleInterrupt(int irqN) override {
		if(irqN == TIMER_16_0_IRQn) {
			if(lpc11::Timer16B0::getIntStatus(lpc11::TmrIntType::MR0_INT)) {
				//XPCC_LOG_DEBUG .printf("timer int %x\n", LPC_TMR16B0->IR);

				uint32_t delta = (xpcc::Clock::now().getTime() - capture_time.getTime());
				capture_time = xpcc::Clock::now();
				speed = (counter_val*1000) / delta;
				timeout.restart(timeout_val);

				//XPCC_LOG_DEBUG .printf("speed1 %d\n", speed);
				lpc11::Timer16B0::clearIntPending(lpc11::TmrIntType::MR0_INT);
			}
		}
	}

	int32_t getSpeed() {
		return speed;
	}

	void setSpeed(int8_t speed) {
		if(speed == 0) {
			stop();
		} else {

			if(speed < 0)
				setDirection(true);
			else
				setDirection(false);
		}

		lpc11::Timer32B1::setPWM(3, (speed<0)?-speed:speed);
	}


	void run() {
		startStop::set(true);
		runBrake::set(true);
		stopped = false;
	}

	void stop() {
		stopping = true;
		startStop::set(false);
	}

	void brake() {
		runBrake::set(false);
		stopping = true;
	}

	void setDirection(bool reverse = false) {

		if(stopped)
			run();

		if(stopping && direction == reverse) {
			stopping = false;
			run();
		} else
		if(direction != reverse) {
			stop();
		}
		new_direction = reverse;
	}

private:
	xpcc::Timeout<> timeout;
	xpcc::Timestamp capture_time;
	uint32_t capture_tick;
	int32_t speed;

	uint8_t direction = 0;
	uint8_t new_direction = 0;
	uint8_t stopping = 0;
	bool stopped = true;

};


#endif /* MOTOR_CONTROL_HPP_ */
