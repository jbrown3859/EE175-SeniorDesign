import cv2 as cv
import numpy as np

def mirror_bits(var, len):
    temp = 0
    for i in range(0, len):
        old = var >> ((len-1) - i) & 0x01
        temp |= old << i
        
    return temp

filename = str(input("filename:"))
#mode = int(input("Mode (1 = B/W, 2 = color, 3 = red, 4 = green, 5 = blue):"))
endianness = int(input("Endianness (1 = big, 2 = little):"))

width = int(input("Width:"))
height = int(input("Height:"))

frame = np.zeros((height, width, 3), dtype=np.uint8)
r_frame = np.zeros((height, width, 3), dtype=np.uint8)
g_frame = np.zeros((height, width, 3), dtype=np.uint8)
b_frame = np.zeros((height, width, 3), dtype=np.uint8)

with open(filename, mode='rb') as f:
    data = list(f.read())

print("File size=", len(data))
print("lines=", len(data)/(width*2))

for i in range(0, (width * height)):
    if (endianness == 1): #big
        high = (data[2*i])
        low = (data[2*i + 1])
    elif (endianness == 2): #little
        low = (data[2*i])
        high = (data[2*i + 1])
    
    #print("========")
    temp = ((high << 8) & 0xFF00) | (low & 0xFF)
    #print(format(temp, '016b'))
    
    red = (temp >> 11) & 0x1f#(temp & 0xF800) >> 11
    green = (temp >> 5) & 0x3f#(temp & 0x03F0) >> 5
    blue = (temp) & 0x1f#(temp & 0x001F)
    
    red = red << 3
    green = green << 2
    blue = blue << 3

    '''
    print("r:{} g:{} b:{}".format(format(red, '08b'),format(green, '08b'),format(blue, '08b')))
    '''
    
    frame[int(i / width)][i % width][2] = red #R (good)
    frame[int(i / width)][i % width][1] = green #G
    frame[int(i / width)][i % width][0] = blue #B
    
    r_frame[int(i / width)][i % width][2] = red
    g_frame[int(i / width)][i % width][1] = green
    b_frame[int(i / width)][i % width][0] = blue
    
    '''
    if (mode == 1):
        avg = int((red + green + blue)/3)
        
        for j in range(0, 3):
            frame[int(i / width)][i % width][j] = avg
        
    elif (mode == 2):
        frame[int(i / width)][i % width][2] = red #R (good)
        frame[int(i / width)][i % width][1] = green #G
        frame[int(i / width)][i % width][0] = blue #B
        
        r_frame[int(i / width)][i % width][2] = red
        g_rame[int(i / width)][i % width][1] = green
        b_frame[int(i / width)][i % width][0] = blue
    elif (mode == 3):
        frame[int(i / width)][i % width][2] = red
        
    elif (mode == 4):
        frame[int(i / width)][i % width][1] = green
        
    elif (mode == 5):
        frame[int(i / width)][i % width][0] = blue
    '''
        

scale = 4        

#frame = cv.cvtColor(frame, cv.COLOR_YUV2BGR_NV12)

frame = cv.resize(frame, (160 * scale, 120 * scale)) 
r_frame = cv.resize(r_frame, (160 * scale, 120 * scale)) 
g_frame = cv.resize(g_frame, (160 * scale, 120 * scale)) 
b_frame = cv.resize(b_frame, (160 * scale, 120 * scale)) 

h1 = np.concatenate((frame, r_frame), axis=1)
h2 = np.concatenate((g_frame, b_frame), axis=1)
v = np.concatenate((h1, h2), axis=0)

'''
cv.imshow('full color', frame)
cv.imshow('red', r_frame)
cv.imshow('green', g_frame)
cv.imshow('blue', b_frame)
'''
cv.imshow('frame',v)

cv.waitKey(0)
  
# closing all open windows
cv.destroyAllWindows()

#data = np.reshape(data,(160, 120)))
#cv.imshow('image', np.array(data))
