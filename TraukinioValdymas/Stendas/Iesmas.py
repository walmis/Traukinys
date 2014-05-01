#coding: utf-8
from PySide.QtCore import *
from PySide.QtGui import *
from Irenginys import Irenginys

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
    
  def __repr__(self):
    return "<" + " ".join(("Iesmas", "%X" % self.address + "."+str(self.port), "(%d)" % self._state)) + ">"
  
  def getInRoad(self):
    return self.in_ruozas
  
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
      if kelias == self.in_ruozas:
	return self.next()
      else:
	if kelias != self.getRoad():
	  return None
	else:
	  if self.in_ruozas.end0 != self:
	    return self.in_ruozas.end0
	  else:
	    return self.in_ruozas.end1
  
  def nextRoad(self, relativeTo = None):
    if relativeTo in (self.in_ruozas, self.ruozas1, self.ruozas2):
      if relativeTo == self.in_ruozas:
	return self.getRoad()
      else:
	if relativeTo != self.getRoad():
	  return None
	else:
	  return self.in_ruozas

    else:
      print u"Iešmas %s su duotu keliu %s nesusijęs" % (self, relativeTo)
      raise Exception

  def update(self):
    self.setState(self._state)
  
  #kai iesmas i sona == 1
  #iesmas tiesiai == 0 
  def getState(self):
    return self._state
  
  def setState(self, state):
    Iesmas.data[self.address] &= ~(1<<self.port)
    Iesmas.data[self.address] |= (int(state) << self.port)

    self.send(Iesmas.data[self.address])
    if self._state != state: 
      print self, "setState", state 
      self._state = state
      self.stateChanged.emit()
  
  stateChanged = Signal()
  state = Property(int, getState, setState, notify=stateChanged)
  
#objektas kaip iešmas. Tik jis sujungia du kelius
class Junction(QObject):
  def __init__(self, road1, road2):
    QObject.__init__(self)
    self.road1 = road1
    self.road2 = road2
    
    road1.addEnd(self)
    road2.addEnd(self)
    
  def __repr__(self):
    return "<Junction(%d, %d)>" % (self.road1.nr, self.road2.nr)
    
  def getState(self):
    return 0
  def setState(self, state):
    pass
  
  def getInRoad(self):
    return self.road1
  
  def getRoad(self):
    return self.road2
  
  def next(self, kelias=None):
    if not kelias:
      road = self.getRoad()
      if road.end0 == self:
	return road.end1
      elif road.end1 == self:
	return road.end0
      else:
	return None 
    else:
      if kelias == self.road1:
	return self.next()
      else:
	if kelias != self.road2:
	  return None
	else:
	  if self.road1.end0 != self:
	    return self.road1.end0
	  else:
	    return self.road1.end1
	  
  #relativeTo - kelias  
  def nextRoad(self, relativeTo=None):
    if not relativeTo:
      return self.road2
    else:
      if self.road1 == relativeTo:
	return self.road2
      elif self.road2 == relativeTo:
	return self.road1
      else:
	raise Exception("Kelias %s nesusijęs su sujungimu %s" % (relativeTo, self))

  #durnas signalas. Naudojamas, kad išlaikyti API. Čia nieko nedaro.
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
  
  def __repr__(self):
    return "<" + " ".join(("SlaveIesmas", hex(self.address)+"."+str(self.master.port)+"."+str(self.port), "(%d)" % self._state)) + ">"
 
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
