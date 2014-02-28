from WirelessDevice import WirelessDevice
from PySide.QtCore import *
import struct 
import usbradio

class Train(WirelessDevice):
  class Cmd:
    SET_SPEED = 16
    STOP = 17
    BRAKE = 18
    GET_SPEED = 19

    GPIO_DIR = 20
    GPIO_READ = 21
    GPIO_SET = 22

    SPEED_REPORT=23
    RFID_READ = 24
    DEBUG_MSG = 25
	
  def __init__(self, address, radio):
    WirelessDevice.__init__(self, address, radio)
    self.speed = 0
    self.currentSpeed = 0
    
    self.associationChanged.connect(self.onAssocChanged)
    
    self.prev_rfid = None
    
    self.speeds = []
  
  currentSpeedChanged = Signal(int)
  speedChanged = Signal(int)
  beaconReceived = Signal()
  onRfidRead = Signal(str)
  onDebugMsg = Signal(str)
  
  def __repr__(self):
    return "<Train(%02X)>" % self.address
	  
  def onAssocChanged(self):
    if self.associated:
      #restore last used speed
      self.setSpeed(self.speed)
	  
  def setSpeed(self, speed):
    #print self, "setSpeed", speed
    self.radio.sendRequest(self.address, Train.Cmd.SET_SPEED, struct.pack("b", speed), int(usbradio.TxFlags.TX_ENCRYPT | usbradio.TxFlags.TX_ACKREQ))
    self.speed = speed
    self.speedChanged.emit(self.speed)
    
  def pushSpeed(self, speed):
    self.speeds.append(self.speed)
    self.setSpeed(speed)
  
  def popSpeed(self):
    if len(self.speeds):
      self.setSpeed(self.speeds.pop(-1))
    
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
      
      self.prev_rfid = code

  def onBeacon(self, data):
    if not self.associated:
      self.associate()

    self.beaconReceived.emit() 
