import serial
import serial.tools.list_ports

class Radio():
    def __init__(self, type, baudrate):
        self.type = type #radio type string
        self.port = serial.Serial()
        self.port.baudrate = baudrate
        self.port.timeout = 0.1 #seconds
        self.portname = None
        self.frequency = None
        self.last_packet = None
    
    '''
    NOTE:
    The serial buffer will repeatedly time out if the MSP430 is connected while the debugger is active
    To get around this: simply don't try to run the ground station while the MSP430 is in debug mode
    '''
    def attempt_connection(self, portname):
        self.port.port = portname
        print("Attempting to connect {} on port {}".format(self.type, portname))
        try:
            self.port.open()
            self.port.reset_input_buffer() #flush buffer
            self.port.write(b'a') #send info request
            reply = self.port.read(13) #get returned info
            #print(reply)
            
            if len(reply) >= 3 and reply[0] == 170 and reply[1] == 170: #valid preamble
                if chr(reply[2]) == 'U':
                    if self.type == "UHF":
                        print("UHF Modem Connection Accepted")
                        self.port.reset_input_buffer()
                    else:
                        self.port.close()
                elif chr(reply[2]) == 'S':
                    if self.type == "SBand":
                        print("S-Band Modem Connection Accepted")
                        self.port.reset_input_buffer()
                    else:
                        self.port.close()
                else:   
                    self.port.close()
            else:
                self.port.close()
        except serial.serialutil.SerialException:
            if self.port.is_open:
                self.port.close()
                
    def get_info(self):
        info = {}
        try:
            if self.port.is_open and self.port.in_waiting == 0: #if there is no flag waiting in the buffer
                self.port.write(b'a') #send info request
                reply = self.port.read(13)
                info["frequency"] = reply[3:13].decode("utf-8")
        except serial.serialutil.SerialException:
            pass

        return info
        
    def monitor_flags(self): #monitor communication initiated by the radio module
        try:
            if self.port.is_open and self.port.in_waiting != 0:
                print("Buffer was not empty!")
                print(self.port.read(self.port.in_waiting)) #read buffer
        except serial.serialutil.SerialException:
            pass