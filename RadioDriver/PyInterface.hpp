/*
 * PyInterface.hpp
 *
 *  Created on: Apr 10, 2013
 *      Author: walmis
 */

#ifndef PYINTERFACE_HPP_
#define PYINTERFACE_HPP_

#include <boost/python.hpp>

#include "UsbRfDriver.hpp"

#define XPCC_LOG_LEVEL xpcc::log::DEBUG
#include <xpcc/communication/TinyRadioProtocol.hpp>

#include <xpcc/driver/connectivity/wireless/mac802.15.4/mac.hpp>
#include <xpcc/container.hpp>

using namespace boost::python;

class Radio : public TinyRadioProtocol<UsbRfDriver, AES_CCM_32> {
public:
	typedef TinyRadioProtocol<UsbRfDriver, AES_CCM_32> Base;
	Radio() : TinyRadioProtocol<UsbRfDriver, AES_CCM_32>(driver) {
		driver.initUSB();
		sem_init(&sem, 0, 0);
	}

	void init() {
		TinyRadioProtocol<UsbRfDriver, AES_CCM_32>::init();
		driver.setRxFrameHandler(rxHandler);
	}

	static void rxHandler() {
		//XPCC_LOG_DEBUG .printf("rx handler\n");
		self->rxHandler();

		sem_post(&(static_cast<Radio*>(self)->sem));
	}

	boost::python::list listNodes() {

		list l;

		for(auto node : connectedNodes) {
			l.append<NodeACL*>(node);
		}
		return l;
	}

	void poll() {
		timespec tm;
		clock_gettime(CLOCK_REALTIME, &tm);

		tm.tv_nsec += 1000*1000;
		sem_timedwait(&sem, &tm);

		self->handleTick();
	}

	bool sendRequest(uint16_t address, uint8_t req, std::string data, uint8_t flags) {
		return Base::sendRequest(address, req, (uint8_t*)data.c_str(), data.length(), flags);

	}

	UsbRfDriver* getDriver() {
		return &driver;
	}

protected:
	UsbRfDriver driver;
	sem_t sem;
};


class RadioWrapper : public Radio, public wrapper<Radio> {
public:


	bool frameHandler(Frame& rxFrame) override {
		if (auto f = this->get_override("frameHandler"))
			return f(rxFrame); // *note*
		return Radio::frameHandler(rxFrame);
	}

	void eventHandler(uint16_t address, EventType event) {
		if (auto f = this->get_override("eventHandler")) {
			f(address, event); // *note*
			return;
		}
		return Radio::eventHandler(address, event);

	}

	void requestHandler(MacFrame &frm, uint16_t address,
				uint8_t request_type, uint8_t* data, uint8_t len) {

		if (auto f = this->get_override("requestHandler")){
			f(frm, address, request_type, data, len); // *note*
			return;
		}

		Radio::requestHandler(frm, address, request_type, data, len);
	}

	void dataHandler(MacFrame &frm, FrameHdr& hdr, uint16_t address,
				uint8_t *data, uint8_t len) {


		std::string str((char*)data, (int)len);
		if (auto f = this->get_override("dataHandler")){
			f(frm, hdr, address, str); // *note*
			return;
		}

		Radio::dataHandler(frm, hdr, address, data, len);

	}

	void beaconHandler(uint16_t address, const BeaconFrame& bcn) {
		if (auto f = this->get_override("beaconHandler")){
			f(address, bcn); // *note*
			return;
		}

		Radio::beaconHandler(address, bcn);

	}
	void prepareBeacon(BeaconFrame& frm) {
		if (auto f = this->get_override("prepareBeacon")){
			f(frm); // *note*
			return;
		}

		Radio::prepareBeacon(frm);
	}

	void stdResponseHandler(MacFrame& frm,
			Request& request, uint8_t* data, uint8_t len) {

		if (auto f = this->get_override("stdResponseHandler")){
			f(frm, request, data, len); // *note*
			return;
		}

		Radio::stdResponseHandler(frm, request, data, len);
	}

	void stdRequestHandler(MacFrame& frm,
			uint16_t src_address, uint8_t requestId, uint8_t* data, uint8_t len) {

		if (auto f = this->get_override("stdRequestHandler")){
			f(frm, src_address, requestId, data, len); // *note*
			return;
		}

		Radio::stdRequestHandler(frm, src_address, requestId, data, len);

	}


};

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(send_overloads, send, 3, 6);


bool (Radio::*sendResponse)(uint8_t*, uint8_t, uint8_t) = &Radio::sendResponse;

void translator(USBException const & x) {
    PyErr_SetString(PyExc_IOError, x.what());
}


object MacFrame_getPayload(MacFrame* f) {
    return object(handle<>(PyBuffer_FromReadWriteMemory(f->getPayload(), f->getPayloadSize())));
}

template <typename O, char* (O::*G)(), int size>
object getMembuffer(O* f) {
    PyObject* buf = PyBuffer_FromReadWriteMemory((f->*G)(), size);

   return object(handle<>(buf));
}

bool FrameHdr_data_pending(FrameHdr* f) {
	return f->data_pending;
}

bool FrameHdr_type(FrameHdr *f) {
	return f->type;
}

