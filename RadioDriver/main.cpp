/*
 * main.cpp
 *
 *  Created on: Mar 25, 2013
 *      Author: walmis
 */


#include <xpcc/architecture.hpp>
#include <xpcc/workflow.hpp>
#include <xpcc/communication/TinyRadioProtocol.hpp>

#include "UsbRfDriver.hpp"

UsbRfDriver driver;

TinyRadioProtocol<typeof(driver), AES_CCM_32> radio(driver);


int main() {
	driver.init();



	xpcc::log::debug .printf("Hello\n");

	while(1) {
		sleep(1);
	}

}



