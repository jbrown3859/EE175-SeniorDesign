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

class TestRXBuffer:
    msg1 = "Hello World"
    msg2 = "Hi"
    
    def test_rx_clear_flags(self):
        SBand.clear_rx_flags()
        SBand.flush_rx()
        for i in range(0,100):
            SBand.write_rx_buffer(self.msg1.encode('utf-8'))
        SBand.flush_rx()
        SBand.clear_rx_flags()
        assert SBand.get_rx_buffer_state().hex() == '0000000000'
    
    def test_rx_flush(self):
        SBand.clear_rx_flags()
        SBand.write_rx_buffer(self.msg1.encode('utf-8'))
        SBand.flush_rx()
        assert SBand.get_rx_buffer_state().hex() == '0000000000'
    
    def test_debug_rx_write_status(self):
        SBand.clear_rx_flags()
        SBand.flush_rx()
        SBand.write_rx_buffer(self.msg1.encode('utf-8'))
        assert SBand.get_rx_buffer_state().hex() == '0001000b0b'
    
    def test_rx_read_packet(self):
        SBand.clear_rx_flags()
        SBand.flush_rx()
        SBand.write_rx_buffer(self.msg1.encode('utf-8'))
        assert SBand.read_rx_buffer(len(self.msg1)).decode('utf-8') == self.msg1
        
    def test_rx_burst_read(self):
        SBand.clear_rx_flags()
        SBand.flush_rx()
        for i in range(0,93):
            SBand.write_rx_buffer(self.msg1.encode('utf-8'))
        assert SBand.burst_read(len(self.msg1),93).decode('utf-8') == (93*self.msg1)
        
    def test_rx_data_overflow(self):
        SBand.clear_rx_flags()
        SBand.flush_rx()
        for i in range(0, 100):
            SBand.write_rx_buffer(self.msg1.encode('utf-8'))
        assert SBand.get_rx_buffer_state().hex() == "015d03ff0b"
        
    def test_rx_packet_overflow(self):
        SBand.clear_rx_flags()
        SBand.flush_rx()
        for i in range(0, 150):
            SBand.write_rx_buffer(self.msg2.encode('utf-8'))
        assert SBand.get_rx_buffer_state().hex() == "027f00fe02"
        
    def test_rx_mixed_packets(self):
        SBand.clear_rx_flags()
        SBand.flush_rx()
        
        for i in range(0,50):
            SBand.write_rx_buffer(self.msg1.encode('utf-8'))
            SBand.write_rx_buffer(self.msg2.encode('utf-8'))
            
        for i in range(0,50):
            SBand.read_rx_buffer(len(self.msg1))
            SBand.read_rx_buffer(len(self.msg2))
            
        assert SBand.get_rx_buffer_state().hex() == "0000000000"
        
    def test_rx_burst_read_halt(self):
        SBand.clear_rx_flags()
        SBand.flush_rx()
        
        for i in range(0,10):
            SBand.write_rx_buffer(self.msg1.encode('utf-8'))
            
        for i in range(0, 10):
            SBand.write_rx_buffer(self.msg2.encode('utf-8'))
            
        SBand.burst_read(len(self.msg1), 20)
        assert SBand.get_rx_buffer_state().hex() == "000a001402"

class TestTXBuffer:
    msg = "Hello World"
    
    def test_tx_clear_flags(self):
        SBand.clear_tx_flags()
        SBand.flush_tx()
        for i in range(0,100):
            SBand.write_tx_buffer(self.msg.encode('utf-8'))
        SBand.flush_tx()
        SBand.clear_tx_flags()
        assert SBand.get_tx_buffer_state().hex() == '0000000000'
    
    def test_tx_flush(self):
        SBand.clear_tx_flags()
        SBand.flush_tx()
        SBand.write_tx_buffer(self.msg.encode('utf-8'))
        SBand.flush_tx()
        assert SBand.get_tx_buffer_state().hex() == "0000000000"
    
    def test_tx_write_status(self):
        SBand.clear_tx_flags()
        SBand.flush_tx()
        assert int.from_bytes(SBand.write_tx_buffer(self.msg.encode('utf-8')), 'big') == 0

    def test_tx_write_tx_buffer(self):
        SBand.clear_tx_flags()
        SBand.flush_tx()
        SBand.write_tx_buffer(self.msg.encode('utf-8'))
        assert SBand.read_tx_buffer(len(self.msg)).decode('utf-8') == self.msg
        
    def test_burst_write_status(self):
        packetnum = 93
    
        SBand.clear_tx_flags()
        SBand.flush_tx()
        SBand.burst_write(len(self.msg), packetnum, (packetnum*self.msg).encode('utf-8'))
        assert SBand.get_tx_buffer_state().hex() == "005d03ff0b"
        
    def test_burst_write_tx_buffer(self):
        packetnum = 93
        
        SBand.clear_tx_flags()
        SBand.flush_tx()
        SBand.burst_write(len(self.msg), packetnum, (packetnum*self.msg).encode('utf-8'))
        assert SBand.flush_tx().decode('utf-8') == (packetnum*self.msg)
        
    def test_burst_write_overflow(self):
        packetnum = 100
    
        SBand.clear_tx_flags()
        SBand.flush_tx()
        SBand.burst_write(len(self.msg), packetnum, (packetnum*self.msg).encode('utf-8'))
        assert SBand.get_tx_buffer_state().hex() == "015d03ff0b"