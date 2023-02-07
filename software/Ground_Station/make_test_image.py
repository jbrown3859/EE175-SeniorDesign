filename = str(input("filename:"))
width = int(input("Width:"))
height = int(input("Height:"))

'''
r = 0
g = 255
b = 0

color = ((r & 0x1f) << 11) | ((g & 0x3f) << 5) | (b & 0x1f)
'''

def generate_color(r, g, b):
    return (((r >> 3) & 0x1f) << 11) | (((g >> 2) & 0x3f) << 5) | ((b >> 3) & 0x1f)

with open(filename, mode='wb') as f:
    for i in range(0, (width * height)):
        line = int(i / width)
        if (line < int(height / 3)):
            color = generate_color(i % 256, 0, 0)
        elif (line < int(2*height / 3)):
            color = generate_color(0, i % 256, 0)
        else:
            color = generate_color(0, 0, i % 256)
        
        f.write(color.to_bytes(2,'big'))