from tkinter import *
from tkinter import ttk
from tkinter import font
import serial
import time
import threading
import queue
import numpy as np
from math import sqrt


VERBOSE = False

## SERIAL COMM GLOBALS
SERIAL_PORT = '/dev/ttyACM0'
SERIAL_BAUD = 9600
SERIAL_TMOT = 1 # s
######

## GUI GLOBALS
TITLE = 'Project FEEL'
WIDTH = 1024
HEIGHT = 800
PROMPT = '::> '
R = 10
ACTUATED = 'red'
DEFENDED = 'green'
NEUTRAL = 'black'
FENCER_IMAGE = 'images/fencer.gif'
#READ_MOTOR_PRD = 50 #ms
GUI_UPDATE_PRD = 20 # ms
#####

## ANALYZER GLOBALS
PROJECTION_FILE = 'new_clusters/2-trans_4,6_projection.csv'
CENTRIOD_FILE = 'new_clusters/2-trans_4,6_centroids.txt'
#PROJECTION_FILE = 'clusters/2-trans_4,6_projection.csv'
#CENTRIOD_FILE = 'clusters/2-trans_4,6_centroids.txt'

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

MIN_PROJ_COMP = 1500.0
#MAX_PROJ_COMP = 20000.0
#MIN_CLUST_DIST = 200.0
#MAX_CLUST_DIST = 7000.0
MAX_CLUST_DIST = 17000.0
BOUNDS_SIX_TO_FOUR = ((-193.30, -3157.05, -706.21), (4695.0, 1291.0, 4141.8))
BOUNDS_FOUR_TO_SIX = ((-3844.12, -1730.82, -3920.7), (344.57, 5681.27, 2791.4))
#BOUNDS_FOUR_TO_SIX = ((-12538.0, -15779), (640.09, 7774.2))
#BOUNDS_FOUR_TO_SIX = ((-12538.0, -15779), (640.09, 7774.2))

DEFENDED_FOUR = [False, False, False, True, False, True, False]
DEFENDED_SIX = [False, False, True, False, True, False, False]

HYSTERESIS_VAL = 24
#HYSTERESIS_VAL = WINDOW_SIZE
#####


def distance(p1, p2):
    x1, y1, z1 = p1[0], p1[1], p1[2]
    x2, y2, z2 = p2[0], p2[1], p2[2]
    return sqrt((x2 - x1)**2 + (y2 - y1)**2 + (z2 - z1)**2)


def in_bounds(p, box):
    (bxl, byl, bzl), (bxh, byh, bzh) = box
    px, py, pz = p[0], p[1], p[2]
    #print('Bounds:', (px >= bxl), (px <= bxh), (py >= byl), (py <= byh))
    return (px >= bxl) and (px <= bxh) and (py >= byl) and (py <= byh) and (pz >= bzl) and (pz <= bzh)


#class Gui(threading.Thread):
class Gui:
    def __init__(self, s_in_q, s_out_q, a_in_q):
        self.s_in_q = s_in_q
        self.s_out_q = s_out_q
        self.a_in_q = a_in_q
        self.motors_actuated = [False]*7
        self.motors_defended = [False]*7
        self.motor_clicked = None
        self.quit_event = threading.Event()

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
        #self.canvas.bind('<1>', self.motor_click_handler)

        for key in self.motors.keys():
            self.draw_motor_status(key, NEUTRAL)
        #self.root.after(READ_MOTOR_PRD, self.read_motor_vals)

        #threading.Thread.__init__(self)
        #self.size = (width, height)

    def run(self):
        #try:
        self.root.after(GUI_UPDATE_PRD, self.update_gui)
        self.root.mainloop()
        #except KeyboardInterrupt:
        #    self.callback()

    def callback(self):
        self.root.quit()

    def motor_click_handler(self, motor_id):
        self.motor_clicked = motor_id
        #if VERBOSE:
        print('GUI: got click to motor', motor_id)
    #def motor_click_handler(self, mouse_event):
        #print('In click handler')
        #mouse_pos = mouse_event.x, mouse_event.y
        #for motor_id, motor_dict in self.motors.items():
        #    d = distance(mouse_pos, motor_dict['pos'])
        #    if d < (R + 5):
        #        self.motor_clicked = motor_id
        #        self.motors_actuated[motor_id] = not self.motors_actuated[motor_id]
        #        print('GUI: got click')
        #        break

    def draw_motor_status(self, motor_id, color):
        motor = self.motors[motor_id]
        if motor['obj']:
            self.canvas.delete(motor['obj'])
        (x, y) = motor['pos']
        item = self.canvas.create_oval((x-R, y-R, x+R, y+R), fill=color)
        #item = self.canvas.create_oval((x-R, y-R, x+R, y+R), fill=motor['state'])
        self.canvas.tag_bind(item, '<1>', lambda e: self.motor_click_handler(motor_id))
        self.motors[motor_id]['obj'] = item

    def get_data_from_serial(self):
        if not self.s_in_q.empty():
            try:
                actuated = self.s_in_q.get_nowait()
                for i, act in enumerate(actuated):
                    self.motors_actuated[i] = act
                self.s_in_q.task_done()
                if VERBOSE:
                    print('GUI: got data from serial')
            except queue.Empty:
                pass

    def get_data_from_analyzer(self):
        if not self.a_in_q.empty():
            try:
                defended = self.a_in_q.get_nowait()
                for i, defd in enumerate(defended):
                    self.motors_defended[i] = defd
                self.a_in_q.task_done()
                if VERBOSE:
                    print('GUI: got data from serial')
            except queue.Empty:
                pass

    def send_data_to_serial(self):
        if not self.s_out_q.full() and self.motor_clicked:
            self.s_out_q.put_nowait(self.motor_clicked)
            if VERBOSE:
                print('GUI: sent data to serial')
        self.motor_clicked = None

    def update_gui(self):
        if self.quit_event.is_set():
            self.callback()
            return
        for i, (actuated, defended) in enumerate(zip(self.motors_actuated, self.motors_defended)):
            color = None
            if defended:
                color = DEFENDED
            elif actuated:
                color = ACTUATED
            else:
                color = NEUTRAL
            self.draw_motor_status(i + 1, color)
        self.get_data_from_serial()
        self.get_data_from_analyzer()
        self.send_data_to_serial()
        self.root.after(GUI_UPDATE_PRD, self.update_gui)


