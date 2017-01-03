import time
import smbus
import logging
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
        self.lastreading = 0

    def run(self):
        while self.cont:
            if not self.myjobs.empty():
                currentjob = self.myjobs.get()
                if currentjob.directive == "CHANGE_SAMPLERATE":
                    self.delay = currentjob.fine
            self.getlatestreadings()

    def terminate(self):
        print 'Terminating...'
        self.cont = False

    def getlatestreadings(self):
        """Grab the latest reading from the client device and throw it on our queue"""
        try:
            rawinput = self.bus.read_i2c_block_data(self.internaddr, 0)
            self.lastreading = rawinput[0]
            self.myreadings.put(rawinput)
        except IOError as err:
            logging.info('i2c encountered a problem. %s', err)
            rawinput = self.lastreading
            self.myreadings.put(self.lastreading)
        time.sleep(self.delay)
