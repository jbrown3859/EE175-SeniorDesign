import cv2 as cv
import PIL
from PIL import ImageTk
from PIL import Image
import numpy as np
from mss import mss

import tkinter as tk
from matplotlib.backends.backend_tkagg import (
    FigureCanvasTkAgg, NavigationToolbar2Tk)
from matplotlib.backend_bases import key_press_handler
from matplotlib.figure import Figure

import serial
import serial.tools.list_ports

import radio
import imaging

res = [160, 120]

class MainWindow():
    def __init__(self, window, cap):
        self.window = window
        self.cap = cap
        self.width = res[0]*4
        self.height = res[1]*4
        self.widgets = {}
        self.frame = np.zeros((120, 160), dtype='uint8')
        
        #radio serial port instances
        self.UHF = radio.Radio("UHF", 115200)
        self.SBand = radio.Radio("SBand", 115200)
        
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
        tk.Label(self.widgets['sband_status'], text="N/A", font=("Arial", 15), fg="red").grid(row=2,column=1)
        
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
        
        #embedded matplotlib
        fig = Figure(figsize=(10, 4), dpi=50)
        t = np.arange(0, 6, 1)
        ax = fig.add_subplot()
        ax.plot([1,4,5,6,2,3], label='x')
        ax.plot([1,2,3,4,5,6], label='y')
        ax.plot([3,3,3,2,4,3], label='z')
        ax.legend()
        ax.set_xlabel("time [s]")
        ax.set_ylabel("Angular Rate")
        
        fig2 = Figure(figsize=(10, 4), dpi=50)
        t = np.arange(0, 6, 1)
        ax2 = fig2.add_subplot()
        ax2.plot([0,0,0,0,0,0], label='x')
        ax2.plot([0,0,1,1,0,0], label='y')
        ax2.plot([9.8,9.8,9.8,9.8,9.8,9.8], label='z')
        ax2.legend()
        ax2.set_xlabel("time [s]")
        ax2.set_ylabel("Acceleration")
        
        figure_canvas = FigureCanvasTkAgg(fig, master=self.window)
        self.widgets['plot1'] = figure_canvas.get_tk_widget()
        self.widgets['plot1'].grid(row=1, column=3, columnspan=2,sticky='NSEW')
        
        figure_canvas = FigureCanvasTkAgg(fig2, master=self.window)
        self.widgets['plot1'] = figure_canvas.get_tk_widget()
        self.widgets['plot1'].grid(row=2, column=3, columnspan=2,sticky='NSEW')
        
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
        #frame = self.cap.read()[1]
        #frame = cv.resize(frame, (res[0], res[1]))
        #frame = imaging.BGRtoGRB422(frame, res[0], res[1])
        
        frame = imaging.GRB422toBGR(self.frame, res[0], res[1])
        frame = cv.resize(frame, (self.width, self.height)) 
        frame = cv.cvtColor(frame, cv.COLOR_BGR2RGB)
        
        frame = Image.fromarray(frame) # to PIL format
        self.image = ImageTk.PhotoImage(frame) # to ImageTk format
        
        self.widgets['canvas'].create_image(0, 0, anchor=tk.NW, image=self.image)
        self.window.after(500, self.update_image)
        
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
            elif not self.SBand.port.is_open:
                self.widgets['sband_frequency'].destroy()
                self.widgets['sband_frequency'] = tk.Label(self.widgets['sband_status'], text="N/A", font=("Arial", 15), fg="red")
                self.widgets['sband_frequency'].grid(row=3,column=1)
        except serial.serialutil.SerialException:
            pass
        except UnicodeDecodeError:
            pass
        except IndexError:
            pass
        
        self.window.after(5000, self.get_radio_info)
        
    def radio_poll_rx(self):
        try:
            if self.SBand.port.is_open:
                status = self.SBand.poll_rx()
                if status[1] != 0:
                    '''
                    packet = self.SBand.get_packet(status[4])
                    if (packet[0] & 0x80 == 0x80) and (len(packet) == 18): #if image
                        index = int.from_bytes(packet[0:2], 'big')
                        index &= ~0x8000
                        
                        for i in range(0,16):
                            self.frame[int(index / 10)][((index % 10) * 16) + i] = packet[2 + i]
                    '''
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
                    
        except serial.serialutil.SerialException:
            pass
        
        self.window.after(10, self.radio_poll_rx)
    
def main():
    print("One day I will be a ground station, big and tall.")
    print("But for now I'm just a placeholder, short and small...")
    
    root = tk.Tk()
    root.title("Groundstation")
    MainWindow(root, cv.VideoCapture(0))
    root.mainloop()
    
if __name__ == "__main__":
    main()