class SerialComm(threading.Thread):
    def __init__(self, a_in_q, a_out_q, g_in_q, g_out_q, serial_port):
        self.a_in_q = a_in_q
        self.a_out_q = a_out_q
        self.g_in_q = g_in_q
        self.g_out_q = g_out_q
        self.ser = serial_port
        self.ds = np.zeros(VECT_SIZE)
        self.motors_defended = [False]*7
        self.motors_actuated = [False]*7
        self.motor_clicked = None
        self.quit_event = threading.Event()
        threading.Thread.__init__(self)

    def send_data_to_serial(self):
        if self.motors_defended:
            defensed = [str(int(x)) for x in self.motors_defended]
            clicked_str = None
            if self.motor_clicked:
                clicked_str = str(int(self.motor_clicked))
            else:
                clicked_str = '0'
            self.motor_clicked = None
            defensed.insert(0, clicked_str)
            data_str = ','.join(defensed).encode('ascii', 'replace')
            self.ser.write(data_str)
            if VERBOSE:
                print('SERIAL: wrote data to serial')
                print(' --> Sent:', data_str)

    def send_data_to_analyzer(self):
        if not self.a_out_q.full():
            self.a_out_q.put_nowait(self.ds)
            if VERBOSE:
                print('SERIAL: send data to analyzer')

    def send_data_to_gui(self):
        if not self.g_out_q.full():
            self.g_out_q.put_nowait(self.motors_actuated)
            if VERBOSE:
                print('SERIAL: sent data to gui')
            #self.out_q.put_nowait((self.ds[:], self.motor_data_actuated[:]))
        #print('ds ==', self.ds)

    def get_data_from_serial(self):
        line = self.ser.readline().decode()
        if len(line) > 0 and not (line[0:6] == 'DEBUG:'):
            try:
                split = [e[e.find(':') + 1:].strip() for e in line.split('\t\t')]
                vals = [int(val) for e in split[0:2] for val in e.split('\t')]
                for i, val in enumerate(vals):
                    self.ds[i] = val
                #TODO
                for i, val in enumerate(split[2].split('\t')):
                    self.motors_actuated[i] = val == '1'
                #print('Actuated motors:', self.motors_actuated)
                if VERBOSE:
                    print('SERIAL: read data from serial')
            except ValueError:
                pass
            except IndexError:
                print('Split: ', split, '\tVals: ', vals)
                #print('Split[2]: ', split[2])
        elif len(line) > 0 and (line[0:6] == 'DEBUG:'):
            #pass
            print(line)

    def get_motor_data_from_analyzer(self):
        if not self.a_in_q.empty():
            try:
                self.motors_defended = self.a_in_q.get_nowait()
                self.a_in_q.task_done()
                if VERBOSE:
                    print('SERIAL: got data from analyzer')
            except queue.Empty:
                self.motors_defended = [False]*7

    def get_data_from_gui(self):
        if not self.g_in_q.empty():
            try:
                self.motor_clicked = self.g_in_q.get_nowait()
                self.g_in_q.task_done()
                if VERBOSE:
                    print('SERIAL: got data from gui')
            except queue.Empty:
                pass

    def run(self):
        while True:
            if self.quit_event.is_set():
                break
            self.get_data_from_serial()
            self.get_motor_data_from_analyzer()
            self.get_data_from_gui()
            self.send_data_to_gui()
            self.send_data_to_analyzer()
            self.send_data_to_serial()


