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
//xpcc::IOStream stdout(device);


xpcc::log::Logger xpcc::log::debug(device);

extern "C" void fault_handler() {
	XPCC_LOG_DEBUG .printf("Fault occurred\n");
	while(1);
}

extern "C" void HardFault_Handler() {
	XPCC_LOG_DEBUG .printf("Hard fault\n");
	while(1);
}

rf230::Driver<lpc::SpiMaster0, rfReset, rfSel, rfSlpTr, rfIrq> at_radio;
class TrainRadio : public TinyRadioProtocol<typeof(at_radio), AES_CCM_32> {
public:
	TrainRadio() : TinyRadioProtocol(at_radio) {

	}


	bool frameHandler(Frame &rxFrame) {
		blinker.blink(50);
		return true;
	}

	Blinker<xpcc::gpio::Invert<led>> blinker;
};

TrainRadio radio;

void systick_handler() {

	if(!progPin::read()) {
		XPCC_LOG_DEBUG .printf("reset\n");
		NVIC_SystemReset();
	}

}

class MotorControl {
public:
	static const int pwm_top = 100;

	static void init() {

		lpc11::Timer32B1::init(lpc11::TimerMode::TIMER_MODE, 2);

		lpc11::Timer32B1::initPWM(pwm_top, 0);
		lpc11::Timer32B1::initPWMChannel(3);

		lpc11::Timer32B1::activate(true);

	}

	static void setSpeed(uint8_t speed) {
		lpc11::Timer32B1::setPWM(3, speed);
	}

};


static int speed = 0;
static xpcc::PeriodicTimer<> tm(100);
void idleTask(){
	if(tm.isExpired()) {
		//XPCC_LOG_DEBUG .printf("idle\n");

		MotorControl::setSpeed(speed);

		speed ++;
		speed %= 100;

	}
}

int main() {
	gpio_init();

	xpcc::Random::seed();

	lpc11::SysTickTimer::enable();
	lpc11::SysTickTimer::attachInterrupt(systick_handler);

	lpc::SpiMaster0::configurePins(lpc::SpiMaster0::MappingSck::SWCLK_PIO0_10, false);
	lpc::SpiMaster0::initialize(lpc::SpiMaster0::Mode::MODE_0,
			lpc::SpiMaster0::Presacler::DIV002, 3);


	MotorControl::init();
	//at_radio.init();

	led::set(1);

	radio.init();
	radio.setAddress(0x8980);
	radio.setPanId(0x1234);

	at_radio.rxOn();

	TickerTask::tasksRun(idleTask);
}
