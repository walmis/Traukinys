/*
 * utils.hpp
 *
 *  Created on: Mar 30, 2013
 *      Author: walmis
 */

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <xpcc/architecture.hpp>
#include <xpcc/workflow.hpp>

//mirksiukas
template <typename Pin>
class Blinker : xpcc::TickerTask {

public:

	Blinker() {
		t.stop();
	}

	void blink(int time = 10) {
		Pin::set(1);
		t.restart(time);
	}

	void run() {
		if(t.isExpired()) {
			Pin::set(0);
		}
	}

	xpcc::Timeout<> t;
};


#endif /* UTILS_HPP_ */
