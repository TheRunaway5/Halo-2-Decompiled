#!/usr/bin/env python3
"""Literal nine-instruction analytic fixture over the immutable238-point packet.

Expected arithmetic is evaluated independently of the native vertex interpreter.
These are source-program arithmetic controls, not original GPU precision or
framebuffer observations. The captured projection/points remain unchanged; only
the separately proved12FA0 target rectangle constants change across extents.
"""
from pathlib import Path
import hashlib,json,struct

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'analysis/multiplayer-shading/weather-particle-packet'
OUT=ROOT/'analysis/multiplayer-shading/weather-point-prototype'
EXTENTS=[(640,480),(641,481),(1920,1080),(3840,2160)]
def f(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def add(a,b):return f(a+b)
def mul(a,b):return 0. if a==0 or b==0 else f(a*b)
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    OUT.mkdir(parents=True,exist_ok=True)
    packet=json.loads((SOURCE/'packet.json').read_text())
    assert packet['inputs']['strict_positive_age_emissions']==238
    for name,row in packet['artifacts'].items():assert digest(SOURCE/name)==row['sha256']
    rows=list(struct.iter_unpack('<4fI',(SOURCE/'emitted-source-v0-logical-color.bin').read_bytes()))
    camera=list(struct.unpack('<48f',(SOURCE/'vertex_constants_c0_c11.bin').read_bytes()))
    outputs=[];stats=[]
    for w,h in EXTENTS:
        c=[camera[i:i+4] for i in range(0,48,4)]
        # Literal source12FA0 flag0 rectangle arithmetic, including half pixel.
        c[10]=[f(w*.5),f(h*-.5),16777215.,0.]
        c[11]=[add(f(w*.5),.5),add(f(h*.5),.5),0.,0.]
        extent_outputs=[]
        for x,y,z,size,color in rows:
            p=[add(add(add(mul(x,q[0]),mul(y,q[1])),mul(z,q[2])),q[3]) for q in c[:4]]
            r=f(1./p[3]);r=max(f(2**-64),min(f(2**64),abs(r)))*(1 if r>=0 else -1)
            o=[[0.,0.,0.,1.] for _ in range(16)]
            o[0]=[add(mul(mul(p[j],c[10][j]),r),c[11][j]) for j in range(3)]+[p[3]]
            o[3]=[f(((color>>shift)&255)/255.) for shift in (16,8,0,24)]
            o[6][0]=mul(r,size)
            extent_outputs.append(o)
            outputs.extend(v for q in o for v in q)
        stats.append({'width':w,'height':h,'points':len(rows),
            'point_diameter_min':min(q[6][0] for q in extent_outputs),
            'point_diameter_max':max(q[6][0] for q in extent_outputs),
            'centers_inside_rectangle':sum(0<=q[0][0]<w and 0<=q[0][1]<h for q in extent_outputs)})
    expected=OUT/'vertex-analytic.bin';expected.write_bytes(struct.pack('<'+'f'*len(outputs),*outputs))
    (OUT/'vertex-analytic.json').write_text(json.dumps({'authority':__doc__,
        'packet_sha256':digest(SOURCE/'packet.json'),'source_program_sha256':digest(SOURCE/'source-vs22-class0.vp'),
        'expected_sha256':digest(expected),'cases':len(rows)*len(EXTENTS),'words':len(outputs),
        'extents':stats,'captured_GPU_outputs':False,'guest_IO':False},indent=2)+'\n')
    print(len(rows)*len(EXTENTS),'analytic cases,',len(outputs),'words')
if __name__=='__main__':main()
