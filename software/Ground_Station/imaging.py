import numpy as np

'''
Arducam OV2640 claims to have the capability to output raw RGB565
This means 5 bits of R, 6 bits of G, 5 bits of B in two bytes as opposed to three

OpenCV uses BGR instead of RGB for some reason

Must convert from RGB565 -> BGR
'''

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