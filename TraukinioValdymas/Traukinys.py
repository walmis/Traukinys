import usbradio
import sys

from PySide.QtUiTools import QUiLoader
from PySide.QtCore import *
from PySide.QtGui import *
import struct
import json

from httpserver import HttpServer

from WirelessDevice import WirelessDevice
from WirelessUART import WirelessUART
from RFIDTag import RFIDTag

import time

from traukinys_ui import Ui_Traukinys

app = None

import Stendas

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
    
    self.prev_rfid = None
  
  currentSpeedChanged = Signal(int)
  speedChanged = Signal(int)
  beaconReceived = Signal()
  onRfidRead = Signal(str)
  onDebugMsg = Signal(str)
	  
  def onAssocChanged(self):
    if self.associated:
      #restore last used speed
      self.setSpeed(self.speed)
      
      for train in app.trains:
	if train.id == self.beacon_data:
	  train.assign(self)
	  
  def setSpeed(self, speed):
    self.radio.sendRequest(self.address, Train.Cmd.SET_SPEED, struct.pack("b", speed), int(usbradio.TxFlags.TX_ENCRYPT | usbradio.TxFlags.TX_ACKREQ))
    self.speed = speed
    self.speedChanged.emit(self.speed)
    
  def updateSpeed(self):
    self.radio.sendRequest(self.address, Train.Cmd.GET_SPEED, "", int(usbradio.TxFlags.TX_ENCRYPT | usbradio.TxFlags.TX_ACKREQ))
  
  def brake(self):
    self.radio.sendRequest(self.address, Train.Cmd.BRAKE, "", int(usbradio.TxFlags.TX_ENCRYPT | usbradio.TxFlags.TX_ACKREQ))
  
  def stop(self):
    self.setSpeed(0)

  def onResponse(self, frame, request, data):
    pass

  def onData(self, frame, header, data):
	  
    if header.req_id == Train.Cmd.SPEED_REPORT:
      self.currentSpeed = struct.unpack("I", data)[0]
      self.currentSpeedChanged.emit(self.currentSpeed)
	    
    elif header.req_id == Train.Cmd.DEBUG_MSG:
      #print "\033[1;31m", data, "\033[1;m",
      self.onDebugMsg.emit(data)
	    
    elif header.req_id == Train.Cmd.RFID_READ:
      #print "Read rfid:", data.encode("hex")
      code = data.encode("hex")
      self.onRfidRead.emit(code)
      RFIDTag.trigger(code, self)
      
      self.prev_rfid = code

  def onBeacon(self, data):
    if not self.associated:
      self.associate()

    self.beaconReceived.emit()


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
				self.signals.deviceAdded.emit(self.devices[address])
			
			self.devices[address].beacon_name = name
			self.devices[address].beacon_data = beacon_data
			self.devices[address].onBeacon(beacon)
			
			
			
		if name == "WirelessUART" and beacon_data == "0102030405060708":
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



class IesmoMygtukas(QToolButton):
  def __init__(self, iesmas):
      QToolButton.__init__(self)
      
      self.setGeometry(QRect(0, 0, 64, 64))
      sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
      sizePolicy.setHorizontalStretch(0)
      sizePolicy.setVerticalStretch(0)
      sizePolicy.setHeightForWidth(self.sizePolicy().hasHeightForWidth())
      self.setSizePolicy(sizePolicy)
      self.setMinimumSize(QSize(64, 64))
      self.setStyleSheet("background-color: rgba(0, 0, 220, 128); font: 12pt \"Sans Serif\";")
      self.setCheckable(True)
      
      self.iesmas = iesmas
      iesmas.stateChanged.connect(self.onIesmasChanged)
      
      self.clicked.connect(self.onClicked)
      
      if type(iesmas) == Stendas.Iesmas:
	self.setText("%X" % (iesmas.address) + "." + str(iesmas.port))
	
  def onIesmasChanged(self):
    self.setChecked(self.iesmas.state)
  
    if self.iesmas.state:
      self.setStyleSheet("background-color: rgba(220, 0, 0, 128); font: 12pt \"Sans Serif\";")
    else:
      self.setStyleSheet("background-color: rgba(0, 0, 220, 128); font: 12pt \"Sans Serif\";")


  def onClicked(self):
    
    self.iesmas.state = self.isChecked()
      

