from PySide.QtCore import *
from PySide.QtGui import *
import Stendas
import sys

class Irenginys(QObject):
  def __init__(self, address):
    QObject.__init__(self)  
    self.address = address
    self.data = 0
    
    self.timer = QTimer()
    self.timer.timeout.connect(self.update)
    self.timer.start(5000)
    
    Stendas.Stendas.inst.UARTConnected.connect(self.onUartConnected)
    
    self.serial = None
    
  def update(self):
    self.send(self.data)
    
  def onUartConnected(self, uart):
    self.serial = uart
    self.serial.associationChanged.connect(self.onAssocChanged)
    
  def onAssocChanged(self):
    self.update()
      
  def send(self, data):
    self.data = data
      
    if self.serial and self.serial.associated:
      #print >> sys.stderr, "send", self.address, data
      
      self.serial.write("\x55")
      self.serial.write(chr(self.address))
      self.serial.write(chr(data))
         
    #else:
    #  print "Nera wireless serial interfeiso!!!"