/*
 * UsbRfDriver.hpp
 *
 *  Created on: Mar 25, 2013
 *      Author: walmis
 */

#ifndef USBRFDRIVER_HPP_
#define USBRFDRIVER_HPP_

typedef void (*FrameHandler)();

#include <xpcc/driver/connectivity/wireless/mac802.15.4/mac.hpp>
#include <libusb.h>
#include <thread>
#include <time.h>

struct Stats {
	uint32_t tx_bytes;
	uint32_t rx_bytes;
	uint32_t tx_frames;
	uint32_t rx_frames;
};

class UsbRfDriver {
public:
	UsbRfDriver();
	virtual ~UsbRfDriver();

	void init(uint16_t vendor = 0xFFFF, uint16_t product = 0xCCFF,
			uint8_t bulkInEp = 0x85, uint8_t bulkOutEp = 0x05);

	RadioStatus setChannel(uint8_t channel);
	uint8_t getChannel();

	void setShortAddress(uint16_t address);
	uint16_t getShortAddress();

	void setPanId(uint16_t panId);

	uint16_t getPanId();

	void setRxFrameHandler(FrameHandler func);

	void rxOn();

	void rxOff();

	RadioStatus sendFrame(bool blocking = false);
	RadioStatus sendFrame(const Frame &frame, bool blocking = false);

	uint8_t getFrameLength();

	bool readFrame(Frame& frame);

	Stats* getStats();

private:
	void connect();
	std::thread* connect_th;

	void txrx();
	std::thread* data_th;


	libusb_context* usb_ctx;

	uint8_t outEp;
	uint8_t inEp;
	uint16_t vendor;
	uint16_t product;

	bool connected;


};

#endif /* USBRFDRIVER_HPP_ */
