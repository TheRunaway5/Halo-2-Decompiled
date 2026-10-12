#!/usr/bin/env python3
"""Independent analytic weather built-in program fixtures, not captured GPU output.

Expected words express the literal five/nine authored instructions directly;
no native vertex interpreter is invoked. Viewport words use the separately
untouched-XBE-validated391F0/12FA0 caller's constants. These synthetic camera
inputs test the implementation and resolution seam, not NV2A precision.
"""
from pathlib import Path
import random
import struct

ROOT=Path(__file__).resolve().parents[1]
def f(value):return struct.unpack('<f',struct.pack('<f',value))[0]
def word(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def add(a,b):return f(a+b)
def mul(a,b):return 0. if a==0 or b==0 else f(a*b)
def main():
    rng=random.Random(0x37a60);rows=[]
    for width,height in [(640,480),(1280,720),(3840,2160),(1920,1080)]:
        for players in [1,2,4]:
            rects=[(0,0,width,height)] if players==1 else ([(0,0,width,height//2),(0,height//2,width,height)] if players==2 else
                [(x*width//2,y*height//2,(x+1)*width//2,(y+1)*height//2) for y in range(2) for x in range(2)])
            for rect in rects:
                for kind in [0,1]:
                    for sample in range(8):
                        attributes=[[0.,0.,0.,1.] for _ in range(16)];constants=[[0.]*4 for _ in range(24)]
                        outputs=[[0.,0.,0.,1.] for _ in range(16)]
                        left,top,right,bottom=rect
                        if kind==0:
                            # Finite source-style common-scale projection.
                            # DPH ignores v0.w; v0.w carries particle diameter.
                            constants[:4]=[[16384.,0.,0.,0.],[0.,16384.,0.,0.],
                                [0.,0.,f(16384./1024.),f(-16384.*.1/1024.)],[0.,0.,16384.,0.]]
                            constants[10]=[f((right-left)*.5),f((bottom-top)*-.5),16777215.,0.]
                            constants[11]=[f((right-left)*.5+left+.5),f((bottom-top)*.5+top+.5),0.,0.]
                            attributes[0]=[f(rng.uniform(-10,10)),f(rng.uniform(-10,10)),f(rng.uniform(.11,100)),mul(f(rng.uniform(.005,.007)),16777215.)]
                            packed=rng.getrandbits(32);attributes[1]=[f(((packed>>shift)&255)/255.) for shift in [16,8,0,24]]
                            projection=[add(add(add(mul(attributes[0][0],c[0]),mul(attributes[0][1],c[1])),mul(attributes[0][2],c[2])),c[3]) for c in constants[:4]]
                            reciprocal=f(1./projection[3])
                            outputs[0]=[add(mul(mul(projection[j],constants[10][j]),reciprocal),constants[11][j]) for j in range(3)]+[projection[3]]
                            outputs[3]=attributes[1][:];outputs[6][0]=mul(reciprocal,attributes[0][3])
                        else:
                            attributes[0]=[f(left-.5 if sample%2==0 else right-.5),f(top-.5 if sample%4<2 else bottom-.5),f(rng.uniform(0,16777215)),f(rng.uniform(0,16777215))]
                            for i in range(1,5):attributes[i]=[f(rng.uniform(-4,4)),f(rng.uniform(-4,4)),0.,1.]
                            for i in range(21,24):constants[i]=[f(rng.uniform(-20,20)),f(rng.uniform(-20,20)),0.,0.]
                            outputs[0]=attributes[0][:];outputs[9]=attributes[1][:]
                            for stage in range(3):outputs[10+stage][:2]=[add(attributes[2+stage][j],constants[21+stage][j]) for j in range(2)]
                        values=[kind,*[word(v) for array in attributes for v in array],*[word(v) for array in constants for v in array],*[word(v) for array in outputs for v in array]]
                        rows.append(' '.join(f'{v:08x}' for v in values))
    path=ROOT/'tests/fixtures/retail_weather_vertex_analytic.tsv'
    path.write_text('# Synthetic inputs, independently evaluated literal weather_particle/weather_plate programs.\n# Not an original Xbox GPU capture or hardware precision fixture.\n'+'\n'.join(rows)+'\n')
    print(f'{len(rows)} analytic weather vertex cases')
if __name__=='__main__':main()
