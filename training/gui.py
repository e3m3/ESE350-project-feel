from tkinter import *
from tkinter import ttk
from tkinter import font
import serial
import time
import threading
import queue
import numpy as np
from math import sqrt


## SERIAL COMM GLOBALS
SERIAL_PORT = '/dev/ttyACM1'
SERIAL_BAUD = 9600
SERIAL_TMOT = 1 # s
######

## GUI GLOBALS
TITLE = 'Project FEEL'
WIDTH = 1024
HEIGHT = 800
PROMPT = '::> '
R = 10
ON = 'red'
OFF = 'green'
FENCER_IMAGE = 'images/fencer.gif'
#READ_MOTOR_PRD = 50 #ms
GUI_UPDATE_PRD = 80 # ms
THRESH = 0.8
#####

## ANALYZER GLOBALS
PROJECTION_FILE = 'clusters/2-trans_4,6_projection.csv'
CENTRIOD_FILE = 'clusters/2-trans_4,6_centroids.txt'

ONE =   '1'
TWO =   '2'
THREE = '3'
FOUR =  '4'
FIVE =  '5'
SIX =   '6'
SEVEN = '7'
EIGHT = '8'
NINE =  '9'
SEP = ','

TRANS1 = '_'.join((FOUR, SIX))
TRANS2 = '_'.join((SIX, FOUR))

NO_ACTUATE = '0'
WINDOW_SIZE = 12
VECT_SIZE = 6
#####


#class Gui(threading.Thread):
class Gui:
    def __init__(self, in_q, out_q, quit_event):
        self.in_q = in_q
        self.out_q = out_q
        self.motors_actuated = [False]*7
        self.motor_clicked = None
        self.quit_event = quit_event

        self.motors = {
            #1: {'pos': (445, 360), 'obj': None, 'state': OFF},
            #2: {'pos': (675, 350), 'obj': None, 'state': OFF},
            #3: {'pos': (500, 500), 'obj': None, 'state': OFF},
            #4: {'pos': (700, 500), 'obj': None, 'state': OFF},
            #5: {'pos': (510, 710), 'obj': None, 'state': OFF},
            #6: {'pos': (680, 710), 'obj': None, 'state': OFF},
            #7: {'pos': (610, 595), 'obj': None, 'state': OFF},
            1: {'pos': (445, 360), 'obj': None},
            2: {'pos': (675, 350), 'obj': None},
            3: {'pos': (500, 500), 'obj': None},
            4: {'pos': (700, 500), 'obj': None},
            5: {'pos': (510, 710), 'obj': None},
            6: {'pos': (680, 710), 'obj': None},
            7: {'pos': (610, 595), 'obj': None},
        }

        # Create GUI elements
        self.root = Tk()
        self.root.title(TITLE)

        big_font = font.Font(family='Helveitca', size=16)

        self.canvas = Canvas(self.root, width=str(WIDTH), height=str(HEIGHT), background='black')
        self.canvas.grid(column=0, row=0, columnspan=2, sticky=(N, W, E, S))

        self.status = Text(self.root, height='1', font=big_font)
        self.status.grid(column=0, row=1, columnspan=2, sticky=(W, E))
        self.status.insert('1.0', PROMPT)

        sep = ttk.Separator(self.root, orient=HORIZONTAL)
        sep.grid(column=0, row=2, columnspan=2, pady='5')

        self.image = PhotoImage(file=FENCER_IMAGE)
        self.canvas.create_image(WIDTH/2, HEIGHT/2, image=self.image)

        for key in self.motors.keys():
            self.draw_motor_status(key, OFF)
        #self.root.after(READ_MOTOR_PRD, self.read_motor_vals)

        #threading.Thread.__init__(self)
        #self.size = (width, height)

    def run(self):
        try:
            if self.quit_event.is_set():
                self.callback()
            self.root.after(GUI_UPDATE_PRD, self.update_gui)
            self.root.mainloop()
        except KeyboardInterrupt:
            self.callback()

    def callback(self):
        self.quit_event.set()
        self.root.quit()

    def motor_click_handler(self, motor_id):
        self.motor_clicked = motor_id
        self.motors_actuated[motor_id] = not self.motors_actuated[motors_id]
        print('GUI: got click')

    def draw_motor_status(self, motor_id, color):
        motor = self.motors[motor_id]
        if motor['obj']:
            self.canvas.delete(motor['obj'])
        (x, y) = motor['pos']
        item = self.canvas.create_oval((x-R, y-R, x+R, y+R), fill=color)
        #item = self.canvas.create_oval((x-R, y-R, x+R, y+R), fill=motor['state'])
        #self.canvas.tag_bind(item, '<l>', lambda e: self.motor_clicked = motor_id; self.motors_actuated[motor_id] = not self.motors_actuated[motors_id])
        self.canvas.tag_bind(item, '<l>', lambda e: self.motor_click_handler(motor_id))
        self.motors[motor_id]['obj'] = item

    def get_data_from_serial(self):
        if not self.in_q.empty():
            try:
                actuated = self.in_q.get_nowait()
                for i, x in actuated:
                    self.motors_actuated[i] = x
                self.in_q.task_done()
                print('GUI: got data from serial')
            except queue.Empty:
                pass

    def send_data_to_serial(self):
        if not self.out_q.full() and self.motor_clicked:
            self.out_q.put_nowait(self.motor_clicked)
        self.motor_clicked = None
        print('GUI: sent data to serial')

    def update_gui(self):
        for i, on in enumerate(self.motors_actuated):
            color = None
            if on:
                color = ON
            else:
                color = OFF
            self.draw_motor_status(i + 1, color)
        self.get_data_from_serial()
        self.send_data_to_serial()
        self.root.after(GUI_UPDATE_PRD, self.update_gui)


