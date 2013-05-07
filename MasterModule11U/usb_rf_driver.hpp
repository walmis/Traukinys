/*
 * usb_rf_driver.hpp
 *
 *  Created on: Apr 6, 2013
 *      Author: walmis
 */

#ifndef USB_RF_DRIVER_HPP_
#define USB_RF_DRIVER_HPP_

#include <xpcc/architecture.hpp>
#include <xpcc/driver/connectivity/wireless/at86rf230.hpp>
#include "UsbRfProtocol.hpp"
#include <pindefs.hpp>
#include <xpcc/driver/ui/led.hpp>
#include <xpcc/driver/connectivity/usb/USBDevice.hpp>

extern xpcc::rf230::Driver<xpcc::lpc::SpiMaster0, rstPin, selPin, slpTr, irqPin> rf230drvr;

using namespace xpcc;

#define CONFIG1_DESC_SIZE (9+9+7+7+7+7)
class USBInterface : public USBDevice {
public:
	USBInterface() : xpcc::USBDevice(0xFFFF, 0x0708, 0) {
		data_pos= 0;
		self = this;
		suspended = true;

		rf230drvr.setRxFrameHandler(rxFrameHandler);
	}

	uint8_t* configurationDesc() override;
	uint8_t* stringIproductDesc() override;
	uint8_t* stringImanufacturerDesc() override;

	bool suspended;

	void handleTick() override;

	bool USBCallback_setConfiguration(uint8_t configuration) override;

	static void rxFrameHandler();

	template <typename retType>
	void funcReturn(retType arg) {
		//XPCC_LOG_DEBUG .printf("write %d\n", sizeof(retType));
		writeNB(EPINT_IN, (uint8_t*)&arg, sizeof(retType), MAX_PACKET_SIZE_EPINT);
	}

	void suspendStateChanged(unsigned int suspended) override {
		XPCC_LOG_DEBUG .printf("suspendStateChanged %d %d\n", suspended, configured());
		ledGreen::set(!suspended);
		this->suspended = suspended;

		if(configured() && suspended) {
			rf230drvr.rxOff();
		}
	}

	void connectStateChanged(unsigned int connected) override {
		XPCC_LOG_DEBUG .printf("connectStateChanged %d\n", connected);
	}

	//interrupt IN callback
	bool EP1_IN_callback() override;

	// interrupt OUT callback
	bool EP1_OUT_callback() override;

	//bulk OUT callback
	bool EP2_OUT_callback() override;

private:
	xpcc::Blinker<ledRed> txBlinker;
	xpcc::Blinker<xpcc::gpio::Invert<ledGreen>> rxBlinker;

	volatile bool cmd_pending = false;
	uint8_t cmd_buf[64];
	uint8_t frame_data[128];

	xpcc::atomic::Queue<HeapFrame, 10> rxFrames;

	uint8_t data_pos;

	static USBInterface* self;
};



#endif /* USB_RF_DRIVER_HPP_ */
