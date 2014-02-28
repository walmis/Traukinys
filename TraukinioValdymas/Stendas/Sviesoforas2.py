from PySide.QtCore import *
from PySide.QtGui import *
from Irenginys import Irenginys

class Sviesoforas2(Irenginys):
  def __init__(self, address, iesmas, road, invert = False):
    Irenginys.__init__(self, address) 
    
    self.iesmas = iesmas
    self.road = road
    self.invert = invert
    
    self.onIesmasStateChanged()
    
    if iesmas:
      self.iesmas.stateChanged.connect(self.onIesmasStateChanged)
      
  def __repr__(self):
    return "<Sviesoforas2(%02X)>" % self.address
    
  def onRoadChanged(self):
    self.sender().stateChanged.disconnect(self.onRoadChanged)
    self.onIesmasStateChanged()
    
  def onIesmasStateChanged(self):
    print self, "state changed"
    state = False
    
    road = self.iesmas.nextRoad(self.road)
    if road:
      road.stateChanged.connect(self.onRoadChanged)
      
    if road and not road.isOccupied():
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
