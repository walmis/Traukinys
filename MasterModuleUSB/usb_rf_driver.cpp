/*
 * usb_rf_driver.cpp
 *
 *  Created on: Apr 6, 2013
 *      Author: walmis
 */

#include "usb_rf_driver.hpp"

USBInterface* USBInterface::self;

uint8_t* USBInterface::configurationDesc() {
	static const uint8_t configDescriptor[] = {
	// configuration descriptor
			9,// bLength
			2, // bDescriptorType
			LSB(CONFIG1_DESC_SIZE), // wTotalLength
			MSB(CONFIG1_DESC_SIZE),
			1, // bNumInterfaces
			1, // bConfigurationValue
			0, // iConfiguration
			0x80, // bmAttributes
			50, // bMaxPower

			// interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
			9,// bLength
			4, // bDescriptorType
			0, // bInterfaceNumber
			0, // bAlternateSetting
			4, // bNumEndpoints
			0x00, // bInterfaceClass
			0x00, // bInterfaceSubClass
			0x00, // bInterfaceProtocol
			0, // iInterface

			// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
			ENDPOINT_DESCRIPTOR_LENGTH, // bLength
			ENDPOINT_DESCRIPTOR, // bDescriptorType
			PHY_TO_DESC(EPINT_IN), // bEndpointAddress
			E_INTERRUPT, // bmAttributes (0x02=bulk)
			LSB(MAX_PACKET_SIZE_EPINT), // wMaxPacketSize (LSB)
			MSB(MAX_PACKET_SIZE_EPINT), // wMaxPacketSize (MSB)
			1, // bInterval

			// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
			ENDPOINT_DESCRIPTOR_LENGTH, // bLength
			ENDPOINT_DESCRIPTOR, // bDescriptorType
			PHY_TO_DESC(EPINT_OUT), // bEndpointAddress
			E_INTERRUPT, // bmAttributes (0x02=bulk)
			LSB(MAX_PACKET_SIZE_EPINT), // wMaxPacketSize (LSB)
			MSB(MAX_PACKET_SIZE_EPINT), // wMaxPacketSize (MSB)
			1,

			// bInterval
			// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
			ENDPOINT_DESCRIPTOR_LENGTH,		// bLength
			ENDPOINT_DESCRIPTOR,		// bDescriptorType
			PHY_TO_DESC(EPBULK_IN),		// bEndpointAddress
			E_BULK,		// bmAttributes (0x02=bulk)
			LSB(MAX_PACKET_SIZE_EPBULK),		// wMaxPacketSize (LSB)
			MSB(MAX_PACKET_SIZE_EPBULK),		// wMaxPacketSize (MSB)
			0,		// bInterval

			// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
			ENDPOINT_DESCRIPTOR_LENGTH,		// bLength
			ENDPOINT_DESCRIPTOR,		// bDescriptorType
			PHY_TO_DESC(EPBULK_OUT),		// bEndpointAddress
			E_BULK,		// bmAttributes (0x02=bulk)
			LSB(MAX_PACKET_SIZE_EPBULK),		// wMaxPacketSize (LSB)
			MSB(MAX_PACKET_SIZE_EPBULK),		// wMaxPacketSize (MSB)
			0		// bInterval
			};
	// configuration descriptor
	// bLength
	// bDescriptorType
	// wTotalLength
	// bNumInterfaces
	// bConfigurationValue
	// iConfiguration
	// bmAttributes
	// bMaxPower
	// interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
	// bLength
	// bDescriptorType
	// bInterfaceNumber
	// bAlternateSetting
	// bNumEndpoints
	// bInterfaceClass
	// bInterfaceSubClass
	// bInterfaceProtocol
	// iInterface
	// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	// bLength
	// bDescriptorType
	// bEndpointAddress
	// bmAttributes (0x02=bulk)
	// wMaxPacketSize (LSB)
	// wMaxPacketSize (MSB)
	// bInterval
	// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	// bLength
	// bDescriptorType
	// bEndpointAddress
	// bmAttributes (0x02=bulk)
	// wMaxPacketSize (LSB)
	// wMaxPacketSize (MSB)
	// bInterval
	// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	// bLength
	// bDescriptorType
	// bEndpointAddress
	// bmAttributes (0x02=bulk)
	// wMaxPacketSize (LSB)
	// wMaxPacketSize (MSB)
	// bInterval
	// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	// bLength
	// bDescriptorType
	// bEndpointAddress
	// bmAttributes (0x02=bulk)
	// wMaxPacketSize (LSB)
	// wMaxPacketSize (MSB)
	// bInterval
	return (uint8_t*) (configDescriptor);
}

