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
#include <xpcc/communication/TinyRadioProtocol.hpp>
#include <xpcc/driver/connectivity/wireless/mac802.15.4/mac.hpp>
#include <xpcc/container.hpp>

using namespace boost::python;

class Radio : public TinyRadioProtocol<UsbRfDriver, AES_CCM_32> {
public:
	Radio() : TinyRadioProtocol<UsbRfDriver, AES_CCM_32>(driver) {
		driver.initUSB();
		sem_init(&sem, 0, 0);
	}

	void init() {
		TinyRadioProtocol<UsbRfDriver, AES_CCM_32>::init();
		driver.setRxFrameHandler(rxHandler);
	}

	static void rxHandler() {
		XPCC_LOG_DEBUG .printf("rx handler\n");
		self->rxHandler();

		self->handleTick();
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

		if (auto f = this->get_override("dataHandler")){
			f(frm, hdr, address, data, len); // *note*
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

//	void vtest() {
//		if (auto f = this->get_override("vtest")) {
//			f(); // *note*
//			return;
//		}
//		Radio::vtest();
//	}

};

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(send_overloads, send, 3, 6);

bool  (Radio::*send_request)(uint16_t, uint8_t, uint8_t*,
		uint8_t, uint8_t)    = &Radio::sendRequest;

void translator(USBException const & x) {
    PyErr_SetString(PyExc_IOError, x.what());
}

void beacon_set_name(BeaconFrame* frm, uint8_t* name) {
	memcpy(frm->name, name, 16);
}

void beacon_set_data(BeaconFrame* frm, uint8_t* data) {
	memcpy(frm->data, data, 8);
}


BOOST_PYTHON_MODULE(usbradio)
{
	register_exception_translator<USBException>(translator);

	class_<Frame>("Frame");
	//class_<MacFrame>("MacFrame");

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
			.def("setName", beacon_set_name)
			.def("setData", beacon_set_data);

	class_<RadioWrapper, boost::noncopyable>("TinyRadioProtocol")
			.def("init", &Radio::init)

			.def("associate", &Radio::associate)
			.def("disassociate", &Radio::disassociate)
			.def("isAssociated", &Radio::isAssociated)

			.def("setAddress", &Radio::setAddress)
			.def("setPanId", &Radio::setPanId)
			.def("getAddress", &Radio::getAddress)

			.def("send", &Radio::send, send_overloads())
			.def("sendRequest", send_request)

			.def("listNodes", &Radio::listNodes)

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
