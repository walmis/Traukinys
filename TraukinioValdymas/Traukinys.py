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
from Stendas.RFIDTag import RFIDTag

import time

from traukinys_ui import Ui_Traukinys

app = None

from Stendas import Stendas

from Train import Train


class RadioDelegate(QObject):
    deviceAdded = Signal(object)
    deviceEvent = Signal(object)
    beaconEvent = Signal(int, str, str)
    

class Radio(usbradio.TinyRadioProtocol):
	def __init__(self):
		usbradio.TinyRadioProtocol.__init__(self)
		
		self.devices = {}
		
		self.signals = RadioDelegate()
	
	def beaconHandler(self, address, beacon):
		#print "beacon: %04x" % address, str(beacon.name) + " [" + str(beacon.data).encode('hex_codec') + "]"
		
		beacon_data = str(beacon.data).encode('hex_codec')
		
		name = str(beacon.name).strip("\0")
		
		if not address in self.devices:
		  device = WirelessDevice(address, self)
		  self.devices[address] = device

		device = self.devices[address]
		device.beacon_name = name
		device.beacon_data = beacon_data
		device.onBeacon(beacon)
		
		self.signals.beaconEvent.emit(address, name, beacon_data)  

	#create a new device of type: type, which is a subclass of WirelessDevice
	#return newly created object
	def addDeviceHandler(self, address, devclass):
	  if type(self.devices[address]) != devclass:
	    old = self.devices[address]
	    
	    #transform generic WirelessDevice into a specific device
	    device = self.devices[address] = devclass(address, self)
	    device.beacon_data = old.beacon_data
	    device.beacon_name = old.beacon_name
	    self.signals.deviceAdded.emit(self.devices[address]) 
	    
	  return self.devices[address]
	  
		
	def eventHandler(self, address, event):
		if address in self.devices:
			self.devices[address].onEvent(event)
			self.signals.deviceEvent.emit(self.devices[address])
		
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
  lock = QMutex()

  def do_GET(self):
    #orig_stdout = sys.stdout
    #sys.stdout = sys.stderr
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
    #print "request", self.client_address[0]
    self.send_response(200)
    self.send_header("Content-type", "text/json")
    self.send_header("Connection", "close")
    self.end_headers()
    
    data = parse_qs(urlparse(self.path).query)
    
    json_data = {}
    
    json_data["traukiniai"] = {}
    
    if self.path == "/?getstatus":
      Handler.lock.lock()
      try:
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
	  if name.startswith("F"):
	    iesmas = app.stendas.iesmai[name]
	    json_data["iesmai"]["%X.%d" % (iesmas.address, iesmas.port)] = {
		"state": int(iesmas.state)
	      }
      except Exception, e:
	print e
      
      Handler.lock.unlock()
      
      self.wfile.write(json.dumps(json_data, sort_keys=True))
      self.wfile.write("\n")
      
    else:
      Handler.lock.lock()
      try:
	if "switch" in data and "state" in data:
	  app.stendas.iesmai[data["switch"][0]].setState(int(data["state"][0]))
	
	if "tr" in data and "speed" in data:
	  #print data["tr"]
	  #print data["speed"]
	  
	  t = app.trains[int(data["tr"][0])]
	  
	  if t.train:
	    t.train.setSpeed(int(data["speed"][0]))
      except Exception, e:
	print e
	
      Handler.lock.unlock()
      
    self.wfile.write("")
    self.wfile.close()
    #sys.stdout = orig_stdout
    
  def log_message(self, format, *args):
    #print "log"
    return

import PySideKick
import PySideKick.Console

class OutputWriter:
    def __init__(self, textArea) :
    	self.textArea = textArea

    def write(self, text) :
      self.textArea.appendPlainText(text)

class App(QApplication):
	def __init__(self):
		QApplication.__init__(self, sys.argv)
	
		a = Radio()
		a.init()

		a.setAddress(0x6565)
		a.setPanId(0x1234)

		driver = a.getDriver()
		driver.rxOn()
		
		a.signals.deviceAdded.connect(self.onRadioDeviceAdded)
		a.signals.beaconEvent.connect(self.onRadioBeaconEvent)

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
		  if name.startswith("F"):
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
		
		
		self.debug_window = QMainWindow(parent=self.interface)
		self.debug_window.setWindowTitle("Console")


		console = PySideKick.Console.QPythonConsole(locals=locals())
		self.debug_window.setCentralWidget(console)
		self.debug_window.setGeometry(0, 0, 640, 480)
		
		sys.stdout = console

		self.debug_window.show()
		

		
	def onRadioBeaconEvent(self, address, bcn_name, bcn_data):
	  if bcn_name == "Traukinys":
	    train = self.radio.addDeviceHandler(address, Train)
		  
	  if bcn_name == "WirelessUART" and bcn_data == "0102030405060708":
	    uart = self.radio.addDeviceHandler(address, WirelessUART)
		
	def onRadioDeviceAdded(self, device):
	  for train in self.trains:
	    if train.id == device.beacon_data and device.beacon_name == "Traukinys":
	      train.assign(device)
	  
		
	def on_speed_changed(self, slider):
		print self.radio.devices
		print self.radio.devices.values()[0].setSpeed(slider)
		

		
	def timerEvent(self, event):
	  self.radio.poll()
	  


		


if __name__ == "__main__":
  app = App()
  app.exec_()


	
#keyboard()
