import cv2 as cv
import PIL
from PIL import ImageTk
from PIL import Image
import numpy as np
from mss import mss
import time
import json

import tkinter as tk
from matplotlib.backends.backend_tkagg import (
    FigureCanvasTkAgg, NavigationToolbar2Tk)
from matplotlib.backend_bases import key_press_handler
from matplotlib.figure import Figure
import matplotlib.animation as animation
from matplotlib import style

import serial
import serial.tools.list_ports

import threading
import queue

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
        self.t = []
        self.a_x = []
        self.a_y = []
        self.a_z = []
        self.g_x = []
        self.g_y = []
        self.g_z = []
        self.m_x = []
        self.m_y = []
        self.m_z = []
        
        #plots
        style.use('ggplot')
        self.ag_plot = Figure(figsize=(18, 9.5), dpi=50)
        self.accel_ax = self.ag_plot.add_subplot(2,2,1)
        self.gyro_ax = self.ag_plot.add_subplot(2,2,2)
        self.mag_ax = self.ag_plot.add_subplot(2,2,3)
        self.ag_plot.tight_layout()
        
        self.accel_ax.set_xlim(0, 100)
        self.accel_ax.set_ylim(0, 255)
        self.accel_x, = self.accel_ax.plot(self.t, self.a_x, label='x acceleration')
        self.accel_y, = self.accel_ax.plot(self.t, self.a_y, label='y acceleration')
        self.accel_z, = self.accel_ax.plot(self.t, self.a_z, label='z acceleration')
        #self.accel_ax.set_xlabel("Time")
        self.accel_ax.legend()
        self.accel_canvas = FigureCanvasTkAgg(self.ag_plot, self.window)
        self.accel_canvas.get_tk_widget().grid(column=3,row=1,columnspan=2, rowspan=2)
        self.accel_animation = animation.FuncAnimation(self.ag_plot, self.animate_plot, interval=250, blit=False)
        
        self.gyro_ax.set_xlim(0, 100)
        self.gyro_ax.set_ylim(0, 255)
        self.gyro_x, = self.gyro_ax.plot(self.t, self.g_x, label='x rate')
        self.gyro_y, = self.gyro_ax.plot(self.t, self.g_y, label='y rate')
        self.gyro_z, = self.gyro_ax.plot(self.t, self.g_z, label='z rate')
        self.gyro_ax.legend()
        
        self.mag_ax.set_xlim(0, 100)
        self.mag_ax.set_ylim(0, 100)
        self.mag_x, = self.mag_ax.plot(self.t, self.m_x, label='x mag')
        self.mag_y, = self.mag_ax.plot(self.t, self.m_y, label='y mag')
        self.mag_z, = self.mag_ax.plot(self.t, self.m_z, label='z mag')
        self.mag_ax.legend()
       
        
        #image canvas
        self.widgets['img_title'] = tk.Label(self.window, text="Image Downlink", font=("Arial", 25))
        self.widgets['img_title'].grid(row=0, column=0, columnspan=2,sticky='NSEW')
        
        self.widgets['canvas'] = tk.Canvas(self.window, width=self.width, height=self.height)
        self.widgets['canvas'].grid(row=1, column=0, rowspan=2, columnspan=2, sticky='NSEW')
        
        self.widgets['telem_title'] = tk.Label(self.window, text="Spacecraft Telemetry", font=("Arial", 25))
        self.widgets['telem_title'].grid(row=0,column=3, columnspan=2,sticky='NSEW')
        
        #radio status
        self.widgets['status_title'] = tk.Label(self.window, text="Radio Status", font=("Arial", 25))
        self.widgets['status_title'].grid(row=3,column=0,columnspan=2)
        
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
        
        #command center
        self.widgets['command_title'] = tk.Label(self.window, text="Command Uplink", font=("Arial", 25))
        self.widgets['command_title'].grid(row=3,column=3,columnspan=4)
        
        self.widgets['command_center'] = tk.Frame(self.window,highlightbackground="black",highlightthickness=2)
        self.widgets['command_center'].grid(row=4,column=3,columnspan=4)
        self.command = tk.StringVar(self.window)
        self.channel = tk.StringVar(self.window)
        
        self.widgets['channel_title'] = tk.Label(self.widgets['command_center'], text="Channel", font=("Arial", 12))
        self.widgets['channel_title'].grid(row=1,column=1)
        self.widgets['channel'] = tk.OptionMenu(self.widgets['command_center'],self.channel,"UHF","S-Band")
        self.widgets['channel'].grid(row=2,column=1)
        self.widgets['channel'].config(width=7)
        
        self.widgets['selcom_title'] = tk.Label(self.widgets['command_center'], text="Command", font=("Arial", 12))
        self.widgets['selcom_title'].grid(row=1,column=2)
        self.widgets['selected_commands'] = tk.OptionMenu(self.widgets['command_center'],self.command,"Capture Image","Configure Radio")
        self.widgets['selected_commands'].grid(row=2,column=2)
        self.widgets['selected_commands'].config(width=15)
        #self.widgets['command_menu'].grid(row=4,column=3)
        
        with open("commands.json") as f:
            data = json.load(f)
            print(data["Configure Radio"])
        
        self.telem_queue = queue.Queue()
        self.img_queue = queue.Queue()
        self.status_queue = queue.Queue(maxsize = 1)
        
        self.thread_running = 1
        self.thread = threading.Thread(target=self.serial_thread)
        self.thread.start()
        
        self.update_image()
        self.update_radio_status()
        
    def update_image(self):
        while not self.img_queue.empty():
            packet = self.img_queue.get()
            index = int.from_bytes(packet[0:2], 'big')
            index &= ~0x8000
            if (index == 0): #clear new frame
                self.frame.fill(0)
            
            try:
                for i in range(0,16):
                    self.frame[int(index / 10)][((index % 10) * 16) + i] = packet[2 + i]
            except IndexError:
                print("Error decoding image packet")

    
        frame = imaging.GRB422toBGR(self.frame, res[0], res[1])
        frame = cv.resize(frame, (self.width, self.height)) 
        frame = cv.cvtColor(frame, cv.COLOR_BGR2RGB)
        
        frame = Image.fromarray(frame) # to PIL format
        self.image = ImageTk.PhotoImage(frame) # to ImageTk format
        
        self.widgets['canvas'].create_image(0, 0, anchor=tk.NW, image=self.image)
        self.window.after(500, self.update_image)
        
    def animate_plot(self, i):
        try:
            while not self.telem_queue.empty():
                if (len(self.telemetry_packets) == 100):
                    self.telemetry_packets.pop(0)
                
                self.telemetry_packets.append(self.telem_queue.get())

            for packet in self.telemetry_packets:
                if (len(self.a_x) == 100):
                    self.a_x.pop(0)
                    self.a_y.pop(0)
                    self.a_z.pop(0)
                    
                    self.g_x.pop(0)
                    self.g_y.pop(0)
                    self.g_z.pop(0)
                    
                    self.m_x.pop(0)
                    self.m_y.pop(0)
                    self.m_z.pop(0)
                else:
                    self.t.append(len(self.a_x))
                
                self.a_x.append(packet['Acceleration'][0])
                self.a_y.append(packet['Acceleration'][1])
                self.a_z.append(packet['Acceleration'][2])
                self.accel_x.set_data(self.t, self.a_x)
                self.accel_y.set_data(self.t, self.a_y)
                self.accel_z.set_data(self.t, self.a_z)
                
                self.g_x.append(packet['Angular Rate'][0])
                self.g_y.append(packet['Angular Rate'][1])
                self.g_z.append(packet['Angular Rate'][2])
                self.gyro_x.set_data(self.t, self.g_x)
                self.gyro_y.set_data(self.t, self.g_y)
                self.gyro_z.set_data(self.t, self.g_z)
                
                self.m_x.append(packet['Magnetic Field'][0])
                self.m_y.append(packet['Magnetic Field'][1])
                self.m_z.append(packet['Magnetic Field'][2])
                self.mag_x.set_data(self.t, self.m_x)
                self.mag_y.set_data(self.t, self.m_y)
                self.mag_z.set_data(self.t, self.m_z)
        except IndexError:
            pass
    
    def update_radio_status(self):
        if not self.status_queue.empty():
            status = self.status_queue.get()
            
            self.widgets['sband_connected'].destroy()
            self.widgets['sband_last'].destroy()
            self.widgets['sband_frequency'].destroy()
            
            if status['SBand_open']:
                self.widgets['sband_connected'] = tk.Label(self.widgets['sband_status'], text="Connected", font=("Arial", 15), fg="green")
                self.widgets['sband_connected'].grid(row=1,column=1)
                sband_color = "green"
                
                self.widgets['sband_last'] = tk.Label(self.widgets['sband_status'], text=str(int(time.time() - status['SBand_last_packet'])) + "s", font=("Arial", 15), fg="green")
                self.widgets['sband_last'].grid(row=2,column=1)
            else:
                self.widgets['sband_connected'] = tk.Label(self.widgets['sband_status'], text="Not Connected", font=("Arial", 15), fg="red")
                self.widgets['sband_connected'].grid(row=1,column=1)
                sband_color = "red"
                
                self.widgets['sband_last'] = tk.Label(self.widgets['sband_status'], text="N/A", font=("Arial", 15), fg="red")
                self.widgets['sband_last'].grid(row=2,column=1)
            
            self.widgets['sband_frequency'] = tk.Label(self.widgets['sband_status'], text=status['SBand_frequency'], font=("Arial", 15), fg=sband_color)
            self.widgets['sband_frequency'].grid(row=3,column=1)
            
        self.window.after(1000, self.update_radio_status)
            
    def serial_thread(self):
        radio_status = {}
        radio_status['SBand_last_packet'] = 0
        while self.thread_running == 1:
            #attempt to connect ports
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
            
            #get radio info          
            if self.status_queue.empty():
                radio_status['SBand_open'] = self.SBand.port.is_open
                radio_status['UHF_open'] = self.UHF.port.is_open
                
                if self.SBand.port.is_open:
                    try:
                        sband_info = self.SBand.get_info()
                        radio_status['SBand_frequency'] = sband_info['frequency'].lstrip('0') + " Hz"
                    except(serial.serialutil.SerialException, IndexError):
                        pass
                else:
                    radio_status['SBand_frequency'] = "N/A"
                
                self.status_queue.put(radio_status)
            
            try:
                #get packets
                if self.SBand.port.is_open:
                    status = self.SBand.poll_rx()
                    if status[1] != 0: #if packet
                        radio_status['SBand_last_packet'] = time.time()
                        packets = self.SBand.burst_read(status[4], status[1])
                        
                        if (status[4] == 18): #if image packet
                            for i in range(0, status[1]):
                                packet = packets[(18*i):(18*(i+1))]
                                self.img_queue.put(packet)
                        
                        elif (status[4] == 32): #if telemetry packet
                            for i in range(0, status[1]):
                                packet = list(packets[(32*i):(32*(i+1))])
                                
                                packet_data = {}
                                packet_data['Acceleration'] = packet[1:4] #x,y,z
                                packet_data['Angular Rate'] = packet[4:7]
                                packet_data['Magnetic Field'] = packet[7:10]
                                self.telem_queue.put(packet_data)
                        
            except (serial.serialutil.SerialException, IndexError):
                pass
        
    def close_window(self):
        print("Closing window")
        self.thread_running = 0
        self.thread.join()
        self.window.destroy()
    
def main():
    print("One day I will be a ground station, big and tall.")
    print("But for now I'm just a placeholder, short and small...")
    
    root = tk.Tk()
    root.title("Groundstation")
    window = MainWindow(root)
    root.protocol("WM_DELETE_WINDOW", window.close_window)
    root.mainloop()
    
if __name__ == "__main__":
    main()