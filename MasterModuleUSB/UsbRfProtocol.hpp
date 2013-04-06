/*
 * UsbRfProtocol.hpp
 *
 *  Created on: Apr 5, 2013
 *      Author: walmis
 */

#ifndef USBRFPROTOCOL_HPP_
#define USBRFPROTOCOL_HPP_

#include <xpcc/driver/connectivity/wireless/mac802.15.4/Frame.h>
#include <type_traits>

enum FuncId : uint8_t {
	NOP,
	SET_CHANNEL,
	GET_CHANNEL,
	SET_SHORT_ADDRESS,
	GET_SHORT_ADDRESS,
	SET_PAN_ID,
	GET_PAN_ID,
	RX_ON,
	RX_OFF,
	SEND_FRAME,
	UPLOAD_SEND_FRAME,
	CLEAR_FRAME,
	INIT
};

enum ReqId : uint8_t {
	FUNC_CALL = 1,
	FUNC_RETURN,
	RX_FRAME
};


class RfFrameData {
	uint8_t lqi;
	uint8_t rssi;
	uint8_t data_len;
} __attribute__((packed));


struct unused{} __attribute__((packed));

template <FuncId id = NOP, typename Targ0 = unused>

struct funcCallPkt {
	ReqId request_id = FUNC_CALL;
	FuncId  function_id = id;
	uint16_t length;
	Targ0 arg0;

	funcCallPkt() {
		length = 0;
	}

	funcCallPkt(Targ0 arg0) {
		this->arg0 = arg0;
		length = sizeof(arg0);
	}

	size_t size() {
		size_t size = 0;
		if(!std::is_same<Targ0, unused>::value) {
			size += sizeof(Targ0);
		}

		return sizeof(length) + sizeof(request_id) + sizeof(function_id) + size;
	}


	uint8_t* data() {
		return (uint8_t*)this;
	}
} __attribute__((packed));


#endif /* USBRFPROTOCOL_HPP_ */
