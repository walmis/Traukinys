/*
 * terminal.hpp
 *
 *  Created on: Apr 18, 2013
 *      Author: walmis
 */

#ifndef TERMINAL_HPP_
#define TERMINAL_HPP_

#include <xpcc/architecture.hpp>




class Terminal : xpcc::TickerTask {
	char buffer[32];
	uint8_t pos = 0;
	void handleTick() override {

		if(device.read(buffer[pos])) {
			if(buffer[pos] == '\n') {
				//remove the newline character
				buffer[pos] = 0;
				handleCommand();
				pos = 0;
				return;
			}
			pos++;
			pos &= 31;
		}
	}
	void handleCommand() {
		stdout.printf(": %s\n", buffer);
		if(strcmp(buffer, "start") == 0) {
			stdout.printf("starting motor\n");
			motorControl.run();
		} else
		if(strcmp(buffer, "reset") == 0) {
			stdout.printf("resetting\n");
			NVIC_SystemReset();
		} else
		if(strcmp(buffer, "stop") == 0) {
			motorControl.stop();
		} else
		if(strcmp(buffer, "brake") == 0) {
			motorControl.brake();
		} else
		if(strncmp(buffer, "speed", 5) == 0) {
			int speed = to_int(&buffer[6]);
			stdout.printf("new speed %d\n", speed);
			motorControl.setSpeed(speed);
		} else
		if(strcmp(buffer, "reverse") == 0) {
			motorControl.setDirection(true);
		} else
		if(strcmp(buffer, "forward") == 0) {
			motorControl.setDirection(false);
		}


	}
private:
	int to_int(char *p) {
		int k = 0;
		bool neg = false;
		if(*p == '-') {
			p++;
			neg = true;
		}

		while (*p) {
			k = (k << 3) + (k << 1) + (*p) - '0';
			p++;
		}
		if(neg) return -k;
		return k;
	}
};



#endif /* TERMINAL_HPP_ */