class DataAnalyzer(threading.Thread):
    def __init__(self, s_in_q, s_out_q, g_out_q, proj, centroids):
        self.s_in_q = s_in_q
        self.s_out_q = s_out_q
        self.g_out_q = g_out_q
        self.ds = np.zeros((VECT_SIZE, 1))
        self.proj = proj
        self.centroids = centroids
        self.window = np.zeros((VECT_SIZE, WINDOW_SIZE))
        self.current_slot = 0
        self.current_parry = SIX
        self.motors_defended = DEFENDED_SIX
        self.hysteresis = HYSTERESIS_VAL
        self.quit_event = threading.Event()
        threading.Thread.__init__(self)

    def get_data_from_serial(self):
        if not self.s_in_q.empty():
            try:
                ds = self.s_in_q.get_nowait()
                #max_reached = False
                for i, x in enumerate(ds):
                    self.ds[i][0] = x
                #    self.ds[i][0] += x
                #    max_reached = max_reached or (self.ds[i][0] > MAX_PROJ_COMP)
                #if max_reached:
                #    self.ds = np.zeros((VECT_SIZE, 1))
                for i, x in enumerate(self.ds):
                    self.window[i][self.current_slot] = x
                    #self.window[i][self.current_slot] = x
                #self.window[self.current_slot] = self.ds
                self.current_slot += 1
                if self.current_slot == WINDOW_SIZE:
                    self.current_slot = 0
                self.s_in_q.task_done()
                if VERBOSE:
                    print('ANALYZER: got data from serial')
            except queue.Empty:
                pass

    def send_data_to_serial(self):
        if not self.s_out_q.full():
            self.s_out_q.put_nowait(self.motors_defended)
            if VERBOSE:
                print('ANALYZER: send data to serial')

    def send_data_to_gui(self):
        if not self.g_out_q.full():
            self.g_out_q.put_nowait(self.motors_defended)
            if VERBOSE:
                print('ANALYZER: send data to gui')

    def process_data(self):
        if self.hysteresis > 0:
            self.hysteresis -= 1
            return
        #vector = np.sum(self.window, axis=1)
        #proj_v = self.proj.dot(vector)
        #if abs(proj_v[0]) < MIN_PROJ_COMP and abs(proj_v[1]) < MIN_PROJ_COMP:
        #    #if VERBOSE:
        #    print('ANALYZER: ignoring window value => MIN')
        #    return
        best = None
        proj = self.proj.dot(self.window)
        #print(str(proj[0:2]))
        votes = {TRANS2: 0, TRANS1: 0, None: 0}
        for x in range(WINDOW_SIZE):
            if abs(proj[0, x]) < MIN_PROJ_COMP and abs(proj[1, x]) < MIN_PROJ_COMP:
               votes[None] += 1
               continue
            d1 = distance(proj[0:3, x], self.centroids[0]['point'])
            d2 = distance(proj[0:3, x], self.centroids[1]['point'])
            #print(self.centroids[0]['tag'])
            #print(self.centroids[1]['tag'])
            #if abs(d1 - d2) < 300:
            #    votes[None] += 1
            #elif d1 <= d2:
            #    votes[TRANS2] += 1
            #else:
            #    votes[TRANS1] += 1
            voted = False
            #if in_bounds(proj[0:3, x], BOUNDS_SIX_TO_FOUR) and not self.current_parry == FOUR:
            #if in_bounds(proj[0:2, x], BOUNDS_SIX_TO_FOUR) and d1 <= d2 and not self.current_parry == FOUR:
            if in_bounds(proj[0:3, x], BOUNDS_SIX_TO_FOUR) and d1 <= d2:
                votes[TRANS2] += 1
                voted = True
            #if in_bounds(proj[0:3, x], BOUNDS_FOUR_TO_SIX) and not self.current_parry == SIX:
            #if in_bounds(proj[0:2, x], BOUNDS_FOUR_TO_SIX) and d2 <= d1 and not self.current_parry == SIX:
            if in_bounds(proj[0:3, x], BOUNDS_FOUR_TO_SIX) and d2 <= d1:
                votes[TRANS1] += 1
                voted = True
            if not voted:
                votes[None] += 1
        #print('Votes:', votes)
        best = None
        #if votes[TRANS2] > votes[TRANS1] and votes[TRANS2] >= votes[None]:
        #    best = (None, TRANS2)
        #elif votes[TRANS1] > votes[TRANS2] and votes[TRANS1] >= votes[None]:
        #    best = (None, TRANS1)
        if votes[TRANS2] > votes[TRANS1] and votes[TRANS2] >= 2:
            best = (None, TRANS2)
        elif votes[TRANS1] > votes[TRANS2] and votes[TRANS1] >= 2:
            best = (None, TRANS1)
        if not best:
            return
        #print(str(proj_v))
        #d1 = distance(proj_v[0:2], self.centroids[0]['point'])
        #d2 = distance(proj_v[0:2], self.centroids[1]['point'])
        #if d1 < d2:
        #    best = (d1, self.centroids[0]['tag'])
        #else:
        #    best = (d2, self.centroids[1]['tag'])
        #print('Distances:', d1, '\t', d2)
        #print('Distances:', self.centroids[0]['tag'], '\t', self.centroids[1]['tag'])
        ##for centroid in self.centroids:
        ##    d = distance(proj_v[0:2], centroid['point'])
        ##    if not best or d < best[0]:
        ##        best = (d, centroid['tag'])
        #if best[0] < MIN_CLUST_DIST or best[0] > MAX_CLUST_DIST:
        #    #if VERBOSE:
        #    print('ANALYZER: ignoring window value => BOUNDS')
        #    return
        #if in_bounds(proj_v, BOUNDS_SIX_TO_FOUR) and d1 > MIN_CLUST_DIST:
        #    best = (None, TRANS2)
        #elif in_bounds(proj_v, BOUNDS_FOUR_TO_SIX) and d2 > MIN_CLUST_DIST:
        #    best = (None, TRANS1)
        #else:
        #    return
        self.parry_transition(best[1])
        self.map_defended_motors()
        #self.window = np.zeros((VECT_SIZE, WINDOW_SIZE))
        #self.ds = np.zeros((VECT_SIZE, 1))
        self.hysteresis = HYSTERESIS_VAL
        if VERBOSE:
            print('HYSTERESIS')
            print('ANALYZER: processed data')

    def parry_transition(self, tag):
        if tag == TRANS1 and self.current_parry == FOUR:
            self.current_parry = SIX
            print('================================================== SIX')
        elif tag == TRANS2 and self.current_parry == SIX:
            self.current_parry = FOUR
            print('================================================== FOUR')

    def map_defended_motors(self):
        if self.current_parry == FOUR:
            self.motors_defended = DEFENDED_FOUR
        elif self.current_parry == SIX:
            self.motors_defended = DEFENDED_SIX
        print('=================================================== CURRENT PARRY:', self.current_parry)

    def run(self):
        while True:
            if self.quit_event.is_set():
                break
            self.get_data_from_serial()
            self.process_data()
            self.send_data_to_serial()
            self.send_data_to_gui()
            time.sleep(0.05)

