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
            
pktnum = 200
msg = "ya"
burst = pktnum*msg

#while True: 
print(SBand.poll_tx().hex())
print(SBand.burst_write(len(msg),pktnum,burst.encode('utf-8')))
print(SBand.poll_tx().hex())