class SerialComm(threading.Thread):
    def __init__(self, a_in_q, a_out_q, g_in_q, g_out_q, serial_port, quit_event):
        self.a_in_q = a_in_q
        self.a_out_q = a_out_q
        self.g_in_q = g_in_q
        self.g_out_q = g_out_q
        self.ser = serial_port
        self.ds = np.zeros(VECT_SIZE)
        self.motors_defended = [False]*7
        self.motors_actuated = [False]*7
        self.motor_clicked = None
        self.quit_event = quit_event
        threading.Thread.__init__(self)

    def send_data_to_serial(self):
        if self.motors_defended:
            actuations = [str(int(x)) for x in self.motors_actuated]
            clicked_str = None
            if self.motor_clicked:
                clicked_str = str(int(self.motor_clicked))
            else:
                clicked_str = '0'
            self.motor_clicked = None
            actuations.insert(0, clicked_str)
            data_str = ','.join(actuations)
            self.ser.write(data_str.encode('ascii', 'replace'))
            print('SERIAL: write data to serial')

    def send_data_to_analyzer(self):
        if not self.a_out_q.full():
            self.a_out_q.put_nowait(self.ds)
            print('SERIAL: send data to analyzer')

    def send_data_to_gui(self):
        if not self.g_out_q.full():
            self.g_out_q.put_nowait(self.motors_actuated)
            print('SERIAL: sent data to gui')
            #self.out_q.put_nowait((self.ds[:], self.motor_data_actuated[:]))
        #print('ds ==', self.ds)

    def get_data_from_serial(self):
        line = self.ser.readline().decode()
        if len(line) > 0:
            try:
                split = [e[e.find(':') + 1:].strip() for e in line.split('\t\t')]
                vals = [int(val) for e in split[1:2] for val in e.split('\t')]
                for i, val in enumerate(vals):
                    self.ds[i] = val
                #TODO
                #for i, val in enumerate(split[3]):
                #    self.motors_actuated[i] = bool(val)
                print('SERIAL: read data from serial')
            except ValueError:
                pass
            except IndexError:
                print('Split: ', split, '\tVals: ', vals)

    def get_motor_data_from_analyzer(self):
        if not self.a_in_q.empty():
            try:
                self.motors_defended = self.a_in_q.get_nowait()
                self.a_in_q.task_done()
                print('SERIAL: got data from analyzer')
            except queue.Empty:
                self.motors_defended = [False]*7

    def get_data_from_gui(self):
        if not self.g_in_q.empty():
            try:
                self.motor_clicked = self.g_in_q.get_nowait()
                self.g_in_q.task_done()
                print('SERIAL: got data from gui')
            except queue.Empty:
                pass

    def run(self):
        try:
            while True:
                if self.quit_event.is_set():
                    break
                self.get_data_from_serial()
                self.get_motor_data_from_analyzer()
                self.get_data_from_gui()
                self.send_data_to_gui()
                self.send_data_to_analyzer()
                self.send_data_to_serial()
        except KeyboardInterrupt:
            self.quit_event.set()


