#coding: utf-8
from PySide.QtCore import *
from PySide.QtGui import * 
from Irenginys import Irenginys
 
class Sviesoforas4(Irenginys):
  class State:
    ZALIA = 0 #2 laisvi, iesmas tiesiai
    RAUDONA = 1 #0 laisvu
    GELTONA = 2 #geltona virsutine, 1 laisvas, iesmas tiesiai
    GELTONA_MIRKS = 3 #geltona virsutine mirksinti, 2 laisvi iesmas tiesiai, SLOW
    GELTONA2 = 4 #dvi geltonos sviecia, 1 laisvas, iesmas i sona
    GELTONA2_MIRKS1 = 5 #apatine geltona sviecia, virsutine mirksi, 2 laisvi, iesmas i sona
    
  def __init__(self, address, iesmas, ruozas, alias, evalfn=None):
    Irenginys.__init__(self, address)
    
    self.alias = alias
    
    self.state = Sviesoforas4.State.ZALIA
    self.blink_tm = self.startTimer(800)
    
    self.iesmas = iesmas
    self.ruozas = ruozas
    
    self.evalfn = evalfn
    
    self.iesmas.stateChanged.connect(self.onIesmasStateChanged)
    
    self.connectedSignals = []
    
    self.test()
    
    self._evalState()
    
  def __repr__(self):
    return "<Sviesoforas4(%02X)>" % self.address
    
  def listen(self, other_object):
    other_object.stateChanged.connect(self._evalState)
    return self
    
  def onNextRoadChanged(self):
    self._evalState()
    
  def test(self):
    for (signal, func) in self.connectedSignals:
      signal.disconnect(func)
      
    self.connectedSignals = []
    
    next_switch = self.iesmas.next(self.ruozas)
    print self, "next switch", next_switch
    if next_switch:
      next_switch.stateChanged.connect(self.onNextSwitchChanged)
      self.connectedSignals.append((next_switch.stateChanged, self.onNextSwitchChanged))
    
      next_road = self.iesmas.nextRoad(self.ruozas)
      next_road.stateChanged.connect(self.onNextRoadChanged)  
      self.connectedSignals.append((next_road.stateChanged, self.onNextRoadChanged))
      
      next_next_road = next_switch.nextRoad(next_road)
      if next_next_road:
	next_next_road.stateChanged.connect(self.onNextRoadChanged)
	self.connectedSignals.append((next_next_road.stateChanged, self.onNextRoadChanged))
    
  def onNextSwitchChanged(self):
    #print self, "next switch changed"
    self.test()
      
    self._evalState()
    
  def onIesmasStateChanged(self):
    self.test()
    self._evalState()
    
  def _evalState(self):
    print self, "evalState"
    
    next_road = self.iesmas.nextRoad(self.ruozas)
    next_switch = self.iesmas.next(self.ruozas)

    if next_road and next_road.pseudo:
      n = next_road
      next_road = next_switch.nextRoad(next_road)
      print "n", next_road
      print "ns", next_switch
      next_switch = next_switch.next(n)
    
    print self, "road+1", next_road
    print self, "switch+1", next_switch
    
    next_next_road = None
    if next_switch:
      next_next_road = next_switch.nextRoad(next_road)
    print self, "road+2", next_next_road
    
    if self.evalfn:
      state = self.evalfn()
      print "evalfn", state
      if state != None:
	self.state = state
	return
    
    #sekantis kelias užimtas
    if not next_road or next_road.isOccupied():
      self.state = Sviesoforas4.State.RAUDONA
      return
    else:
      #iešmas į šoną
      if self.iesmas.state:
	if not next_next_road or next_next_road.isOccupied():
	  self.state = Sviesoforas4.State.GELTONA2
	  return
	else:
	  self.state = Sviesoforas4.State.GELTONA2_MIRKS1
	  return
      #Iešmas tiesiai
      else:
	if not next_next_road or next_next_road.isOccupied():
	  self.state = Sviesoforas4.State.GELTONA
	  return	
	else:
	  self.state = Sviesoforas4.State.ZALIA
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