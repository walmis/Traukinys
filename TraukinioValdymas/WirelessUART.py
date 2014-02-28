from PySide.QtCore import *
from array import array
from WirelessDevice import WirelessDevice

class WirelessUART(WirelessDevice):
  
  def __init__(self, address, radio):
    WirelessDevice.__init__(self, address, radio)
    self.startTimer(10)
    
    self.tx_buffer = array('B')
    
    self.associationChanged.connect(self.onAssocChanged)
    
  def onBeacon(self, data):
    if not self.associated:
	self.associate()
	    
  def write(self, data):
    for c in data:
      self.tx_buffer.append(ord(c))

    if len(self.tx_buffer) >= 90:
      #print "send", len(self.tx_buffer)
      self.radio.sendData(self.address, self.tx_buffer.tostring())
      
      while len(self.tx_buffer):
	self.tx_buffer.pop(-1)
    
  def onAssocChanged(self):
	pass
      
  def timerEvent(self, event):
   # print "timer"
    if len(self.tx_buffer) > 0:
      #print "send_", len(self.tx_buffer)
      self.radio.sendData(self.address, self.tx_buffer.tostring())
      while len(self.tx_buffer):
	self.tx_buffer.pop(-1)
      #print len(self.tx_buffer) 
