import sys
import serial
import sched
import time
import numpy as np
import pca

SERIAL_PORT = '/dev/ttyACM0'
SERIAL_BAUD = 9600
SERIAL_TMOT = 1 # s

ISR_INTERVAL = 0.005
PRINT_INTERVAL = 0.050
PROC_INTERVAL = 0.007

VECT_SIZE = 6
PCA_BLK_SIZE = 12


ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=SERIAL_TMOT)

print_task = None
isr_task = None
proc_task = None

ds = np.zeros(VECT_SIZE)
pca_block = np.zeros((PCA_BLK_SIZE, VECT_SIZE))
pca_elems = 0


def print_handler():
    global ds, pca_elems, pca_block
    print 'ds(t): ', ds
    if pca_elems == PCA_BLK_SIZE:
        print 'blk(t-', PCA_BLK_SIZE, ', t): ', pca_block


def get_ds_isr():
    global ser, ds, pca_elems, pca_block
    line = ser.readline()
    if len(line) > 0:
        try:
            split = [e[e.find(':') + 1:].strip() for e in line.split('\t\t')]
            vals = [int(val) for e in split for val in e.split('\t')]

            if pca_elems == PCA_BLK_SIZE:
                pca_elems = 0
            for i, val in enumerate(vals):
                ds[i] = val
                pca_block[pca_elems][i] = val
            pca_elems += 1
            #print 'Line split: ', vals
        except ValueError:
            print 'ValueError: get_ds_isr()'


def processing():
    global ds
    pass
    #for i, ds_i in enumerate(ds):
    #    s[i] += ds_i


class PeriodicTask:
    def __init__(self, scheduler, period, handler, vargs=None):
        self.sched = scheduler
        self.period = period
        self.handler = handler
        self.vargs = vargs
        self.event = None

    def start(self):
        self.event = self.sched.enter(0, 0, self.action, ())

    def action(self):
        self.event = self.sched.enter(self.period, 0, self.action, ())
        if self.vargs:
            self.handler(*self.vargs)
        else:
            self.handler()

    def cancel(self):
        self.sched.cancel(self.event)


try:
  task_sched = sched.scheduler(time.time, time.sleep)
  print_task = PeriodicTask(task_sched, PRINT_INTERVAL, print_handler)
  isr_task = PeriodicTask(task_sched, ISR_INTERVAL, get_ds_isr)
  proc_task = PeriodicTask(task_sched, PROC_INTERVAL, processing)

  print_task.start()
  isr_task.start()
  #proc_task.start()

  print 'Starting...'
  task_sched.run()
except KeyboardInterrupt:
    print_task.cancel()
    isr_task.cancel()
    #proc_task.cancel()
    ser.close()
    print 'Exiting.'
    sys.exit()
