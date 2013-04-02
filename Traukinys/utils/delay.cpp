/*
 * delay.c
 *
 *  Created on: Jan 31, 2013
 *      Author: walmis
 */
#include <xpcc/architecture.hpp>
#include <xpcc/workflow.hpp>

extern "C" {
	void _delay_us(uint32_t us) {
		int32_t ticks = (us*1000)/(1000000000UL/SystemCoreClock);
		if(ticks < 0) return;
		int32_t cur = SysTick->VAL;

		while((cur - SysTick->VAL) < ticks);
	}

	void _delay_ms(uint32_t time) {
		xpcc::Timeout<> t(time);

		while(!t.isExpired()) {};


	}
}