class DataAnalyzer(threading.Thread):
    def __init__(self, in_q, out_q, proj, centroids, quit_event):
        self.in_q = in_q
        self.out_q = out_q
        #self.ds = None
        self.proj = proj
        self.centroids = centroids
        self.window = np.zeros((VECT_SIZE, WINDOW_SIZE))
        self.current_slot = 0
        self.motors_defended = [False]*7
        self.current_parry = SIX
        self.quit_event = quit_event
        threading.Thread.__init__(self)

    def get_data_from_serial(self):
        if not self.in_q.empty():
            try:
                ds = self.in_q.get_nowait()
                for i, x in enumerate(ds):
                    self.window[i][self.current_slot] = x
                self.current_slot += 1
                if self.current_slot == WINDOW_SIZE:
                    self.current_slot = 0
                self.in_q.task_done()
                print('ANALYZER: got data from serial')
            except queue.Empty:
                pass

    def send_data_to_serial(self):
        if not self.out_q.full():
            self.out_q.put_nowait(self.motors_defended)
            print('ANALYZER: send data to serial')

    def distance(p1, p2):
        x1, y1 = p1[1], p1[2]
        x2, y2 = p2[1], p2[2]
        return sqrt((x2 - x1)**2 + (y2 - y1)**2)

    def process_data(self):
        vector = np.sum(self.window, axis=1)
        proj_v = proj*vector
        print(str(proj_v))
        best = None
        for centriod in self.centroids:
            d = self.distance(proj_v, centroid['point'])
            if not best or d < best[1]:
                best = (d, centroid['tag'])
        self.parry_transition(best[2])
        self.map_defended_motors()
        print('ANALYZER: processed data')

    def parry_transition(self, tag):
        if tag == TRANS1 and self.current_parry == FOUR:
            self.current_parry = SIX
        elif tag == TRANS2 and self.current_parry == SIX:
            self.current_parry = FOUR

    def map_defended_motors(self):
        if self.current_parry == FOUR:
            self.motors_defended = [False, False, False, True, False, True, False]
        if self.current_parry == SIX:
            self.motors_defended = [False, False, True, False, True, False, False]

    def run(self):
        try:
            while True:
                if self.quit_event.is_set():
                    break
                self.get_data_from_serial()
                self.process_data()
                self.send_data_to_serial()
        except KeyboardInterrupt:
            self.quit_event.set()

serial_port = None
try:
    serial_to_analyzer_q = queue.Queue()
    analyzer_to_serial_q = queue.Queue()
    serial_to_gui_q = queue.Queue()
    gui_to_serial_q = queue.Queue()
    quit_event = threading.Event()
    quit_event.clear()

    serial_port = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=SERIAL_TMOT)
    proj = np.genfromtxt(PROJECTION_FILE, delimiter=',')
    centroids = []
    with open(CENTRIOD_FILE, 'r') as f:
        for line in f.readlines():
            (x, y, tag) = tuple(line.split(','))
            centroids.append({'point':(x, y), 'tag':tag})

    gui = Gui(serial_to_gui_q, gui_to_serial_q, quit_event)
    serial_comm = SerialComm(analyzer_to_serial_q, serial_to_analyzer_q, gui_to_serial_q, serial_to_gui_q, serial_port, quit_event)
    data_analyzer = DataAnalyzer(serial_to_analyzer_q, analyzer_to_serial_q, proj, centroids, quit_event)

    #gui.start()
    serial_comm.start()
    data_analyzer.start()
    #gui.join()
    gui.run()
    serial_comm.join()
    data_analyzer.join()
except KeyboardInterrupt:
    pass
#finally:
#    if serial_port:
#        serial_port.close()
#    print('Exiting.')
#    sys.exit()
