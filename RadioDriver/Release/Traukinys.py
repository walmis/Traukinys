import usbradio
import sys

from PySide.QtUiTools import QUiLoader
from PySide.QtCore import *
from PySide.QtGui import *
import struct

f = open("rfid.txt", "w")

import time

from traukinys_ui import Ui_Traukinys

app = None

class WirelessDevice(QObject):
	associationChanged = Signal()
	
	rssiUpdated = Signal()
	

	def __init__(self, address):
		QObject.__init__(self)
		self.address = address
		self.associated = False
		
		self.rssi = 0
		self.lqi = 0
		
		self.beacon_data = ""
		self.beacon_name = ""
		
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
		RFID_READ = 24
		DEBUG_MSG = 25
	
	def __init__(self, address):
		WirelessDevice.__init__(self, address)
		self.speed = 0
		self.currentSpeed = 0
		
		self.associationChanged.connect(self.onAssocChanged)
		
	currentSpeedChanged = Signal(int)
	speedChanged = Signal(int)
	beaconReceived = Signal()
		
	def onAssocChanged(self):
		if self.associated:
			print "Associated"
			#restore last used speed
			self.setSpeed(self.speed)
			
			for train in app.trains:
				if train.id == self.beacon_data:
					train.assign(self)
		else:
			print "disassoc"
		
	def setSpeed(self, speed):
		self.radio.sendRequest(self.address, Train.Cmd.SET_SPEED, struct.pack("b", speed), int(usbradio.TxFlags.TX_ENCRYPT))
		self.speed = speed
		self.speedChanged.emit(self.speed)

		
	def onData(self, frame, header, data):
		
		if header.req_id == Train.Cmd.SPEED_REPORT:
			self.currentSpeed = struct.unpack("I", data)[0]
			self.currentSpeedChanged.emit(self.currentSpeed)
			
		elif header.req_id == Train.Cmd.DEBUG_MSG:
			print "\033[1;31m", data, "\033[1;m",
			
		elif header.req_id == Train.Cmd.RFID_READ:
			print "Read rfid:", data.encode("hex")
			f.write("rfid: %s\n" % data.encode("hex"))
			f.flush()
	
	def onBeacon(self, data):
		print "train beacon"
		if not self.associated:
			self.associate()
		
		self.beaconReceived.emit()

class Radio(usbradio.TinyRadioProtocol):
	
	def __init__(self):
		usbradio.TinyRadioProtocol.__init__(self)
		#QObject.__init__(self)
		
		self.devices = {}
	
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
		
	def eventHandler(self, address, event):
		if address in self.devices:
			self.devices[address].onEvent(event)
		
	def dataHandler(self, frame, header, address, data):
		if address in self.devices:
			self.devices[address].onData(frame, header, data)
			
	def frameHandler(self, frame):
		m = usbradio.MacFrame(frame)
		address = m.getSrcAddress()
		
		if address in self.devices:
			self.devices[address].rssi = frame.rssi
			self.devices[address].lqi = frame.lqi
			self.devices[address].rssiUpdated.emit()
		
		return True



class TrainUI(QMainWindow, Ui_Traukinys):

	def __init__(self, id, parent=None):
		super(TrainUI, self).__init__(parent)
		self.setupUi(self)	
		self.id = id
		self.train = None
		
		self.dGreitis.valueChanged.connect(self.onSetSpeed)
		
	def assign(self, train):
		print "assign", self, train
		self.train = train
		
		train.rssiUpdated.connect(self.onRssiUpdated)
		train.associationChanged.connect(self.onAssocChanged)
		train.currentSpeedChanged.connect(self.onTrainSpeedChanged)
		
		self.setEnabled(True)
		
		
	def onRssiUpdated(self):
		self.pSignalas.setProperty("value", -91 + self.train.rssi)
		
	def onAssocChanged(self):
		if not self.train.associated:
			self.setEnabled(False)
			self.pSignalas.setValue(-91)
		else:
			self.setEnabled(True)
			
	def onTrainSpeedChanged(self, speed):
		self.lSpeed.setText("%s" % speed)
		
	def onSetSpeed(self, speed):
		self.train.setSpeed(speed)
		

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
		
		#self.interface.dial.valueChanged.connect(self.on_speed_changed)
		
		self.trains = []
		
		self.train_blue = TrainUI("c13f4443366ce3b8");
		self.train_blue.dGreitis.setStyleSheet("background: rgba(0, 25, 255, 128)")

		self.train_red = TrainUI("e73e4443ca6be3b8");
		self.train_red.dGreitis.setStyleSheet("background: rgba(255, 0, 0, 128)")

		self.train_green = TrainUI("dc0844430470e3b8");
		self.train_green.dGreitis.setStyleSheet("background: rgba(0, 255, 0, 128)")
		
		self.trains.append(self.train_blue)
		self.trains.append(self.train_green)
		self.trains.append(self.train_red)

		
		
		self.interface.traukiniai.addWidget(self.train_green)
		self.interface.traukiniai.addWidget(self.train_red)
		self.interface.traukiniai.addWidget(self.train_blue)
		
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