void USBInterface::handleTick() {

	if(!rxFrames.isEmpty()) {
		Frame *fr = rxFrames.get();
		rxFrames.pop();

		blinker.blink(50);

		static RfFrameData usb_rx_frame;

		usb_rx_frame.lqi = fr->lqi;
		usb_rx_frame.rssi = fr->rssi;
		usb_rx_frame.data_len = fr->data_len;

		memcpy(usb_rx_frame.frame_data, fr->data, fr->data_len);

		uint8_t* data = (uint8_t*) &usb_rx_frame;
		uint8_t left = sizeof(usb_rx_frame.data_len) + sizeof(usb_rx_frame.lqi)
				+ sizeof(usb_rx_frame.rssi) + fr->data_len;

		XPCC_LOG_DEBUG.printf("write rx frame [%d]...", left);
		uint8_t res;
		while (left > 0) {
			uint8_t len = (left > 64) ? 64 : left;
			res = write(EPBULK_IN, data, len, 64, 0);
			left -= len;
			data += len;
		}
		XPCC_LOG_DEBUG.printf("OK\n");

		delete fr;

	}

}

bool USBInterface::USBCallback_setConfiguration(uint8_t configuration) {
	if (configuration != 1) {
		return false;
	}

	addEndpoint(EPBULK_OUT, MAX_PACKET_SIZE_EPBULK);
	addEndpoint(EPBULK_IN, MAX_PACKET_SIZE_EPBULK);
	addEndpoint(EPINT_OUT, MAX_PACKET_SIZE_EPINT);
	addEndpoint(EPINT_IN, MAX_PACKET_SIZE_EPINT);

	readStart(EPINT_OUT, 64);
	readStart(EPBULK_OUT, 64);

	return true;
}

void USBInterface::rxFrameHandler() {
	//XPCC_LOG_DEBUG.printf("*%d\n", self->rxFrames.stored());
	if(!self->rxFrames.isFull()) {
		HeapFrame *frame = new HeapFrame;
		//XPCC_LOG_DEBUG .printf("fr %x\n", frame);
		if(frame && frame->allocate(rf230drvr.getFrameLength())) {
			rf230drvr.readFrame(*frame);

			self->rxFrames.push(frame);
		} else {
			//XPCC_LOG_DEBUG .printf("alloc failed\n");
			if(frame)
				delete frame;
		}
	}
}

bool USBInterface::EP1_IN_callback()
{
	//XPCC_LOG_DEBUG .printf("EP1 IN callback\n");
	return true;
}

bool USBInterface::EP1_OUT_callback() {
	uint8_t buf[64];
	uint32_t size;
	readEP(EPINT_OUT, buf, &size, 64);
	funcCallPkt<>* pkt = (funcCallPkt<>*) (buf);
	XPCC_LOG_DEBUG.printf("Call func id:%d\n", pkt->function_id);

	switch (pkt->function_id) {
	case SET_CHANNEL: {
		auto *p = (funcCallPkt<SET_CHANNEL, uint8_t>*) buf;
		XPCC_LOG_DEBUG.printf("set channel %d\n", p->arg0);

		rf230drvr.setChannel(p->arg0);
		break;
	}

	case INIT:
		rf230drvr.init();
		break;

	case GET_CHANNEL:
		funcReturn<uint16_t>(rf230drvr.getChannel());

		break;
	case SET_SHORT_ADDRESS: {
		auto *p = (funcCallPkt<SET_SHORT_ADDRESS, uint16_t>*) buf;
		rf230drvr.setShortAddress(p->arg0);

		break;
	}
	case GET_SHORT_ADDRESS:
		funcReturn<uint16_t>(rf230drvr.getShortAddress());
		break;

	case SET_PAN_ID: {
		auto *p = (funcCallPkt<SET_PAN_ID, uint16_t>*) buf;
		rf230drvr.setPanId(p->arg0);

		break;
	}
	case GET_PAN_ID:
		funcReturn<uint16_t>(rf230drvr.getPanId());
		break;

	case RX_ON:
		rf230drvr.rxOn();
		break;

	case RX_OFF:
		rf230drvr.rxOff();
		break;

	case SEND_FRAME:
		funcReturn<RadioStatus>(rf230drvr.sendFrame(true));

		data_pos = 0;
		break;

	case UPLOAD_SEND_FRAME: {
		Frame frm;
		frm.data = frame_data;
		frm.data_len = data_pos;

		XPCC_LOG_DEBUG.printf("send frame len:%d\n", frm.data_len);
		funcReturn<RadioStatus>(rf230drvr.sendFrame(frm, true));

		data_pos = 0;
		break;
	}
	case CLEAR_FRAME:
		data_pos = 0;
		break;

	}
	return true;
}

bool USBInterface::EP2_OUT_callback() {
	XPCC_LOG_DEBUG.printf("OUT2\n");
	if (data_pos < 128) {
		uint32_t read;
		readEP(EPBULK_OUT, frame_data + data_pos, &read, 128 - data_pos);
		data_pos += read;
		//XPCC_LOG_DEBUG.printf("EP2_OUT_callback(): Read packet len:%d\n", read);
	}
	return true;
}
