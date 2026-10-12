"""Editable Potrace cubic contours and deterministic independent-channel rasterizer.

SVG is the canonical intermediate: the rasterizer reads the saved paths, not
source pixels. data-weight layers integrate grayscale coverage independently;
packed-channel SVG previews show alpha by default. Zero-area specks are retained.
"""
import ctypes as C
import ctypes.util
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET
import cairo
import numpy as np
from PIL import Image

class Point(C.Structure):
    _fields_=[('x',C.c_double),('y',C.c_double)]
class Curve(C.Structure):
    _fields_=[('n',C.c_int),('tag',C.POINTER(C.c_int)),('c',C.POINTER(Point*3))]
class TracePath(C.Structure):pass
TracePath._fields_=[('area',C.c_int),('sign',C.c_int),('curve',Curve),('next',C.POINTER(TracePath)),('childlist',C.POINTER(TracePath)),('sibling',C.POINTER(TracePath)),('priv',C.c_void_p)]
class State(C.Structure):
    _fields_=[('status',C.c_int),('plist',C.POINTER(TracePath)),('priv',C.c_void_p)]
class Progress(C.Structure):
    _fields_=[('callback',C.c_void_p),('data',C.c_void_p),('min',C.c_double),('max',C.c_double),('epsilon',C.c_double)]
class Parameters(C.Structure):
    _fields_=[('turdsize',C.c_int),('turnpolicy',C.c_int),('alphamax',C.c_double),('opticurve',C.c_int),('opttolerance',C.c_double),('progress',Progress)]
class Bitmap(C.Structure):
    _fields_=[('w',C.c_int),('h',C.c_int),('dy',C.c_int),('map',C.POINTER(C.c_ulong))]
LIB=C.CDLL(ctypes.util.find_library('potrace'))
LIB.potrace_param_default.restype=C.POINTER(Parameters)
LIB.potrace_trace.argtypes=[C.POINTER(Parameters),C.POINTER(Bitmap)];LIB.potrace_trace.restype=C.POINTER(State)
LIB.potrace_state_free.argtypes=[C.POINTER(State)]
PARAM=LIB.potrace_param_default();PARAM.contents.turdsize=0;PARAM.contents.alphamax=.8;PARAM.contents.opttolerance=.15


def trace_mask(mask, subsample=2):
    """Trace all components and holes, with corners and cubic Bezier segments."""
    h,w=mask.shape;dy=(w+63)//64
    packed=np.packbits(np.pad(mask,((0,0),(0,dy*64-w))),axis=1,bitorder='big')
    words=np.ascontiguousarray(packed).view('>u8').astype(np.uint64)
    state=LIB.potrace_trace(PARAM,C.byref(Bitmap(w,h,dy,words.ctypes.data_as(C.POINTER(C.c_ulong)))))
    if not state or state.contents.status:raise RuntimeError('Incomplete Potrace trace')
    commands=[];p=state.contents.plist
    def xy(point):return f'{point.x/subsample:.5f},{point.y/subsample:.5f}'
    try:
        while p:
            curve=p.contents.curve
            commands.append('M'+xy(curve.c[curve.n-1][2]))
            for i in range(curve.n):
                points=curve.c[i]
                if curve.tag[i]==1:commands.append('C'+' '.join(xy(points[j]) for j in range(3)))
                else:commands.append('L'+xy(points[1])+' L'+xy(points[2]))
            commands.append('Z');p=p.contents.next
    finally:LIB.potrace_state_free(state)
    return ' '.join(commands)


def rectangles(plane):
    """Lossless vector regions for discrete shader labels, merged vertically."""
    h,w=plane.shape;active={};result=[]
    for y in range(h+1):
        runs={}
        if y<h:
            row=plane[y];edges=np.r_[0,np.where(row[1:]!=row[:-1])[0]+1,w]
            for a,b in zip(edges[:-1],edges[1:]):
                value=int(row[a])
                if value:runs[(int(a),int(b),value)]=active.get((int(a),int(b),value),y)
        for (a,b,value),top in active.items():
            if (a,b,value) not in runs:result.append((a,top,b-a,y-top,value))
        active=runs
    return result


