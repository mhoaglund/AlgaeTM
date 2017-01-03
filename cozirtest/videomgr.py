import logging
from time import sleep
import schedule
from multiprocessing import Queue
from omxplayer import OMXPlayer
from utils import I2cMgrSettings
from utils import ProcessJob
from i2cagent import I2CAgent

PATH = 'path/to/file.mp4'
PROCESSES = []

JOBQUEUE = Queue()
READINGSQUEUE = Queue()
COLLECTION_SPEED = 0.025


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

PLAYER = OMXPlayer(PATH)
PROCESSES.append(PLAYER)

def pulseplayer():
    """Glorified test function"""
    PLAYER.play()
    sleep(5)
    PLAYER.pause()

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
        if hasattr(schedule, 'run_pending'):
            schedule.run_pending()
except (KeyboardInterrupt, SystemExit):
    print 'Interrupted!'
    quitplayer()
    stopworkerthreads()

