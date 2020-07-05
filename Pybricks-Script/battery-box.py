import uos
from pybricks.hubs import CityHub
from pybricks.pupdevices import Motor
from pybricks.iodevices import LUMPDevice
from pybricks.parameters import Port, Stop
from pybricks.parameters import Color
from pybricks.tools import wait, StopWatch
import uerrno

print(uos.uname())

hub=CityHub()

def get_motor(port):
    """Returns a ``Motor`` object if a motor is connected to ``port`` or ``None`` if no motor is connected."""
    try:
        return Motor(port)
    except OSError as ex:
        if ex.args[0] != uerrno.ENODEV:
            raise
        return None

#Show a red blinking if not motor if present
def warning_LED(speed):
    iCount=0
    while iCount < 4:
        hub.light.on(Color.RED)
        wait(speed)
        hub.light.off()
        wait(speed)
        iCount +=1
        
# don't care which port motor is connected to
mot = get_motor(Port.A) or get_motor(Port.B)

if not mot:
    warning_LED(100)
    raise RuntimeError("No motor connected on Port A or Port B")

crono=StopWatch()
lastClick=0
ramp=0
#with less of 20% Duty Cycle the motor don't turn
x=2 
mot.dc(10*x)

while True:
    try:
        if (x==10):
            hub.light.on(Color.RED)
        else:
            hub.light.on(Color.GREEN)
        wait(100)
    except KeyboardInterrupt as e:
        #debug: print("LAST: " + str(lastClick) + " Time:" + str(crono.time()))
        if (lastClick==0):
            lastClick=crono.time()
        elif ((crono.time() - lastClick) < 350):
            #debug: print("STOOOOP LAST: " + str(lastClick) + " Time:" + str(crono.time()))
            mot.stop()  
            raise Exception("Stop by user")
        lastClick=crono.time()
        if (ramp==0):    
            x +=1
            mot.dc(10*x)
            if (x==10):
                ramp=1
            mot.dc(10*x)
        else:
            x -=1
            if (x==1):
                mot.stop() 
                raise RuntimeError("End of program")
            else:
                mot.dc(10*x)
