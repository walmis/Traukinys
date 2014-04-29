#coding: utf-8
from PySide.QtCore import *
from PySide.QtGui import *
from Irenginys import Irenginys

from Sviesoforas4 import Sviesoforas4

class Sviesoforas3(Sviesoforas4):
  class State:
    ZALIA = 0
    GELTONA = 1
    RAUDONA = 2
  
  def __init__(self, address, iesmas, ruozas, alias):
    Sviesoforas4.__init__(self, address, iesmas, ruozas, alias)
    
    self._evalState()
  
  def __repr__(self):
    return "<Sviesoforas3(%02X)>" % self.address

  def _evalState(self):
    print self, "evalState"
    
    next_road = self.iesmas.nextRoad(self.ruozas)
    next_switch = self.iesmas.next(self.ruozas)
    
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
	self.setState(state)
	return
    
    #sekantis kelias užimtas
    if not next_road or next_road.isOccupied():
      self.setState(Sviesoforas3.State.RAUDONA)
      return
    else:
      if not next_next_road or next_next_road.isOccupied():
	self.setState(Sviesoforas3.State.GELTONA)
	return	
      else:
	self.setState(Sviesoforas3.State.ZALIA)
	return

    
    self.setState(Sviesoforas3.State.ZALIA)
    
   
  def setState(self, state):
    if state == Sviesoforas3.State.ZALIA:
      self.send(0b001)
      
    elif state == Sviesoforas3.State.RAUDONA:
      self.send(0b010)
      
    elif state == Sviesoforas3.State.GELTONA:
      self.send(0x08)
    else:
      print self, "setState: unknown state"
      
    self._state = state
  
  def timerEvent(self, event):
    pass
  