/*
 * main.cpp
 *
 *  Created on: Feb 28, 2013
 *      Author: walmis
 */
#include <lpc17xx_nvic.h>
#include <lpc17xx_uart.h>
#include <lpc17xx_pinsel.h>

#include <xpcc/architecture.hpp>
#include <xpcc/workflow.hpp>
#include <xpcc/debug.hpp>

#include <xpcc/driver/connectivity/wireless/at86rf230.hpp>

#include "usb_rf_driver.hpp"

using namespace xpcc;

const char fwversion[16] __attribute__((used, section(".fwversion"))) = "v0.11";

#include "pindefs.hpp"

class UARTDevice : public IODevice {

public:
	UARTDevice(int baud) {

		UART_CFG_Type cfg;
		UART_FIFO_CFG_Type fifo_cfg;

		UART_ConfigStructInit(&cfg);
		cfg.Baud_rate = baud;

		UART_Init(LPC_UART0, &cfg);

		PINSEL_CFG_Type PinCfg;

		PinCfg.Funcnum = 1;
		PinCfg.OpenDrain = 0;
		PinCfg.Pinmode = 0;
		PinCfg.Pinnum = 2;
		PinCfg.Portnum = 0;
		PINSEL_ConfigPin(&PinCfg);
		PinCfg.Pinnum = 3;
		PINSEL_ConfigPin(&PinCfg);

		UART_Init(LPC_UART0, &cfg);

		UART_FIFOConfigStructInit(&fifo_cfg);

		UART_FIFOConfig(LPC_UART0, &fifo_cfg);

		UART_TxCmd(LPC_UART0, ENABLE);

	}

	void
	write(char c) {
		while (!(LPC_UART0->LSR & UART_LSR_THRE)) {
		}

		UART_SendByte(LPC_UART0, c);
	};

	void
	flush(){};

	/// Read a single character
	bool read(char& c) {
		return false;
	}
};

xpcc::rf230::Driver<xpcc::lpc::SpiMaster1, radioRst, radioSel, radioSlpTr, radioIrq> rf230drvr;


USBInterface usb;


void boot_jump( uint32_t address ){
   __asm("LDR SP, [R0]\n"
   "LDR PC, [R0, #4]");
}



//xpcc::lpc17::USBSerial device(0xffff);

//xpcc::IOStream stdout(device);

UARTDevice uart(115200);
//xpcc::log::Logger xpcc::log::debug(device);
xpcc::log::Logger xpcc::log::debug(uart);




enum { r0, r1, r2, r3, r12, lr, pc, psr};

extern "C" void HardFault_Handler(void)
{
  asm volatile("MRS r0, MSP;"
		       "B Hard_Fault_Handler");
}

extern "C"
void Hard_Fault_Handler(uint32_t stack[]) {

	//register uint32_t* stack = (uint32_t*)__get_MSP();

	XPCC_LOG_DEBUG .printf("Hard Fault\n");

	XPCC_LOG_DEBUG .printf("r0  = 0x%08x\n", stack[r0]);
	XPCC_LOG_DEBUG .printf("r1  = 0x%08x\n", stack[r1]);
	XPCC_LOG_DEBUG .printf("r2  = 0x%08x\n", stack[r2]);
	XPCC_LOG_DEBUG .printf("r3  = 0x%08x\n", stack[r3]);
	XPCC_LOG_DEBUG .printf("r12 = 0x%08x\n", stack[r12]);
	XPCC_LOG_DEBUG .printf("lr  = 0x%08x\n", stack[lr]);
	XPCC_LOG_DEBUG .printf("pc  = 0x%08x\n", stack[pc]);
	XPCC_LOG_DEBUG .printf("psr = 0x%08x\n", stack[psr]);

	while(1) {
		if(!progPin::read()) {
			for(int i = 0; i < 10000; i++) {}
			NVIC_SystemReset();
		}
	}
}

void sysTick() {
	LPC_WDT->WDFEED = 0xAA;
	LPC_WDT->WDFEED = 0x55;

	if(progPin::read() == 0) {
		//NVIC_SystemReset();

		NVIC_DeInit();

		LPC_WDT->WDFEED = 0x56;

		//boot_jump(0);
	}
}


int main() {
	lpc17::SysTickTimer::enable();
	lpc17::SysTickTimer::attachInterrupt(sysTick);

	greenLed::setOutput(0);
	redLed::setOutput(1);

	xpcc::Random::seed();

	lpc::SpiMaster1::configurePins();
	lpc::SpiMaster1::initialize(lpc::SpiMaster1::Mode::MODE_0,
			lpc::SpiMaster1::Prescaler::DIV002, 4);


	xpcc::PeriodicTimer<> t(500);

	usbConnPin::setOutput(true);
	usb.connect();

	NVIC_SetPriority(USB_IRQn, 5);

	TickerTask::tasksRun();

}
