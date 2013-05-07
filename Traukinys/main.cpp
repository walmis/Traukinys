#include <xpcc/architecture.hpp>
#include <xpcc/workflow.hpp>
#include <xpcc/debug.hpp>
#include <xpcc/driver/ui/led.hpp>
//#include <xpcc/platform>
#include <xpcc/driver/connectivity/wireless/at86rf230.hpp>

#define NODE_TIMEOUT 3500 //3.5s
#include <xpcc/communication/TinyRadioProtocol.hpp>

#include "pindefs.hpp"
#include "motor_control.hpp"
#include "rfid.hpp"

using namespace xpcc;

lpc::Uart1 uart(57600);
xpcc::IODeviceWrapper<lpc::Uart1> device;



//NullIODevice null;
//xpcc::log::Logger xpcc::log::debug(null);


RFID rfid(device);

extern "C" void fault_handler() {
	XPCC_LOG_DEBUG .printf("Fault occurred\n");
	while(1);
}

extern "C" void HardFault_Handler() {
	XPCC_LOG_DEBUG .printf("Hard fault\n");
	while(1);
}

enum TrainCommands {
	SET_SPEED = 16,
	STOP,
	BRAKE,
	GET_SPEED,

	GPIO_DIR,
	GPIO_READ,
	GPIO_SET,

	SPEED_REPORT, //data frame
	RFID_READ, //data frame
	DEBUG_MSG //data frame
};



MotorControl motorControl;


rf230::Driver<lpc::SpiMaster0, rfReset, rfSel, rfSlpTr, rfIrq> at_radio;
class TrainRadio : public TinyRadioProtocol<typeof(at_radio), AES_CCM_32>,
		public xpcc::IODevice {
public:
	uint8_t train_id[8];

	TrainRadio() :
	  TinyRadioProtocol(at_radio), 
	  speedReport(250), 
	  tm(64)
	{
		uint32_t a[5];
		lpc11::Core::getUniqueId(a);
		a[1] = a[1] ^ a[2];
		a[3] = a[3] ^ a[4];
		memcpy(train_id, &a[1], sizeof(uint32_t));
		memcpy(train_id+4, &a[3], sizeof(uint32_t));
	}

	bool frameHandler(Frame &rxFrame) {
		blinker.blink(50);
		return true;
	}

	void prepareBeacon(BeaconFrame &beacon) override {
		strcpy(beacon.name, "Traukinys");
		memcpy(beacon.data, train_id, 8);
	}

	void handleTick() override {
		if(motorControl.getSpeed() != 0 && speedReport.isExpired()) {
			for(auto node : connectedNodes) {
				uint32_t speed = motorControl.getSpeed();

				blinker.blink(50);
				send(node->address, (uint8_t*)&speed, sizeof(speed), SPEED_REPORT);
			}
		}
		if(tm.isExpired() && buffer_pos != 0) {

			if(connectedNodes.getSize() != 0) {
				for(auto node : connectedNodes) {
					send(node->address, tx_buffer, buffer_pos, DEBUG_MSG);
				}
				buffer_pos = 0;
			}
			tm.restart(64);
		}

		TinyRadioProtocol<typeof(at_radio), AES_CCM_32>::handleTick();
	}

	void eventHandler(uint16_t address, EventType event) override {
		if(event == EventType::DISASSOCIATION_EVENT) {

			if(connectedNodes.getSize() == 1) { //last one
				motorControl.stop();
			}

		}
	}

	void requestHandler(MacFrame &frm, uint16_t address,
			uint8_t request_type, uint8_t* data, uint8_t len) override {

		if(!isAssociated(address)) {
			//send dissasociation packet
			disassociate(address, true);
			return;
		}
		//XPCC_LOG_DEBUG .printf("Request %d\n", request_type);

		bool res = true;
		switch(request_type) {

			case STOP: {
				motorControl.stop();
				sendResponse<bool>(res);
				break;
			}
			case BRAKE:
				motorControl.brake();
				sendResponse<bool>(res);
				break;

			case SET_SPEED: {
				if(len == 1) {
					motorControl.setSpeed((int8_t)data[0]);
				}
				sendResponse<bool>(res);

				break;
			}

			case GET_SPEED:
				break;

			case GPIO_DIR:
				break;

			case GPIO_READ:
				break;

			case GPIO_SET:
				break;

		}

	}

	void write(char c) override {
		if(buffer_pos == 100) return;

		tx_buffer[buffer_pos++] = c;
		if(buffer_pos == 100) {
			if(connectedNodes.getSize() != 0) {
				for(auto node : connectedNodes) {
					send(node->address, tx_buffer, 100, DEBUG_MSG);
				}
				buffer_pos = 0;
				tm.restart(64);
			}
		}
	}

	bool read(char& c) override {
		return false;
	}

	void flush() override {

	}

private:
	xpcc::PeriodicTimer<> speedReport;
	Blinker<xpcc::gpio::Invert<ledRed>> blinker;

	xpcc::Timeout<> tm;
	uint8_t tx_buffer[100];
	uint8_t buffer_pos = 0;
};

