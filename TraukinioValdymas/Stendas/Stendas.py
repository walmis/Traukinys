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
      
    self.ruozai = [
      Ruozas(0),
      Ruozas(1),
      Ruozas(2),
      Ruozas(3),
      Ruozas(4, pseudo=True),
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
      Ruozas(22),
      Ruozas(23),
      Ruozas(24, speed=25),
      Ruozas(25)
    ]
    
    #self.iesmai = {
	#"F0.0": Iesmas(0xF0, 0, self.ruozai[6], self.ruozai[7], self.ruozai[1]),
	#"F0.1": Iesmas(0xF0, 1, self.ruozai[7], self.ruozai[2], self.ruozai[3]),
	
	#"F1.0": Iesmas(0xF1, 0, self.ruozai[0], self.ruozai[4], self.ruozai[1]),
	#"F1.1": Iesmas(0xF1, 1, self.ruozai[4], self.ruozai[2], self.ruozai[3]),
	
	#"F2.0": Iesmas(0xF2, 0, self.ruozai[0], self.ruozai[18], self.ruozai[16]),
	#"F2.1": Iesmas(0xF2, 1, self.ruozai[9], self.ruozai[19], self.ruozai[20]),
	
	#"F3.0": Iesmas(0xF3, 0, self.ruozai[11], self.ruozai[12], self.ruozai[13]),
	#"F3.1": Iesmas(0xF3, 1, self.ruozai[10], self.ruozai[14], self.ruozai[11]),
	
	#"F4.0": Iesmas(0xF4, 0, self.ruozai[6], self.ruozai[9], self.ruozai[8]),
    #}
    self.iesmai = {}
    
    c = db.cursor()
    c.execute("select * from switches")
    
    for row in c:
      switch = row[0].split(".")
      if len(switch) == 2:
	self.iesmai[str(row[0])] = Iesmas(int(switch[0], 16), int(switch[1]), 
				   self.ruozai[int(row[1])], 
				   self.ruozai[int(row[2])], 
				   self.ruozai[int(row[3])])
      elif len(switch) == 3:
	self.iesmai[str(row[0])] = SlaveIesmas(self.iesmai[str(switch[0] + "." + switch[1])], 
					int(switch[2]), 
					self.ruozai[int(row[1])], 
					self.ruozai[int(row[2])],
					self.ruozai[int(row[3])]
				   )
	
      elif switch[0].startswith("J"):
	self.iesmai[str(switch[0])] = Junction(self.ruozai[int(row[1])], self.ruozai[int(row[2])],)
    
    c.execute("select * from rfid")
    
    self.rfid = {}
    
    for row in c:
      i = int(row[0])
      tag = row[1]
      road = None
      next_switch = None
      
      if row[2]:
	road = self.ruozai[int(row[2])]

      if row[3]:
	next_switch = self.iesmai[str(row[3])]
	
      self.rfid[i] = RFIDTag(tag, road, next_switch)
      
    print self.rfid
	
    self.sviesoforai = {
      
      0x17 : Sviesoforas4(0x17, self.iesmai["F0.0"], self.ruozai[6]),
			  #lambda: Sviesoforas4.State.RAUDONA if self.iesmai["F1.0"].state else None).listen(self.iesmai["F1.0"]),
      
      0x21 : Sviesoforas4(0x21, self.iesmai["F1.1"], self.ruozai[2]),
			 # lambda: Sviesoforas4.State.RAUDONA if self.iesmai["F1.0"].state else None).listen(self.iesmai["F1.0"]),
      
      0x25 : Sviesoforas4(0x25, self.iesmai["F1.0"], self.ruozai[1]),
      
      0x23 : Sviesoforas4(0x23, self.iesmai["F1.1"], self.ruozai[3]),
      
      0x24 : Sviesoforas4(0x24, self.iesmai["F2.0.2"], self.ruozai[10]),
      0x1A : Sviesoforas4(0x1A, self.iesmai["F2.0"], self.ruozai[25]),
       
      0x1B : Sviesoforas4(0x1B, self.iesmai["F2.0.0"], self.ruozai[19]),
      0x20 : Sviesoforas4(0x20, self.iesmai["F2.0.1"], self.ruozai[8]),
      
      0x22 : Sviesoforas4(0x22, self.iesmai["F4.0"], self.ruozai[8]),
      0x3B : Sviesoforas4(0x3B, self.iesmai["F4.0"], self.ruozai[9]),
      
      #0x0D : Sviesoforas2(0x0D, self.iesmai["F3.1"]),
      
      0x3A : Sviesoforas4(0x3A, self.iesmai["F0.1"], self.ruozai[7]),
      
      0x14 : Sviesoforas2(0x14, self.iesmai["F0.1"], self.ruozai[2]),
      0x0A : Sviesoforas2(0x0A, self.iesmai["F0.0"], self.ruozai[7]),
      0x10 : Sviesoforas2(0x10, self.iesmai["F0.0"], self.ruozai[1]),
      
      0x08 : Sviesoforas2(0x08, self.iesmai["F3.0"], self.ruozai[12]),
      0x02 : Sviesoforas2(0x02, self.iesmai["F3.0"], self.ruozai[13]),
      0x0C : Sviesoforas2(0x0C, self.iesmai["F3.1"], self.ruozai[14]),
      
      0x26 : Sviesoforas3(0x26, self.iesmai["J0"], self.ruozai[0]),
      0x27 : Sviesoforas3(0x27, self.iesmai["J1"], self.ruozai[22]),
      0x1E : Sviesoforas3(0x1E, self.iesmai["J2"], self.ruozai[23]),
      0x16 : Sviesoforas2(0x16, self.iesmai["J3"], self.ruozai[24]),
    }
  
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
