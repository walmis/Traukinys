import usbradio
import sys

from PySide.QtUiTools import QUiLoader
from PySide.QtCore import *
from PySide.QtGui import *
import struct

START  =    16
STOP   =    17
BRAKE  =    18
SET_SPEED = 19
REVERSE   = 20
FORWARD   = 21

GET_SPEED = 22

GPIO_DIR  = 23
GPIO_READ = 24
GPIO_SET  = 25

SPEED_REPORT = 26

import time
class Radio(usbradio.TinyRadioProtocol):
	
	neighbors = {}		
	
	def beaconHandler(self, address, beacon):
		print "beacon: %04x" % address, str(beacon.name) + " [" + str(beacon.data).encode('hex_codec') + "]"
		
		name = str(beacon.name).strip("\0")
		
		if name == "Traukinys" and not self.isAssociated(address):
			self.associate(address)
		
	def eventHandler(self, address, event):
		print "event", address, event
		
		if event == usbradio.EventType.ASSOCIATION_EVENT:
			self.sendRequest(0x8980, START, "", int(usbradio.TxFlags.TX_ENCRYPT))
		
	def dataHandler(self, frame, header, address, data):
		if header.req_id == SPEED_REPORT:
			print address, struct.unpack("I", data)[0]
		

class App(QApplication):
	def __init__(self):
		QApplication.__init__(self, sys.argv)
	
		a = Radio()
		a.init()

		a.setAddress(0x6565)
		a.setPanId(0x1234)

		driver = a.getDriver()
		driver.rxOn()

		self.radio = a
		
		loader = QUiLoader()
		
		self.interface = loader.load("interface.ui")
		
		self.interface.show()
		
		self.interface.dial.valueChanged.connect(self.on_speed_changed)
		
		self.prev_speed = 0
		
		self.startTimer(0)
		
	def on_speed_changed(self, slider):
		print "x", slider
		self.setSpeed(slider)
		
	def setSpeed(self, speed):
		if speed < 0 and self.prev_speed >= 0:
				self.radio.sendRequest(0x8980, REVERSE, "", int(usbradio.TxFlags.TX_ENCRYPT))
				print "reverse"
		if speed > 0 and self.prev_speed <= 0:
				self.radio.sendRequest(0x8980, FORWARD, "", int(usbradio.TxFlags.TX_ENCRYPT))
				print "forward"
				
		self.radio.poll()	
				
		self.prev_speed = speed
			
		speed = abs(speed)	
		a = struct.pack("b", speed)
		self.radio.sendRequest(0x8980, SET_SPEED, a, int(usbradio.TxFlags.TX_ENCRYPT))
		
	def start(self):
		print "start"	
		a = struct.pack("b", 20)
		print str(a)
		
		self.radio.sendRequest(0x8980, SET_SPEED, a, int(usbradio.TxFlags.TX_ENCRYPT))
		time.sleep(0.2)
		self.radio.poll()
		self.radio.sendRequest(0x8980, START, "", int(usbradio.TxFlags.TX_ENCRYPT))
		
	def timerEvent(self, event):
		
		self.radio.poll()
		
		

def keyboard(banner=None):
    import code, sys

    # use exception trick to pick up the current frame
    try:
        raise None
    except:
        frame = sys.exc_info()[2].tb_frame.f_back

    # evaluate commands in current namespace
    namespace = frame.f_globals.copy()
    namespace.update(frame.f_locals)

    code.interact(banner=banner, local=namespace)


app = App()
app.exec_()


	
#keyboard()
