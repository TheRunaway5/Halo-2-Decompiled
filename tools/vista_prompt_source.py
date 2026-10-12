"""Native Vista font references and lossless bitmap export for prompt redraws."""
import struct
import numpy as np
from PIL import Image


def write_dds(path, image):
    """One A8R8G8B8 base level, matching the existing native UI DDS encoder."""
    rgba = np.array(image.convert('RGBA'))
    payload = rgba[:, :, [2, 1, 0, 3]].tobytes()
    header = [124, 0x100f, image.height, image.width, image.width*4, 0, 1] + [0]*11
    header += [32, 0x41, 0, 32, 0xff0000, 0xff00, 0xff, 0xff000000, 0x1000, 0, 0, 0, 0]
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix('.tmp')
    temporary.write_bytes(b'DDS '+struct.pack('<31I', *header)+payload)
    temporary.replace(path)


def decode_glyph(data,p):
    width,height=struct.unpack_from('<HH',data,p+4)
    size=struct.unpack_from('<H',data,p+2)[0];offset=struct.unpack_from('<I',data,p+12)[0]
    source=data[offset:offset+size];pixels=[];color=0xfff;i=0
    def append(alpha,count):pixels.extend([((((alpha<<1)|(alpha&1))&15)<<12)|color]*count)
    while i<len(source):
        code=source[i];i+=1;kind=code>>6
        if kind==3:append((code>>3)&7,1);append(code&7,1)
        elif kind==2:
            append((code>>3)&7,1);low=code&7
            if 1<=low<=3:append(7,5-low)
            if low>=4:append(0,9-low)
        elif kind==0 and not code&63:
            value=int.from_bytes(source[i:i+2],'big');i+=2;color=value&0xfff;pixels.append(value)
        else:append(7 if kind==1 else 0,code&63)
    if len(pixels)>width*height:raise ValueError('Font stream exceeds glyph')
    pixels+=[0]*(width*height-len(pixels))
    a=np.array(pixels,np.uint16).reshape(height,width)
    return Image.fromarray(np.stack([((a>>shift)&15).astype(np.uint8)*17 for shift in (8,4,0,12)],axis=2))
