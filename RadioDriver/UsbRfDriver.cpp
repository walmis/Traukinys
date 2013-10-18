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
	int result;

	rxHandler = 0;
	connect_th = 0;
	connected = false;

	sem_init(&wait_sem, 0, 0);

	result = libusb_init(&usb_ctx);
	if(result != 0) {
		throw USBException("Failed to initialize libUSB", result);
	}
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
			throw USBException("USB: Failed to claim interface 0", ret);
		}
		connected = true;



		XPCC_LOG_INFO.printf("USB: Device successfully initialized\n");

		std::thread th(&UsbRfDriver::frameDispatcher, this);
		th.detach();

		data_th = new std::thread(&UsbRfDriver::txRx, this);

		init();
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
	rxHandler = func;
}

void UsbRfDriver::rxOn() {
	remoteCall<RX_ON, None>();
}

void UsbRfDriver::rxOff() {
	remoteCall<RX_OFF, None>();
}

uint8_t UsbRfDriver::getFrameLength() {
	if(rx_frames.isEmpty())
		return 0;

	return rx_frames.get()->data_len;
}


void UsbRfDriver::txRx() {
	RfFrameData data;

	XPCC_LOG_INFO .printf("start read thread\n");
	int res, read;

	do {
		//XPCC_LOG_DEBUG .printf("dump\n");
		res = libusb_bulk_transfer(device, inBulk, (uint8_t*)&data, sizeof(RfFrameData), &read, 10);
	} while(res == 0);

	while(connected && device) {
		res = libusb_bulk_transfer(device, inBulk, (uint8_t*)&data, sizeof(RfFrameData), &read, 0);
		if(res == 0) {
			//XPCC_LOG_INFO .printf("read frame %d\n", read);

			std::shared_ptr<HeapFrame> frm(new HeapFrame);

			if(frm->allocate(read)) {
				memcpy(frm->data, data.frame_data, data.data_len);

				frm->data_len = data.data_len;
				frm->rx_flag = true;
				frm->rssi = data.rssi;
				frm->lqi = data.lqi;

				//XPCC_LOG_DEBUG .printf("f %x\n", frm->data[2]);
				rx_frames.push(frm);
				sem_post(&wait_sem);
				//int value;
				//sem_getvalue(&wait_sem, &value);
				//XPCC_LOG_DEBUG .printf("sem_post value %d\n", value);
			}

		} else if(res == LIBUSB_ERROR_NO_DEVICE) {
			XPCC_LOG_INFO .printf("Disconnected\n");
			connected = false;
			libusb_close(device);
			device = 0;
		} else {
			XPCC_LOG_ERROR .printf("txRx: libusb_bulk_transfer: %s\n", libusb_error_name(res));
		}
	}
}

void UsbRfDriver::frameDispatcher() {
	while(connected) {
		//wait for new frames
		sem_wait(&wait_sem);
		//int value;
		//sem_getvalue(&wait_sem, &value);
		//XPCC_LOG_DEBUG .printf("frames pending %d (sem value %d)\n", rx_frames.stored(), value);

		if(rxHandler)
			rxHandler();

		rx_frames.pop();
	}
}

RadioStatus UsbRfDriver::sendFrame(bool blocking) {
	stats.tx_frames++;
	return remoteCall<SEND_FRAME, RadioStatus>();
}

RadioStatus UsbRfDriver::sendFrame(const Frame& frame, bool blocking) {
	int written;
	int result;
	if(!device) {
		throw USBException("USB not connected");
	}
	result = libusb_bulk_transfer(device, outBulk, frame.data, frame.data_len, &written, 500);
	if(result != 0) {
		XPCC_LOG_DEBUG .printf("Frame Bulk write failed\n");
		return RadioStatus::TIMED_OUT;
	}

	stats.tx_frames++;
	stats.tx_bytes += frame.data_len;

	return remoteCall<UPLOAD_SEND_FRAME, RadioStatus>();

}

bool UsbRfDriver::readFrame(Frame& frame) {

	if(rx_frames.isEmpty())
		return false;

	auto f = rx_frames.get();

	//XPCC_LOG_DEBUG .printf("Read frame (left %d)\n", rx_frames.stored());

	stats.rx_frames++;
	stats.rx_bytes += f->data_len;

	frame.rx_flag = true;
	memcpy(frame.data, f->data, f->data_len);
	frame.data_len = f->data_len;
	frame.lqi = f->lqi;
	frame.rssi = f->rssi;
	return true;
}

Stats* UsbRfDriver::getStats() {
	return &stats;
}
