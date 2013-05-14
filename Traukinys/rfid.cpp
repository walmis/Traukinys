/*
 * RFID.cpp - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT.
 * Based on code Dr.Leong   ( WWW.B2CQSHOP.COM )
 * Created by Miguel Balboa, Jan, 2012.
 * Released into the public domain.
 */

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <rfid.hpp>
#include <xpcc/architecture.hpp>

/******************************************************************************
 * User API
 ******************************************************************************/

/**
 * Construct RFID
 * int chipSelectPin RFID /ENABLE pin
 */
RFID::RFID(xpcc::IODevice &dev)
{
	device = &dev;
}
/******************************************************************************
 * User API
 ******************************************************************************/

 bool RFID::isCard()
 {
	unsigned char status;
	unsigned char str[MAX_LEN];

	status = MFRC522Request(PICC_REQIDL, str);
    if (status == MI_OK) {
		return true;
	} else {
		return false;
	}
 }

 bool RFID::setBaudRate(MFRC522Baud baud) {

	return writeMFRC522(SerialSpeedReg, baud);
 }

 bool RFID::select(uint8_t* sn) {

	    unsigned char status;
	    unsigned int unLen;

	    //ClearBitMask(Status2Reg, 0x08);		//TempSensclear
	    //ClearBitMask(CollReg,0x80);			//ValuesAfterColl
		writeMFRC522(BitFramingReg, 0x00);		//TxLastBists = BitFramingReg[2..0]

		uint8_t tmp[MAX_LEN];
	    tmp[0] = PICC_ANTICOLL;
	    tmp[1] = 0x70;
	    memcpy(tmp+2, sn, 5);

	    calculateCRC(tmp, 7, &tmp[7]);
	    XPCC_LOG_DEBUG .dump_buffer(tmp, 9);

	    status = MFRC522ToCard(PCD_TRANSCEIVE, tmp, 9, tmp, &unLen);
	    //XPCC_LOG_DEBUG .printf("select unlen %d\n", unLen);

	    //XPCC_LOG_DEBUG .dump_buffer(tmp, 10);
	    if(tmp[0] == 0x04) {
	    	return MI_OK;
	    }

	    //SetBitMask(CollReg, 0x80);		//ValuesAfterColl=1

	    return status;

 }
 extern xpcc::log::Logger stdout;
 bool RFID::readCardSerial(){
	unsigned char status;
	unsigned char str[MAX_LEN];

	status = anticoll(str, PICC_ANTICOLL);
	//XPCC_LOG_DEBUG .printf("anticoll phase 1: %d\n", status);
	if(status != MI_OK)
		return false;


	status = read(0, str);
	uint8_t csum = 0x88; //CT byte according to datasheet
	int i;
	for(i=0;i<3;i++){
		csum ^= str[i];
	}
	if(csum != str[3])
		return false;

	csum = 0;
	for(i=4;i<8;i++) {
		csum ^= str[i];
	}
	if(str[8] != csum)
		return false;

	//stdout.dump_buffer(str, 16);

	//XPCC_LOG_DEBUG .printf("read status %d\n", status);
	//XPCC_LOG_DEBUG .dump_buffer(str, 16);
	memcpy(serNum, str, 7);

//	XPCC_LOG_DEBUG .dump_buffer(str, 5);
//
//	memcpy(serNum, str+1, 3);
//
//	status = select(str);
//	XPCC_LOG_DEBUG .printf("select status %d\n", status);
//
//
//	status = anticoll(str, PICC_ANTICOLL2);
//	XPCC_LOG_DEBUG .printf("anticoll phase 2: %d\n", status);
//	memcpy(serNum+3, str, 4);

	if (status == MI_OK) {
		return true;
	} else {
		return false;
	}

 }

/******************************************************************************
 * Dr.Leong   ( WWW.B2CQSHOP.COM )
 ******************************************************************************/