serial_port = None
try:
    serial_to_analyzer_q = queue.Queue()
    analyzer_to_serial_q = queue.Queue()
    serial_to_gui_q = queue.Queue()
    gui_to_serial_q = queue.Queue()
    analyzer_to_gui_q = queue.Queue()

    serial_port = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=SERIAL_TMOT)
    proj = np.genfromtxt(PROJECTION_FILE, delimiter=',')
    centroids = []
    with open(CENTRIOD_FILE, 'r') as f:
        for line in f.readlines():
            (x, y, z, tag) = tuple(line.split(','))
            centroids.append({'point':(float(x), float(y), float(z)), 'tag':tag.strip()})

    gui = Gui(serial_to_gui_q, gui_to_serial_q, analyzer_to_gui_q)
    serial_comm = SerialComm(analyzer_to_serial_q, serial_to_analyzer_q, gui_to_serial_q, serial_to_gui_q, serial_port)
    data_analyzer = DataAnalyzer(serial_to_analyzer_q, analyzer_to_serial_q, analyzer_to_gui_q, proj, centroids)

    serial_comm.start()
    data_analyzer.start()
    gui.run()
    serial_comm.quit_event.set()
    data_analyzer.quit_event.set()
except KeyboardInterrupt:
    gui.quit_event.set()
    serial_comm.quit_event.set()
    data_analyzer.quit_event.set()
finally:
    serial_comm.join()
    data_analyzer.join()
    if serial_port:
        serial_port.close()
    print('Exiting.')
    sys.exit()
