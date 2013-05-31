import sys
import BaseHTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
from SocketServer import ThreadingMixIn

from PySide.QtCore import *
from PySide.QtGui import *

Protocol     = "HTTP/1.1"

#if sys.argv[1:]:
    #port = int(sys.argv[1])
#else:
    #port = 8000
server_address = ('0.0.0.0', 8000)

class ThreadedHTTPServer(ThreadingMixIn, BaseHTTPServer.HTTPServer):
  pass

class HttpServer(QObject):
  
  def __init__(self, handler):
    QObject.__init__(self)
    
    self.server = ThreadedHTTPServer(('0.0.0.0', 8000), handler)
    
    self.notifier = QSocketNotifier(self.server.fileno(), QSocketNotifier.Read)
    self.notifier.activated.connect(self.onActivated)
    
  def onActivated(self):
    self.server.handle_request()


