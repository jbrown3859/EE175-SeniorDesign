import time
import serial
import serial.tools.list_ports

import radio

SBand = radio.Radio("SBand", 115200)
SBand.port.timeout = 0.2

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
while status != 0:
    time.sleep(1)
    status = int.from_bytes(SBand.disable_radio(), "big")
    print(status)

for i in range(0,0x30):
    print("{}: {}".format(hex(i), SBand.read_radio_register(i).hex()))

print(SBand.enable_radio())

#print(SBand.get_rx_buffer_state().hex())

#print(SBand.program_radio_register(0x0f, 0xff).hex())

#SBand.write_rx_buffer("Test packet".encode('utf-8'))

while True:
    status = SBand.get_rx_buffer_state()
    #print(status.hex())
    if status and status[1] != 0:
        print(SBand.read_rx_buffer(status[4]))
        
    #time.sleep(0.1)
