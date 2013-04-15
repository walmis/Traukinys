#include <LPC11xx.h>

#include <xpcc/architecture.hpp>
#include <xpcc/workflow.hpp>
#include <xpcc/debug.hpp>
//#include <xpcc/platform>
#include <xpcc/driver/connectivity/wireless/at86rf230.hpp>
#include <xpcc/communication/TinyRadioProtocol.hpp>

#include "pindefs.hpp"
#include "utils.hpp"
#include "motor_control.hpp"

using namespace xpcc;

lpc::Uart1 uart(57600);
static xpcc::IODeviceWrapper<lpc::Uart1> device;

xpcc::IOStream stdout(device);
xpcc::log::Logger xpcc::log::debug(device);

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

	SPEED_REPORT
};



void systick_handler() {

	if(!progPin::read()) {
		NVIC_SystemReset();
	}

}



static xpcc::PeriodicTimer<> tm(100);
void idleTask(){
	if(tm.isExpired()) {
		//XPCC_LOG_DEBUG .printf("idle\n");

		//MotorControl::setSpeed(speed);

		//speed ++;
		//speed %= 100;

	}
}

int to_int(char *p) {
	int k = 0;
	bool neg = false;
	if(*p == '-') {
		p++;
		neg = true;
	}

	while (*p) {
		k = (k << 3) + (k << 1) + (*p) - '0';
		p++;
	}
	if(neg) return -k;
	return k;
}

MotorControl motorControl;


rf230::Driver<lpc::SpiMaster0, rfReset, rfSel, rfSlpTr, rfIrq> at_radio;
class TrainRadio : public TinyRadioProtocol<typeof(at_radio), AES_CCM_32> {
public:
	uint8_t train_id[8];

	TrainRadio() : TinyRadioProtocol(at_radio),	speedReport(250)
	{
		uint32_t a[5];
		lpc11::Core::getUniqueId(a);
		a[1] = a[1] ^ a[2];
		a[3] = a[3] ^ a[4];
		//memcpy(train_id, (uint8_t*)(a[1]), sizeof(uint32_t));
		//memcpy(train_id+4, (uint8_t*)(a[3]), sizeof(uint32_t));
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
private:
	xpcc::PeriodicTimer<> speedReport;
	Blinker<xpcc::gpio::Invert<led>> blinker;
};

TrainRadio radio;

class Terminal : xpcc::TickerTask {
	char buffer[32];
	uint8_t pos = 0;
	void handleTick() override {

		if(device.read(buffer[pos])) {
			if(buffer[pos] == '\n') {
				//remove the newline character
				buffer[pos] = 0;
				handleCommand();
				pos = 0;
				return;
			}
			pos++;
			pos &= 31;
		}
	}
	void handleCommand() {
		stdout.printf(": %s\n", buffer);
		if(strcmp(buffer, "start") == 0) {
			stdout.printf("starting motor\n");
			motorControl.run();
		} else
		if(strcmp(buffer, "reset") == 0) {
			stdout.printf("resetting\n");
			NVIC_SystemReset();
		} else
		if(strcmp(buffer, "stop") == 0) {
			motorControl.stop();
		} else
		if(strcmp(buffer, "brake") == 0) {
			motorControl.brake();
		} else
		if(strncmp(buffer, "speed", 5) == 0) {
			int speed = to_int(&buffer[6]);
			stdout.printf("new speed %d\n", speed);
			motorControl.setSpeed(speed);
		} else
		if(strcmp(buffer, "reverse") == 0) {
			motorControl.setDirection(true);
		} else
		if(strcmp(buffer, "forward") == 0) {
			motorControl.setDirection(false);
		}


	}
};


Terminal terminal;

uint16_t getAddress() {
	uint32_t a[5];
	lpc11::Core::getUniqueId(a);

	XPCC_LOG_DEBUG .printf("Device id %x %x %x %x\n", a[1], a[2], a[3], a[4]);


	uint16_t *ptr = (uint16_t*)a;
	uint16_t res = 0;
	for(int i = 0; i < sizeof(uint32_t)*5/2; i++) {
		res ^= ptr[i];
	}
	XPCC_LOG_DEBUG .printf("Address: %02x\n", res);
	return res;
}

int main() {
	gpio_init();

	xpcc::Random::seed();

	lpc11::SysTickTimer::enable();
	lpc11::SysTickTimer::attachInterrupt(systick_handler);

	lpc::SpiMaster0::configurePins(lpc::SpiMaster0::MappingSck::SWCLK_PIO0_10, false);
	lpc::SpiMaster0::initialize(lpc::SpiMaster0::Mode::MODE_0,
			lpc::SpiMaster0::Presacler::DIV002, 3);

	motorControl.init();

	led::set(1);

	XPCC_LOG_DEBUG .printf("Starting\n");

	radio.init();
	radio.setAddress(getAddress());
	radio.setPanId(0x1234);

	at_radio.rxOn();

	TickerTask::tasksRun(idleTask);
}
