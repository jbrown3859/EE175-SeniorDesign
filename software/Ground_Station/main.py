import cv2 as cv
import PIL
from PIL import ImageTk
from PIL import Image
import numpy as np
from mss import mss
import time
import json
import math

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
        
        '''
        Radio Serial Port Instances
        '''
        self.UHF = radio.Radio("UHF", 115200)
        self.SBand = radio.Radio("SBand", 115200)
        
        '''
        Telemetry Database
        '''
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

        '''
        Telemetry Plots
        '''
        style.use('ggplot')
        self.ag_plot = Figure(figsize=(18, 9.5), dpi=50)
        self.accel_ax = self.ag_plot.add_subplot(2,2,1)
        self.gyro_ax = self.ag_plot.add_subplot(2,2,2)
        self.mag_ax = self.ag_plot.add_subplot(2,2,3)
        self.ag_plot.tight_layout()
        
        #self.accel_ax.set_xlim(0, 100)
        self.accel_ax.set_ylim(0, 255)
        self.accel_x, = self.accel_ax.plot(self.t, self.a_x, label='x acceleration')
        self.accel_y, = self.accel_ax.plot(self.t, self.a_y, label='y acceleration')
        self.accel_z, = self.accel_ax.plot(self.t, self.a_z, label='z acceleration')
    
        self.accel_ax.legend()
        self.accel_canvas = FigureCanvasTkAgg(self.ag_plot, self.window)
        self.accel_canvas.get_tk_widget().grid(column=3,row=1,columnspan=2, rowspan=2)
        self.accel_animation = animation.FuncAnimation(self.ag_plot, self.animate_plot, interval=250, blit=False)
        
        #self.gyro_ax.set_xlim(0, 100)
        self.gyro_ax.set_ylim(0, 255)
        self.gyro_x, = self.gyro_ax.plot(self.t, self.g_x, label='x rate')
        self.gyro_y, = self.gyro_ax.plot(self.t, self.g_y, label='y rate')
        self.gyro_z, = self.gyro_ax.plot(self.t, self.g_z, label='z rate')
        self.gyro_ax.legend()
        
        #self.mag_ax.set_xlim(0, 100)
        self.mag_ax.set_ylim(0, 255)
        self.mag_x, = self.mag_ax.plot(self.t, self.m_x, label='x mag')
        self.mag_y, = self.mag_ax.plot(self.t, self.m_y, label='y mag')
        self.mag_z, = self.mag_ax.plot(self.t, self.m_z, label='z mag')
        self.mag_ax.legend()
       
        '''
        Image Canvas
        '''
        self.widgets['img_title'] = tk.Label(self.window, text="Image Downlink", font=("Arial", 25))
        self.widgets['img_title'].grid(row=0, column=0, columnspan=2,sticky='NSEW')
        
        self.widgets['canvas'] = tk.Canvas(self.window, width=self.width, height=self.height)
        self.widgets['canvas'].grid(row=1, column=0, rowspan=2, columnspan=2, sticky='NSEW')
        
        self.widgets['telem_title'] = tk.Label(self.window, text="Spacecraft Telemetry", font=("Arial", 25))
        self.widgets['telem_title'].grid(row=0,column=3, columnspan=2,sticky='NSEW')
        
        '''
        Radio Status
        '''
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
        
        '''
        Radio Programming
        '''
        self.widgets['sband_program'] = tk.Frame(self.window,highlightbackground="black",highlightthickness=2)
        self.widgets['sband_program'].grid(row=5,column=0, sticky='N')
        
        self.widgets['sband_freq_label'] = tk.Label(self.widgets['sband_program'], text="Frequency", font=("Arial", 12))
        self.widgets['sband_freq_label'].grid(row=0,column=0)
        
        self.widgets['sband_freq'] = tk.Entry(self.widgets['sband_program'], width = 16)
        self.widgets['sband_freq'].grid(row=1,column=0,pady=5,padx=5)
        
        self.widgets['sband_rate_label'] = tk.Label(self.widgets['sband_program'], text="Data Rate", font=("Arial", 12))
        self.widgets['sband_rate_label'].grid(row=0,column=1)
        
        self.widgets['sband_rate'] = tk.OptionMenu(self.widgets['sband_program'], None, None)
        self.widgets['sband_rate'].grid(row=1,column=1)
        self.widgets['sband_rate'].config(width=10,padx=5)
        
        '''
        Command Center
        '''
        self.num_args = 6
        
        self.command_boxes = []
        for i in range(0,self.num_args):
            box = "box_{}".format(i)
            self.command_boxes.append(tk.StringVar(self.window, value=''))
        
        self.commands = []
        with open("commands.json") as f:
            self.command_data = json.load(f)
            for command in self.command_data.keys():
                self.commands.append(command)
        
        self.widgets['command_title'] = tk.Label(self.window, text="Command Uplink", font=("Arial", 25))
        self.widgets['command_title'].grid(row=3,column=3,columnspan=4)
        
        self.widgets['command_center'] = tk.Frame(self.window,highlightbackground="black",highlightthickness=2)
        self.widgets['command_center'].grid(row=4,column=3,columnspan=4)
        
        self.channel = tk.StringVar(self.window)
        self.widgets['channel_title'] = tk.Label(self.widgets['command_center'], text="Channel", font=("Arial", 12))
        self.widgets['channel_title'].grid(row=1,column=1)
        self.widgets['channel'] = tk.OptionMenu(self.widgets['command_center'],self.channel,"UHF","S-Band")
        self.widgets['channel'].grid(row=2,column=1)
        self.widgets['channel'].config(width=7)
        
        self.command = tk.StringVar(self.window)
        self.widgets['selcom_title'] = tk.Label(self.widgets['command_center'], text="Command", font=("Arial", 12))
        self.widgets['selcom_title'].grid(row=1,column=2)
        self.widgets['selected_commands'] = tk.OptionMenu(self.widgets['command_center'],self.command,*self.commands)
        self.widgets['selected_commands'].grid(row=2,column=2)
        self.widgets['selected_commands'].config(width=15)
        
        
        for i in range(0, self.num_args):
            box = "box_{}".format(i)
            arg = "arg_{}".format(i)
            self.widgets[arg] = tk.Label(self.widgets['command_center'], text=None, font=("Arial",12))
            self.widgets[arg].grid(row=1,column=3+i)
            self.widgets[box] = tk.OptionMenu(self.widgets['command_center'], self.command_boxes[i], None)
            self.widgets[box].grid(row=2,column=3+i)
            self.widgets[box].config(width=10)
        
        self.widgets['command_button'] = tk.Button(self.widgets['command_center'], 
                                                    text = "Issue Command", command = self.issue_command, 
                                                    width = 20, height = 2, bg='#c5e6e1')
        self.widgets['command_button'].grid(row=3,column=1,columnspan=2+self.num_args)
        
        self.last_command = None
        
        '''
        Console
        '''
        self.widgets['console'] = tk.Text(window,height=5,width=105,wrap=tk.WORD)
        self.widgets['console'].grid(row=5,column=3,columnspan=4,pady=10)

        '''
        Serial Threading
        '''
        self.telem_queue = queue.Queue()
        self.img_queue = queue.Queue()
        self.status_queue = queue.Queue(maxsize = 1)
        self.command_queue = queue.Queue()
        
        self.thread_running = 1
        self.thread = threading.Thread(target=self.serial_thread)
        self.thread.start()
        
        '''
        Tkinter function calls
        '''
        self.update_image()
        self.update_radio_status()
        self.update_command()
        
    def write_console(self, string):
        self.widgets['console'].config(state=tk.NORMAL)
        self.widgets['console'].insert(tk.END, time.strftime("[%Y-%m-%d %H:%M:%S] ",time.gmtime()) + string + "\n")
        self.widgets['console'].see(tk.END)
        self.widgets['console'].config(state=tk.DISABLED)
        
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
            except IndexError as e:
                print("Error decoding image packet: " + str(packet))
    
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
                
                telem = self.telem_queue.get()
                self.telemetry_packets.append(telem)

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
                    
                    self.t.pop(0)
                
                self.t.append(packet['Timestamp'])
                
                #adjust x scale to match packet timestamps
                self.mag_ax.set_xlim(self.telemetry_packets[0]['Timestamp'], packet['Timestamp'])
                self.accel_ax.set_xlim(self.telemetry_packets[0]['Timestamp'], packet['Timestamp'])
                self.gyro_ax.set_xlim(self.telemetry_packets[0]['Timestamp'], packet['Timestamp'])
                
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
        except (IndexError, ValueError):
            pass
    
    def update_command(self):
        channel = self.channel.get()
        command = self.command.get()
        
        if command in self.commands and command != self.last_command:
            for i in range(0,self.num_args):
                box = "box_{}".format(i)
                arg = "arg_{}".format(i)
                self.widgets[box].destroy()
                self.widgets[arg].destroy()
                self.command_boxes[i].set('')
            
            for i, argument in enumerate(self.command_data[command]['arguments']):
                box = "box_{}".format(i)
                arg = "arg_{}".format(i)
                
                if argument['type'] == 'dropdown':
                    options = []
                    for option in argument['options'].keys():
                        options.append(option)
                    
                    self.widgets[box] = tk.OptionMenu(self.widgets['command_center'], self.command_boxes[i], *options)
                    self.widgets[box].grid(row=2,column=3+i)
                    self.widgets[box].config(width=10)
                elif argument['type'] == 'number':
                    self.widgets[box] = tk.Entry(self.widgets['command_center'], textvariable=self.command_boxes[i], width = 16)
                    self.widgets[box].grid(row=2,column=3+i)
                
                self.widgets[arg] = tk.Label(self.widgets['command_center'], text=argument['label'], font=("Arial",12))
                self.widgets[arg].grid(row=1,column=3+i)
                
            
            for i in range(len(self.command_data[command]['arguments']), self.num_args):
                box = "box_{}".format(i)
                self.widgets[box] = tk.OptionMenu(self.widgets['command_center'], self.command_boxes[i], None)
                self.widgets[box].grid(row=2,column=3+i)
                self.widgets[box].config(width=10)
        
        self.last_command = command
        self.window.after(100, self.update_command)
        
    def issue_command(self):
        command_bytes = bytearray()
                
        ch = self.channel.get()
        cmd = self.command.get()
        
        if cmd in self.command_data.keys():
            command_bytes.append(self.command_data[cmd]['header'])
            for i, argument in enumerate(self.command_data[cmd]['arguments']):
                if argument['type'] == 'dropdown':
                    for option in argument['options'].keys():
                        if option == self.command_boxes[i].get():
                            command_bytes.append(argument['options'][option])
                        if self.command_boxes[i].get() == '':
                            self.write_console("Error: cannot have empty command argument")
                            return
                
                elif argument['type'] == 'number':
                    try:
                        val = int(self.command_boxes[i].get())
                        min = argument['range'][0]
                        max = argument['range'][1]                  
                        if (min <= val <= max):
                            command_bytes += bytearray(val.to_bytes(int(argument['bytes']), 'big'))
                        else:
                            self.write_console("Error: command argument out of range")
                            return
                    except ValueError:
                        self.write_console("Error: invalid command data")
                        return
            
            self.write_console("Command hex: {}".format(command_bytes.hex()))
    
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
                self.write_console("S-Band Modem Disconnected")
            
            if self.UHF.port.is_open and self.UHF.port.port not in portnames: #close port if disconnected
                self.UHF.port.close()
                self.write_console("UHF Modem Disconnected")      
            
            for port in portnames:
                if not self.SBand.port.is_open: #if port is closed
                    self.SBand.attempt_connection(port)
                    if self.SBand.port.is_open:
                        self.write_console("S-Band Modem Connection Accepted")
                '''
                if not self.UHF.port.is_open:
                    self.UHF.attempt_connection(port)
                    if self.UHF.port.is_open:
                        self.write_console("UHF Modem Connection Accepted")
                '''
            #get radio info          
            if self.status_queue.empty():
                radio_status['SBand_open'] = self.SBand.port.is_open
                radio_status['UHF_open'] = self.UHF.port.is_open
                
                if self.SBand.port.is_open:
                    try:
                        sband_info = self.SBand.get_radio_info()
                        radio_status['SBand_frequency'] = sband_info['frequency'].lstrip('0') + " Hz"
                    except(serial.serialutil.SerialException, IndexError):
                        pass
                else:
                    radio_status['SBand_frequency'] = "N/A"
                
                self.status_queue.put(radio_status)
            
            try:
                #get packets
                if self.SBand.port.is_open:
                    status = self.SBand.get_rx_buffer_state()
                    if status[1] != 0: #if packet
                        radio_status['SBand_last_packet'] = time.time()
                        packets = self.SBand.burst_read(status[4], status[1])
                        print(status.hex())
                        if (status[4] == 18): #if image packet
                            for i in range(0, math.floor(len(packets)/18)):
                                packet = packets[(18*i):(18*(i+1))]
                                #print(packet)
                                self.img_queue.put(packet)
                        
                        elif (status[4] == 32): #if telemetry packet
                            for i in range(0, status[1]):
                                packet = list(packets[(32*i):(32*(i+1))])
                                
                                packet_data = {}
                                packet_data['Timestamp'] = int.from_bytes(bytes(packet[1:5]), byteorder='big', signed=False)
                                packet_data['Acceleration'] = packet[5:8] #x,y,z
                                packet_data['Angular Rate'] = packet[8:11]
                                packet_data['Magnetic Field'] = packet[11:14]
                                self.telem_queue.put(packet_data)
                        
            except (serial.serialutil.SerialException, IndexError):
                pass
        
    def close_window(self):
        #self.write_console("Closing window")
        self.thread_running = 0
        self.thread.join()
        self.window.destroy()
    
def main():
    root = tk.Tk()
    root.title("Groundstation")
    window = MainWindow(root)
    root.protocol("WM_DELETE_WINDOW", window.close_window)
    root.mainloop()
    
if __name__ == "__main__":
    main()