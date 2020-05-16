from PIL import Image, ImageDraw, ImageFont, ImageOps
from cairosvg import svg2png
from io import BytesIO

def frame(im, thickness=5):
    # Get input image width and height, and calculate output width and height
    iw, ih = im.size
    ow, oh = iw+2*thickness, ih+2*thickness

    # Draw outer black rounded rect into memory as PNG
    #outer = f'<svg width="{ow}" height="{oh}" style="background-color:none"><rect width="{ow}" height="{oh}" fill="black"/></svg>'
    #png   = svg2png(bytestring=outer)
    #outer = Image.open(BytesIO(png))
    outer = Image.new('RGB', (ow, oh), color = (0, 0, 0))
    # Draw inner white rounded rect, offset by thickness into memory as PNG
    inner = f'<svg width="{ow}" height="{oh}"><rect x="{thickness}" y="{thickness}" rx="10" ry="10" width="{iw}" height="{ih}" fill="white"/></svg>'
    png   = svg2png(bytestring=inner)
    inner = Image.open(BytesIO(png)).convert('L')

    # Expand original canvas with black to match output size
    expanded = ImageOps.expand(im, border=thickness, fill=(0,0,0)).convert('RGB')

    # Paste expanded image onto outer black border using inner white rectangle as mask
    outer.paste(expanded, None, inner)
    return outer

fnt = ImageFont.truetype('/home/johannes/Downloads/Gotham-Font/GothamMedium.ttf', 36)


# for i in ["C"]:
#      img = Image.new('RGB', (48, 48), color = (180, 0, 0))
#      d = ImageDraw.Draw(img)
#      d.text((2,3), str(i),font=fnt, fill=(0,0,0,255))
#      filename = "res/calc_buttons/calc_symbol_new.png"
#      # img.save(filename)
#      # # Open image, frame it and save
#      # im = Image.open(filename)
#      result = frame(img, thickness=2)
#      result.save(filename)

# #import os

#for i in range(0,10):
#for i in [":","-","(",")","*","+"]:
#    print("Hi")
#    os.system("python3 tools/rle_encode.py --2bit res/calc_buttons/calc_symbol_"+str(i)+".png >> wasp/icons.py ")
#os.system('python')

# For Undo- Button:

img = Image.new('RGB', (28, 32), color = (180, 0, 0))
d = ImageDraw.Draw(img)
d.text((2,2), "<",font=fnt, fill=(0,0,0,255))
filename = "res/calc_buttons/calc_symbol_undo.png"
result = frame(img, thickness=2)
result.save(filename)
