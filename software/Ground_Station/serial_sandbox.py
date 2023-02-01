import time
import serial
import serial.tools.list_ports

import radio

SBand = radio.Radio("SBand", 115200)
SBand.port.timeout = 2#0.2
SBand.port.inter_byte_timeout = 1#0.08

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

print(SBand.get_radio_info())
print(SBand.get_rx_buffer_state().hex())

status = 1
while status != 0x0:
    time.sleep(1)
    status = int.from_bytes(SBand.radio_idle_mode(), "big")
    print(status)

for i in range(0,0x30):
    print("{}: {}".format(hex(i), SBand.read_radio_register(i).hex()))


print("Setting to RX")
status = 1
while status != 0x10:
    time.sleep(1)
    status = int.from_bytes(SBand.radio_rx_mode(), "big")
    print(status)

while True:
    print("Requesting RX status")
    rx_status = SBand.get_rx_buffer_state()
    print(rx_status.hex())
        
    try:
        if rx_status and rx_status[1] != 0:
            
            print("Requesting {} packets of size {}".format(rx_status[1], rx_status[4]))
            packets = SBand.burst_read(rx_status[4], rx_status[1])
            print(packets)
           
            #print("Requesting packet of size {}".format(rx_status[4]))
            #print(SBand.read_rx_buffer(rx_status[4
            
    except IndexError:
        print(rx_status)

'''
while True:
    print("Setting to idle")
    status = 1
    while status != 0x0:
        time.sleep(1)
        status = int.from_bytes(SBand.radio_idle_mode(), "big")
        print(status)

    for i in range(0,100):
        SBand.write_tx_buffer("Test packet".encode('utf-8'))
        
        tx_status = SBand.get_tx_buffer_state()
        print(tx_status.hex())

    print("Setting to TX")
    status = 1
    while status != 0x0:
        time.sleep(1)
        status = int.from_bytes(SBand.radio_tx_mode(), "big")
        print(status)

    for i in range(0,20):
        tx_status = SBand.get_tx_buffer_state()
        print(tx_status.hex())
    
    print("Setting to RX")
    status = 1
    while status != 0x10:
        time.sleep(1)
        status = int.from_bytes(SBand.radio_rx_mode(), "big")
        print(status)
        
    for i in range(0,200):
        print("Requesting RX status")
        rx_status = SBand.get_rx_buffer_state()
            #tx_status = SBand.get_tx_buffer_state()
            #print(tx_status.hex())
            
        try:
            if rx_status:
                print(rx_status.hex())
                if rx_status[1] != 0:
                    print("Requesting {} packets of size {}".format(rx_status[1], rx_status[4]))
                    packets = SBand.burst_read(rx_status[4], rx_status[1])
                    print(packets)
        except IndexError:
            print(rx_status)
'''