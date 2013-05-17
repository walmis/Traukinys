import serial

import sys
import random
import time
count = 0

from PySide.QtCore import *
from PySide.QtGui import *

import json

class Irenginys(QObject):
  def __init__(self, address):
    QObject.__init__(self)  
    self.address = address
    self.data = 0
    
    self.timer = QTimer()
    self.timer.timeout.connect(self.update)
    self.timer.start(5000)
    
    
  def update(self):
    self.send(self.data)
      
  def send(self, data):
    self.data = data
    
    if Stendas.app.serial:
      print "send", self.address, data
      Stendas.app.serial.write("\x55")
      Stendas.app.serial.write(chr(self.address))
      Stendas.app.serial.write(chr(data))
      
      
    else:
      print "Nera wireless serial interfeiso!!!"
    
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
    
  def next(self, kelias=None):
    if not kelias:
      road = self.getRoad()
      if road.end0 == self:
	return road.end1
      elif road.end1 == self:
	return road.end0
      else:
	return 
    else:
      #TODO
      pass
    
  def prev(self):
    road = self.in_ruozas
    if road.end0 == self:
      return road.end1
    elif road.end1 == self:
      return road.end0
    else:
      return None  
    
  def update(self):
    pass
    
    
  
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
  
  
class SlaveIesmas(Iesmas):
  
  def __init__(self, masterIesmas, port, in_ruozas, out_ruozas1, out_ruozas2):
    QObject.__init__(self)
    
    self.in_ruozas = in_ruozas
    self.ruozas1 = out_ruozas1
    self.ruozas2 = out_ruozas2
    
    self.in_ruozas.addEnd(self)
    self.ruozas1.addEnd(self)
    self.ruozas2.addEnd(self)  
    
    self._state = 0
    
    self.master = masterIesmas
    
    self.address = masterIesmas.address
    self.port = port
    
    masterIesmas.stateChanged.connect(self.onMasterChanged)
  
  def __str__(self):
    return " ".join(("Iesmas", hex(self.address)+"."+str(self.master.port)+"."+str(self.port), "(%d)" % self._state)) 
 
  def onMasterChanged(self):
    self.state = self.master.state
    
  def getState(self):
    return self.master.state
  
  def update(self):
    print "---update"
  
  def setState(self, state):
    print "---setstate"
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
    
  def __init__(self, address, iesmas, ruozas=None, evalfn=None):
    Irenginys.__init__(self, address)
    
    self.state = Sviesoforas4.State.ZALIA
    self.blink_tm = self.startTimer(800)
    
    self.iesmas = iesmas
    self.ruozas = ruozas
    
    self.evalfn = evalfn
    
    self.iesmas.stateChanged.connect(self.onIesmasStateChanged)
    
    self._evalState()
    
  def listen(self, other_object):
    other_object.stateChanged.connect(self._evalState)
    return self
    
  def onIesmasStateChanged(self):
    self._evalState()
    
  def _evalState(self):
    print self, "evalState"
    
    if self.evalfn:
      state = self.evalfn()
      print "evalfn", state
      if state != None:
	self.state = state
	return
    
    if self.iesmas.in_ruozas == self.ruozas:
      pass
    elif self.ruozas == self.iesmas.ruozas1 and self.iesmas.state == 1:
      self.state = Sviesoforas4.State.RAUDONA
      return
    
    elif self.ruozas == self.iesmas.ruozas2 and self.iesmas.state == 0:
      self.state = Sviesoforas4.State.RAUDONA
      return
    
    self.state = Sviesoforas4.State.ZALIA
    
   
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
    
    

  
                    
