from loguru import logger
from time import sleep
import json

class Detection():
    def __init__(self, boxes, alarm, width, height, timestamp):
        self.boxes = boxes
        self.alarm = alarm
        self.width = width
        self.height = height
        self.timestamp = timestamp

class ListenProtocols():
    def __init__(self, mw):
        self.mw = mw
        self.cameras = {}
        self.thread_lock = False
        self.detections = {}
        self.last_timestamp = ""

    def error(self, msg):
        if msg.find("WSACancelBlockingCall") < 0:
            logger.error(msg)

    def lock(self):
        while self.thread_lock:
            sleep(0.001)
        self.thread_lock = True

    def unlock(self):
        self.thread_lock = False

    def getDetection(self, uri):
        result = None
        self.lock()
        result = self.detections.get(uri)
        self.unlock()
        return result
    
    def setDetection(self, uri, detection):
        self.lock()
        self.detections[uri] = detection
        self.unlock()

    def callback(self, msg):
        arguments = msg.split("\n\n")
        timestamp = arguments.pop(0)
        if timestamp == self.last_timestamp:
            return
        
        self.last_timestamp = timestamp
        cmd = arguments.pop(0)

        if cmd == "DETECTIONS":
            boxes = []
            camera = None
            width = 0
            height = 0
            alarm = 0
            for idx, arg in enumerate(arguments):
                match idx:
                    case 0:
                        if not self.cameras.get(arg):
                            camera = self.mw.cameraPanel.getCameraBySerialNumber(arg)
                            if camera:
                                self.cameras[arg] = camera
                        else:
                            camera = self.cameras[arg]
                    case 1:
                        width = int(arg)
                    case 2:
                        height = int(arg)
                    case 3:
                        alarm = int(arg)
                    case 4:
                        boxes = json.loads(arg)

            if camera:
                self.setDetection(camera.uri(), Detection(boxes, alarm, width, height, timestamp))