BOOST_PYTHON_MODULE(usbradio)
{
	register_exception_translator<USBException>(translator);

	class_<Frame>("Frame")
		.def_readonly("rssi", &Frame::rssi)
		.def_readonly("lqi", &Frame::lqi);

	enum_<MacFrame::FcfFrameType>("FcfFrameType")
		.value("BEACON", MacFrame::FcfFrameType::BEACON)
		.value("DATA", MacFrame::FcfFrameType::DATA)
		.value("ACK", MacFrame::FcfFrameType::ACK)
		.value("MAC_COMMAND", MacFrame::FcfFrameType::MAC_COMMAND);

	enum_<MacFrame::AddrMode>("AddrMode")
		.value("ADDRESS_NOT_PRESENT", MacFrame::AddrMode::ADDRESS_NOT_PRESENT)
		.value("RESERVED0",MacFrame::AddrMode::RESERVED0)
		.value("ADDRESS_16BIT",MacFrame::AddrMode::ADDRESS_16BIT)
		.value("ADDRESS_64BIT_EXTENDED",MacFrame::AddrMode::ADDRESS_64BIT_EXTENDED);

	enum_<TxFlags>("TxFlags")
			.value("TX_ENCRYPT", TxFlags::TX_ENCRYPT)
			.value("TX_ACKREQ", TxFlags::TX_ACKREQ);

	class_<MacFrame>("MacFrame", init<Frame&>())
			.def("assign", &MacFrame::assign)
			.def("isSecure", &MacFrame::isSecure)
			.def("setType", &MacFrame::setType)
			.def("ackRequired", &MacFrame::ackRequired)
			.def("getFrame", &MacFrame::getFrame, return_value_policy<reference_existing_object>())
			.def("getSrcAddress", &MacFrame::getSrcAddress)
			.def("getSrcPan", &MacFrame::getSrcAddress)
			.def("getDstAddress", &MacFrame::getDstAddress)
			.def("getDstPan", &MacFrame::getDstPan)
			.def("build", &MacFrame::build)
			.def("setSrcAddress", &MacFrame::setSrcAddress)
			.def("setDstAddress", &MacFrame::setDstAddress)
			.def("setSeq", &MacFrame::setSeq)
			.def("getSeq", &MacFrame::getSeq)
			.def("getMaxPayload", &MacFrame::getMaxPayload)
			.def("getPayload", MacFrame_getPayload)
			.def("getPayloadSize", &MacFrame::getPayloadSize)
			.def("addData", &MacFrame::addData)
			.def("clear", &MacFrame::clear);

	enum_<EventType>("EventType")
			.value("ASSOCIATION_EVENT", EventType::ASSOCIATION_EVENT)
			.value("DISASSOCIATION_EVENT", EventType::DISASSOCIATION_EVENT)
			.value("ASSOCIATION_TIMEOUT", EventType::ASSOCIATION_TIMEOUT);

	enum_<FrameType>("FrameType")
			.value("BEACON", FrameType::BEACON)
			.value("DATA", FrameType::DATA)
			.value("REQUEST", FrameType::REQUEST)
			.value("RESPONSE", FrameType::RESPONSE)
			.value("DISSASOC", FrameType::DISSASOC);

	class_<NodeACL>("NodeACL")
			.def_readonly("address", &NodeACL::address)
			.def_readonly("last_activity", &NodeACL::last_activity)
			.def_readonly("associated", &NodeACL::associated);

	class_<UsbRfDriver>("UsbRfDriver")
			.def("rxOn", &UsbRfDriver::rxOn)
			.def("rxOff", &UsbRfDriver::rxOff);

	class_<BeaconFrame>("BeaconFrame")
			.add_property("name", getMembuffer<BeaconFrame, &BeaconFrame::getName, 16>)
			.add_property("data", getMembuffer<BeaconFrame, &BeaconFrame::getData, 8>);

	class_<FrameHdr>("FrameHdr")
			.add_property("type", FrameHdr_type)
			.add_property("data_pending", FrameHdr_data_pending)
			.def_readonly("req_id", &FrameHdr::req_id);

	class_<RadioWrapper, boost::noncopyable>("TinyRadioProtocol")
			.def("init", &Radio::init)

			.def("associate", &Radio::associate)
			.def("disassociate", &Radio::disassociate)
			.def("isAssociated", &Radio::isAssociated)

			.def("setAddress", &Radio::setAddress)
			.def("setPanId", &Radio::setPanId)
			.def("getAddress", &Radio::getAddress)

			.def("send", &Radio::send, send_overloads())
			.def("sendRequest", &Radio::sendRequest)

			.def("sendResponse", sendResponse)

			.def("listNodes", &Radio::listNodes)
			.def("findNode", &Radio::findNode, return_value_policy<reference_existing_object>())

			.def("frameHandler", &RadioWrapper::frameHandler)
			.def("dataHandler", &RadioWrapper::dataHandler)
			.def("requestHandler", &RadioWrapper::requestHandler)
			.def("beaconHandler", &RadioWrapper::beaconHandler)
			.def("eventHandler", &RadioWrapper::eventHandler)
			.def("prepareBeacon", &RadioWrapper::prepareBeacon)
			.def("stdRequestHandler", &RadioWrapper::stdRequestHandler)
			.def("stdResponseHandler", &RadioWrapper::stdResponseHandler)

			.def("poll", &Radio::poll)

			.def("getDriver", &Radio::getDriver, return_value_policy<reference_existing_object>());
};





#endif /* PYINTERFACE_HPP_ */
