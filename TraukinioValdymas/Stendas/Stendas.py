#coding: utf-8
import serial

import sys
import random
import time
count = 0

from PySide.QtCore import *
from PySide.QtGui import *

from WirelessUART import WirelessUART
from RFIDTag import RFIDTag
from Train import Train
from Ruozas import Ruozas
from Sviesoforas2 import Sviesoforas2
from Sviesoforas4 import Sviesoforas4
from Sviesoforas3 import Sviesoforas3
from Iesmas import *

import json
import sqlite3

db = sqlite3.connect('traukinys.db')
   
   
   
class Ruozai:
  def __init__(self):
    self.ruozai = []
    
  def __getitem__(self, key):
    i = self.find(key)
    if not i:
      raise IndexError, key
    return i
    
  def __iter__(self):
      self._cur = 0
      return self

  def next(self): # Python 3: def __next__(self)
    r = None
    try:
      r = self.ruozai[self._cur]
    except:
      raise StopIteration
    
    self._cur += 1
    return r
    
  def add(self, id, alias, speedlimit, pseudo):
    self.ruozai.append(Ruozas(int(id), alias, speedlimit, pseudo))
    
  def find(self, id):
    for ruozas in self.ruozai:
      try:
	if int(id) == ruozas.nr:
	  return ruozas
      except:
	pass
      
      if id == ruozas.alias:
	return ruozas

class Stendas(QObject):
  app = None
  inst = None
  
  UARTConnected = Signal(object)
  
  def __init__(self, app = None):
    QObject.__init__(self)
    
    Stendas.app = app
    Stendas.inst = self
    self.serial = None
    
    try:
      app.radio.signals.deviceAdded.connect(self.onDeviceAdded)
      #app.serial.associationChanged.connect(self.onSerialAssoc)
    except:
      print "Unable to connect to radio"
      
    self.ruozai = Ruozai()
    
    c = db.cursor()
    c.execute("select * from roads")

    for row in c:
      id = row[0]
      alias = row[1]
      pseudo = row[2]
      speedlimit = row[3]
      
      self.ruozai.add(id, alias, pseudo, speedlimit)


    self.iesmai = {}
    
    c = db.cursor()
    c.execute("select * from switches")
    
    for row in c:
      switch = row[0].split(".")
      if len(switch) == 2:
	self.iesmai[str(row[0])] = Iesmas(int(switch[0], 16), int(switch[1]), 
				   self.ruozai[row[1]], 
				   self.ruozai[row[2]], 
				   self.ruozai[row[3]])
      elif len(switch) == 3:
	self.iesmai[str(row[0])] = SlaveIesmas(self.iesmai[str(switch[0] + "." + switch[1])], 
					int(switch[2]), 
					self.ruozai[row[1]], 
					self.ruozai[row[2]],
					self.ruozai[row[3]]
				   )
	
      elif switch[0].startswith("J"):
	self.iesmai[str(switch[0])] = Junction(self.ruozai[row[1]], self.ruozai[row[2]],)
    
    c.execute("select * from rfid")
    
    self.rfid = {}
    
    for row in c:
      i = int(row[0])
      tag = row[1]
      road = None
      next_switch = None
      
      if row[2]:
	road = self.ruozai[row[2]]

      if row[3]:
	next_switch = self.iesmai[str(row[3])]
	
      self.rfid[i] = RFIDTag(tag, road, next_switch)
      
    print self.rfid
    
    self.sviesoforai = {}
	
    c = db.cursor()
    c.execute("select * from lights")
    
    for row in c:
      address = row[0]
      type = row[1]
      switch = self.iesmai[row[2]]
      next_road = self.ruozai[row[3]]
      alias = row[4]
      
      S = None
      if type == 2:
	S = Sviesoforas2
      elif type == 3:
	S = Sviesoforas3
      elif type == 4:
	S = Sviesoforas4

      if address != None:
	self.sviesoforai[int(address,16)] = S(int(address,16), switch, next_road, alias)
      
    print self.sviesoforai
  
    for r in self.ruozai:
      print r
    
    #print "sekantis kelias nuo f00", self.iesmai["F0.0"].nextRoad(self.ruozai[6])
    #print "sekantis kelias nuo f00", self.iesmai["F0.0"].nextRoad(self.ruozai[1])
    
    self.trainPos = {}
  
  def getRoadBetweenNodes(self, prev_node, current_node):
    for ruozas in self.ruozai:
      if (prev_node and prev_node in (ruozas.end0, ruozas.end1)) and (current_node and current_node in (ruozas.end0, ruozas.end1)):
	return ruozas
    return None
  
  def onRfidRead(self, rfid):
    train = self.sender()
    
    cur = db.cursor()
    cur.execute("select * from rfid where tag_id = ?", (rfid,))
    row = cur.fetchone()
    
    if not row:
      cur.execute("insert into rfid(tag_id) values (?)", (rfid,))
      cur.execute("select * from rfid where tag_id = ?", (rfid,))
      row = cur.fetchone()
    

    id = int(row[0])

    print "RFID read", train, "CARD ID:", id
    
    cur = db.cursor()
    cur.execute("insert into tracking(train_id, rfid, timestamp) values (?, ?, strftime('%s','now'))", (train.address, id))
    db.commit()
    
    cur.execute("select rfid from tracking where train_id=? order by timestamp desc limit 1,5", (train.address,))
    
    current_rfid = self.rfid[id]
    
    prev_rfids = []
    for row in cur:
      prev_rfids.append(self.rfid[int(row[0])])

    print prev_rfids
    
    rfid_road = self.rfid[id].road
    rfid_node = self.rfid[id].node

    print rfid_road
    try:
      prev_road = self.getRoadBetweenNodes(current_rfid.node, prev_rfids[0].node)
    except:
      prev_road = None
    
    #try:
    current_road = self.getRoadBetweenNodes(prev_rfids[0].node, prev_rfids[1].node)
      
    print train, "Current road", current_road
      
    print train, "Prev. road", prev_road
    
    if rfid_node:
      print train, "Passing", rfid_node
      
    next_road = rfid_node.nextRoad(prev_road)
    print train, "Next road", next_road
    
    self.trainPos[train] = next_road
    
    #currentRoad = None
    #print len(self.trainRoutes[train])
    #if len(self.trainRoutes[train]) > 0:
      #currentRoad = self.rfid[self.trainRoutes[train][-1]].road
      
    #if currentRoad and currentRoad != rfid_road: 
      #currentRoad.leave(train)
      #rfid_road.enter(train)
      
      
    #self.trainRoutes[train].append(id)
	
    
  def onDeviceAdded(self, device):

    if isinstance(device, WirelessUART):
      self.serial = device
      self.UARTConnected.emit(self.serial)
      
    if type(device) == Train:
      device.onRfidRead.connect(self.onRfidRead)
   #   self.serial.associationChanged.connect(self.onUartAssoc)
      
  #def 
    
    #self.sviesoforai[0x3A].setState(Sviesoforas4.State.GELTONA2_MIRKS1)
    #self.startTimer(1000)
    
    #print self.iesmai["F0.0"]
    #print self.iesmai["F0.0"].next()pas
    #self.iesmai["F0.0"].setState(0)
    self.state = 0
    #while 1:
    #  time.sleep(2)
    #  state = not state
    #  self.iesmai["F0.1"].setState(state)

 
    #print self.iesmai["F0.0"]
    #print self.iesmai["F0.0"].next()
    #print self.iesmai["F0.0"].next().next()
    #print self.iesmai["F0.0"].next().next().prev()
