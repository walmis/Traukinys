import sys
import BaseHTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler

from PySide.QtCore import *
from PySide.QtGui import *

Protocol     = "HTTP/1.1"

#if sys.argv[1:]:
    #port = int(sys.argv[1])
#else:
    #port = 8000
server_address = ('0.0.0.0', 8000)


class HttpServer(QObject):
  
  def __init__(self, handler):
    QObject.__init__(self)
    
    self.server = BaseHTTPServer.HTTPServer(('0.0.0.0', 8000), handler)
    
    self.notifier = QSocketNotifier(self.server.fileno(), QSocketNotifier.Read)
    self.notifier.activated.connect(self.onActivated)
    
  def onActivated(self):
    self.server.handle_request()