bool RFID::init()
{
    //digitalWrite(_NRSTPD,HIGH);
	XPCC_LOG_DEBUG .printf("RFID: init()\n");

	//writeMFRC522(TestPinEnReg, )

	uint8_t ver = readMFRC522(VersionReg);
	if(ver != 0x91 && ver != 0x92) {
		return false;
	}

	//reset();

	//Timer: TPrescaler*TreloadVal/6.78MHz = 24ms

	writeMFRC522(TModeReg, 0x8D);		//Tauto=1; f(Timer) = 6.78MHz/TPreScaler
    writeMFRC522(TPrescalerReg, 0x3E);	//TModeReg[3..0] + TPrescalerReg
    writeMFRC522(TReloadRegL, 30);
    writeMFRC522(TReloadRegH, 0);

	writeMFRC522(TxAutoReg, 0x40);		//100%ASK

	writeMFRC522(ModeReg, 0x3D);		// CRC valor inicial de 0x6363

	//ClearBitMask(Status2Reg, 0x08);	//MFCrypto1On=0
	writeMFRC522(RxSelReg, 0x86);		//RxWait = RxSelReg[5..0]
	writeMFRC522(RFCfgReg, 0x7F);   	//RxGain = 48dB

	antennaOn();		//Abre  la antena

	connected = true;

	return true;
}
void RFID::reset()
{
	writeMFRC522(CommandReg, PCD_RESETPHASE);
}

bool RFID::writeMFRC522(unsigned char addr, unsigned char val)
{

	//XPCC_LOG_DEBUG .printf("Write MFRC522 (0x%02x) -> (0x%02x)\n", val, addr);

	flush();

	device->write((addr)&0x7F);
	device->write(val);

	char c = 0;
	xpcc::Timeout<> t(10);

	while(!device->read(c)) {
		if(t.isExpired()) {
			return false;
		}
	}
	if(c != addr) {
		XPCC_LOG_DEBUG .printf("RFID:writeMFRC522 error\n");
		connected = false;
	}
	return true;
}

void RFID::antennaOn(void)
{
	unsigned char temp;

	temp = readMFRC522(TxControlReg);
	if (!(temp & 0x03))
	{
		setBitMask(TxControlReg, 0x03);
	}
}

/*
 *  Read_MFRC522 Nombre de la función: Read_MFRC522
 *  Descripción: Desde el MFRC522 leer un byte de un registro de datos
 *  Los parámetros de entrada: addr - la dirección de registro
 *  Valor de retorno: Devuelve un byte de datos de lectura
 */
unsigned char RFID::readMFRC522(unsigned char addr)
{
	unsigned char val;

	flush();

	//XPCC_LOG_DEBUG .printf("Read MFRC522 %02x -> ", addr);
	device->write((addr & 0x3f) | 0x80);

	char c;
	xpcc::Timeout<> t(10);
	while(!device->read(c)) {
		if(t.isExpired()) {
			XPCC_LOG_DEBUG .printf("RFID read timeout\n");
			connected = false;
			return 0;
		}
	}
	//XPCC_LOG_DEBUG .printf("%02x\n", c);

	return (unsigned char)c;
}

void RFID::flush() {
	char c;
	while(device->read(c));
}

void RFID::setBitMask(unsigned char reg, unsigned char mask)
{
    unsigned char tmp;
    tmp = readMFRC522(reg);
    writeMFRC522(reg, tmp | mask);  // set bit mask
}

void RFID::clearBitMask(unsigned char reg, unsigned char mask)
{
    unsigned char tmp;
    tmp = readMFRC522(reg);
    writeMFRC522(reg, tmp & (~mask));  // clear bit mask
}

void RFID::calculateCRC(unsigned char *pIndata, unsigned char len, unsigned char *pOutData)
{
    unsigned char i, n;

    clearBitMask(DivIrqReg, 0x04);			//CRCIrq = 0
    setBitMask(FIFOLevelReg, 0x80);			//Claro puntero FIFO
    //Write_MFRC522(CommandReg, PCD_IDLE);

	//Escribir datos en el FIFO
    for (i=0; i<len; i++)
    {
		writeMFRC522(FIFODataReg, *(pIndata+i));
	}
    writeMFRC522(CommandReg, PCD_CALCCRC);

	// Esperar a la finalización de cálculo del CRC
    i = 0xFF;
    do
    {
        n = readMFRC522(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));			//CRCIrq = 1

	//Lea el cálculo de CRC
    pOutData[0] = readMFRC522(CRCResultRegL);
    pOutData[1] = readMFRC522(CRCResultRegM);
}

