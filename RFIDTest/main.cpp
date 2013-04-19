/*
 * main.cpp
 *
 *  Created on: Feb 28, 2013
 *      Author: walmis
 */
#include <lpc17xx_nvic.h>
#include <lpc17xx_uart.h>
#include <lpc17xx_pinsel.h>
#include <lpc17xx_clkpwr.h>

#include <xpcc/architecture.hpp>
#include <xpcc/workflow.hpp>
#include <xpcc/debug.hpp>

#include <xpcc/driver/connectivity/wireless/at86rf230.hpp>


using namespace xpcc;

const char fwversion[16] __attribute__((used, section(".fwversion"))) = "v0.11";

#include "pindefs.hpp"

class UARTDevice : public IODevice {

public:
	UARTDevice(int baud) {
		setBaud(baud);
	}

	void setBaud(uint32_t baud) {

		CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_UART0, CLKPWR_PCLKSEL_CCLK_DIV_1);

		UART_CFG_Type cfg;
		UART_FIFO_CFG_Type fifo_cfg;

		UART_ConfigStructInit(&cfg);
		cfg.Baud_rate = baud;

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
	flush(){
		while(!(LPC_UART0->LSR & UART_LSR_TEMT));

	};

	/// Read a single character
	bool read(char& c) {
		if(!(LPC_UART0->LSR & 0x01)) {
			return false;
		}
		c = UART_ReceiveByte(LPC_UART0);
		return true;
	}
};

xpcc::rf230::Driver<xpcc::lpc::SpiMaster1, radioRst, radioSel, radioSlpTr, radioIrq> rf230drvr;

xpcc::lpc17::USBSerial usb(0xffff, 0x1234);


void boot_jump( uint32_t address ){
   __asm("LDR SP, [R0]\n"
   "LDR PC, [R0, #4]");
}

class NullDevice: public xpcc::IODevice {
public:
	void write(char c) {

	}
	void write(const char* str) {
	}

	void flush() {

	}

	/// Read a single character
	bool read(char& c) {
		return false;
	}
};

//xpcc::IOStream stdout(device);

UARTDevice uart(9600);

//xpcc::log::Logger xpcc::log::debug(device);

NullDevice nulldev;

xpcc::IOStream stdout(usb);

xpcc::log::Logger xpcc::log::debug(usb);

xpcc::log::Logger xpcc::log::info(usb);

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

#include "rfid.hpp"

RFID rfid(uart);

xpcc::PeriodicTimer<> t(20);
void idle() {
	if(t.isExpired()) {
		//XPCC_LOG_DEBUG .printf("p\n");
		if(!rfid.isConnected()) {

			uart.setBaud(460800);

			if(!rfid.init()) {
				XPCC_LOG_DEBUG .printf("init at 4860800 baud failed\n");
				uart.setBaud(9600);
				if(!rfid.init()) {
					XPCC_LOG_DEBUG .printf("init at 9600 failed\n");
				} else {
					rfid.setBaudRate(BAUD_460800);
					uart.setBaud(460800);
				}
			}

		} else {

			if(rfid.isCard()) {
				XPCC_LOG_DEBUG .printf("card found %d\n", __get_IPSR());

				XPCC_LOG_DEBUG .printf("read %d\n", rfid.readCardSerial());
				XPCC_LOG_DEBUG .dump_buffer(rfid.serNum, 7);

				rfid.halt();
			}
		}


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

	stdout.printf("Labas\n");

	NVIC_SetPriority(USB_IRQn, 5);




	TickerTask::tasksRun(idle);

}