TrainRadio radio;
xpcc::log::Logger stdout(radio);


//Terminal terminal;

uint16_t getAddress() {
	uint32_t a[5];
	lpc11::Core::getUniqueId(a);

	stdout .printf("Device id %x %x %x %x\n", a[1], a[2], a[3], a[4]);


	uint16_t *ptr = (uint16_t*)a;
	uint16_t res = 0;
	for(int i = 0; i < sizeof(uint32_t)*5/2; i++) {
		res ^= ptr[i];
	}
	stdout .printf("Address: %02x\n", res);
	return res;
}

bool rfid_init() {
	//reset rfid module
	progPin::setOutput(0);
	__WFI(); //sleep until interrupt, ~1ms
	progPin::setInput(xpcc::lpc::InputType::PULLUP);
	__WFI();

	uart.setBaudrate(9600);
	if(!rfid.init()) {
		return false;
	}
	rfid.setBaudRate(BAUD_921600);
	uart.setBaudrate(921600);

	return true;
}


void idleTask(){
	static xpcc::PeriodicTimer<> tm(1000);
	static xpcc::PeriodicTimer<> rfid_tm(10);

	if(!progPin::read()) {
		NVIC_SystemReset();
	}

	if(!rfid.isConnected() && tm.isExpired()) {
		stdout.printf("Connecting RFID\n");
		if(!rfid_init()) {
			stdout.printf("Failed to initialize RFID\n");
		}
	}

	if(rfid.isConnected() && rfid_tm.isExpired() && rfid.isCard()) {

		stdout.printf("RFID: Card Detected\n");
		for(int i = 0; i < 3; i++) {
			if(rfid.readCardSerial()) {

				for(auto node : radio.connectedNodes) {
					radio.send(node->address, rfid.serNum, 7, RFID_READ);
				}

				rfid.halt();
				stdout.printf("RFID: successfully read\n");
				break;
			}
		}
	}
	
	//put processor to sleep until next interrupt
	__WFI();

}



int main() {
	gpio_init();

	xpcc::Random::seed();

	lpc11::SysTickTimer::enable();

	lpc::SpiMaster0::configurePins(lpc::SpiMaster0::MappingSck::SWCLK_PIO0_10, false);
	lpc::SpiMaster0::initialize(lpc::SpiMaster0::Mode::MODE_0,
			lpc::SpiMaster0::Presacler::DIV002, 3);

	motorControl.init();

	ledRed::set(1);

	stdout.printf("Starting:\n");
	stdout.printf("Core Freq: %d\n", SystemCoreClock);

	radio.init();
	at_radio.setCLKM(rf230::no_clock);

	radio.setAddress(getAddress());
	radio.setPanId(0x1234);

	at_radio.rxOn();

	rfid_init();

	TickerTask::tasksRun(idleTask);
}
