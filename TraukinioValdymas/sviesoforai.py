import serial

ser = serial.Serial("/dev/ttyUSB0", 38400, timeout=0)

import sys
import random
import time
count = 0

from PySide.QtCore import *
from PySide.QtGui import *

class Irenginys(QObject):
  def __init__(self, address):
    QObject.__init__(self)  
    self.address = address
    self.data = 0
      
  def send(self, data):
    print "send", self.address, data
    ser.write("\x55")
    ser.write(chr(self.address))
    ser.write(chr(data))
    
    self.data = data
    
class Ruozas(QObject):
  def __init__(self, nr, slow=False):
    QObject.__init__(self)
    self._busy = False
    self.nr = nr
    self.slow = slow
    
    self.end0 = None
    self.end1 = None
    
  def __str__(self):
    return " ".join(("Ruozas", str(self.nr)+":", str(self.end0), " <--> ", str(self.end1)))
    
  def addEnd(self, iesmas):
    if not self.end0:
      self.end0 = iesmas
    elif not self.end1:
      self.end1 = iesmas
    else:
      print "cannot and end node, check connections"
    
  def getBusy(self):
    return self._busy
  
  def setBusy(self, busy):
    self._busy = busy
    self.busyChanged.emit()
    
  def nextSwitch(self):
    pass
   
  busyChanged = Signal() 
  busy = Property(bool, getBusy, setBusy, notify=busyChanged)
    
class Iesmas(Irenginys):
  data = {}
  
  def __init__(self, address, port, in_ruozas, out_ruozas1, out_ruozas2):
    Irenginys.__init__(self, address)
    self.port = port # iesmo portas prie valdiklio
    if not address in Iesmas.data:
      Iesmas.data[address] = 0
    
    self._state = 0
    
    self.in_ruozas = in_ruozas
    self.ruozas1 = out_ruozas1
    self.ruozas2 = out_ruozas2
    
    self.in_ruozas.addEnd(self)
    self.ruozas1.addEnd(self)
    self.ruozas2.addEnd(self)
    
  def __str__(self):
    return " ".join(("Iesmas", hex(self.address)+"."+str(self.port), "(%d)" % self._state))
  
  def getRoad(self):
    if self._state == 0:
      return self.ruozas1
    else:
      return self.ruozas2
    
  def next(self):
    road = self.getRoad()
    if road.end0 == self:
      return road.end1
    elif road.end1 == self:
      return road.end0
    else:
      return None
    
  def prev(self):
    road = self.in_ruozas
    if road.end0 == self:
      return road.end1
    elif road.end1 == self:
      return road.end0
    else:
      return None   
    
    
  
  #kai iesmas i sona == 1
  #iesmas tiesiai == 0 
  def getState(self):
    return self._state
  
  def setState(self, state):
    print self, "setState", state 
    Iesmas.data[self.address] &= ~(1<<self.port)
    Iesmas.data[self.address] |= (int(state) << self.port)
    self.send(Iesmas.data[self.address])
    
    self._state = state
    self.stateChanged.emit()
  
  stateChanged = Signal()
  state = Property(int, getState, setState, notify=stateChanged)
    
class Sviesoforas2(Irenginys):
  def __init__(self, address, iesmas, invert=False):
    Irenginys.__init__(self, address) 
    
    self.iesmas = iesmas
    self.invert = invert
    
    self.onIesmasStateChanged()
    
    if iesmas:
      self.iesmas.stateChanged.connect(self.onIesmasStateChanged)
    
  def onIesmasStateChanged(self):
    print "state changed"
    state = self.iesmas.state
    if self.invert:
      state = not state
      
    if state:
      self.send(0b01)
    else:
      self.send(0b10)
    
  #def getState(self):
    #return self._state
  
  #def setState(self, state):
    #print self, "setState", state  
    #self.send(int(state) << self.port)
    #self._state = state
  
  #stateChanged = Signal()
  #state = Property(int, getState, setState, notify=stateChanged)
    
