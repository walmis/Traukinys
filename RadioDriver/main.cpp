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
	driver.initUSB();

	driver.init(); //initialize radio

	xpcc::log::debug .printf("Hello\n");

	driver.setPanId(0x1234);
	driver.setShortAddress(23000);
	driver.rxOn();

	StaticFrame frm;
	frm.data_len = 100;

	XPCC_LOG_DEBUG .printf("%d\n", driver.getShortAddress());

	//driver.sendFrame(frm);

	while(1) {
		sleep(1);
	}

}



