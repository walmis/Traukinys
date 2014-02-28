from PySide.QtCore import *

class RFIDTag(QObject):
  
  def __init__(self, id, road = None, next_switch = None):
    QObject.__init__(self)
    self.actions = []
    
    self.id = id
    self.road = road
    self.node = next_switch
    
  def __repr__(self):
    return "<RFIDTag(%s, %s, %s)>\n" % (self.id, self.node, self.road)
    
  def addAction(self, func):
    self.actions.append(func)
    return self
    
  def _trigger(self, train):
    self.triggered.emit(train)
    
    for action in self.actions:
      action(train)    
    
  triggered = Signal(object) 
