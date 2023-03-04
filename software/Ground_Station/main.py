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

def to_signed(n):
    if (n >= 128):
        n = n - 256
    return n

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
        self.accel_ax.set_ylim(-128, 128)
        self.accel_x, = self.accel_ax.plot(self.t, self.a_x, label='x acceleration')
        self.accel_y, = self.accel_ax.plot(self.t, self.a_y, label='y acceleration')
        self.accel_z, = self.accel_ax.plot(self.t, self.a_z, label='z acceleration')
    
        self.accel_ax.legend()
        self.accel_canvas = FigureCanvasTkAgg(self.ag_plot, self.window)
        self.accel_canvas.get_tk_widget().grid(column=3,row=1,columnspan=2, rowspan=2)
        self.accel_animation = animation.FuncAnimation(self.ag_plot, self.animate_plot, interval=250, blit=False)
        
        #self.gyro_ax.set_xlim(0, 100)
        self.gyro_ax.set_ylim(-128, 128)
        self.gyro_x, = self.gyro_ax.plot(self.t, self.g_x, label='x rate')
        self.gyro_y, = self.gyro_ax.plot(self.t, self.g_y, label='y rate')
        self.gyro_z, = self.gyro_ax.plot(self.t, self.g_z, label='z rate')
        self.gyro_ax.legend()
        
        #self.mag_ax.set_xlim(0, 100)
        self.mag_ax.set_ylim(-128, 128)
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
        #S-Band
        self.widgets['status_title'] = tk.Label(self.window, text="Radio Status", font=("Arial", 25))
        self.widgets['status_title'].grid(row=3,column=0,columnspan=2)
        
        self.widgets['sband_status'] = tk.Frame(self.window,highlightbackground="black",highlightthickness=2)
        self.widgets['sband_status'].grid(row=4,column=0,sticky='NSEW',rowspan=2)
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
        
        tk.Label(self.widgets['sband_status'], text="Data Rate:", font=("Arial", 15)).grid(row=4,column=0)
        self.widgets['sband_rate'] = tk.Label(self.widgets['sband_status'], text="N/A", font=("Arial", 15), fg="red")
        self.widgets['sband_rate'].grid(row=4,column=1)
        
        #UHF
        self.widgets['uhf_status'] = tk.Frame(self.window,highlightbackground="black",highlightthickness=2)
        self.widgets['uhf_status'].grid(row=4,column=1,sticky='NSEW',rowspan=2)
        tk.Label(self.widgets['uhf_status'], text="UHF Radio", font=("Arial", 15)).grid(row=0,column=0,columnspan=2)
        tk.Label(self.widgets['uhf_status'], text="Serial:", font=("Arial", 15)).grid(row=1,column=0)
        self.widgets['uhf_connected'] = tk.Label(self.widgets['uhf_status'], text="Not Connected", font=("Arial", 15), fg="red")
        self.widgets['uhf_connected'].grid(row=1,column=1)
        
        tk.Label(self.widgets['uhf_status'], text="Last Packet:", font=("Arial", 15)).grid(row=2,column=0)
        self.widgets['uhf_last'] = tk.Label(self.widgets['uhf_status'], text="N/A", font=("Arial", 15), fg="red")
        self.widgets['uhf_last'].grid(row=2,column=1)
        
        tk.Label(self.widgets['uhf_status'], text="Frequency:", font=("Arial", 15)).grid(row=3,column=0)
        self.widgets['uhf_frequency'] = tk.Label(self.widgets['uhf_status'], text="N/A", font=("Arial", 15), fg="red")
        self.widgets['uhf_frequency'].grid(row=3,column=1)
        
        tk.Label(self.widgets['uhf_status'], text="Bandwidth:", font=("Arial", 15)).grid(row=4,column=0)
        self.widgets['uhf_bandwidth'] = tk.Label(self.widgets['uhf_status'], text="N/A", font=("Arial", 15), fg="red")
        self.widgets['uhf_bandwidth'].grid(row=4,column=1)
        
        tk.Label(self.widgets['uhf_status'], text="Spreading Factor:", font=("Arial", 15)).grid(row=5,column=0)
        self.widgets['uhf_spread'] = tk.Label(self.widgets['uhf_status'], text="N/A", font=("Arial", 15), fg="red")
        self.widgets['uhf_spread'].grid(row=5,column=1)
        
        tk.Label(self.widgets['uhf_status'], text="Coding Rate:", font=("Arial", 15)).grid(row=6,column=0)
        self.widgets['uhf_coding'] = tk.Label(self.widgets['uhf_status'], text="N/A", font=("Arial", 15), fg="red")
        self.widgets['uhf_coding'].grid(row=6,column=1)
        
        '''
        Radio Programming
        '''
        prog_pad = 8
        prog_width = 20
        #Sband
        self.widgets['sband_program'] = tk.Frame(self.window,highlightbackground="black",highlightthickness=2)
        self.widgets['sband_program'].grid(row=6,column=0, sticky='N')
        
        self.widgets['sband_prog_label'] = tk.Label(self.widgets['sband_program'], text="S-Band Programming", font=("Arial", 15))
        self.widgets['sband_prog_label'].grid(row=0,column=0, columnspan=2)
        
        self.SBand_param = tk.StringVar(self.window)
        self.widgets['sband_prog_param'] = tk.OptionMenu(self.widgets['sband_program'],self.SBand_param,"Frequency","Data Rate")
        self.widgets['sband_prog_param'].grid(row=1,column=0, padx=prog_pad)
        self.widgets['sband_prog_param'].config(width=prog_width)
        
        self.Sband_prog_val = tk.StringVar(self.window)
        self.widgets['sband_prog_box'] = tk.Entry(self.widgets['sband_program'], textvariable=self.Sband_prog_val, width=prog_width)
        self.widgets['sband_prog_box'].grid(row=1,column=1, padx=prog_pad)
        
        self.widgets['sband_button'] = tk.Button(self.widgets['sband_program'], 
                                                    text = "Program Radio", command = self.program_sband, 
                                                    width = 20, height = 2, bg='#c5e6e1')
        self.widgets['sband_button'].grid(row=3,column=0,columnspan=2)
        
        #UHF
        self.widgets['uhf_program'] = tk.Frame(self.window,highlightbackground="black",highlightthickness=2)
        self.widgets['uhf_program'].grid(row=6,column=1, sticky='N')
        
        self.widgets['uhf_prog_label'] = tk.Label(self.widgets['uhf_program'], text="UHF Programming", font=("Arial", 15))
        self.widgets['uhf_prog_label'].grid(row=0,column=0, columnspan=2)
        
        self.SBand_param = tk.StringVar(self.window)
        self.widgets['uhf_prog_param'] = tk.OptionMenu(self.widgets['uhf_program'],self.SBand_param,"Frequency","Data Rate")
        self.widgets['uhf_prog_param'].grid(row=1,column=0, padx=prog_pad)
        self.widgets['uhf_prog_param'].config(width=prog_width)
        
        self.Sband_prog_val = tk.StringVar(self.window)
        self.widgets['uhf_prog_box'] = tk.Entry(self.widgets['uhf_program'], textvariable=self.Sband_prog_val, width=prog_width)
        self.widgets['uhf_prog_box'].grid(row=1,column=1, padx=prog_pad)
        
        self.widgets['uhf_button'] = tk.Button(self.widgets['uhf_program'], 
                                                    text = "Program Radio", command = self.program_uhf, 
                                                    width = 20, height = 2, bg='#c5e6e1')
        self.widgets['uhf_button'].grid(row=3,column=0,columnspan=2)
        
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
        self.widgets['console'] = tk.Text(window,height=10,width=105,wrap=tk.WORD)
        self.widgets['console'].grid(row=5,column=3,columnspan=4,rowspan=2,pady=10)

        '''
        Serial Threading
        '''
        self.telem_queue = queue.Queue()
        self.img_queue = queue.Queue()
        self.status_queue = queue.Queue(maxsize = 1)
        
        self.SBand_command_queue = queue.Queue()
        self.UHF_command_queue = queue.Queue()
        
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
            #print(index)
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
                if (len(self.telemetry_packets) >= 100):
                    self.telemetry_packets.pop(0)
                
                telem = self.telem_queue.get()
                self.telemetry_packets.append(telem)

            for packet in self.telemetry_packets:
                '''
                print('===')
                print(len(self.t))
                print(len(self.a_x))
                '''
                #print(packet)
                if (len(self.a_x) >= 100):
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
                
                
                if (len(self.t) > 1 and packet['Timestamp'] < self.t[-1] and packet['Timestamp'] != 0): #clear if causality is violated
                    self.a_x.clear()
                    self.a_y.clear()
                    self.a_z.clear()
                    
                    self.g_x.clear()
                    self.g_y.clear()
                    self.g_z.clear()
                    
                    self.m_x.clear()
                    self.m_y.clear()
                    self.m_z.clear()
                    
                    self.t.clear()
                
                
                if (packet['Timestamp'] not in self.t and packet['Acceleration']): #discard packets with no data in them
                    self.t.append(packet['Timestamp'])
                    
                    #adjust x scale to match packet timestamps
                    self.mag_ax.set_xlim(self.telemetry_packets[0]['Timestamp'], packet['Timestamp'])
                    self.accel_ax.set_xlim(self.telemetry_packets[0]['Timestamp'], packet['Timestamp'])
                    self.gyro_ax.set_xlim(self.telemetry_packets[0]['Timestamp'], packet['Timestamp'])
                    
                    self.a_x.append(to_signed(packet['Acceleration'][0]))
                    self.a_y.append(to_signed(packet['Acceleration'][1]))
                    self.a_z.append(to_signed(packet['Acceleration'][2]))
                    self.accel_x.set_data(self.t, self.a_x)
                    self.accel_y.set_data(self.t, self.a_y)
                    self.accel_z.set_data(self.t, self.a_z)
                    
                    self.g_x.append(to_signed(packet['Angular Rate'][0]))
                    self.g_y.append(to_signed(packet['Angular Rate'][1]))
                    self.g_z.append(to_signed(packet['Angular Rate'][2]))
                    self.gyro_x.set_data(self.t, self.g_x)
                    self.gyro_y.set_data(self.t, self.g_y)
                    self.gyro_z.set_data(self.t, self.g_z)
                    
                    self.m_x.append(to_signed(packet['Magnetic Field'][0]))
                    self.m_y.append(to_signed(packet['Magnetic Field'][1]))
                    self.m_z.append(to_signed(packet['Magnetic Field'][2]))
                    self.mag_x.set_data(self.t, self.m_x)
                    self.mag_y.set_data(self.t, self.m_y)
                    self.mag_z.set_data(self.t, self.m_z)
        except (ValueError):
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
                        
            if ch == "S-Band":
                self.SBand_command_queue.put(command_bytes)
                self.write_console("Command hex: {}".format(command_bytes.hex()))
            elif ch == "UHF":
                self.UHF_command_queue.put(command_bytes)
                self.write_console("Command hex: {}".format(command_bytes.hex()))
    
    def update_radio_status(self):
        if not self.status_queue.empty():
            status = self.status_queue.get()
            
            #SBand
            self.widgets['sband_connected'].destroy()
            self.widgets['sband_last'].destroy()
            self.widgets['sband_frequency'].destroy()
            self.widgets['sband_rate'].destroy()
            
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
            
            self.widgets['sband_rate'] = tk.Label(self.widgets['sband_status'], text=status['SBand_rate'], font=("Arial", 15), fg=sband_color)
            self.widgets['sband_rate'].grid(row=4,column=1)
            
            #UHF
            self.widgets['uhf_connected'].destroy()
            self.widgets['uhf_last'].destroy()
            self.widgets['uhf_frequency'].destroy()
            self.widgets['uhf_bandwidth'].destroy()
            self.widgets['uhf_spread'].destroy()
            self.widgets['uhf_coding'].destroy()
            
            if status['UHF_open']:
                self.widgets['uhf_connected'] = tk.Label(self.widgets['uhf_status'], text="Connected", font=("Arial", 15), fg="green")
                self.widgets['uhf_connected'].grid(row=1,column=1)
                sband_color = "green"
                
                self.widgets['uhf_last'] = tk.Label(self.widgets['uhf_status'], text=str(int(time.time() - status['UHF_last_packet'])) + "s", font=("Arial", 15), fg="green")
                self.widgets['uhf_last'].grid(row=2,column=1)
            else:
                self.widgets['uhf_connected'] = tk.Label(self.widgets['uhf_status'], text="Not Connected", font=("Arial", 15), fg="red")
                self.widgets['uhf_connected'].grid(row=1,column=1)
                sband_color = "red"
                
                self.widgets['uhf_last'] = tk.Label(self.widgets['uhf_status'], text="N/A", font=("Arial", 15), fg="red")
                self.widgets['uhf_last'].grid(row=2,column=1)
                
            self.widgets['uhf_frequency'] = tk.Label(self.widgets['uhf_status'], text=status['UHF_frequency'], font=("Arial", 15), fg=sband_color)
            self.widgets['uhf_frequency'].grid(row=3,column=1)
            
            self.widgets['uhf_bandwidth'] = tk.Label(self.widgets['uhf_status'], text=status['UHF_bandwidth'], font=("Arial", 15), fg=sband_color)
            self.widgets['uhf_bandwidth'].grid(row=4,column=1)
            
            self.widgets['uhf_spread'] = tk.Label(self.widgets['uhf_status'], text=status['UHF_spread'], font=("Arial", 15), fg=sband_color)
            self.widgets['uhf_spread'].grid(row=5,column=1)
            
            self.widgets['uhf_coding'] = tk.Label(self.widgets['uhf_status'], text=status['UHF_coding'], font=("Arial", 15), fg=sband_color)
            self.widgets['uhf_coding'].grid(row=6,column=1)
            
        self.window.after(1000, self.update_radio_status)
        
    def program_sband(self):
        print("Programming S-Band beep boop")
        
    def program_uhf(self):
        print("Programming UHF beep boop")
            
    def serial_thread(self):
        radio_status = {}
        radio_status['SBand_last_packet'] = 0
        radio_status['UHF_last_packet'] = 0
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
            
            #only attempt to connect radios if there is no incoming data
            if (int(time.time() - radio_status['SBand_last_packet']) > 2) and (int(time.time() - radio_status['UHF_last_packet']) > 2):
                for port in portnames:
                    if not self.SBand.port.is_open: #if port is closed
                        self.SBand.attempt_connection(port)
                        if self.SBand.port.is_open:
                            self.write_console("S-Band Modem Connection Accepted")
                    
                    if not self.UHF.port.is_open:
                        self.UHF.attempt_connection(port)
                        if self.UHF.port.is_open:
                            self.write_console("UHF Modem Connection Accepted")
                
            #get radio info          
            if self.status_queue.empty():
                radio_status['SBand_open'] = self.SBand.port.is_open
                radio_status['UHF_open'] = self.UHF.port.is_open
                
                if self.SBand.port.is_open:
                    try:
                        sband_info = self.SBand.get_radio_info()
                        radio_status['SBand_frequency'] = sband_info['frequency'].lstrip('0') + " Hz"
                        radio_status['SBand_rate'] = sband_info['data_rate'].lstrip('0') + " Baud"
                    except(serial.serialutil.SerialException, IndexError, KeyError):
                        pass
                else:
                    radio_status['SBand_frequency'] = "N/A"
                    radio_status['SBand_rate'] = "N/A"
                    
                if self.UHF.port.is_open:
                    try:
                        uhf_info = self.UHF.get_radio_info()
                        radio_status['UHF_frequency'] = uhf_info['frequency'].lstrip('0') + " Hz"
                        radio_status['UHF_bandwidth'] = str(float(uhf_info['bandwidth'].lstrip('0'))/100) + " kHz"
                        radio_status['UHF_spread'] = uhf_info['spreading_factor'].lstrip('0')
                        radio_status['UHF_coding'] = uhf_info['coding_rate'][0] + '/' + uhf_info['coding_rate'][1]
                    except(serial.serialutil.SerialException, IndexError, KeyError):
                        pass
                else:
                    radio_status['UHF_frequency'] = "N/A"
                    radio_status['UHF_bandwidth'] = "N/A"
                    radio_status['UHF_spread'] = "N/A"
                    radio_status['UHF_coding'] = "N/A"
                
                self.status_queue.put(radio_status)
            
            #get packets
            try:
                if self.SBand.port.is_open:
                    status = self.SBand.get_rx_buffer_state()
                    
                    if (status[0] & 0x03) != 0:
                        self.write_console("Warning: S-Band RX buffer overflowed!")
                    
                    if status[1] != 0: #if packet
                        radio_status['SBand_last_packet'] = time.time()
                        packets = self.SBand.burst_read(status[4], status[1])
                        
                        if (status[4] == 18): #if image packet
                            for i in range(0, math.floor(len(packets)/18)):
                                packet = packets[(18*i):(18*(i+1))]
                                self.img_queue.put(packet)
                        elif (status[4] == 32 and packets[0] == 0x54): #if telemetry packet
                            for i in range(0, status[1]):
                                packet = list(packets[(32*i):(32*(i+1))])
                                
                                packet_data = {}
                                packet_data['Timestamp'] = int.from_bytes(bytes(packet[1:5]), byteorder='big', signed=False)
                                packet_data['Acceleration'] = packet[5:8] #x,y,z
                                packet_data['Angular Rate'] = packet[8:11]
                                packet_data['Magnetic Field'] = packet[11:14]
                                self.telem_queue.put(packet_data)
                        else:
                            self.write_console("S-Band received uncategorized packets: {}".format(packets))
                        
                if self.UHF.port.is_open:
                    status = self.UHF.get_rx_buffer_state()
                    
                    if (status[0] & 0x03) != 0:
                        self.write_console("Warning: UHF RX buffer overflowed!")
                    
                    if status[1] != 0: #if packet
                        radio_status['UHF_last_packet'] = time.time()
                        packets = self.UHF.burst_read(status[4], status[1])
                        
                        if (status[4] == 18): #if image packet
                            for i in range(0, math.floor(len(packets)/18)):
                                packet = packets[(18*i):(18*(i+1))]
                                self.img_queue.put(packet)
                        elif (status[4] == 32 and packets[0] == 0x54): #if telemetry packet
                            for i in range(0, status[1]):
                                packet = list(packets[(32*i):(32*(i+1))])
                                
                                packet_data = {}
                                packet_data['Timestamp'] = int.from_bytes(bytes(packet[1:5]), byteorder='big', signed=False)
                                packet_data['Acceleration'] = packet[5:8] #x,y,z
                                packet_data['Angular Rate'] = packet[8:11]
                                packet_data['Magnetic Field'] = packet[11:14]
                                self.telem_queue.put(packet_data)
                        else:
                            self.write_console("UHF received uncategorized packets: {}".format(packets))
                        
                        
            except (serial.serialutil.SerialException, IndexError):
                pass
                
            #transmit packets
            #S-Band
            if not self.SBand_command_queue.empty():
                if self.SBand.port.is_open:                
                    while not self.SBand_command_queue.empty(): #fill buffer
                        command = self.SBand_command_queue.get()
                        for i in range(0, 3): #repeat command (maybe remove later)
                            self.SBand.write_tx_buffer(command)
                    
                    self.write_console("S-Band Entering Transmit Mode")
                    status = 1
                    while status != 0x80:
                        #time.sleep(0.1)
                        status = int.from_bytes(self.SBand.radio_tx_mode(), "big")
                        print(status)
                    
                    tx_packets = 255
                    radio_state = 0xF0
                    while tx_packets != 0x0 or (radio_state & 0xF0) == 0xC0: #wait until empty
                        try:
                            state = self.SBand.get_tx_buffer_state()
                            tx_packets = state[1]
                            radio_state = state[0]
                            if (radio_state != 0xC0):
                                self.SBand.radio_tx_mode()
                        except IndexError:
                            tx_packets = 255
                        print("TX packets: {}".format(tx_packets))
                    
                    while status != 0x40:
                        #time.sleep(0.1)
                        status = int.from_bytes(self.SBand.radio_rx_mode(), "big")
                        print(status)
                    
                    self.write_console("S-Band Transmission Complete")
                else:
                    self.write_console("Error: S-Band radio is not connected, flushing command queue")
                    with self.SBand_command_queue.mutex:
                        self.SBand_command_queue.queue.clear()
                
            #UHF
            if not self.UHF_command_queue.empty():
                while not self.UHF_command_queue.empty(): #fill buffer
                    command = self.UHF_command_queue.get()
                    #for i in range(0, 3): #repeat command (maybe remove later)
                    self.UHF.write_tx_buffer(command)
                
                self.write_console("UHF Entering Transmit Mode")
                time.sleep(0.1)
                status = int.from_bytes(self.UHF.radio_tx_mode(), "big")
                print(status)
                
                tx_packets = 255
                radio_state = 0xF0
                while tx_packets != 0x0 or (radio_state & 0xF0) == 0xC0: #wait until empty
                    try:
                        state = self.UHF.get_tx_buffer_state()
                        tx_packets = state[1]
                        radio_state = state[0]
                        if (radio_state != 0xC0):
                            self.UHF.radio_tx_mode()
                    except IndexError:
                        tx_packets = 255
                    print("TX packets: {}".format(tx_packets))
                
                time.sleep(0.1)
                status = int.from_bytes(self.UHF.radio_rx_mode(), "big")
                print(status)
                
                self.write_console("UHF Transmission Complete")
                
                
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