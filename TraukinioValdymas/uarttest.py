from WirelessUART import WirelessUART

from PySide.QtCore import *
from PySide.QtGui import *

import sys
import usbradio

class RadioDelegate(QObject):
    deviceAdded = Signal(object)
class Radio(usbradio.TinyRadioProtocol):
	def __init__(self):
		usbradio.TinyRadioProtocol.__init__(self)
		#QObject.__init__(self)
		
		self.devices = {}
		
		self.signals = RadioDelegate()
	
	def beaconHandler(self, address, beacon):
		print "beacon: %04x" % address, str(beacon.name) + " [" + str(beacon.data).encode('hex_codec') + "]"
		
		beacon_data = str(beacon.data).encode('hex_codec')
		
		name = str(beacon.name).strip("\0")
		
		if name == "Traukinys":
			if not address in self.devices:
				self.devices[address] =	Train(address)
				self.devices[address].radio = self
			
			self.devices[address].beacon_name = name
			self.devices[address].beacon_data = beacon_data
			self.devices[address].onBeacon(beacon)
			
			self.signals.deviceAdded.emit(self.devices[address])
			
		if name == "WirelessUART":
			if not address in self.devices:
				self.devices[address] =	WirelessUART(address)
				self.devices[address].radio = self
				self.signals.deviceAdded.emit(self.devices[address])
			
			self.devices[address].beacon_name = name
			self.devices[address].beacon_data = beacon_data
			self.devices[address].onBeacon(beacon)	
			
			
		
	def eventHandler(self, address, event):
		if address in self.devices:
			self.devices[address].onEvent(event)
		
	def dataHandler(self, frame, header, address, data):
		if address in self.devices:
			self.devices[address].onData(frame, header, data)
			
	def responseHandler(self, frame, request, data):
	  address = frame.getSrcAddress()
	  
	  if address in self.devices:
	    self.devices[address].onResponse(frame, request, data)
			
	def frameHandler(self, frame):
		m = usbradio.MacFrame(frame)
		address = m.getSrcAddress()
		
		if address in self.devices:
			self.devices[address].rssi = frame.rssi
			self.devices[address].lqi = frame.lqi
			self.devices[address].rssiUpdated.emit()
		
		return True

class App(QApplication):
	def __init__(self):
		QApplication.__init__(self, sys.argv)
		
		self.radio = Radio()
		
		self.radio.init()

		self.radio.setAddress(0x6565)
		self.radio.setPanId(0x1234)

		driver = self.radio.getDriver()
		driver.rxOn()
		
		self.radio.signals.deviceAdded.connect(self.onDeviceAdded)
		
		self.startTimer(0)
		
		self.uart = None
		
		self.button = QPushButton("send")
		self.button.clicked.connect(self.onClicked)
		self.button.show()
		
	def onClicked(self):
	  print "c"
	  
	  #self.radio.sendData(0xFFFF, "test")
	   
	  str = "qwertyuiopasdfghjklzxcvbnm\n"*6
	  
	  self.uart.write(str)
		
	def onDeviceAdded(self, dev):
	  print "added", dev
	  
	  self.uart = dev
		
		
	def timerEvent(self, event):
	  self.radio.poll()
	  
	 
		
		
		
app = App()
app.exec_()
