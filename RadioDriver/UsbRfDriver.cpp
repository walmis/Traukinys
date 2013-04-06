/*
 * UsbRfDriver.cpp
 *
 *  Created on: Mar 25, 2013
 *      Author: walmis
 */

#include "UsbRfDriver.hpp"
#include <assert.h>
#include <thread>

#include <unistd.h>

#include "UsbRfProtocol.hpp"


UsbRfDriver::UsbRfDriver() {
	connect_th = 0;
	int result;

	result = libusb_init(&usb_ctx);
	assert(result == 0);

}

UsbRfDriver::~UsbRfDriver() {
	libusb_exit(usb_ctx);
	if(connect_th)
		delete connect_th;
}

void UsbRfDriver::initUSB(uint16_t vendor, uint16_t product) {

	outBulk = 0x02;
	inBulk = 0x82;

	inInt = 0x81;
	outInt = 0x01;

	this->vendor = vendor;
	this->product = product;

	connect();

	connect_th = new std::thread(&UsbRfDriver::reconnect, this);
	data_th = new std::thread(&UsbRfDriver::txRx, this);

}

void UsbRfDriver::init() {

	remoteCall<INIT, None>();

}

void UsbRfDriver::connect() {
	device = libusb_open_device_with_vid_pid(usb_ctx, vendor, product);

	if (device) {
		XPCC_LOG_INFO.printf("USB: Connecting to USB device %04x:%04x\n",
				vendor, product);
		int ret;

		ret = libusb_claim_interface(device, 0);
		if (ret != 0) {
			XPCC_LOG_ERROR.printf("USB: Failed to claim interface 0\n%s\n",
					libusb_error_name(ret));

			return;
		}

		XPCC_LOG_INFO.printf("USB: Device successfully initialized\n");

		connected = true;

		init();

		//
	}
}

void UsbRfDriver::reconnect() {
	while(1) {

		if(!connected) {
			connect();
		}

		usleep(500000);
	}


}

RadioStatus UsbRfDriver::setChannel(uint8_t channel) {
	return remoteCall<SET_CHANNEL, RadioStatus, uint8_t>(channel);
}

uint8_t UsbRfDriver::getChannel() {
	return remoteCall<GET_CHANNEL, uint8_t>();
}

void UsbRfDriver::setShortAddress(uint16_t address) {
	remoteCall<SET_SHORT_ADDRESS, None, uint16_t>(address);
}

uint16_t UsbRfDriver::getShortAddress() {
	return remoteCall<GET_SHORT_ADDRESS, uint16_t>();
}


void UsbRfDriver::setPanId(uint16_t panId) {
	remoteCall<SET_PAN_ID, None, uint16_t>(panId);
}

uint16_t UsbRfDriver::getPanId() {
	return remoteCall<GET_PAN_ID, uint16_t>();
}

void UsbRfDriver::setRxFrameHandler(FrameHandler func) {
}

void UsbRfDriver::rxOn() {
	remoteCall<RX_ON, None>();
}

void UsbRfDriver::rxOff() {
	remoteCall<RX_OFF, None>();
}

uint8_t UsbRfDriver::getFrameLength() {
}


void UsbRfDriver::txRx() {
	uint8_t buffer[128];
	XPCC_LOG_INFO .printf("start read thread\n");
	while(connected && device != 0) {
		int res, read;

		res = libusb_bulk_transfer(device, inBulk, buffer, 128, &read, 500);
		if(res == 0) {
			XPCC_LOG_INFO .printf("read frame %d\n", read);
		}

	}
}


RadioStatus UsbRfDriver::sendFrame(bool blocking) {

	return remoteCall<SEND_FRAME, RadioStatus>();
}

RadioStatus UsbRfDriver::sendFrame(const Frame& frame, bool blocking) {
	int written;
	int result;
	result = libusb_bulk_transfer(device, outBulk, frame.data, frame.data_len, &written, 500);
	if(result != 0) {
		XPCC_LOG_DEBUG .printf("Frame Bulk write failed\n");
		return RadioStatus::TIMED_OUT;
	}

	return remoteCall<UPLOAD_SEND_FRAME, RadioStatus>();

}

bool UsbRfDriver::readFrame(Frame& frame) {
}

Stats* UsbRfDriver::getStats() {
}
