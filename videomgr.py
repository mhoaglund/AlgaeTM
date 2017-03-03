import os
import logging
import numpy
from time import sleep

from multiprocessing import Queue
import schedule
from omxplayer import OMXPlayer
from utils import I2cMgrSettings
from utils import ProcessJob
from i2cagent import I2CAgent

FILENAME = 'params.txt'
PATH = '/home/pi/IMG_1881_12.mov'
PROCESSES = []

JOBQUEUE = Queue()
READINGSQUEUE = Queue()
COLLECTION_SPEED = 1
PARAMS = {
    'MAX': 0,
    'MIN': 0,
    'STDDEV': 0,
    'LOCALAVG': 0,
    'MEDIAN': 0
}
PARAMS_UPDATE_RATE = 50
PASTREADINGS = []
MAX_PASTREADINGS = 800
PLAYBACK_INDICES = 11

MAX_POINT_READING = 150
MIN_POINT_READING = 10

MAX_AMBIENT_READING = 530
MIN_AMBIENT_READING = 350

logging.basicConfig(format='%(asctime)s %(message)s', filename='logs.log', level=logging.INFO)

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

PLAYER = OMXPlayer(PATH, ['-b', '--loop', '--no-osd'])
PROCESSES.append(PLAYER)

def pulseplayer():
    """Glorified test function"""
    PLAYER.play()
    sleep(5)
    PLAYER.pause()

LAST_READING = [0, 0]
PLAYRATE = 10 #the current playback rate
def updateplayer(_readingset):
    """Apply the latest change in reading to the video playback.
    """
    global LAST_READING
    global PLAYRATE
    print _readingset
    if _readingset[0] == 0 and _readingset[1] == 0:
        _readingset = [5,1]
    reconcileplayrate(_readingset[0], _readingset[1])
    LAST_READING = _readingset

def reconcileplayrate(_rate, _mod):
    global PLAYRATE
    #print _rate
    global IS_FF
    if _mod > 1:
        if PLAYRATE < 10:
            PLAYER.action(2)
            PLAYRATE += 1
    else:
        if _rate > PLAYRATE:
            PLAYER.action(2)
            print 'speeding up'
            PLAYRATE += 1
        elif _rate < PLAYRATE:
            PLAYER.action(1)
            print 'slowing'
            PLAYRATE -= 1


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

def checkforparamsfile():
    """Check to see if we have a params file"""
    if os.path.isfile(FILENAME) and os.stat(FILENAME).st_size > 0:
        openparams()
        return
    else:
        saveparams()

def saveparams():
    """Save highs, lows, standard devs, etc."""
    paramsfile = open(FILENAME, 'w+')
    paramsfile.seek(0)
    paramsfile.truncate()
    for key in PARAMS:
        paramsfile.write(key)
        paramsfile.write(',')
        paramsfile.write(str(PARAMS[key]))
        paramsfile.write('\n')
    paramsfile.close()

def openparams():
    """Open saved params info on boot"""
    global PARAMS
    paramsfile = open(FILENAME, 'r')
    for entry in paramsfile.readlines():
        record = entry.split(',')
        if record[0] in PARAMS:
            PARAMS[record[0]] = int(record[1])
            print record[0], record[1]
    paramsfile.close()

updatesteps = 0
def updateparams(_reading):
    """Update our parameter set"""
    global updatesteps
    global PARAMS
    if _reading > PARAMS['MAX']:
        PARAMS['MAX'] = _reading
        if PARAMS['MIN'] == 0:
            PARAMS['MIN'] = _reading
    if _reading < PARAMS['MIN'] and _reading > 0:
        PARAMS['MIN'] = _reading
    if updatesteps > PARAMS_UPDATE_RATE:
        median = numpy.median(numpy.array(PASTREADINGS))
        avg = sum(PASTREADINGS)/len(PASTREADINGS)
        stddev = numpy.std(numpy.array(PASTREADINGS))
        PARAMS['LOCALAVG'] = avg
        PARAMS['MEDIAN'] = int(median)
        PARAMS['STDDEV'] = int(stddev)/2
        updatesteps = 0
    else:
        updatesteps += 1

def stopworkerthreads():
    """Stop any currently running threads"""
    for proc in PROCESSES:
        print 'found worker'
        if proc.is_alive():
            print 'stopping worker'
            proc.terminate()

def cleanreboot():
    """Superstitious daily restart"""
    schedule.clear()
    stopworkerthreads()
    os.system('sudo reboot now')

schedule.every().day.at("7:00").do(cleanreboot)
spinupi2c()

try:
    while True:
        while not READINGSQUEUE.empty():
            readingset = READINGSQUEUE.get() #array with two values
            if len(PASTREADINGS) > MAX_PASTREADINGS:
                PASTREADINGS.pop(0)
            PASTREADINGS.append(readingset)
            #updateparams(readingset)
            updateplayer(readingset)
        if hasattr(schedule, 'run_pending'):
            schedule.run_pending()
except (KeyboardInterrupt, SystemExit):
    print 'Interrupted!'
    saveparams()
    quitplayer()
    stopworkerthreads()

