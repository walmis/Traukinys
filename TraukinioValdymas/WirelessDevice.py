from PySide.QtCore import *
import usbradio

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
      print self, "ASSOCIATED"
      self.associated = True
      self.associationChanged.emit()
    elif event == usbradio.EventType.DISASSOCIATION_EVENT:
      print self, "DISASSOCIATED"
      self.associated = False
      self.associationChanged.emit()
    elif event == usbradio.EventType.ASSOCIATION_TIMEOUT:
      print self, "ASSOCIATION_TIMEOUT"
      self.associationChanged.emit()
		  
  def onBeacon(self, data):
    pass
  
  def onData(self, frame, header, data):
    print "data received", "len:", len(data)
    print repr(data)
	  
  def associate(self):
    self.radio.associate(self.address) 