class TrainUI(QMainWindow, Ui_Traukinys):

  def __init__(self, id, parent=None):
	  super(TrainUI, self).__init__(parent)
	  self.setupUi(self)	
	  self.id = id
	  self.train = None
	  
	  self.dGreitis.valueChanged.connect(self.onSetSpeed)
	  self.bStabis.clicked.connect(self.onBrake)
	  
  def assign(self, train):
	  print "assign", self, train
	  
	  if not self.train:
	    self.train = train

	    train.rssiUpdated.connect(self.onRssiUpdated)
	    train.associationChanged.connect(self.onAssocChanged)
	    train.currentSpeedChanged.connect(self.onTrainSpeedChanged)
	    train.onRfidRead.connect(self.onRfidRead)
	    train.onDebugMsg.connect(self.onDebugMsg)
	    train.speedChanged.connect(self.onSpeedChanged)
	    train.associationChanged.connect(self.onTrainAssociated)
	    
	    if train.associated:
	      self.setEnabled(True)

  def onBrake(self):
    self.train.brake()
	    
  def onTrainAssociated(self):
    if self.train.associated:
      self.setEnabled(True)
      
    else:
      self.setEnabled(False)
	  
  def onSpeedChanged(self):
    if self.train.speed != int(self.dGreitis.value()):
      self.dGreitis.setValue(self.train.speed)
	  
  def onDebugMsg(self, msg):
    self.infoText.appendPlainText(msg)

  def onRfidRead(self, rfid):
    self.infoText.appendPlainText("RFID: " + rfid + "\n")
      
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
	      
		
		
from urlparse import urlparse, parse_qs
from BaseHTTPServer import BaseHTTPRequestHandler
import threading

class Handler(BaseHTTPRequestHandler):

  authorized = []
  lock = threading.Lock()

  def do_GET(self):
  
    #if not self.client_address[0] in Handler.authorized:
      #msgBox = QMessageBox()
      #msgBox.setText("Client %s wants to connect, allow?" % self.client_address[0])
      #msgBox.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
      #msgBox.setDefaultButton(QMessageBox.No)
      #ret = msgBox.exec_()
      #if ret == QMessageBox.Yes:
	#Handler.authorized.append(self.client_address[0])
      #else:
	#self.send_response(403)
	#self.send_header("Content-type", "text/plain")
	#self.send_header("Connection", "close")
	#self.end_headers()	
	#return
  
    self.send_response(200)
    self.send_header("Content-type", "text/json")
    self.send_header("Connection", "close")
    self.end_headers()
    
    data = parse_qs(urlparse(self.path).query)
    
    json_data = {}
    
    json_data["traukiniai"] = {}
    
    
    
    if self.path == "/?getstatus":
      Handler.lock.acquire()
      
      c = 0
      for t in app.trains:
	t = t.train
	  
	if t:  
	  json_data["traukiniai"][str(c)] = {
	      "signal": -91 + t.rssi,
	      "speed": t.currentSpeed,
	      "active": bool(t.associated)
	    }
	else:
	  json_data["traukiniai"][str(c)] = {
	    "signal": -91,
	    "speed": 0,
	    "active": False
	  }
	
	c+=1
	 
      json_data["iesmai"] = {}
	
      for name in sorted(app.stendas.iesmai.keys()):
	iesmas = app.stendas.iesmai[name]
	json_data["iesmai"]["%X.%d" % (iesmas.address, iesmas.port)] = {
	    "state": int(iesmas.state)
	  }
      
      Handler.lock.release()
      self.wfile.write(json.dumps(json_data, sort_keys=True))
 
      self.wfile.write("\n")
      
    else:
      Handler.lock.acquire()
      
      if "switch" in data and "state" in data:
	app.stendas.iesmai[data["switch"][0]].state = int(data["state"][0])
      
      if "tr" in data and "speed" in data:
	print data["tr"]
	print data["speed"]
	
	t = app.trains[int(data["tr"][0])]
	
	if t.train:
	
	  t.train.setSpeed(int(data["speed"][0]))
      
      Handler.lock.release()
      
    self.wfile.write("")
    self.wfile.close()
    
  def log_message(self, format, *args):
    return

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
		
		self.serial = None
		
		#self.interface.dial.valueChanged.connect(self.on_speed_changed)
		
		self.trains = []
		
		self.train_blue = TrainUI("d8014443a76ce3b8");
		self.train_blue.dGreitis.setStyleSheet("background: rgba(0, 25, 255, 128)")

		self.train_red = TrainUI("e73e4443ca6be3b8");
		self.train_red.dGreitis.setStyleSheet("background: rgba(255, 0, 0, 128)")

		self.train_green = TrainUI("dc0844430470e3b8");
		self.train_green.dGreitis.setStyleSheet("background: rgba(0, 255, 0, 128)")
		
		self.trains.append(self.train_blue)
		self.trains.append(self.train_green)
		self.trains.append(self.train_red)
		
		self.stendas = Stendas.Stendas(self)
		
		self.iesmai = []
		
		
		for name, iesmas in sorted(self.stendas.iesmai.items()):
		  self.iesmai.append( IesmoMygtukas(iesmas) )
		
		cnt = 0
		for i in self.iesmai:
		  self.interface.iesmai.insertWidget(cnt, i)
		  cnt+=1

		
		for i in self.trains:
		  self.interface.traukiniai.addWidget(i)
		
		self.prev_speed = 0
		
		self.polltm = self.startTimer(0)
		
		
		self.httpserver = HttpServer(Handler)
		
		self.synctime = time.time()
	      
		
	def on_speed_changed(self, slider):
		print self.radio.devices
		print self.radio.devices.values()[0].setSpeed(slider)
		

		
	def timerEvent(self, event):
	  self.radio.poll()
	  


		


if __name__ == "__main__":
  app = App()
  app.exec_()


	
#keyboard()
