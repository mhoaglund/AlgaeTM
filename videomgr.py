import os
import logging
from time import sleep

from multiprocessing import Queue
import schedule
from omxplayer import OMXPlayer
from utils import I2cMgrSettings
from utils import ProcessJob
from i2cagent import I2CAgent

FILENAME = 'readings.txt'
PATH = '/home/pi/algaevid.mov'
PROCESSES = []

JOBQUEUE = Queue()
READINGSQUEUE = Queue()
COLLECTION_SPEED = 0.5
PARAMS = {
    'MAX': 0,
    'MIN': 0,
    'STDDEV': 0,
    'LOCALAVG': 0
}
PASTREADINGS = []
MAX_PASTREADINGS = 800

logging.basicConfig(format='%(asctime)s %(message)s', filename='logs.log', level=logging.DEBUG)

I2C_SETTINGS = I2cMgrSettings(
    COLLECTION_SPEED,
    8
)

def spinupi2c():
    """Activate the worker process that handles i2c"""
    if __name__ == '__main__':
        _i2cthread = I2CAgent(I2C_SETTINGS, JOBQUEUE, READINGSQUEUE)
        PROCESSES.append(_i2cthread)
        _i2cthread.start()

PLAYER = OMXPlayer(PATH, ['--loop --no-osd'])
PROCESSES.append(PLAYER)

def pulseplayer():
    """Glorified test function"""
    PLAYER.play()
    sleep(5)
    PLAYER.pause()

#TODO: make this into what it needs to be. change speed based on value, not delta increments
LAST = 0
RATE = 0
def updateplayer(_reading):
    """Apply the latest change in reading to the video playback.
    Seems like its a lot of extra work to query the dbus proxy
    for current playback speed, so it's just being done simply here"""
    global LAST
    global RATE
    if LAST == 0 or LAST == _reading:
        if RATE > 3:
            RATE -= 1
    else:
        if _reading > LAST:
            PLAYER.action(1)
            if RATE < 10:
                RATE += 1
        if _reading < LAST:
            PLAYER.action(2)
            if RATE > -10:
                RATE -= 1
    LAST = reading

def quitplayer():
    """Gracefully quit the omxplayer"""
    PLAYER.quit()

def adjustsamplerate(adjustment):
    """Order the i2c manager to change its sample rate"""
    job = ProcessJob(
        "CHANGE_SAMPLERATE",
        adjustment,
        0
    )
    JOBQUEUE.put(job)

def saveparams():
    """Save highs, lows, standard devs, etc."""
    paramsfile = open(FILENAME, 'w+')
    paramsfile.seek(0)
    paramsfile.truncate()
    for key in PARAMS:
        paramsfile.write(key)
        paramsfile.write(',')
        paramsfile.write(PARAMS[key])
        paramsfile.write('\n')
        paramsfile.close()

def openparams():
    """Open saved params info on boot"""
    paramsfile = open(FILENAME, 'r')
    for entry in paramsfile.readlines():
        record = entry.split(',')
        if record[0] in PARAMS:
            PARAMS[record[0]] = record[1]
    paramsfile.close()

def updateparams(_reading):
    """Update our parameter set"""
    if _reading > PARAMS['MAX']:
        PARAMS['MAX'] = _reading
    if _reading < PARAMS['MIN']:
        PARAMS['MIN'] = _reading
    avg = sum(PASTREADINGS)/len(PASTREADINGS)
    PARAMS['LOCALAVG'] = avg

def stopworkerthreads():
    """Stop any currently running threads"""
    for proc in PROCESSES:
        print 'found worker'
        if proc.is_alive():
            print 'stopping worker'
            proc.terminate()

spinupi2c()
try:
    while True:
        while not READINGSQUEUE.empty():
            reading = READINGSQUEUE.get()
            if len(PASTREADINGS) > MAX_PASTREADINGS:
                PASTREADINGS.pop(0)
            PASTREADINGS.append(reading)
            updateparams(reading)
            updateplayer(reading)
            print reading
        if hasattr(schedule, 'run_pending'):
            schedule.run_pending()
except (KeyboardInterrupt, SystemExit):
    print 'Interrupted!'
    quitplayer()
    stopworkerthreads()