unsigned char RFID::MFRC522ToCard(unsigned char command,
		unsigned char *sendData, unsigned char sendLen, unsigned char *backData,
		unsigned int *backLen) {

    unsigned char status = MI_ERR;
    unsigned char irqEn = 0x00;
    unsigned char waitIRq = 0x00;
	unsigned char lastBits;
    unsigned char n;
    unsigned int i;

    switch (command)
    {
        case PCD_AUTHENT:		// Tarjetas de certificación cerca
		{
			irqEn = 0x12;
			waitIRq = 0x10;
			break;
		}
		case PCD_TRANSCEIVE:	//La transmisión de datos FIFO
		{
			irqEn = 0x77;
			waitIRq = 0x30;
			break;
		}
		default:
			break;
    }

    writeMFRC522(CommIEnReg, irqEn|0x80);	//De solicitud de interrupción
    clearBitMask(CommIrqReg, 0x80);			// Borrar todos los bits de petición de interrupción
    setBitMask(FIFOLevelReg, 0x80);			//FlushBuffer=1, FIFO de inicialización

	writeMFRC522(CommandReg, PCD_IDLE);	//NO action;Y cancelar el comando

	//Escribir datos en el FIFO
    for (i=0; i < sendLen; i++)
    {
		writeMFRC522(FIFODataReg, sendData[i]);
	}

	//???? ejecutar el comando
	writeMFRC522(CommandReg, command);
    if (command == PCD_TRANSCEIVE)
    {
		setBitMask(BitFramingReg, 0x80);		//StartSend=1,transmission of data starts
	}

    xpcc::Timeout<> t(5);
	// A la espera de recibir datos para completar
    do
    {
		//CommIrqReg[7..0]
		//Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
        n = readMFRC522(CommIrqReg);
    }
    while ((!t.isExpired()) && !(n&0x01) && !(n&waitIRq));
    bool expired = t.isExpired();

    clearBitMask(BitFramingReg, 0x80);			//StartSend=0

    if (!expired)
    {
        if(!(readMFRC522(ErrorReg) & 0x1B))	//BufferOvfl Collerr CRCErr ProtecolErr
        {
            status = MI_OK;
            if (n & irqEn & 0x01)
            {
				status = MI_NOTAGERR;			//??
			}

            if (command == PCD_TRANSCEIVE)
            {
               	n = readMFRC522(FIFOLevelReg);
              	lastBits = readMFRC522(ControlReg) & 0x07;
                if (lastBits)
                {
					*backLen = (n-1)*8 + lastBits;
				}
                else
                {
					*backLen = n*8;
				}

                if (n == 0)
                {
					n = 1;
				}
                if (n > MAX_LEN)
                {
					n = MAX_LEN;
				}

				//??FIFO??????? Lea los datos recibidos en el FIFO
                for (i=0; i<n; i++)
                {
					backData[i] = readMFRC522(FIFODataReg);
				}
            }
        }
        else
        {
			status = MI_ERR;
		}

    }

    //SetBitMask(ControlReg,0x80);           //timer stops
    //Write_MFRC522(CommandReg, PCD_IDLE);

    return status;
}


/*
 *  Nombre de la función: MFRC522_Request
 *  Descripción: Buscar las cartas, leer el número de tipo de tarjeta
 *  Los parámetros de entrada: reqMode - encontrar el modo de tarjeta,
 *			   Tagtype - Devuelve el tipo de tarjeta
 *			 	0x4400 = Mifare_UltraLight
 *				0x0400 = Mifare_One(S50)
 *				0x0200 = Mifare_One(S70)
 *				0x0800 = Mifare_Pro(X)
 *				0x4403 = Mifare_DESFire
 *  Valor de retorno: el retorno exitoso MI_OK
 */
unsigned char  RFID::MFRC522Request(unsigned char reqMode, unsigned char *TagType)
{
	unsigned char status;
	unsigned int backBits;			//   Recibió bits de datos

	writeMFRC522(BitFramingReg, 0x07);		//TxLastBists = BitFramingReg[2..0]	???

	TagType[0] = reqMode;
	status = MFRC522ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

	if ((status != MI_OK) || (backBits != 0x10))
	{
		status = MI_ERR;
	}

	return status;
}

/**
 *  MFRC522Anticoll -> anticoll
 *  Anti-detección de colisiones, la lectura del número de serie de la tarjeta de tarjeta
 *  @param serNum - devuelve el número de tarjeta 4 bytes de serie, los primeros 5 bytes de bytes de paridad
 *  @return retorno exitoso MI_OK
 */
