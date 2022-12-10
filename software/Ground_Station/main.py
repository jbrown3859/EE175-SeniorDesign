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

'''
Arducam OV2640 claims to have the capability to output raw RGB565
This means 5 bits of R, 6 bits of G, 5 bits of B in two bytes as opposed to three

OpenCV uses BGR instead of RGB for some reason

Must convert from RGB565 -> BGR
'''

res = [160, 120]

def BGRtoGRB422(image, width, height):
    converted_image = np.zeros((height, width), dtype='uint8')
    image = image.astype(int)

    for i in range(0, width):
        for j in range(0, height):
            converted_image[j][i] = 0
            converted_image[j][i] |= (image[j][i][0] >> 6) & 0x03 #B
            converted_image[j][i] |= (image[j][i][1]) & 0xF0 #G
            converted_image[j][i] |= (image[j][i][2] >> 4) & 0x0C #R
    
    return converted_image
    
def GRB422toBGR(image, width, height):
    converted_image = np.zeros((height, width, 3), dtype='uint8')
    image = image.astype(int)
    
    for i in range(0, width):
        for j in range(0, height):
            converted_image[j][i][0] = ((image[j][i] & 0x03) << 6) #B
            converted_image[j][i][1] = ((image[j][i] & 0xF0)) #G
            converted_image[j][i][2] = ((image[j][i] & 0x0C) << 4) #R
            
    return converted_image
    
class Radio():
    def __init__(self, type, baudrate):
        self.type = type #radio type string
        self.port = serial.Serial()
        self.port.baudrate = baudrate
        self.port.timeout = 0.1 #seconds
        self.portname = None
        self.frequency = None
        self.last_packet = None
        
    def attempt_connection(self, portname):
        self.port.port = portname
        print("Attempting to connect {} on port {}".format(self.type, portname))
        try:
            self.port.open()
            self.port.reset_input_buffer() #flush buffer
            self.port.write(b'a') #send info request
            reply = self.port.read(13) #get returned info
            #print(reply)
            
            if len(reply) >= 3 and reply[0] == 170 and reply[1] == 170: #valid preamble
                if chr(reply[2]) == 'U':
                    if self.type == "UHF":
                        print("UHF Modem Connection Accepted")
                    else:
                        self.port.close()
                elif chr(reply[2]) == 'S':
                    if self.type == "SBand":
                        print("S-Band Modem Connection Accepted")
                    else:
                        self.port.close()
                else:   
                    self.port.close()
            else:
                self.port.close()
        except serial.serialutil.SerialException:
            if self.port.is_open:
                self.port.close()

class MainWindow():
    def __init__(self, window, cap):
        self.window = window
        self.cap = cap
        self.width = res[0]*4
        self.height = res[1]*4
        self.interval = 20
        self.widgets = {}
        
        #radio serial port instances
        self.UHF = Radio("UHF", 115200)
        self.SBand = Radio("SBand", 115200)
        '''
        self.UHF = serial.Serial()
        self.UHF.baudrate = 115200
        self.SBand = serial.Serial()
        self.SBand.baudrate = 115200
        self.SBand.port = 'COM6'
        '''
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
        #tk.Label(self.widgets['sband_status'], text="Not Connected", font=("Arial", 15), fg="red").grid(row=1,column=1)
        self.widgets['sband_connected'] = tk.Label(self.widgets['sband_status'], text="Not Connected", font=("Arial", 15), fg="red")
        self.widgets['sband_connected'].grid(row=1,column=1)
        
        tk.Label(self.widgets['sband_status'], text="Last Packet:", font=("Arial", 15)).grid(row=2,column=0)
        tk.Label(self.widgets['sband_status'], text="N/A", font=("Arial", 15), fg="red").grid(row=2,column=1)
        
        tk.Label(self.widgets['sband_status'], text="Frequency:", font=("Arial", 15)).grid(row=3,column=0)
        tk.Label(self.widgets['sband_status'], text="N/A", font=("Arial", 15), fg="red").grid(row=3,column=1)
        
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
        
    def update_image(self):
        frame = self.cap.read()[1]
        frame = cv.resize(frame, (res[0], res[1]))
        frame = BGRtoGRB422(frame, res[0], res[1])
        frame = GRB422toBGR(frame, res[0], res[1])
        frame = cv.resize(frame, (self.width, self.height)) 
        frame = cv.cvtColor(frame, cv.COLOR_BGR2RGB)
        
        frame = Image.fromarray(frame) # to PIL format
        self.image = ImageTk.PhotoImage(frame) # to ImageTk format
        
        self.widgets['canvas'].create_image(0, 0, anchor=tk.NW, image=self.image)
        self.window.after(self.interval, self.update_image)
        
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
        
        #print(self.SBand.read(1000))
        self.window.update()
        self.window.after(5000, self.connect_radios)
        

def main():
    print("One day I will be a ground station, big and tall.")
    print("But for now I'm just a placeholder, short and small...")
    
    root = tk.Tk()
    root.title("Groundstation")
    MainWindow(root, cv.VideoCapture(0))
    root.mainloop()
    
if __name__ == "__main__":
    main()