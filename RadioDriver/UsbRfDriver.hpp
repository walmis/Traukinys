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

#include "Semaphore.hpp"
#include <libusb.h>

#include <time.h>
#include <exception>
#include <string>

struct Stats {
	uint32_t tx_bytes;
	uint32_t rx_bytes;
	uint32_t tx_frames;
	uint32_t rx_frames;
};

struct DeviceState {
	bool initialized;
	uint16_t address;
	uint16_t panId;
	bool rxOn;
	uint8_t channel;
};

typedef int None;

class USBException : public std::exception {
public:
	USBException(std::string msg, int code = 0) {
		this->msg = msg;
		if(code) {
			this->msg.append(": ");
			this->msg.append(libusb_error_name(code));
		}
		this->code = code;
	}
	virtual const char* what() const throw () {
		return msg.c_str();
	}
private:
	std::string msg;
	int code;
};

class UsbRfDriver {
public:
	UsbRfDriver();
	virtual ~UsbRfDriver();

	void initUSB(uint16_t vendor = 0xFFFF, uint16_t product = 0x0708);

	void init();

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

	RadioStatus checkTxStatus() {
		return RadioStatus::SUCCESS;
	}

	uint8_t getFrameLength();

	bool readFrame(Frame& frame);

	Stats* getStats();

private:
	void connect();
	void reconnect();

	boost::thread* connect_th;
	boost::thread* dispatcher_th;
	boost::thread* io_th;

	void txRx();

	void frameDispatcher();
	Semaphore wait_sem;

	FrameHandler rxHandler;

	libusb_context* usb_ctx;

	uint8_t outBulk;
	uint8_t inBulk;

	uint8_t outInt;
	uint8_t inInt;

	uint16_t vendor;
	uint16_t product;

	libusb_device_handle* device;

	bool connected;

	Stats stats;

	xpcc::atomic::Queue<std::shared_ptr<Frame>, 256> rx_frames;


	template<FuncId id, typename Tret, typename Targ0 = unused>
	Tret remoteCall(Targ0 arg = {}) {
		if(!device) {
			throw USBException("USB not connected");
		}
		funcCallPkt<id, Targ0> f(arg);
		int transferred = 0;

		int status = libusb_interrupt_transfer(device, outInt, f.data(), f.size(), &transferred, 500);
		if(status != 0) {
			throw USBException("USB: Transfer failed", status);
		}

		if(!std::is_same<Tret, None>::value) {
			Tret ret;
			int len;

			//XPCC_LOG_DEBUG .printf("read\n");
			status = libusb_interrupt_transfer(device, inInt, (uint8_t*)&ret, sizeof(Tret), &len, 500);
			if(status != 0) {
				throw USBException("USB: Failed to read function return value", status);
			}
			return ret;
		}
		return (Tret)0;
	}
};

#endif /* USBRFDRIVER_HPP_ */
