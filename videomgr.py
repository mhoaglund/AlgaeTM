import logging
from time import sleep
import schedule
from multiprocessing import Queue
from omxplayer import OMXPlayer
from utils import I2cMgrSettings
from utils import ProcessJob
from i2cagent import I2CAgent

PATH = '/home/pi/algaevid.mov'
PROCESSES = []

JOBQUEUE = Queue()
READINGSQUEUE = Queue()
COLLECTION_SPEED = 1


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

PLAYER = OMXPlayer(PATH, ['--loop'])
PROCESSES.append(PLAYER)

def pulseplayer():
    """Glorified test function"""
    PLAYER.play()
    sleep(5)
    PLAYER.pause()

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

def stopworkerthreads():
    """Stop any currently running threads"""
    for proc in PROCESSES:
        print 'found worker'
        if proc.is_alive():
            print 'stopping worker'
            proc.terminate()

try:
    while True:
        while not READINGSQUEUE.empty():
            reading = READINGSQUEUE.get()
            updateplayer(reading)
            print reading
        if hasattr(schedule, 'run_pending'):
            schedule.run_pending()
except (KeyboardInterrupt, SystemExit):
    print 'Interrupted!'
    quitplayer()
    stopworkerthreads()