class RFIDTag(QObject):
  def __init__(self, id, trigger):
    QObject.__init__(self)
    
    
    
  tagPassed = Signal()

class Sviesoforas4(Irenginys):
  class State:
    ZALIA = 0
    RAUDONA = 1
    GELTONA = 2 #geltona virsutine
    GELTONA_MIRKS = 3 #geltona virsutine mirksinti
    GELTONA2 = 4 #dvi geltonos sviecia
    GELTONA2_MIRKS1 = 5 #apatine geltona sviecia, virsutine mirksi
    
  def __init__(self, address, iesmas):
    Irenginys.__init__(self, address)
    
    self.state = Sviesoforas4.State.ZALIA
    self.blink_tm = self.startTimer(800)
    
    
  def setState(self, state):
    if state == Sviesoforas4.State.ZALIA:
      self.send(0b0010)
      
    elif state == Sviesoforas4.State.RAUDONA:
      self.send(0b0100)
      
    elif state == Sviesoforas4.State.GELTONA:
      self.send(0b0001)
      
    elif state == Sviesoforas4.State.GELTONA_MIRKS:
      self.send(0b0001)
      
    elif state == Sviesoforas4.State.GELTONA2:
      self.send(0b1001)
    
    elif state == Sviesoforas4.State.GELTONA2_MIRKS1:
      self.send(0b1001)
    else:
      print self, "setState: unknown state"
      
    self._state = state

  
  def getState(self):
    return self._state
  
  def timerEvent(self, event):
    if event.timerId() == self.blink_tm:
      if self._state == Sviesoforas4.State.GELTONA_MIRKS or \
	 self._state == Sviesoforas4.State.GELTONA2_MIRKS1:
	self.send(self.data ^ 0b0001)
	
    
  stateChanged = Signal()
  state = Property(int, getState, setState, notify=stateChanged)
    

class Sviesoforas3(Irenginys):
  def __init__(self, address):
    Irenginys.__init__(self, address)
    
    

  
                    
class App(QApplication):
  def __init__(self):
    QApplication.__init__(self, sys.argv)
    
    self.ruozai = \
    [
      Ruozas(0),
      Ruozas(1),
      Ruozas(2),
      Ruozas(3),
      Ruozas(4),
      Ruozas(5),
      Ruozas(6),
      Ruozas(7),
    ]
    
    self.iesmai = {
	"F0.0": Iesmas(0xF0, 0, self.ruozai[6], self.ruozai[7], self.ruozai[1]),
	"F0.1": Iesmas(0xF0, 1, self.ruozai[7], self.ruozai[2], self.ruozai[3]),
	
	"F1.0": Iesmas(0xF1, 0, self.ruozai[0], self.ruozai[1], self.ruozai[4]),
	"F1.1": Iesmas(0xF1, 1, self.ruozai[4], self.ruozai[2], self.ruozai[3]),
    }
	
    self.sviesoforai = {
      0x07 : Sviesoforas2(0x07, self.iesmai["F0.0"], invert=True),
      0x08 : Sviesoforas2(0x08, self.iesmai["F0.1"], invert=True),
      0x23 : Sviesoforas4(0x23, self.iesmai["F0.0"])
    }
    
    for r in self.ruozai:
      print r
    print self.iesmai["F0.0"]
    self.iesmai["F0.0"].setState(0)
    
    #print self.iesmai["F0.0"]
    #print self.iesmai["F0.0"].next()
    #self.iesmai["F0.0"].setState(0)
    state = 0
    while 1:
      time.sleep(2)
      state = not state
      self.iesmai["F0.1"].setState(state)
      
    
    #print self.iesmai["F0.0"]
    #print self.iesmai["F0.0"].next()
    #print self.iesmai["F0.0"].next().next()
    #print self.iesmai["F0.0"].next().next().prev()

app = App()
app.exec_()