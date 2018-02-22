import time
import struct
import smbus
import logging
import math
from multiprocessing import Process, Queue

class I2CAgent(Process):
    """Process to retrieve i2c readings from a client device"""
    def __init__(self, _settings, _jobqueue, _readingsqueue):
        super(I2CAgent, self).__init__()
        print 'starting i2c agent'
        self.cont = True
        self.myjobs = _jobqueue
        self.myreadings = _readingsqueue
        self.bus = smbus.SMBus(1)
        self.internaddr = _settings.internaddr
        self.delay = _settings.delay
        self.lastreading = [5, 1]

    def run(self):
        while self.cont:
            if not self.myjobs.empty():
                currentjob = self.myjobs.get()
                if currentjob.directive == "CHANGE_SAMPLERATE":
                    self.delay = currentjob.fine
            self.getlatestreadings()
        while not self.cont:
            return

    def stop(self):
        print 'Terminating...'
        self.cont = False
        super(I2CAgent, self).terminate()

    def getlatestreadings(self):
        """Grab the latest reading from the client device and throw it on our queue"""
        try:
            rawinput = self.bus.read_i2c_block_data(0x08, 0)
            self.lastreading[0] = rawinput[0] #between 1 and 11, usu. 5-7
            self.lastreading[1] = rawinput[1]
            self.lastreading[1] = rawinput[1] #between 1 and 11. usu. 1
            self.myreadings.put(self.lastreading)
        except IOError as err:
            logging.info('i2c encountered a problem. %s', err)
            rawinput = self.lastreading
            self.myreadings.put(self.lastreading)
            print 'Problem'
        time.sleep(self.delay)
