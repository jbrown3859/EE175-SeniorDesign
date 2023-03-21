import time
import json
import math
import numpy as np
import matplotlib.pyplot as plt

packets = []

def sum_bits(data, length):
    sum = 0
    for i in range(length):
        sum += (data >> i) & 0x01;
        
    return sum
    
def get_parity(data, length):
    return sum_bits(data, length) % 2
    
def get_bit(data, index):
    return (data >> index) & 0x01

def divide_lists(a, b):
    list = []
    
    for i in range(len(a)):
        if b[i] != 0:
            list.append(a[i]/b[i])
        else:
            list.append(a[i])
        
    return list
    
def multiply_lists(a,b):
    list = []
    
    for i in range(len(a)):
        list.append(a[i] * b[i])
        
    return list

with open('range_test_results.txt') as f: #import data
    lines = f.readlines()
    
for line in lines: #format data
    unix_time = float(line[0:13])
    data = json.loads(line[15:])
    
    packets.append((unix_time, data))

failed_checksums = 0
failures = 0
total_failures = 0
pkt_count = 0
total_packets = 0
num_pkts = len(packets)

fails = []
pkt_counts = []
cumulative_pkts = []

times = []

interval = 6

timestamp = 0
for i in range(0, math.floor((math.floor(packets[-1][0]) - math.floor(packets[0][0]))/interval) + 1): #generate bins for every interval seconds of the test
    times.append(timestamp)
    fails.append(0)
    pkt_counts.append(0)
    cumulative_pkts.append(0)
    timestamp += interval
    
print(times)

for packet in packets:
    total_packets += 1
    time_index = math.floor((math.floor(packet[0]) - math.floor(packets[0][0]))/interval)
    
    print(time_index, end = ' ')
    
    if (packet[1]['Timestamp Checksum'] != sum_bits(packet[1]['Timestamp'], 32)):
        #print("Timestamp failed checksum at t={}".format(packet[0]))
        failed_checksums += 1
        
    for i, datapoint in enumerate(packet[1]['Acceleration']):
        if (get_parity(datapoint, 8) != get_bit(packet[1]['Parity 0'], 7-i)):
            fails[time_index] += 1
                
    for i, datapoint in enumerate(packet[1]['Angular Rate']):
        if (get_parity(datapoint, 8) != get_bit(packet[1]['Parity 0'], 4-i)):
            fails[time_index]  += 1
            
    for i, datapoint in enumerate(packet[1]['Magnetic Field']):
        if (get_parity(datapoint, 8) != get_bit(packet[1]['Parity 1'], 7-i)):
            fails[time_index]  += 1
            
    if get_parity(int(packet[1]['Temperature'] * 2), 8) != get_bit(packet[1]['Parity 1'], 4):
        fails[time_index]  += 1
        
    pkt_counts[time_index] += 1
    
for i in range(0, len(times)):
    sum = 0
    for j in range(0, i):
        sum += pkt_counts[j]
        
    cumulative_pkts[i] = sum

print()
print(len(times))

#times = [((x-times[0]) * interval) for x in times]
pkts_per_minute = [(x * (60/interval)) for x in pkt_counts]
errors_per_packet = divide_lists(fails, pkt_counts)

print(times)
print(fails)
print(pkt_counts)
print(errors_per_packet)

print('=============')
        
print("Total failed timestamp checksums = {}".format(failed_checksums))
print("Failure Rate = {}/{} = {}%".format(failed_checksums, num_pkts, (failed_checksums/num_pkts) * 100))
print("Total data parity failures = {}".format(total_failures))
print("Failure Rate = {}/{} = {}%".format(total_failures, num_pkts*10, (total_failures/(num_pkts*10)) * 100))

times.pop(-1)
errors_per_packet.pop(-1)
pkts_per_minute.pop(-1)
fails.pop(-1)
cumulative_pkts.pop(-1)

figure, axis = plt.subplots(2,2)

axis[0,0].set(xlabel='Time (s)', ylabel='Errors per packet')
axis[0,0].set_title("Parity Failure Rate vs. Time")
axis[0,0].plot(times,errors_per_packet)

axis[1,0].set(xlabel='Time (s)', ylabel='Packets per minute')
axis[1,0].set_title("Packet Rate vs. Time")
axis[1,0].plot(times,pkts_per_minute)

axis[0,1].set_title("Failures vs. Time")
axis[0,1].set(xlabel='Time (s)', ylabel='Failures')
axis[0,1].plot(times,fails)

axis[1,1].set_title("Total Packets vs. Time")
axis[1,1].set(xlabel='Time (s)', ylabel='Packets')
axis[1,1].plot(times,cumulative_pkts)


plt.tight_layout()
plt.show()