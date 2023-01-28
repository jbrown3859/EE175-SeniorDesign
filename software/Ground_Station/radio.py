import serial
import serial.tools.list_ports
import time

class Radio():
    def __init__(self, type, baudrate):
        self.type = type #radio type string
        self.port = serial.Serial()
        self.port.baudrate = baudrate
        self.port.timeout = 0.2 #seconds
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
        #print("Attempting to connect {} on port {}".format(self.type, portname))
        try:
            self.port.open()
            self.port.reset_input_buffer() #flush buffer
            self.port.write(b'a') #send info request
            reply = self.port.read(13) #get returned info
            #print(reply)
            
            if len(reply) >= 3 and reply[0] == 170 and reply[1] == 170: #valid preamble
                if chr(reply[2]) == 'U':
                    if self.type == "UHF":
                        #print("UHF Modem Connection Accepted")
                        self.port.reset_input_buffer()
                    else:
                        self.port.close()
                elif chr(reply[2]) == 'S':
                    if self.type == "SBand":
                        #print("S-Band Modem Connection Accepted")
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
                
    def get_radio_info(self):
        info = {}
        self.port.write(b'a') #send info request
        reply = self.port.read(13)
        if reply[0] == 170 and reply[1] == 170:
            info["frequency"] = reply[3:13].decode("utf-8")
        
        return info
        
    def get_rx_buffer_state(self): #poll rx to get the number of packets in queue
        self.port.write(b'b') #get rx info
        reply = self.port.read(5)
        return reply
            
    def read_rx_buffer(self, size):
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
        
    def flush_rx(self):
        self.port.write(b'e')
        return self.port.read(10000)
        
    def get_tx_buffer_state(self):
        self.port.write(b'f')
        reply = self.port.read(5)
        return reply
        
    def write_tx_buffer(self, packet):
        self.port.write(b'g')
        self.port.write(len(packet).to_bytes(1, byteorder='big'))
        self.port.write(bytearray(packet))
        flags = self.port.read(1)
        return flags
        
    def burst_write(self, packetsize, packetnum, packets):
        self.port.write(b'h')
        self.port.write(packetsize.to_bytes(1, byteorder='big'))
        self.port.write(packetnum.to_bytes(1, byteorder='big'))
        self.port.write(packets)
        flags = self.port.read(1)
        return flags
        
    def flush_tx(self):
        self.port.write(b'i')
        return self.port.read(10000)
        
    def write_rx_buffer(self, packet):
        self.port.write(b'p')
        self.port.write(len(packet).to_bytes(1, byteorder='big'))
        self.port.write(bytearray(packet))
        flags = self.port.read(1)
        return flags
        
    def read_tx_buffer(self, size):
        self.port.write(b'q')
        packet = self.port.read(size)
        return packet
        
    def clear_rx_flags(self):
        self.port.write(b'r')
        flags = self.port.read(1)
        return flags
        
    def clear_tx_flags(self):
        self.port.write(b's')
        flags = self.port.read(1)
        return flags
        
    def program_radio_register(self, addr, data):
        command = 0x80
        self.port.write(command.to_bytes(1, byteorder='big'))
        self.port.write(addr.to_bytes(1, byteorder='big'))
        self.port.write(data.to_bytes(1, byteorder='big'))
        return self.port.read(1) #return updated register value
        
    def read_radio_register(self, addr):
        command = 0x81
        self.port.write(command.to_bytes(1, byteorder='big'))
        self.port.write(addr.to_bytes(1, byteorder='big'))
        return self.port.read(1)
        
    def disable_radio(self):
        command = 0x82
        self.port.write(command.to_bytes(1, byteorder='big'))
        return self.port.read(1)
        
    def enable_radio(self):
        command = 0x83
        self.port.write(command.to_bytes(1, byteorder='big'))
        return self.port.read(1)