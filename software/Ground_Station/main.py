import cv2 as cv
import PIL
from PIL import ImageTk
from PIL import Image
import numpy as np
from mss import mss
import time

import tkinter as tk
from matplotlib.backends.backend_tkagg import (
    FigureCanvasTkAgg, NavigationToolbar2Tk)
from matplotlib.backend_bases import key_press_handler
from matplotlib.figure import Figure
import matplotlib.animation as animation
from matplotlib import style

import serial
import serial.tools.list_ports

import radio
import imaging

res = [160, 120]

class MainWindow():
    def __init__(self, window):
        self.window = window
        self.width = res[0]*4
        self.height = res[1]*4
        self.widgets = {}
        self.frame = np.zeros((120, 160), dtype='uint8') #image frame
        
        #radio serial port instances
        self.UHF = radio.Radio("UHF", 115200)
        self.SBand = radio.Radio("SBand", 115200)
        
        #telemetry database
        self.telemetry_packets = [] #list of dicts
        self.t = [];
        self.a_x = []
        self.a_y = []
        self.a_z = []
        
        #plots
        style.use('ggplot')
        self.accel_plot = Figure(figsize=(10, 4), dpi=50)
        self.accel_ax = self.accel_plot.add_subplot()
        self.accel_ax.set_xlim(0, 100)
        self.accel_ax.set_ylim(0, 255)
        self.accel_x, = self.accel_ax.plot(self.t, self.a_x, label='x')
        self.accel_y, = self.accel_ax.plot(self.t, self.a_y, label='y')
        self.accel_z, = self.accel_ax.plot(self.t, self.a_z, label='z')
        self.accel_ax.set_xlabel("Time")
        self.accel_ax.set_ylabel("Acceleration")
        self.accel_ax.legend()
        self.accel_canvas = FigureCanvasTkAgg(self.accel_plot, self.window)
        self.accel_canvas.get_tk_widget().grid(column=3,row=1,columnspan=2)
        self.accel_animation = animation.FuncAnimation(self.accel_plot, self.animate_plot, interval=100, blit=False)
       
        
        #image canvas
        self.widgets['img_title'] = tk.Label(self.window, text="S-Band Image Downlink", font=("Arial", 25))
        self.widgets['img_title'].grid(row=0, column=0, columnspan=2,sticky='NSEW')
        
        self.widgets['canvas'] = tk.Canvas(self.window, width=self.width, height=self.height)
        self.widgets['canvas'].grid(row=1, column=0, rowspan=2, columnspan=2, sticky='NSEW')
        
        self.widgets['telem_title'] = tk.Label(self.window, text="UHF Telemetry", font=("Arial", 25))
        self.widgets['telem_title'].grid(row=0,column=3, columnspan=2,sticky='NSEW')
        
        #radio status
        self.widgets['status_title'] = tk.Label(self.window, text="Radio Status", font=("Arial", 25))
        self.widgets['status_title'].grid(row=3,column=0,columnspan=2,sticky='NSEW')
        
        self.widgets['sband_status'] = tk.Frame(self.window,highlightbackground="black",highlightthickness=2)
        self.widgets['sband_status'].grid(row=4,column=0,sticky='NSEW')
        tk.Label(self.widgets['sband_status'], text="S-Band Radio", font=("Arial", 15)).grid(row=0,column=0,columnspan=2)
        tk.Label(self.widgets['sband_status'], text="Serial:", font=("Arial", 15)).grid(row=1,column=0)
        self.widgets['sband_connected'] = tk.Label(self.widgets['sband_status'], text="Not Connected", font=("Arial", 15), fg="red")
        self.widgets['sband_connected'].grid(row=1,column=1)
        
        tk.Label(self.widgets['sband_status'], text="Last Packet:", font=("Arial", 15)).grid(row=2,column=0)
        self.widgets['sband_last'] = tk.Label(self.widgets['sband_status'], text="N/A", font=("Arial", 15), fg="red")
        self.widgets['sband_last'].grid(row=2,column=1)
        
        tk.Label(self.widgets['sband_status'], text="Frequency:", font=("Arial", 15)).grid(row=3,column=0)
        self.widgets['sband_frequency'] = tk.Label(self.widgets['sband_status'], text="N/A", font=("Arial", 15), fg="red")
        self.widgets['sband_frequency'].grid(row=3,column=1)
        
        self.widgets['uhf_status'] = tk.Frame(self.window,highlightbackground="black",highlightthickness=2)
        self.widgets['uhf_status'].grid(row=4,column=1,sticky='NSEW')
        tk.Label(self.widgets['uhf_status'], text="UHF Radio", font=("Arial", 15)).grid(row=0,column=0,columnspan=2)
        tk.Label(self.widgets['uhf_status'], text="Serial:", font=("Arial", 15)).grid(row=1,column=0)
        tk.Label(self.widgets['uhf_status'], text="Not Connected", font=("Arial", 15), fg="red").grid(row=1,column=1)
        
        tk.Label(self.widgets['uhf_status'], text="Last Packet:", font=("Arial", 15)).grid(row=2,column=0)
        tk.Label(self.widgets['uhf_status'], text="N/A", font=("Arial", 15), fg="red").grid(row=2,column=1)
        
        tk.Label(self.widgets['uhf_status'], text="Frequency:", font=("Arial", 15)).grid(row=3,column=0)
        tk.Label(self.widgets['uhf_status'], text="N/A", font=("Arial", 15), fg="red").grid(row=3,column=1)
        
        '''
        for i in range(0,4):
            window.grid_columnconfigure(i,weight=1)
            window.grid_rowconfigure(i,weight=1)
        '''
        self.update_image()
        self.connect_radios()
        self.get_radio_info()
        self.radio_poll_rx()
        
    def update_image(self):
        frame = imaging.GRB422toBGR(self.frame, res[0], res[1])
        frame = cv.resize(frame, (self.width, self.height)) 
        frame = cv.cvtColor(frame, cv.COLOR_BGR2RGB)
        
        frame = Image.fromarray(frame) # to PIL format
        self.image = ImageTk.PhotoImage(frame) # to ImageTk format
        
        self.widgets['canvas'].create_image(0, 0, anchor=tk.NW, image=self.image)
        self.window.after(500, self.update_image)
        
    def animate_plot(self, i):
        try:
            for packet in self.telemetry_packets:
                if (len(self.a_x) == 100):
                    self.a_x.pop(0)
                    self.a_y.pop(0)
                    self.a_z.pop(0)
                else:
                    self.t.append(len(self.a_x))
                
                self.a_x.append(packet['Acceleration'][0])
                self.a_y.append(packet['Acceleration'][1])
                self.a_z.append(packet['Acceleration'][2])
                self.accel_x.set_data(self.t, self.a_x)
                self.accel_y.set_data(self.t, self.a_y)
                self.accel_z.set_data(self.t, self.a_z)
        except IndexError:
            pass
    
    def connect_radios(self):
        ports = serial.tools.list_ports.comports()
        portnames = []
        
        for port, desc, hwid in ports:
            portnames.append(port)
        
        if self.SBand.port.is_open and self.SBand.port.port not in portnames: #close port if disconnected
            self.SBand.port.close()
        if self.UHF.port.is_open and self.UHF.port.port not in portnames: #close port if disconnected
            self.UHF.port.close()
        
        for port in portnames:
            if not self.SBand.port.is_open: #if port is closed
                self.SBand.attempt_connection(port)
            if not self.UHF.port.is_open:
                self.UHF.attempt_connection(port)
            
        if self.SBand.port.is_open:
            self.widgets['sband_connected'].destroy()
            self.widgets['sband_connected'] = tk.Label(self.widgets['sband_status'], text="Connected", font=("Arial", 15), fg="green")
            self.widgets['sband_connected'].grid(row=1,column=1)
        else:
            self.widgets['sband_connected'].destroy()
            self.widgets['sband_connected'] = tk.Label(self.widgets['sband_status'], text="Not Connected", font=("Arial", 15), fg="red")
            self.widgets['sband_connected'].grid(row=1,column=1)
        
        self.window.update()
        self.window.after(5000, self.connect_radios)
        
    def get_radio_info(self):
        try:
            if self.SBand.port.is_open:
                info = self.SBand.get_info()
                if info:
                    self.widgets['sband_frequency'].destroy()
                    self.widgets['sband_frequency'] = tk.Label(self.widgets['sband_status'], text=info['frequency'].lstrip('0') + " Hz", font=("Arial", 15), fg="green")
                    self.widgets['sband_frequency'].grid(row=3,column=1)
                
                if self.SBand.last_packet:
                    self.widgets['sband_last'].destroy()
                    self.widgets['sband_last'] = tk.Label(self.widgets['sband_status'], text=str(int(time.time() - self.SBand.last_packet)) + "s", font=("Arial", 15), fg="green")
                    self.widgets['sband_last'].grid(row=2,column=1)
            elif not self.SBand.port.is_open:
                self.widgets['sband_frequency'].destroy()
                self.widgets['sband_frequency'] = tk.Label(self.widgets['sband_status'], text="N/A", font=("Arial", 15), fg="red")
                self.widgets['sband_frequency'].grid(row=3,column=1)
                
                self.widgets['sband_last'].destroy()
                self.widgets['sband_last'] = tk.Label(self.widgets['sband_status'], text="N/A", font=("Arial", 15), fg="red")
                self.widgets['sband_last'].grid(row=2,column=1)
        except serial.serialutil.SerialException:
            pass
        except UnicodeDecodeError:
            pass
        except IndexError:
            pass
            
        
        self.window.after(1000, self.get_radio_info)
        
    def radio_poll_rx(self):
        try:
            if self.SBand.port.is_open:
                status = self.SBand.poll_rx()
                if status[1] != 0:
                    packets = self.SBand.burst_read(status[4], status[1])
                    
                    if (status[4] == 18): #if image packet
                        for i in range(0, status[1]):
                            packet = packets[(18*i):(18*(i+1))]
                            index = int.from_bytes(packet[0:2], 'big')
                            index &= ~0x8000
                            if (index == 0): #clear new frame
                                self.frame.fill(0)
                            
                            try:
                                for i in range(0,16):
                                    self.frame[int(index / 10)][((index % 10) * 16) + i] = packet[2 + i]
                            except IndexError:
                                print(status)
                                print(packet)
                    
                    elif (status[4] == 32): #if telemetry packet
                        for i in range(0, status[1]):
                            packet = list(packets[(32*i):(32*(i+1))])
                            
                            packet_data = {}
                            packet_data['Acceleration'] = packet[1:4] #x,y,z
                            packet_data['Angular Rate'] = packet[4:7]
                            packet_data['Magnetic Field'] = packet[7:10]
                            
                            if (len(self.telemetry_packets) == 100):
                                self.telemetry_packets.pop(0)
                            
                            self.telemetry_packets.append(packet_data)
                        
                    
        except (serial.serialutil.SerialException, IndexError):
            pass
        
        self.window.after(10, self.radio_poll_rx)
    
def main():
    print("One day I will be a ground station, big and tall.")
    print("But for now I'm just a placeholder, short and small...")
    
    root = tk.Tk()
    root.title("Groundstation")
    MainWindow(root)
    root.mainloop()
    
if __name__ == "__main__":
    main()