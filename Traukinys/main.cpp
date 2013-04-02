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

rf230::Driver<lpc::SpiMaster0, rfReset, rfSel, rfSlpTr> at_radio;

xpcc::log::Logger xpcc::log::debug(device);




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

	xpcc::TickerTask::tick();

	if(!progPin::read()) {
		NVIC_SystemReset();
	}

}

void Gpio0IntHandler(uint8_t pin) {
	if(pin == 6) {
		at_radio.IRQHandler();
	}
}

int main() {
	gpio_init();

	xpcc::Random::seed();

	lpc11::SysTickTimer::enable();
	lpc11::SysTickTimer::attachInterrupt(systick_handler);

	XPCC_LOG_DEBUG << SystemCoreClock << xpcc::endl;

	lpc::GpioInterrupt::enableInterrupt(0, 6, lpc::IntSense::EDGE, lpc::IntEdge::SINGLE,
			lpc::IntEvent::RISING_EDGE);
	lpc::GpioInterrupt::registerPortHandler(0, Gpio0IntHandler);
	lpc::GpioInterrupt::enableGlobalInterrupts();

	lpc::SpiMaster0::configurePins(lpc::SpiMaster0::MappingSck::SWCLK_PIO0_10, false);
	lpc::SpiMaster0::initialize(lpc::SpiMaster0::Mode::MODE_0,
			lpc::SpiMaster0::Presacler::DIV002, 3);

	led::set(1);

	radio.init();
	radio.setAddress(0x8980);
	radio.setPanId(0x1234);

	at_radio.rxOn();

	xpcc::PeriodicTimer<> tm(1000);
	while(1) {
		radio.poll();
	}
}
