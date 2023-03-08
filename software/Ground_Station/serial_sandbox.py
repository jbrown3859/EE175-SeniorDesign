import time
import serial
import serial.tools.list_ports

import radio

type = int(input("Radio Type (1 = SBand, 2 = UHF, 3 = SBand RX, 4 = both set to TX):"))

if (type == 1  or type == 3):
    rad = radio.Radio("SBand", 115200)
elif (type == 2):
    rad = radio.Radio("UHF", 115200)
elif (type == 4):
    rad = radio.Radio("UHF", 115200)
    sband = radio.Radio("SBand", 115200)

rad.port.timeout = 2#0.2
rad.port.inter_byte_timeout = 1#0.08

while not rad.port.is_open:
    ports = serial.tools.list_ports.comports()
    portnames = []
                
    for port, desc, hwid in ports:
        portnames.append(port)

    for port in portnames:
        rad.attempt_connection(port)
        if rad.port.is_open:
            print("Modem Connection Accepted")
            break
            
if (type == 4):
    while not sband.port.is_open:
        ports = serial.tools.list_ports.comports()
        portnames = []
                    
        for port, desc, hwid in ports:
            portnames.append(port)

        for port in portnames:
            sband.attempt_connection(port)
            if sband.port.is_open:
                print("Modem Connection Accepted")
                break

print(rad.get_radio_info())
print(rad.get_rx_buffer_state().hex())

if (type == 1):
    status = 1
    while status != 0x0:
        time.sleep(1)
        status = int.from_bytes(rad.radio_idle_mode(), "big")
        print(status)

    for i in range(0,0x30):
        print("{}: {}".format(hex(i), rad.read_radio_register(i).hex()))
        
    print("Resetting Chip")
    print(rad.radio_manual_reset())
    
    for i in range(0,0x30):
        print("{}: {}".format(hex(i), rad.read_radio_register(i).hex()))

    while True:
        print("Setting to idle")
        status = 1
        while status != 0x0:
            time.sleep(1)
            status = int.from_bytes(rad.radio_idle_mode(), "big")
            print(status)

        for i in range(0,100):
            rad.write_tx_buffer("Test packet".encode('utf-8'))
            
            tx_status = rad.get_tx_buffer_state()
            print(tx_status.hex())

        print("Setting to TX")
        status = 1
        while status != 0x80:
            time.sleep(1)
            status = int.from_bytes(rad.radio_tx_mode(), "big")
            print(status)

        for i in range(0,20):
            tx_status = rad.get_tx_buffer_state()
            print(tx_status.hex())
        
        print("Setting to RX")
        status = 1
        while status != 0x40:
            time.sleep(1)
            status = int.from_bytes(rad.radio_rx_mode(), "big")
            print(status)
            
        for i in range(0,200):
            rx_status = rad.get_rx_buffer_state()
                
            try:
                if rx_status:
                    if rx_status[1] != 0:
                        print("Requesting {} packets of size {}".format(rx_status[1], rx_status[4]))
                        packets = rad.burst_read(rx_status[4], rx_status[1])
                        print(packets)
            except IndexError:
                print(rx_status)

elif(type == 2):
    #print(rad.radio_idle_mode().hex())
    #time.sleep(1)
    #print(rad.radio_idle_mode().hex())
    
    '''
    print("Register Dump")
    for i in range(0, 0x80):
        print("{}: {}".format(hex(i),rad.read_radio_register(i).hex()))
    '''

    #test TX buffer    
    
    while True:
        print("Switching to RX")
        print(rad.radio_rx_mode().hex())
        
        print("Resetting Chip")
        print(rad.radio_manual_reset())
        
        print("Buffer State")
        print(rad.get_rx_buffer_state())
        
        print("Switching to TX")
        print(rad.radio_tx_mode().hex())
        
        print("Resetting Chip")
        print(rad.radio_manual_reset())
        
        print("Buffer State")
        print(rad.get_rx_buffer_state())
        
        print("Switching to IDLE")
        print(rad.radio_idle_mode().hex())
        
        print("Resetting Chip")
        print(rad.radio_manual_reset())
        
        print("Buffer State")
        print(rad.get_rx_buffer_state())
        
        print("Filling TX buffer")
        for i in range(0,10):
            rad.write_tx_buffer("WHAT HATH GOD WROUGHT".encode('utf-8'))
            rad.write_tx_buffer("Hello World!".encode('utf-8'))
            
            tx_status = rad.get_tx_buffer_state()
            print(tx_status.hex())
        
        print("Setting to TX")
        print(rad.radio_tx_mode().hex())
        tx_status = rad.get_tx_buffer_state()
        while tx_status[1] != 0x0 or (tx_status[0] & 0xF0) == 0xC0:
            #rad.radio_tx_mode()
            tx_status = rad.get_tx_buffer_state()
            print(tx_status.hex())
        print(rad.get_tx_buffer_state().hex())
        print("done with TX")
    
        print("Switching to RX")
        print(rad.radio_rx_mode().hex())
        time.sleep(1)
        print(rad.radio_rx_mode().hex())
        
        for i in range(0,50):
            #print("Requesting RX status")
            rx_status = rad.get_rx_buffer_state()
                
            try:
                if rx_status:
                    if rx_status[1] != 0:
                        print(rx_status.hex())
                        print("Requesting {} packets of size {}".format(rx_status[1], rx_status[4]))
                        packets = rad.burst_read(rx_status[4], rx_status[1])
                        print(packets)
            except IndexError:
                print(rx_status)
                
elif (type == 3):
    print("Switching to RX")
    print(rad.radio_rx_mode().hex())
    time.sleep(1)
    print(rad.radio_rx_mode().hex())
    
    while True:
        rx_status = rad.get_rx_buffer_state()
                
        try:
            if rx_status:
                if rx_status[1] != 0:
                    print(rx_status.hex())
                    print("Requesting {} packets of size {}".format(rx_status[1], rx_status[4]))
                    packets = rad.burst_read(rx_status[4], rx_status[1])
                    print(packets)
        except IndexError:
            print(rx_status)
            
elif (type == 4):
    print("Starting")

    status = 1
    while status != 0x80:
        time.sleep(1)
        status = int.from_bytes(rad.radio_tx_mode(), "big")
        print(status)
    
    print("UHF entered TX mode")
        
    status = 1
    while status != 0x80:
        time.sleep(1)
        status = int.from_bytes(sband.radio_tx_mode(), "big")
        print(status)

    print("SBand entered TX mode")
    
    while True:
        print("Filling UHF TX buffer")
        for i in range(0,10):
            rad.write_tx_buffer("WHAT HATH GOD WROUGHT".encode('utf-8'))
            rad.write_tx_buffer("Hello World!".encode('utf-8'))
            
            tx_status = rad.get_tx_buffer_state()
            print(tx_status.hex())
    
    
    