#include <LPC11xx.h>

#include <xpcc/architecture.hpp>
#include <xpcc/workflow.hpp>
#include <xpcc/debug.hpp>
//#include <xpcc/platform>
#include <xpcc/driver/connectivity/wireless/at86rf230.hpp>
#include <xpcc/communication/TinyRadioProtocol.hpp>

#include "pindefs.hpp"
#include "utils.hpp"

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
	START = 16,
	STOP,
	BRAKE,
	SET_SPEED,
	REVERSE,
	FORWARD,

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

class MotorControl : xpcc::TickerTask {
public:
	static const int pwm_top = 100;
	static const int counter_val = 30*10; //10 motor revolutions
	static const int timeout_val = 500; //200ms

	void init() {
		lpc11::Timer32B1::init(lpc11::TimerMode::TIMER_MODE, 2);

		lpc11::Timer32B1::initPWM(pwm_top, 0);
		lpc11::Timer32B1::initPWMChannel(3);

		lpc11::Timer32B1::activate(true);

		lpc11::Timer16B0::init(lpc11::TimerMode::COUNTER_FALLING_MODE, 1);

		lpc11::Timer16B0::enableCapturePins();
		lpc11::Timer16B0::configureMatch(0, counter_val,
				lpc11::ExtMatchOpt::EXTMATCH_NOTHING, true, false, true);

		NVIC_EnableIRQ(TIMER_16_0_IRQn);
		lpc11::Timer16B0::activate();

		timeout.restart(timeout_val);
	}

	void handleTick() override {
		if(timeout.isExpired()) {
			speed = (lpc11::Timer16B0::getCounterValue()*1000) / timeout_val;

			lpc11::Timer16B0::resetCounter();
			timeout.restart(timeout_val);
		}
		if(stopping && speed == 0) {
			stopped = true;
			stopping = false;
		}
		if(new_direction != direction && stopped) {
			if(new_direction) {
				rotDirection::set(true);
			} else {
				rotDirection::set(false);
			}
			direction = new_direction;
			run();
		}
	}

	void handleInterrupt(int irqN) override {
		if(irqN == TIMER_16_0_IRQn) {
			if(lpc11::Timer16B0::getIntStatus(lpc11::TmrIntType::MR0_INT)) {
				//XPCC_LOG_DEBUG .printf("timer int %x\n", LPC_TMR16B0->IR);

				uint32_t delta = (xpcc::Clock::now().getTime() - capture_time.getTime());
				capture_time = xpcc::Clock::now();
				speed = (counter_val*1000) / delta;
				timeout.restart(timeout_val);

				//XPCC_LOG_DEBUG .printf("speed1 %d\n", speed);
				lpc11::Timer16B0::clearIntPending(lpc11::TmrIntType::MR0_INT);
			}
		}
	}

	int32_t getSpeed() {
		return speed;
	}

	void setSpeed(uint8_t speed) {
		lpc11::Timer32B1::setPWM(3, speed);
	}


	void run() {
		startStop::set(true);
		runBrake::set(true);
		stopping = false;
		stopped = false;
	}

	void stop() {
		startStop::set(false);
		stopping = true;
	}

	void brake() {
		runBrake::set(false);
		stopping = true;
	}

	void setDirection(bool reverse = false) {
		stop();
		new_direction = reverse;
	}

private:
	xpcc::Timeout<> timeout;
	xpcc::Timestamp capture_time;
	uint32_t capture_tick;
	int32_t speed;

	uint8_t direction = 0;
	uint8_t new_direction = 0;
	uint8_t stopping = 0;
	bool stopped;

};

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
	while (*p) {
		k = (k << 3) + (k << 1) + (*p) - '0';
		p++;
	}
	return k;
}

MotorControl motorControl;


rf230::Driver<lpc::SpiMaster0, rfReset, rfSel, rfSlpTr, rfIrq> at_radio;
class TrainRadio : public TinyRadioProtocol<typeof(at_radio), AES_CCM_32> {
public:
	TrainRadio() : TinyRadioProtocol(at_radio),	speedReport(250)
	{
	}

	bool frameHandler(Frame &rxFrame) {
		blinker.blink(50);
		return true;
	}

	void prepareBeacon(BeaconFrame &beacon) override {
		strcpy(beacon.name, "Traukinys");
		strcpy(beacon.data, "0");
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
		XPCC_LOG_DEBUG .printf("Request %d\n", request_type);

		bool res = true;
		switch(request_type) {

			case START: {
				motorControl.run();
				sendResponse<bool>(res);
				break;
			}
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
					motorControl.setSpeed(data[0]);
				}
				sendResponse<bool>(res);

				break;
			}
			case REVERSE:
				motorControl.setDirection(true);
				sendResponse<bool>(res);
				break;

			case FORWARD:
				motorControl.setDirection(false);
				sendResponse<bool>(res);
				break;

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

	radio.init();
	radio.setAddress(0x8980);
	radio.setPanId(0x1234);

	at_radio.rxOn();

	TickerTask::tasksRun(idleTask);
}
