/*
 * UsbRfDriver.hpp
 *
 *  Created on: Mar 25, 2013
 *      Author: walmis
 */

#ifndef USBRFDRIVER_HPP_
#define USBRFDRIVER_HPP_

typedef void (*FrameHandler)();

#include "UsbRfProtocol.hpp"
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

typedef int None;

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

	void txRx();
	std::thread* data_th;


	libusb_context* usb_ctx;

	uint8_t outEp;
	uint8_t inEp;
	uint16_t vendor;
	uint16_t product;

	libusb_device_handle* device;

	bool connected;


	template<FuncId id, typename Tret, typename Targ0 = unused>
	Tret remoteCall(Targ0 arg = {}) {
		funcCallPkt<id, Targ0> f(arg);
		int transferred = 0;

		int status = libusb_bulk_transfer(device, outEp, f.data(), f.size(), &transferred, 500);
		if(status != 0) {

			return (Tret)0;
		}

		if(!std::is_same<Tret, None>::value) {
			Tret ret;
			libusb_bulk_transfer(device, inEp, (uint8_t*)&ret, sizeof(ret), 0, 500);
			return ret;
		}
		return (Tret)0;
	}
};

#endif /* USBRFDRIVER_HPP_ */
