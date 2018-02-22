import os
import sys
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
<<<<<<< HEAD
PATH = '/home/pi/IMG_1881_15.mov'
=======
PATH = '/home/pi/IMG_1881_8.mov'
>>>>>>> 4210870d7be2d8cd3a738f02b74de7002d75a9d1
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
<<<<<<< HEAD

MAX_POINT_READING = 150
MIN_POINT_READING = 10

MAX_AMBIENT_READING = 530
MIN_AMBIENT_READING = 350
=======
>>>>>>> 4210870d7be2d8cd3a738f02b74de7002d75a9d1

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
    global PROCESSES
    if __name__ == '__main__':
        _i2cthread = I2CAgent(I2C_SETTINGS, JOBQUEUE, READINGSQUEUE)
        PROCESSES.append(_i2cthread)
        _i2cthread.start()

PLAYER = OMXPlayer(PATH, ['-b', '--loop', '--no-osd'])
<<<<<<< HEAD
#PROCESSES.append(PLAYER)
=======
PROCESSES.append(PLAYER)
>>>>>>> 4210870d7be2d8cd3a738f02b74de7002d75a9d1

def pulseplayer():
    """Glorified test function"""
    PLAYER.play()
    sleep(5)
    PLAYER.pause()

LAST_READING = [0, 0]
<<<<<<< HEAD
PLAYRATE = 3 #the current playback rate
TARGETRATE = 1
=======
PLAYRATE = 10 #the current playback rate
>>>>>>> 4210870d7be2d8cd3a738f02b74de7002d75a9d1
def updateplayer(_readingset):
    """Apply the latest change in reading to the video playback.
    """
    global LAST_READING
    global PLAYRATE
<<<<<<< HEAD
    global TARGETRATE
    RATE = 1
    if _readingset[0] == 0 and _readingset[1] == 0:
        _readingset = [5,1]
    if _readingset[1] == 1:
        RATE = 1
    if _readingset[1] > 1 and _readingset[1] <= 2:
        RATE = 2
    if _readingset[1] >= 3 and _readingset[1] <= 8:
        RATE = 3
    if _readingset[1] > 8 and _readingset[1] <= 11:
        RATE = 4

    TARGETRATE = RATE
    reconcileplayrate()
    LAST_READING = _readingset

def reconcileplayrate():
    global PLAYRATE
    global TARGETRATE
    if TARGETRATE > PLAYRATE:
        PLAYER.action(2)
        print 'speeding up'
        PLAYRATE += 1
        return
    elif TARGETRATE < PLAYRATE:
        PLAYER.action(1)
        print 'slowing'
        PLAYRATE -= 1
        return
=======

    reconcileplayrate(_readingset[0], _readingset[1])
    LAST_READING = _readingset

def reconcileplayrate(_rate, _mod):
    global PLAYRATE
    print _mod
    global IS_FF
    if _mod > 1:
        PLAYER.action(2)
        if PLAYRATE < 11:
            PLAYRATE += 1
    else:
        if _rate > PLAYRATE:
            PLAYER.action(2)
            print 'speeding up'
            PLAYRATE += 1
            IS_FF = False
        elif _rate < PLAYRATE:
            PLAYER.action(1)
            print 'slowing'
            PLAYRATE -= 1
            IS_FF = False
>>>>>>> 4210870d7be2d8cd3a738f02b74de7002d75a9d1


def quitplayer():
    """Gracefully quit the omxplayer"""
    logging.info('Stopping OMX...')
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
    global PROCESSES
    logging.info('Shutting down workers...')
    for proc in PROCESSES:
        print 'found worker'
        #if proc.is_alive():
        print 'stopping worker'
        proc.stop()
        proc.join()

def cleanreboot():
    """Superstitious daily restart"""
    logging.info('Rebooting...')
    schedule.clear()
    #stopworkerthreads()
    #quitplayer()
    os.system('sudo reboot now')

def cleanstop():
    logging.info('Stopping IO and video...')
    quitplayer()
    stopworkerthreads()
    #time.sleep(2)
    #sys.exit()

#stop the vdeo at 10pm
schedule.every().day.at("7:00").do(cleanreboot)
schedule.every().day.at("22:00").do(cleanstop)
spinupi2c()
<<<<<<< HEAD
=======
#checkforparamsfile()
>>>>>>> 4210870d7be2d8cd3a738f02b74de7002d75a9d1

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
    logging.info('System or key interrupt triggered.')
    saveparams()
    quitplayer()
    stopworkerthreads()

