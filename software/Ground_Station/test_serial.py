import pytest

import serial
import serial.tools.list_ports

import radio

SBand = radio.Radio("SBand", 115200)

while not SBand.port.is_open:
    ports = serial.tools.list_ports.comports()
    portnames = []
                
    for port, desc, hwid in ports:
        portnames.append(port)

    for port in portnames:
        SBand.attempt_connection(port)
        if SBand.port.is_open:
            print("S-Band Modem Connection Accepted")
            break
            
#pktnum = 100
#msg = "Hello World"
#burst = pktnum*msg

class TestTXBuffer:
    msg = "Hello World"

    def test_tx_write_status(self):
        SBand.flush_tx()
        assert int.from_bytes(SBand.write_packet(self.msg.encode('utf-8')), 'big') == 0

    def test_tx_write_packet(self):
        SBand.flush_tx()
        SBand.write_packet(self.msg.encode('utf-8'))
        assert SBand.read_tx(len(self.msg)).decode('utf-8') == self.msg
        
    def test_burst_write_status(self):
        packetnum = 93
    
        SBand.flush_tx()
        SBand.burst_write(len(self.msg), packetnum, (packetnum*self.msg).encode('utf-8'))
        assert SBand.poll_tx().hex() == "005d03ff0b"
        
    def test_burst_write_packet(self):
        packetnum = 93
        
        SBand.flush_tx()
        SBand.burst_write(len(self.msg), packetnum, (packetnum*self.msg).encode('utf-8'))
        assert SBand.flush_tx().decode('utf-8') == (packetnum*self.msg)
        
    def test_burst_write_overflow(self):
        packetnum = 100
    
        SBand.flush_tx()
        SBand.burst_write(len(self.msg), packetnum, (packetnum*self.msg).encode('utf-8'))
        assert SBand.poll_tx().hex() == "015d03ff0b"