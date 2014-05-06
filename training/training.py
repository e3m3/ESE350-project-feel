import sys
import serial
import sched
import time
import numpy as np
import pca
import threading
import Queue

SERIAL_PORT = '/dev/ttyACM1'
SERIAL_BAUD = 9600
SERIAL_TMOT = 1 # s

ISR_INTERVAL = 0.005
PRINT_INTERVAL = 0.010
PROC_INTERVAL = 0.007

VECT_SIZE = 6
PCA_BLK_SIZE = 12

DIR = 'datasets/'
FILE_OUT = 'dummy.csv'
#FILE_OUT = 'datasets/training_data_board_5.6.14_1.45.csv'

# String constants
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
TOGGLE = ''
QUIT = 'quit'

# Parry positions 1, 4, 6, 9
_1_4 = ONE + SEP + FOUR
_1_6 = ONE + SEP + SIX
_1_9 = ONE + SEP + NINE

_4_1 = FOUR + SEP + ONE
_4_6 = FOUR + SEP + SIX
_4_9 = FOUR + SEP + NINE

_6_1 = SIX + SEP + ONE
_6_4 = SIX + SEP + FOUR
_6_9 = SIX + SEP + SIX

_9_1 = NINE + SEP + ONE
_9_4 = NINE + SEP + FOUR
_9_6 = NINE + SEP + SIX

PARRY_TRANSITIONS = [
        _6_4,
        _4_6,
        ]
#PARRY_TRANSITIONS = [
#        _1_4, _1_6, _1_9,
#        _4_1, _4_6, _4_9,
#        _6_1, _6_4, _6_9,
#        _9_1, _9_4, _9_6
#        ]

class KeyboardListener(threading.Thread):
    def __init__(self, queue):
        self.t_comm_link = queue
        self.prompt = '--> '
        self.input_str = None
        threading.Thread.__init__(self)

    def run(self):
        try:
            while True:
                self.input_str = raw_input(self.prompt).lower()
                if self.input_str in PARRY_TRANSITIONS or\
                        self.input_str == TOGGLE or\
                        self.input_str == QUIT:
                    self.t_comm_link.put(self.input_str)
                    self.t_comm_link.join()
                if self.input_str == QUIT:
                    break
        except KeyboardInterrupt:
            pass


class DataRecorder(threading.Thread):
    def __init__(self, queue, serial_port, data_file_map):
        self.t_comm_link = queue
        self.ser = serial_port
        self.file_map = data_file_map
        self.ds = np.zeros(VECT_SIZE)
        self.recording = False
        self.current_handle = None
        threading.Thread.__init__(self)

    def get_ds(self):
        line = self.ser.readline()
        if len(line) > 0:
            try:
                split = [e[e.find(':') + 1:].strip() for e in line.split('\t\t')]
                vals = [int(val) for e in split for val in e.split('\t')]
                for i, val in enumerate(vals):
                    self.ds[i] = val
            except ValueError:
                pass
            except IndexError:
                print 'Split: ', split, '\tVals: ', vals

    def write_ds_to_file(self):
        print 'ds ==', self.ds 
        #
        #line = ','.join([str(x) for x in self.ds])
        #self.file_map[self.current_handle].write(line + '\n')

    def run(self):
        try:
            while True:
                if self.t_comm_link.full():
                    q_event = self.t_comm_link.get() 
                    #print 'Got event: ', q_event
                    if q_event == QUIT:
                        print 'Closing data recorder.'
                        self.t_comm_link.task_done()
                        break
                    elif q_event == TOGGLE:
                        self.recording = not self.recording
                        if self.recording:
                            print ':: RECORDING'
                        else:
                            print ':: IDLE'
                    else:
                        (a, b) = q_event.split(SEP)
                        print 'Switching to parry', a, '=>', b
                        self.current_handle = q_event
                    self.t_comm_link.task_done()
                if self.recording and self.current_handle:
                    self.ser.flushInput()
                    self.get_ds()
                    self.write_ds_to_file()
        except KeyboardInterrupt:
            pass


try:
    kbd_event_q = Queue.Queue(maxsize=1)
    serial_port = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=SERIAL_TMOT)
    training_file_map = {p:open(DIR + 'parry-' + p + '.csv', 'w')\
            for p in PARRY_TRANSITIONS}

    listener = KeyboardListener(kbd_event_q)
    recorder = DataRecorder(kbd_event_q, serial_port, training_file_map)

    def close_all():
        serial_port.close()
        for f in training_file_map.values():
            f.close()
        print 'Exiting.'
        sys.exit()

    listener.start()
    recorder.start()
    recorder.join()
finally:
    close_all()
