from PySide.QtCore import *

class RFIDTag(QObject):
  tags = []
  
  def __init__(self, id):
    QObject.__init__(self)
    self.actions = []
    
    self.id = id
    
    RFIDTag.tags.append(self)
    
  def addAction(self, func):
    self.actions.append(func)
    
    return self
    
  def _trigger(self, train):
    self.triggered.emit(train)
    
    for action in self.actions:
      action(train)
    
  @staticmethod
  def trigger(tagid, train):
    print "trigger", tagid, train
    for tag in RFIDTag.tags:
      if tag.id == tagid:
	tag._trigger(train)
    
    
  triggered = Signal(object) 
