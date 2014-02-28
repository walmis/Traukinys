#coding: utf-8
from PySide.QtCore import *
from PySide.QtGui import *

class Ruozas(QObject):
  def __init__(self, nr, speed=None, pseudo = False):
    QObject.__init__(self)
    self._busy = False
    self._train = None
    self.pseudo = pseudo
    
    self.nr = nr
    self.speed = speed
    
    self.end0 = None
    self.end1 = None
    
    self.entered = []
    
  def __repr__(self):
    return "<Ruozas(%s): %s <=> %s>" % (str(self.nr), self.end0, self.end1)
    
  def addEnd(self, iesmas):
    if not self.end0:
      self.end0 = iesmas
    elif not self.end1:
      self.end1 = iesmas
    else:
      print "cannot add end node (already connected), check connections"
  
  def isOccupied(self):
    return len(self.entered) != 0
  
  def isSlow(self):
    return self.slow
  
  def setBusy(self, busy):
    self._busy = busy
    self.stateChanged.emit()
    
  def enter(self, train):
    if not train in self.entered:
      self.entered.append(train)
      self.stateChanged.emit()
      
      if self.speed != None:
	if train.speed < 0:
	  train.pushSpeed(-self.speed)
	else:
	  train.pushSpeed(self.speed)
    
  def leave(self, train):
    if train in self.entered:
      self.entered.remove(train)
      self.stateChanged.emit()
      
      if self.speed != None:
	train.popSpeed()

      

  stateChanged = Signal() 