unsigned char RFID::anticoll(unsigned char *serNum, uint8_t cmd)
{
    unsigned char status;
    unsigned char i;
	unsigned char serNumCheck=0;
    unsigned int unLen;


    //ClearBitMask(Status2Reg, 0x08);		//TempSensclear
    //ClearBitMask(CollReg,0x80);			//ValuesAfterColl
	writeMFRC522(BitFramingReg, 0x00);		//TxLastBists = BitFramingReg[2..0]

    serNum[0] = cmd;
    serNum[1] = 0x20;
    status = MFRC522ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK)
	{
		//?????? Compruebe el número de serie de la tarjeta
		for (i=0; i<4; i++)
		{
		 	serNumCheck ^= serNum[i];
		}
		if (serNumCheck != serNum[i])
		{
			status = MI_ERR;
		}
    }

    //SetBitMask(CollReg, 0x80);		//ValuesAfterColl=1

    return status;
}

/*
 * MFRC522Auth -> auth
 * Verificar la contraseña de la tarjeta
 * Los parámetros de entrada: AuthMode - Modo de autenticación de contraseña
                 0x60 = A 0x60 = validación KeyA
                 0x61 = B 0x61 = validación KeyB
             BlockAddr--  bloque de direcciones
             Sectorkey-- sector contraseña
             serNum--,4? Tarjeta de número de serie, 4 bytes
 * MI_OK Valor de retorno: el retorno exitoso MI_OK
 */
unsigned char RFID::auth(unsigned char authMode, unsigned char BlockAddr, unsigned char *Sectorkey, unsigned char *serNum)
{
    unsigned char status;
    unsigned int recvBits;
    unsigned char i;
	unsigned char buff[12];

	//????+???+????+???? Verifique la dirección de comandos de bloques del sector + + contraseña + número de la tarjeta de serie
    buff[0] = authMode;
    buff[1] = BlockAddr;
    for (i=0; i<6; i++)
    {
		buff[i+2] = *(Sectorkey+i);
	}
    for (i=0; i<4; i++)
    {
		buff[i+8] = *(serNum+i);
	}
    status = MFRC522ToCard(PCD_AUTHENT, buff, 12, buff, &recvBits);

    if ((status != MI_OK) || (!(readMFRC522(Status2Reg) & 0x08)))
    {
		status = MI_ERR;
	}

    return status;
}

/*
 * MFRC522Read -> read
 * Lectura de datos de bloque
 * Los parámetros de entrada: blockAddr - dirección del bloque; recvData - leer un bloque de datos
 * MI_OK Valor de retorno: el retorno exitoso MI_OK
 */
unsigned char RFID::read(unsigned char blockAddr, unsigned char *recvData)
{
    unsigned char status;
    unsigned int unLen;

    recvData[0] = PICC_READ;
    recvData[1] = blockAddr;
    calculateCRC(recvData,2, &recvData[2]);
    status = MFRC522ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);
    //XPCC_LOG_DEBUG .printf("recv data %d %d\n", status, unLen);

    if ((status != MI_OK) || (unLen != 0x90))
    {
        status = MI_ERR;
    }

    return status;
}

/*
 * MFRC522Write -> write
 * La escritura de datos de bloque
 * blockAddr - dirección del bloque; WriteData - para escribir 16 bytes del bloque de datos
 * Valor de retorno: el retorno exitoso MI_OK
 */
unsigned char RFID::write(unsigned char blockAddr, unsigned char *writeData)
{
    unsigned char status;
    unsigned int recvBits;
    unsigned char i;
	unsigned char buff[18];

    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;
    calculateCRC(buff, 2, &buff[2]);
    status = MFRC522ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

    if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
    {
		status = MI_ERR;
	}

    if (status == MI_OK)
    {
        for (i=0; i<16; i++)		//?FIFO?16Byte?? Datos a la FIFO 16Byte escribir
        {
        	buff[i] = *(writeData+i);
        }
        calculateCRC(buff, 16, &buff[16]);
        status = MFRC522ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);

		if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
        {
			status = MI_ERR;
		}
    }

    return status;
}


/*
 * MFRC522Halt -> halt
 * Cartas de Mando para dormir
 * Los parámetros de entrada: Ninguno
 * Valor devuelto: Ninguno
 */
void RFID::halt()
{
	unsigned char status;
    unsigned int unLen = 0;
    unsigned char buff[4];

    buff[0] = PICC_HALT;
    buff[1] = 0;
    calculateCRC(buff, 2, &buff[2]);

   // XPCC_LOG_DEBUG .dump_buffer(buff, 4);

    status = MFRC522ToCard(PCD_TRANSCEIVE, buff, 4, buff, &unLen);
    //XPCC_LOG_DEBUG .printf("halt status %d %d\n", status, unLen);
}
