
#include <xpcc/architecture.hpp>
#include <xpcc/workflow.hpp>
#include <xpcc/debug.hpp>
//#include <xpcc/platform>
#include <xpcc/driver/connectivity/wireless/at86rf230.hpp>

#include "usb_rf_driver.hpp"

#include "pindefs.hpp"

using namespace xpcc;

rf230::Driver<lpc::SpiMaster0, rstPin, selPin, slpTr, irqPin> rf230drvr;

//xpcc::USBDevice usb(0xffff, 0x0708, 0);

USBInterface usb;

xpcc::lpc::Uart1 uart(115200);
xpcc::IODeviceWrapper<lpc::Uart1> device;
xpcc::log::Logger xpcc::log::debug(device);

enum { r0, r1, r2, r3, r12, lr, pc, psr};

extern "C" void HardFault_Handler(void)
{
  asm volatile("MRS r0, MSP;"
		       "B Hard_Fault_Handler");
}

extern "C"
void Hard_Fault_Handler(uint32_t stack[]) {

	//register uint32_t* stack = (uint32_t*)__get_MSP();

	ledGreen::set(0);
	ledRed::set();

	XPCC_LOG_DEBUG .printf("Hard Fault\n");

	XPCC_LOG_DEBUG .printf("r0  = 0x%08x\n", stack[r0]);
	XPCC_LOG_DEBUG .printf("r1  = 0x%08x\n", stack[r1]);
	XPCC_LOG_DEBUG .printf("r2  = 0x%08x\n", stack[r2]);
	XPCC_LOG_DEBUG .printf("r3  = 0x%08x\n", stack[r3]);
	XPCC_LOG_DEBUG .printf("r12 = 0x%08x\n", stack[r12]);
	XPCC_LOG_DEBUG .printf("lr  = 0x%08x\n", stack[lr]);
	XPCC_LOG_DEBUG .printf("pc  = 0x%08x\n", stack[pc]);
	XPCC_LOG_DEBUG .printf("psr = 0x%08x\n", stack[psr]);

	while(1);
}

extern "C" void fault_handler() {
	ledGreen::set(0);
	ledRed::set();
}

void usbclk_init() {

	LPC_SYSCON->SYSOSCCTRL |= 1; //enable BYPASS

	LPC_SYSCON->PDRUNCFG     &= ~(1 << 8); //power up usb pll

	LPC_SYSCON->USBPLLCLKSEL = 1; //switch to external oscillator

	LPC_SYSCON->USBPLLCLKUEN = 1;
	LPC_SYSCON->USBPLLCLKUEN = 0;
	LPC_SYSCON->USBPLLCLKUEN = 1;
	while (!(LPC_SYSCON->USBPLLCLKUEN & 0x01)); //wait until updated

	LPC_SYSCON->USBPLLCTRL = 0x25; //input clk 16MHZ, out 48MHZ

	xpcc::Timeout<> t(10);
	while (!(LPC_SYSCON->USBPLLSTAT & 0x01) && !t.isExpired()); //wait until PLL locked
	if(t.isExpired()) {
		XPCC_LOG_DEBUG .printf("USB PLL lock failed\n");
	}
}

void test_osc() {
	LPC_SYSCON->SYSOSCCTRL |= 1; //enable BYPASS

	LPC_SYSCON->SYSPLLCLKSEL = 1; //switch to external oscillator

	LPC_SYSCON->SYSPLLCLKUEN = 1;
	LPC_SYSCON->SYSPLLCLKUEN = 0;
	LPC_SYSCON->SYSPLLCLKUEN = 1;
	while (!(LPC_SYSCON->SYSPLLCLKUEN & 0x01)); //wait until updated

	LPC_SYSCON->SYSPLLCTRL = 0x25; //input clk 16MHZ, out 48MHZ

	LPC_SYSCON->CLKOUTSEL = 3;
	LPC_SYSCON->CLKOUTUEN =1;
	LPC_SYSCON->CLKOUTUEN =0;
	LPC_SYSCON->CLKOUTUEN =1;
	LPC_SYSCON->CLKOUTDIV = 1;

	lpc11::IOCon::setPinFunc(0, 1, 1);
}

void tick() {
	if(!progPin::read()) {
		NVIC_SystemReset();
	}
}

void idle() {

	uint8_t c;
	uart.read(c);
	if(c=='r') {
		NVIC_SystemReset();
	}

	__WFI();
}


int main() {
	irqPin::setInput(lpc::InputType::PULLDOWN);

	usbConnect::setOutput(false);

	lpc11::SysTickTimer::enable();
	lpc11::SysTickTimer::attachInterrupt(tick);

	xpcc::Random::seed();

	lpc::SpiMaster0::configurePins(lpc::SpiMaster0::MappingSck::PIO0_10, false);
	lpc::SpiMaster0::initialize(lpc::SpiMaster0::Mode::MODE_0,
			lpc::SpiMaster0::Presacler::DIV002, 3);

	ledRed::setOutput(false);
	ledGreen::setOutput(false);

	XPCC_LOG_DEBUG .printf("Hello\n");
	NVIC_EnableIRQ((IRQn_Type)0);
	rf230drvr.init();
	rf230drvr.setCLKM(rf230::CLKMClkCmd::CLKM_8MHz, true);

	XPCC_LOG_DEBUG .printf("Core freq %d\n", SystemCoreClock);

	usbclk_init();

	usbConnect::setOutput(true);
	usb.connect();

	NVIC_SetPriority(USB_IRQn, 4);

	xpcc::TickerTask::tasksRun(idle);

}
