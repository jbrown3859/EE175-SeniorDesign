import serial
import serial.tools.list_ports
import time

'''
Helper Functions
'''
def get_packets_from_flush(flush):
    i = 0
    num_bytes = 0
    packets = []
    while (i < len(flush)):
        num_bytes = int.from_bytes(flush[i:i+1], byteorder='big') #get len of next packet
        print(num_bytes)
        packets.append(flush[i+1:i+num_bytes+1])
        i += num_bytes + 1
    
    #print(flush)    
    return packets

class Radio():
    def __init__(self, type, baudrate):
        self.type = type #radio type string
        self.port = serial.Serial()
        self.port.baudrate = baudrate
        self.port.timeout = 0.5 #seconds
        self.port.inter_byte_timeout = 0.25
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
            reply = self.port.read(27) #get returned info
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
        reply = self.port.read(29)
        if reply[0] == 170 and reply[1] == 170:
            info["frequency"] = reply[3:13].decode("utf-8")
            info["data_rate"] = reply[13:19].decode("utf-8")
            info["bandwidth"] = reply[19:25].decode("utf-8")
            info["spreading_factor"] = reply[25:27].decode("utf-8")
            info["coding_rate"] = reply[27:29].decode("utf-8")
        
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
        if (packetsize > 0 and packetnum > 0):
            self.port.write(b'd')
            self.port.write(packetsize.to_bytes(1, byteorder='big'))
            self.port.write(packetnum.to_bytes(1, byteorder='big'))
            
            data = self.port.read(packetsize * packetnum)
            print("fetched {} bytes".format(len(data)))
            packets = []
            for i in range(int(len(data)/packetsize)):
                packets.append(data[i*packetsize:(i+1)*packetsize])
            
            self.last_packet = time.time()
            return packets
        
    def flush_rx(self):
        state = self.get_rx_buffer_state()
        len = int.from_bytes(state[2:4], byteorder='big', signed=False)
        num_pkts = int(state[1])
    
        self.port.write(b'e')
        
        #print("Buffer size: {}".format(len))
        return self.port.read(2048)
        
    def get_tx_buffer_state(self):
        self.port.write(b'f')
        reply = self.port.read(5)
        return reply
        
    def write_tx_buffer(self, packet):
        self.port.write(b'g')
        self.port.write(len(packet).to_bytes(1, byteorder='big'))
        self.port.write(bytearray(packet))
        #flags = self.port.read(1)
        #return flags
        
    def burst_write(self, packetsize, packetnum, packets):
        self.port.write(b'h')
        self.port.write(packetsize.to_bytes(1, byteorder='big'))
        self.port.write(packetnum.to_bytes(1, byteorder='big'))
        self.port.write(packets)
        #flags = self.port.read(1)
        #return flags
        
    def flush_tx(self):
        self.port.write(b'i')
        return self.port.read(10000)
        
    def write_rx_buffer(self, packet):
        self.port.write(b'p')
        self.port.write(len(packet).to_bytes(1, byteorder='big'))
        self.port.write(bytearray(packet))
        #flags = self.port.read(1)
        #return flags
        
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
        
    def set_packet_length(self, length):
        self.port.write(b'u')
        self.port.write(length.to_bytes(1, byteorder='big'))
        return self.port.read(1)
        
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
        
    def radio_idle_mode(self):
        command = 0x82
        self.port.write(command.to_bytes(1, byteorder='big'))
        return self.port.read(1)
        
    def radio_rx_mode(self):
        command = 0x83
        self.port.write(command.to_bytes(1, byteorder='big'))
        return self.port.read(1)
        
    def radio_tx_mode(self):
        command = 0x84
        self.port.write(command.to_bytes(1, byteorder='big'))
        return self.port.read(1)
        
    def radio_manual_reset(self):
        command = 0x85
        self.port.write(command.to_bytes(1, byteorder='big'))
        return self.port.read(1)