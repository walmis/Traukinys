/*
 * pindefs.hpp
 *
 *  Created on: May 6, 2013
 *      Author: walmis
 */

#ifndef PINDEFS_HPP_
#define PINDEFS_HPP_

#include <xpcc/architecture.hpp>

GPIO__OUTPUT(ledRed, 0, 2);
GPIO__OUTPUT(ledGreen, 0, 3);

GPIO__INPUT(progPin, 0, 1);

GPIO__OUTPUT(usbConnect, 0, 21);

GPIO__OUTPUT(rstPin, 0, 14);
GPIO__IO(slpTr, 0, 22);
GPIO__INPUT(irqPin, 0, 6);
GPIO__OUTPUT(selPin, 0, 7);


#endif /* PINDEFS_HPP_ */
