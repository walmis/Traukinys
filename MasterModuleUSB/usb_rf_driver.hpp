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

	uint8_t * configurationDesc() override {
	    static const uint8_t configDescriptor[] = {
	        // configuration descriptor
	        9,                      // bLength
	        2,                      // bDescriptorType
	        LSB(CONFIG1_DESC_SIZE), // wTotalLength
	        MSB(CONFIG1_DESC_SIZE),
	        1,                      // bNumInterfaces
	        1,                      // bConfigurationValue
	        0,                      // iConfiguration
	        0x80,                   // bmAttributes
	        50,                     // bMaxPower

	        // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
	        9,                          // bLength
	        4,                          // bDescriptorType
	        0,                          // bInterfaceNumber
	        0,                          // bAlternateSetting
	        4,                          // bNumEndpoints
	        0x00,                       // bInterfaceClass
	        0x00,                       // bInterfaceSubClass
	        0x00,                       // bInterfaceProtocol
	        0,                          // iInterface

	        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	        ENDPOINT_DESCRIPTOR_LENGTH, // bLength
	        ENDPOINT_DESCRIPTOR,        // bDescriptorType
	        PHY_TO_DESC(EPINT_IN),     // bEndpointAddress
	        E_INTERRUPT,                     // bmAttributes (0x02=bulk)
	        LSB(MAX_PACKET_SIZE_EPINT),// wMaxPacketSize (LSB)
	        MSB(MAX_PACKET_SIZE_EPINT),// wMaxPacketSize (MSB)
	        1,                          // bInterval

	        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	        ENDPOINT_DESCRIPTOR_LENGTH, // bLength
	        ENDPOINT_DESCRIPTOR,        // bDescriptorType
	        PHY_TO_DESC(EPINT_OUT),    // bEndpointAddress
	        E_INTERRUPT,                     // bmAttributes (0x02=bulk)
	        LSB(MAX_PACKET_SIZE_EPINT),// wMaxPacketSize (LSB)
	        MSB(MAX_PACKET_SIZE_EPINT),// wMaxPacketSize (MSB)
	        1,

	        // bInterval
	        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	        ENDPOINT_DESCRIPTOR_LENGTH, // bLength
	        ENDPOINT_DESCRIPTOR,        // bDescriptorType
	        PHY_TO_DESC(EPBULK_IN),     // bEndpointAddress
	        E_BULK,                     // bmAttributes (0x02=bulk)
	        LSB(MAX_PACKET_SIZE_EPBULK),// wMaxPacketSize (LSB)
	        MSB(MAX_PACKET_SIZE_EPBULK),// wMaxPacketSize (MSB)
	        0,                          // bInterval

	        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	        ENDPOINT_DESCRIPTOR_LENGTH, // bLength
	        ENDPOINT_DESCRIPTOR,        // bDescriptorType
	        PHY_TO_DESC(EPBULK_OUT),    // bEndpointAddress
	        E_BULK,                     // bmAttributes (0x02=bulk)
	        LSB(MAX_PACKET_SIZE_EPBULK),// wMaxPacketSize (LSB)
	        MSB(MAX_PACKET_SIZE_EPBULK),// wMaxPacketSize (MSB)
	        0                           // bInterval
	    };
	    return (uint8_t*)configDescriptor;
	}

	void handleTick() override {

		if(rx_frame.rx_flag) {
			uint8_t* data = rx_frame.data;
			uint8_t left = rx_frame.data_len;

			XPCC_LOG_DEBUG .printf("write rx frame %d...", left);
			while(left > 0) {
				uint8_t len = (left > 64)? 64 : left;
				write(EPBULK_IN, data, len, 64);
				left -= len;
				data += len;
			}
			XPCC_LOG_DEBUG .printf("OK\n");

			rx_frame.rx_flag = 0;
		}

	}

	bool USBCallback_setConfiguration(uint8_t configuration) override {
		if(configuration != 1) {
			return false;
		}

		addEndpoint(EPBULK_OUT, MAX_PACKET_SIZE_EPBULK);
		addEndpoint(EPBULK_IN,  MAX_PACKET_SIZE_EPBULK);
		addEndpoint(EPINT_OUT, MAX_PACKET_SIZE_EPINT);
		addEndpoint(EPINT_IN, MAX_PACKET_SIZE_EPINT);

		readStart(EPINT_OUT, 64);
		readStart(EPBULK_OUT, 64);

		return true;
	}

	static void rxFrameHandler() {
		XPCC_LOG_DEBUG .printf("rx frame\n");
		self->blinker.blink(20);
		if(!self->rx_frame.rx_flag)
			rf230drvr.readFrame(self->rx_frame);
		else
			XPCC_LOG_DEBUG .printf("RX FRAME OVERRUN\n");
	}

	template <typename retType>
	void funcReturn(retType arg) {
		XPCC_LOG_DEBUG .printf("write %d\n", sizeof(retType));

		writeNB(EPINT_IN, (uint8_t*)&arg, sizeof(retType), MAX_PACKET_SIZE_EPINT);

	}

	//interrupt IN callback
	bool EP1_IN_callback() override {
		XPCC_LOG_DEBUG .printf("IN callback\n");

		return true;
	}

	// interrupt OUT callback
	bool EP1_OUT_callback() override {
		XPCC_LOG_DEBUG .printf("OUT1\n");
		uint8_t buf[64];
		uint32_t size;

		readEP(EPINT_OUT, buf, &size, 64);

		funcCallPkt<> *pkt = (funcCallPkt<>*)buf;

		XPCC_LOG_DEBUG .printf("func %d\n", pkt->function_id);

		switch(pkt->function_id) {
		case SET_CHANNEL: {
			auto *p = (funcCallPkt<SET_CHANNEL, uint8_t>*)buf;
			XPCC_LOG_DEBUG .printf("set channel %d\n", p->arg0);

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
			auto *p = (funcCallPkt<SET_SHORT_ADDRESS, uint16_t>*)buf;
			XPCC_LOG_DEBUG .printf("set short address %d\n", p->arg0);
			rf230drvr.setShortAddress(p->arg0);

			break;
		}
		case GET_SHORT_ADDRESS:
			funcReturn<uint16_t>(rf230drvr.getShortAddress());
			break;

		case SET_PAN_ID: {
			auto *p = (funcCallPkt<SET_PAN_ID, uint16_t>*)buf;
			XPCC_LOG_DEBUG .printf("set pan id %d\n", p->arg0);
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

			XPCC_LOG_DEBUG .printf("send %d\n", frm.data_len);
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

	//bulk OUT callback
	bool EP2_OUT_callback() override {
		XPCC_LOG_DEBUG .printf("OUT2\n");

		if(data_pos < 128) {
			uint32_t read;
			readEP(EPBULK_OUT, frame_data+data_pos, &read, 128-data_pos);
			data_pos += read;
			XPCC_LOG_DEBUG .printf("Read %d\n", read);
		}

		return true;
	}

private:
	xpcc::Blinker<xpcc::gpio::Invert<redLed>> blinker;

	uint8_t buffer[64];
	uint8_t frame_data[128];

	StaticFrame rx_frame;

	uint8_t data_pos;

	static USBInterface* self;
};

#endif /* USB_RF_DRIVER_HPP_ */
