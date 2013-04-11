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

extern xpcc::rf230::Driver<xpcc::lpc::SpiMaster1, radioRst, radioSel, radioSlpTr, radioIrq> rf230drvr;

using namespace xpcc;

#define CONFIG1_DESC_SIZE (9+9+7+7+7+7)
class USBInterface : public lpc17::USBDevice {
public:
	USBInterface() : xpcc::lpc17::USBDevice(0xFFFF, 0x0708, 0) {
		data_pos= 0;
		self = this;

		rf230drvr.setRxFrameHandler(rxFrameHandler);
	}

	uint8_t* configurationDesc() override;

	void handleTick() override;

	bool USBCallback_setConfiguration(uint8_t configuration) override;

	static void rxFrameHandler();

	template <typename retType>
	void funcReturn(retType arg) {
		XPCC_LOG_DEBUG .printf("write %d\n", sizeof(retType));

		writeNB(EPINT_IN, (uint8_t*)&arg, sizeof(retType), MAX_PACKET_SIZE_EPINT);

	}

	//interrupt IN callback
	bool EP1_IN_callback() override;

	// interrupt OUT callback
	bool EP1_OUT_callback() override;

	//bulk OUT callback
	bool EP2_OUT_callback() override;

private:
	xpcc::Blinker<xpcc::gpio::Invert<redLed>> blinker;

	uint8_t buffer[64];
	uint8_t frame_data[128];

	StaticFrame rx_frame;

	uint8_t data_pos;

	static USBInterface* self;
};



#endif /* USB_RF_DRIVER_HPP_ */
