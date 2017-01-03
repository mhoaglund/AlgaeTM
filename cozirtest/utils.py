"""Helper objects for configuring the system"""

class I2cMgrSettings(object):
    """
        Args:
        Delay (double, portion of a second),
        InternAddress (int, IIC address of input device),
    """
    def __init__(self, _delay, _internaddr):
        self.delay = _delay
        self.internaddr = _internaddr

class ProcessJob(object):
    """
        Args:
        Type (string: CHANGE_SAMPLERATE, etc.)
        Fine (double)
        Coarse (int)
    """
    def __init__(self, _directive, _fine, _coarse):
        self.directive = _directive
        self.fine = _fine
        self.coarse = _coarse
