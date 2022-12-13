import serial
import serial.tools.list_ports
import time

class Radio():
    def __init__(self, type, baudrate):
        self.type = type #radio type string
        self.port = serial.Serial()
        self.port.baudrate = baudrate
        self.port.timeout = 0.5 #seconds
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
        self.port.write(b'a') #send info request
        reply = self.port.read(13)
        if reply[0] == 170 and reply[1] == 170:
            info["frequency"] = reply[3:13].decode("utf-8")
        
        return info
        
    def poll_rx(self): #poll rx to get the number of packets in queue
        self.port.write(b'b') #get rx info
        reply = self.port.read(5)
        #print(reply)
        return reply
            
    def get_packet(self, size):
        self.port.write(b'c')
        packet = self.port.read(size)
        self.last_packet = time.time()
        return packet
        
    def burst_read(self, packetsize, packetnum):
        self.port.write(b'd')
        self.port.write(packetsize.to_bytes(1, byteorder='big'))
        self.port.write(packetnum.to_bytes(1, byteorder='big'))
        packets = self.port.read(packetsize * packetnum)
        self.last_packet = time.time()
        return packets