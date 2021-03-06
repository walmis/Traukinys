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

	return (uint8_t*) (configDescriptor);
}

uint8_t * USBInterface::stringImanufacturerDesc() {
    static const uint8_t stringImanufacturerDescriptor[] = {
        14,                                            /*bLength*/
        STRING_DESCRIPTOR,                               /*bDescriptorType 0x03*/
        'w',0,'a',0,'l',0, 'm',0,'i',0,'s',0 /*bString iManufacturer - mbed.org*/
    };
    return (uint8_t*)stringImanufacturerDescriptor;
}

uint8_t * USBInterface::stringIproductDesc() {
    static const uint8_t stringIproductDescriptor[] = {
        22*2+2,                                                       /*bLength*/
        STRING_DESCRIPTOR,                                          /*bDescriptorType 0x03*/
	    'U', 0, 'S', 0, 'B', 0, ' ', 0, 'R', 0, 'F', 0, ' ', 0, '8', 0, '0', 0, '2',
			0, '.', 0, '1', 0, '5', 0, '.', 0, '4', 0, ' ', 0, 'M', 0, 'o', 0,
			'd', 0, 'u', 0, 'l', 0, 'e', 0, /*bString iProduct - USB DEVICE*/
    };
    return (uint8_t*)stringIproductDescriptor;
}

//xpcc::gpio::Invert<ledGreen> inv;
void USBInterface::handleTick() {

	if(suspended) {
		rxBlinker.blink(500);
	}

	if(cmd_pending) {
		funcCallPkt<>* pkt = (funcCallPkt<>*) (cmd_buf);
		//XPCC_LOG_DEBUG.printf("func %d\n", pkt->function_id);

		switch (pkt->function_id) {
		case SET_CHANNEL: {
			auto *p = (funcCallPkt<SET_CHANNEL, uint8_t>*) cmd_buf;
			XPCC_LOG_DEBUG.printf("set channel %d\n", p->arg0);

			rf230drvr.setChannel(p->arg0);
			break;
		}

		case INIT:
			//rf230drvr.init();
			//rf230drvr.setCLKM(rf230::CLKMClkCmd::CLKM_8MHz, false);
			break;

		case GET_CHANNEL:
			funcReturn<uint16_t>(rf230drvr.getChannel());

			break;
		case SET_SHORT_ADDRESS: {
			auto *p = (funcCallPkt<SET_SHORT_ADDRESS, uint16_t>*) cmd_buf;
			XPCC_LOG_DEBUG.printf("set short address %d\n", p->arg0);
			rf230drvr.setShortAddress(p->arg0);

			break;
		}
		case GET_SHORT_ADDRESS:
			funcReturn<uint16_t>(rf230drvr.getShortAddress());
			break;

		case SET_PAN_ID: {
			auto *p = (funcCallPkt<SET_PAN_ID, uint16_t>*) cmd_buf;
			XPCC_LOG_DEBUG.printf("set pan id %d\n", p->arg0);
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

		case SEND_FRAME: {
			NVIC_DisableIRQ(USB_IRQn);

			txBlinker.blink();

			funcReturn<RadioStatus>(rf230drvr.sendFrame(true));

			data_pos = 0;

			NVIC_EnableIRQ(USB_IRQn);

			break;
		}

		case UPLOAD_SEND_FRAME: {
			NVIC_DisableIRQ(USB_IRQn);

			Frame frm;
			frm.data = frame_data;
			frm.data_len = data_pos;

			txBlinker.blink();

			RadioStatus res = rf230drvr.sendFrame(frm, true);
			funcReturn<RadioStatus>(res);

			XPCC_LOG_DEBUG.printf("send len:%d res:%d\n", frm.data_len, res);

			data_pos = 0;

			NVIC_EnableIRQ(USB_IRQn);
			break;
		}
		case CLEAR_FRAME:
			data_pos = 0;
			break;

		}

		cmd_pending = false;
		readStart(EPINT_OUT, 64);
	}


	if(!rxFrames.isEmpty()) {
		const Frame &fr = rxFrames.get();

		rxBlinker.blink(50);

		RfFrameData usb_rx_frame;
		usb_rx_frame.lqi = fr.lqi;
		usb_rx_frame.rssi = fr.rssi;
		usb_rx_frame.data_len = fr.data_len;

		memcpy(usb_rx_frame.frame_data, fr.data, fr.data_len);

		uint8_t* data = (uint8_t*) &usb_rx_frame;
		uint8_t left = sizeof(usb_rx_frame.data_len) + sizeof(usb_rx_frame.lqi)
				+ sizeof(usb_rx_frame.rssi) + fr.data_len;

		XPCC_LOG_DEBUG.printf("write rx frame [%d]...", left);
		while (left > 0) {
			uint8_t len = (left > 64) ? 64 : left;
			write(EPBULK_IN, data, len, 64);
			left -= len;
			data += len;
		}

		XPCC_LOG_DEBUG.printf("OK\n");

		rxFrames.pop();

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
	HeapFrame frame;
	if(frame.allocate(rf230drvr.getFrameLength())) {
		rf230drvr.readFrame(frame);

		self->rxFrames.push(frame);
	}

}

bool USBInterface::EP1_IN_callback()
{
	//XPCC_LOG_DEBUG .printf("IN callback\n");
	return true;
}

bool USBInterface::EP1_OUT_callback() {
	//XPCC_LOG_DEBUG.printf("OUT1\n");
	uint32_t size;

	readEP(EPINT_OUT, cmd_buf, &size, 64);

	cmd_pending = true;

	return true;
}

bool USBInterface::EP2_OUT_callback() {
	//XPCC_LOG_DEBUG.printf("OUT2\n");
	if (data_pos < 128) {
		uint32_t read;
		readEP(EPBULK_OUT, frame_data + data_pos, &read, sizeof(frame_data) - data_pos);
		data_pos += read;
		XPCC_LOG_DEBUG.printf("Read %d %d\n", read, data_pos);
	}
	readStart(EPBULK_OUT, 64);
	return true;
}
