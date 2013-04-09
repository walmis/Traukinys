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

void handleFrame() {
	XPCC_LOG_DEBUG .printf("handle frame\n");

	StaticFrame frm;

	driver.readFrame(frm);

	XPCC_LOG_DEBUG .printf("Frame rssi:%d lqi:%d len:%d\n", frm.rssi, frm.lqi, frm.data_len);
	XPCC_LOG_DEBUG .dump_buffer(frm.data, frm.data_len);
}

int main() {
	driver.initUSB();

	driver.init(); //initialize radio
	driver.setRxFrameHandler(handleFrame);


	driver.setPanId(0x1234);
	driver.setShortAddress(23000);
	driver.rxOn();

	StaticFrame frm;
	MacFrame m(frm);
	m.build();
	m.setDstAddress(0xFFFF);
	m.setSrcAddress(0x5454, 0x1234);

	XPCC_LOG_DEBUG .dump_buffer(frm.data, frm.data_len);

	XPCC_LOG_DEBUG .printf("%d\n", driver.getShortAddress());

	XPCC_LOG_DEBUG .printf("Send result:%d\n", driver.sendFrame(frm));

	while(1) {
		sleep(1);
	}

}



