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

void UsbRfDriver::init(uint16_t vendor, uint16_t product,
		uint8_t bulkInEp, uint8_t bulkOutEp) {

	outEp = bulkOutEp;
	inEp = bulkInEp;

	this->vendor = vendor;
	this->product = product;

	connect_th = new std::thread(&UsbRfDriver::connect, this);

}

void UsbRfDriver::connect() {
	while(1) {
		if(!connected) {
			libusb_device_handle* device  =
					libusb_open_device_with_vid_pid (usb_ctx, vendor, product);

			if(device) {
				XPCC_LOG_INFO .printf("USB: Connected to USB device\n");

				if(libusb_set_configuration(device, 0) != 0) {
					XPCC_LOG_ERROR .printf("USB: Failed to set configuration\n");
					continue;
				}

				if(libusb_claim_interface(device, 0) != 0) {
					XPCC_LOG_ERROR .printf("USB: Failed to claim interface 0\n");
					continue;
				}

				XPCC_LOG_INFO .printf("USB: Device successfully initialized\n");
				this->device = device;
				connected = true;

				//data_th = new std::thread(&UsbRfDriver::txRx, this);
			}
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
	//funcCallPkt<SET_SHORT_ADDRESS, uint16_t> f(address);
	remoteCall<SET_SHORT_ADDRESS, None, uint32_t>(address);
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
	funcCallPkt<RX_ON> f;
}

void UsbRfDriver::rxOff() {
	funcCallPkt<RX_OFF> f;
}

uint8_t UsbRfDriver::getFrameLength() {
}

void UsbRfDriver::txRx() {

	while(connected) {


	}
}



RadioStatus UsbRfDriver::sendFrame(bool blocking) {
}

RadioStatus UsbRfDriver::sendFrame(const Frame& frame, bool blocking) {
}

bool UsbRfDriver::readFrame(Frame& frame) {
}

Stats* UsbRfDriver::getStats() {
}
