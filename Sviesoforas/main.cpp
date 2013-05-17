#include <LPC11xx.h>

#include <xpcc/architecture.hpp>
#include <xpcc/workflow/periodic_timer.hpp>
//#include <xpcc/debug.hpp>
//#include <xpcc/platform>


using namespace xpcc;

lpc::Uart1 uart(38400);
//static xpcc::IODeviceWrapper<lpc::Uart1> device;
//xpcc::IOStream stdout(device);

const uint8_t address = 0x3B;

#define SVIESOFORAS4

#ifdef SVIESOFORAS2
GPIO__OUTPUT(led1, 2, 0);
GPIO__OUTPUT(led2, 0, 10);
GPIO__OUTPUT(led3, 0, 10); //LED3
GPIO__OUTPUT(led4, 1, 1); //LED4
#endif

#ifdef SVIESOFORAS3
GPIO__OUTPUT(led1, 2, 0); //LED1
GPIO__OUTPUT(led2, 1, 10); //LED2
GPIO__OUTPUT(led3, 0, 10); //LED3
GPIO__OUTPUT(led4, 1, 1); //LED4
#endif

#ifdef SVIESOFORAS4
GPIO__OUTPUT(led1, 2, 0); //LED1
GPIO__OUTPUT(led2, 1, 10); //LED2
GPIO__OUTPUT(led4, 0, 10); //LED4
GPIO__OUTPUT(led3, 1, 1); //LED3
#endif

GPIO__INPUT(progPin, 0, 1);

//xpcc::log::Logger xpcc::log::debug(device);

int main() {
	lpc11::SysTickTimer::enable();

	led1::setOutput(true);
	led2::setOutput(true);
	led3::setOutput(true);
	led4::setOutput(true);

	progPin::setInput(lpc::InputType::PULLUP);

	uint8_t c;
	uint8_t data[2];

	xpcc::delay_ms(200);

	led1::set(0);
	led2::set(0);
	led3::set(0);
	led4::set(0);

	while(1) {

		if(!progPin::read()) {
			NVIC_SystemReset();
		}

		if(uart.read(c)) {

			if(c == 0x55) {
				uart.read(data, 2, true);

				//patikrinam adresa
				if(data[0] == address) {
					led1::set(data[1] & 0x01);
					led2::set(data[1] & 0x02);
					led3::set(data[1] & 0x04);
					led4::set(data[1] & 0x08);
				}
			}
		}

	}


	while(1);
}
