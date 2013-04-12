import usbradio
import sys

from PySide.QtUiTools import QUiLoader
from PySide.QtCore import *
from PySide.QtGui import *
import struct



import time

class WirelessDevice(QObject):
	associationChanged = Signal()

	def __init__(self, address):
		QObject.__init__(self)
		self.address = address
		self.associated = False
		
	def onEvent(self, event):
		if event == usbradio.EventType.ASSOCIATION_EVENT:
			self.associated = True
			self.associationChanged.emit()
		elif event == usbradio.EventType.DISASSOCIATION_EVENT:
			self.associated = False
			self.associationChanged.emit()
			
	def onBeacon(self, data):
		pass
		
	def associate(self):
		self.radio.associate(self.address)

class Train(WirelessDevice):
	class Cmd:
		SET_SPEED = 16
		STOP =17
		BRAKE =18
		GET_SPEED=19

		GPIO_DIR=20
		GPIO_READ=21
		GPIO_SET=22

		SPEED_REPORT=23
	
	def __init__(self, address):
		WirelessDevice.__init__(self, address)
		self.speed = 0
		
		self.associationChanged.connect(self.onAssocChanged)
		
	def onAssocChanged(self):
		if self.associated:
			print "Associated"
			#restore last used speed
			self.setSpeed(self.speed)
			
		else:
			print "disassoc"
		
	def setSpeed(self, speed):
		self.radio.sendRequest(self.address, Train.Cmd.SET_SPEED, struct.pack("b", speed), int(usbradio.TxFlags.TX_ENCRYPT))
		self.speed = speed

		
	def onData(self, frame, header, data):
		
		if header.req_id == Train.Cmd.SPEED_REPORT:
			print self.address, struct.unpack("I", data)[0]
	
	def onBeacon(self, data):
		print "train beacon"
		if not self.associated:
			self.associate()

class Radio(usbradio.TinyRadioProtocol):
	
	def __init__(self):
		usbradio.TinyRadioProtocol.__init__(self)
		#QObject.__init__(self)
		
		self.devices = {}
	
	def beaconHandler(self, address, beacon):
		print "beacon: %04x" % address, str(beacon.name) + " [" + str(beacon.data).encode('hex_codec') + "]"
		
		name = str(beacon.name).strip("\0")
		
		if name == "Traukinys":
			if not address in self.devices:
				self.devices[address] =	Train(address)
				self.devices[address].radio = self
			
			self.devices[address].onBeacon(beacon)
		
	def eventHandler(self, address, event):
		if address in self.devices:
			self.devices[address].onEvent(event)
		
	def dataHandler(self, frame, header, address, data):
		if address in self.devices:
			self.devices[address].onData(frame, header, data)

		

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
		print self.radio.devices
		print self.radio.devices.values()[0].setSpeed(slider)
		

		
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