class Stendas(QObject):
  app = None
  def __init__(self, app):
    QObject.__init__(self)
    
    Stendas.app = app
    
    self.ruozai = [
      Ruozas(0),
      Ruozas(1),
      Ruozas(2),
      Ruozas(3),
      Ruozas(4),
      Ruozas(5),
      Ruozas(6),
      Ruozas(7),
      Ruozas(8),
      Ruozas(9),
      Ruozas(10),
      Ruozas(11),
      Ruozas(12),
      Ruozas(13),
      Ruozas(14),
      Ruozas(15),
      Ruozas(16),
      Ruozas(17),
      Ruozas(18),
      Ruozas(19),
      Ruozas(20),
      Ruozas(21),
    ]
    
    self.iesmai = {
	"F0.0": Iesmas(0xF0, 0, self.ruozai[6], self.ruozai[7], self.ruozai[1]),
	"F0.1": Iesmas(0xF0, 1, self.ruozai[7], self.ruozai[2], self.ruozai[3]),
	
	"F1.0": Iesmas(0xF1, 0, self.ruozai[0], self.ruozai[4], self.ruozai[1]),
	"F1.1": Iesmas(0xF1, 1, self.ruozai[4], self.ruozai[2], self.ruozai[3]),
	
	"F2.0": Iesmas(0xF2, 0, self.ruozai[0], self.ruozai[18], self.ruozai[16]),
	"F2.1": Iesmas(0xF2, 1, self.ruozai[9], self.ruozai[19], self.ruozai[20]),
	
	"F4.0": Iesmas(0xF4, 0, self.ruozai[6], self.ruozai[9], self.ruozai[8]),
	
	
    }
    
    self.iesmai["F2.0.0"] = SlaveIesmas(self.iesmai["F2.0"], 0, self.ruozai[19], self.ruozai[18], self.ruozai[15])
    self.iesmai["F2.0.1"] = SlaveIesmas(self.iesmai["F2.0"], 1, self.ruozai[8],  self.ruozai[17], self.ruozai[16])
    self.iesmai["F2.0.2"] = SlaveIesmas(self.iesmai["F2.0"], 2, self.ruozai[10], self.ruozai[17], self.ruozai[15])
	
    self.sviesoforai = {
      #0x06 : Sviesoforas2(0x06, self.iesmai["F4.0"], invert=True),
      
      0x08 : Sviesoforas2(0x08, self.iesmai["F0.1"], invert=True),
      0x09 : Sviesoforas2(0x09, self.iesmai["F0.1"], invert=True),
      
      0x07 : Sviesoforas2(0x07, self.iesmai["F0.0"], invert=True),
      
      0x08 : Sviesoforas2(0x08, self.iesmai["F0.1"], invert=False),
      0x0E : Sviesoforas2(0x0E, self.iesmai["F0.0"], invert=False),
      
      0x23 : Sviesoforas4(0x23, self.iesmai["F0.0"]),
      
      0x17 : Sviesoforas4(0x17, self.iesmai["F1.1"], self.ruozai[3], 
			  lambda: Sviesoforas4.State.RAUDONA if self.iesmai["F1.0"].state else None).listen(self.iesmai["F1.0"]),
      
      0x21 : Sviesoforas4(0x21, self.iesmai["F1.1"], self.ruozai[2], 
			  lambda: Sviesoforas4.State.RAUDONA if self.iesmai["F1.0"].state else None).listen(self.iesmai["F1.0"]),
      
      0x25 : Sviesoforas4(0x25, self.iesmai["F1.0"], self.ruozai[1]),
      
      0x24 : Sviesoforas4(0x24, self.iesmai["F2.0.2"]),
      0x1A : Sviesoforas4(0x1A, self.iesmai["F2.0"]),
       
      0x1B : Sviesoforas4(0x1B, self.iesmai["F2.0.0"]),
      0x20 : Sviesoforas4(0x20, self.iesmai["F2.0.1"]),
      
      0x22 : Sviesoforas4(0x22, self.iesmai["F4.0"], self.ruozai[8]),
      0x3B : Sviesoforas4(0x3B, self.iesmai["F4.0"], self.ruozai[9]),
    }

    
    for r in self.ruozai:
      print r
    print self.iesmai["F0.0"]
    self.iesmai["F0.0"].setState(0)
    self.iesmai["F0.1"].setState(0)
    
    #self.startTimer(1000)
    
    #print self.iesmai["F0.0"]
    #print self.iesmai["F0.0"].next()pas
    #self.iesmai["F0.0"].setState(0)
    self.state = 0
    #while 1:
    #  time.sleep(2)
    #  state = not state
    #  self.iesmai["F0.1"].setState(state)
      
  def timerEvent(self, event):
      print "tm--------------------"
      self.state = not self.state
      self.iesmai["F0.1"].setState(self.state)  
    #print self.iesmai["F0.0"]
    #print self.iesmai["F0.0"].next()
    #print self.iesmai["F0.0"].next().next()
    #print self.iesmai["F0.0"].next().next().prev()
