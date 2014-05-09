from gui import Gui
from queue import Queue
from threading import Event

g = Gui(Queue(), Queue())
g.run()