def vectorize(image,path,scale,discrete=(),font=False,subsample=2):
    pixels=np.array(image.convert('RGBA'));h,w=pixels.shape[:2]
    if font and np.any(pixels[:,:,3]):
        from scipy.ndimage import distance_transform_edt
        transparent=pixels[:,:,3]==0
        nearest=distance_transform_edt(transparent,return_distances=False,return_indices=True)
        pixels[transparent,:3]=pixels[nearest[0][transparent],nearest[1][transparent],:3]
    root=ET.Element('svg',{'xmlns':'http://www.w3.org/2000/svg','width':str(w*scale),'height':str(h*scale),
        'viewBox':f'0 0 {w} {h}','data-schema':'H2_CHANNEL_VECTORS_V1','data-scale':str(scale),
        'data-source-rgba-sha256':hashlib.sha256(pixels.tobytes()).hexdigest(),'data-font':str(int(font))})
    ET.SubElement(root,'title').text='Source-traced HUD cubic outlines; independent packed shader channels'
    ET.SubElement(root,'desc').text='Paths are editable. data-weight is integrated coverage for each independent channel; rasterize with tools/hud_vector_trace.py. Blue label rectangles retain authored meter thresholds. Alpha channel shown by default.'
    cache={}
    for k,name in enumerate(('red','green','blue','alpha')):
        plane=pixels[:,:,k]
        group=ET.SubElement(root,'g',{'id':name,'fill-rule':'evenodd','style':'display:inline' if k==3 else 'display:none'})
        key=(plane.tobytes(),k in discrete,font and k==3)
        if key in cache:
            attributes,items=cache[key]
            group.attrib.update(attributes)
            for item in items:group.append(ET.fromstring(ET.tostring(item)))
            continue
        if k in discrete:
            group.set('data-mode','labels')
            for x,y,rw,rh,value in rectangles(plane):
                ET.SubElement(group,'rect',{'x':str(x),'y':str(y),'width':str(rw),'height':str(rh),
                     'fill':f'rgb({value},{value},{value})','data-value':str(value)})
        elif plane.min()==plane.max():
            group.set('data-mode','constant');group.set('data-value',str(int(plane[0,0])))
            ET.SubElement(group,'rect',{'width':str(w),'height':str(h),'fill':f'rgb({plane[0,0]},{plane[0,0]},{plane[0,0]})'})
        else:
            group.set('data-mode','coverage')
            # Twice-source threshold sampling preserves antialiased source
            # contours before cubic fitting, without discarding tiny features.
            expanded=np.asarray(Image.fromarray(plane).resize((w*subsample,h*subsample),Image.Resampling.BILINEAR))
            maximum=int(plane.max())
            # Font alpha is the actual ink outline. Source AA becomes new
            # analytic edge coverage rather than a magnified blurred band.
            bands=[(maximum/2,maximum)] if font and k==3 else [(float(low+high)/2,int(high-low)) for low,high in zip(np.r_[0,np.unique(expanded[expanded>0])][:-1].astype(int),np.r_[0,np.unique(expanded[expanded>0])][1:].astype(int))]
            for level,weight in bands:
                d=trace_mask(expanded>=level,subsample)
                if d:ET.SubElement(group,'path',{'d':d,'data-threshold':str(level),'data-weight':str(weight),
                    'fill':f'rgb({min(255,round(level+weight/2))},{min(255,round(level+weight/2))},{min(255,round(level+weight/2))})'})
        cache[key]=({k:v for k,v in group.attrib.items() if k.startswith('data-')},list(group))
    path.parent.mkdir(parents=True,exist_ok=True)
    ET.ElementTree(root).write(path,encoding='utf-8',xml_declaration=True)
    result=rasterize(path)
    return result,metrics(pixels,result)

TOKEN=re.compile(r'[MLCZ]|-?(?:\d*\.\d+|\d+)(?:[eE][+-]?\d+)?')
def draw_path(context,d):
    tokens=TOKEN.findall(d);i=0
    while i<len(tokens):
        command=tokens[i];i+=1
        if command=='Z':context.close_path();continue
        n={'M':2,'L':2,'C':6}[command];values=list(map(float,tokens[i:i+n]));i+=n
        {'M':context.move_to,'L':context.line_to,'C':context.curve_to}[command](*values)


def rasterize(path,scale=None):
    root=ET.parse(path).getroot();_,_,w,h=map(float,root.attrib['viewBox'].split());w,h=int(w),int(h)
    scale=int(root.attrib['data-scale']) if scale is None else scale
    output=[]
    for group in root.findall('{*}g'):
        mode=group.attrib['data-mode']
        if mode=='constant':output.append(np.full((h*scale,w*scale),int(group.attrib['data-value']),np.uint8));continue
        if mode=='labels':
            plane=np.zeros((h*scale,w*scale),np.uint8)
            for item in group:
                a=item.attrib;x,y,rw,rh=[int(a[k])*scale for k in ('x','y','width','height')]
                plane[y:y+rh,x:x+rw]=int(a['data-value'])
            output.append(plane);continue
        surface=cairo.ImageSurface(cairo.FORMAT_A8,w*scale,h*scale);context=cairo.Context(surface)
        context.scale(scale,scale);context.set_fill_rule(cairo.FILL_RULE_EVEN_ODD)
        result=np.zeros((h*scale,w*scale),np.float32)
        for item in group:
            context.set_operator(cairo.OPERATOR_CLEAR);context.paint()
            context.set_operator(cairo.OPERATOR_SOURCE);context.set_source_rgba(1,1,1,1)
            draw_path(context,item.attrib['d']);context.fill();surface.flush()
            coverage=np.frombuffer(surface.get_data(),np.uint8).reshape(h*scale,surface.get_stride())[:,:w*scale]
            result+=coverage*(float(item.attrib['data-weight'])/255)
        output.append(np.rint(np.clip(result,0,255)).astype(np.uint8))
    rgba=np.stack(output,axis=2)
    if root.attrib['data-font']=='1':
        # Font RGB is paint, not selector data. Restore source white in fully
        # transparent surrounds to avoid dark edges under linear sampling.
        rgba[rgba[:,:,3]==0,:3]=255
    return Image.fromarray(rgba)


def metrics(source,result):
    a=np.asarray(result);h,w=source.shape[:2];s=a.shape[1]//w
    down=a.reshape(h,s,w,s,4).mean((1,3));error=np.abs(down-source.astype(float))
    return dict(mean_channel_error=round(float(error.mean()),4),p99_channel_error=round(float(np.percentile(error,99)),4),max_channel_error=round(float(error.max()),4))


def decode_glyph(data,p):
    width,height=__import__('struct').unpack_from('<HH',data,p+4)
    size=__import__('struct').unpack_from('<H',data,p+2)[0];offset=__import__('struct').unpack_from('<I',data,p+12)[0]
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

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('svg',type=Path);p.add_argument('png',type=Path);p.add_argument('--scale',type=int);a=p.parse_args()
    rasterize(a.svg,a.scale).save(a.png)
