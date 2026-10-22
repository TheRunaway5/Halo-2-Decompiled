#!/usr/bin/env python3
"""Execute Vista PC14 geometry owners with explicit resource/GPU services.

The original raster setup, projection, mouse inverse, button bounds and grid
origin instructions are unchanged. This is bounded instruction evidence, not a
running Vista game, native pixel comparison, font raster or animated UI proof.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from oracle_vista_pcr_screen import machine, STOP, STACK, OBJ, SHA, u32, w32

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/pc14-layout-ownership-original'
sys.path.insert(0,str(OUT.parent/'local-session-inputs-original'))
from audit import PE,EXE,EXE_SHA
from export_vista_menu_ui import VistaCache

def sha(b):return hashlib.sha256(b).hexdigest()
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def raw32(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def wfloat(u,a,v):u.mem_write(a,struct.pack('<f',v))
def words(u,a,n):return list(struct.unpack('<'+'h'*n,u.mem_read(a,2*n)))
def floats(u,a,n):return list(struct.unpack('<'+'f'*n,u.mem_read(a,4*n)))

class Oracle:
    def __init__(self,image):
        self.u=machine(image);self.visits=0;self.services=[];self.service=None
        def hook(u,pc,size,_):
            self.visits+=1
            if pc==STOP:u.emu_stop()
            elif self.service:self.service(pc)
        self.u.hook_add(UC_HOOK_CODE,hook)
    def ret(self,value=None,pop=0):
        u=self.u;s=u.reg_read(UC_X86_REG_ESP)
        if value is not None:u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        u.reg_write(UC_X86_REG_EIP,u32(u,s));u.reg_write(UC_X86_REG_ESP,s+4+pop)
    def run(self,pc,args=(),this=OBJ,stop=STOP):
        u=self.u;s=STACK+0x18000
        u.mem_write(s,struct.pack('<'+'I'*(1+len(args)),STOP,*args))
        u.reg_write(UC_X86_REG_ESP,s);u.reg_write(UC_X86_REG_ECX,this)
        u.emu_start(pc,STOP+1,count=100000)
        assert u.reg_read(UC_X86_REG_EIP)==stop,hex(u.reg_read(UC_X86_REG_EIP))
    def raster(self,width,height):
        def stop(pc):
            if pc==0x664a18:self.u.emu_stop()
        self.service=stop;self.run(0x65e191,(width,height),stop=0x664a18);self.service=None
        u=self.u
        assert u32(u,0xe3e408)==width and u32(u,0xe3e40c)==height
        return dict(width=width,height=height,scale=floats(u,0xe3e424,1)[0],
            scale_bits=f'{u32(u,0xe3e424):08X}',viewport=words(u,0xe3e410,4),
            safe_bounds=words(u,0xe3e418,4),wide_offset=words(u,0xe3e420,2))
    def project(self,x,y,z=0,projection_distance=0):
        u=self.u;point=OBJ+0x1000;view=OBJ+0x1020
        u.mem_write(point,struct.pack('<fff',x,y,z))
        u.mem_write(view,bytes(u.mem_read(0xe3e410,8)));wfloat(u,0xd718f8,projection_distance)
        self.run(0x61d05f,(point,view));return dict(source=[x,y,z],distance=projection_distance,
            result=floats(u,point,3),result_bits=[f'{u32(u,point+4*i):08X}' for i in range(3)])
    def pointer(self,x,y):
        u=self.u;point=OBJ+0x1000
        def service(pc):
            if pc==0x42ec6d:
                s=u.reg_read(UC_X86_REG_ESP)
                u.mem_write(u32(u,s+4),struct.pack('<hh',x,y))
                self.services.append('explicit_platform_mouse_position_42EC6D');self.ret()
        self.service=service;self.run(0x61d620,(point,));self.service=None
        return dict(mouse=[x,y],authored=words(u,point,2))
    def button(self,button):
        u=self.u;source=OBJ+0x1000;bounds=OBJ+0x1100;pane=OBJ+0x1200
        u.mem_write(source,bytes.fromhex(button['raw_hex']));w32(u,pane+4,64)
        def service(pc):
            s=u.reg_read(UC_X86_REG_ESP)
            if pc==0x63f809:self.services.append('actual_source_record_lookup_63F809');self.ret(source)
            elif pc==0x611ba1:self.services.append('parent_screen_lookup_611BA1');self.ret(OBJ+0x2000)
            elif pc==0x6101a4:
                u.mem_write(u32(u,s+8),b'\0\0');self.services.append('localized_label_6101A4');self.ret(0,8)
            elif pc==0x60f870:self.services.append('pane_pointer_60F870');self.ret(pane)
            elif pc==0x62df53:self.services.append('font_configuration_boundary_62DF53');self.ret(0,32)
            elif pc==0x6875d3:self.ret()
        self.service=service;self.run(0x63f85b);self.run(0x63fff8,(bounds,));self.service=None
        return dict(source=button['bounds'],hit_top_offset=struct.unpack('<h',u.mem_read(source+54,2))[0],
            source_flags=u32(u,source+56),retained_draw=words(u,OBJ+0x20,4),
            hit=words(u,bounds,4),runtime_flags=u32(u,OBJ+0xf4),
            animation=words(u,OBJ+0x68,1)[0],layer=words(u,OBJ+0x6a,1)[0])
    def grid(self,grid,index,offset=(0,0)):
        u=self.u;source=OBJ+0x1000;origin=OBJ+0x1100;skin=OBJ+0x1200;result=[]
        u.mem_write(source,bytes.fromhex(grid['raw_hex']));u.mem_write(origin,struct.pack('<hh',*offset))
        w32(u,OBJ+0x74,source);w32(u,OBJ+0x70,index)
        def service(pc):
            s=u.reg_read(UC_X86_REG_ESP)
            if pc==0x639640:self.services.append('source_skin_lookup_639640');self.ret(skin)
            elif pc==0x642910:
                result.extend(words(u,u32(u,s+4),2));assert u32(u,s+8)==skin
                self.services.append('skin_construction_boundary_642910');self.ret(0,8)
            elif pc==0x6425bf:
                u.mem_write(u32(u,s+4),b'\0'*8);self.services.append('merged_bounds_service_6425BF');self.ret(0,4)
            elif pc==0x6875d3:self.ret()
        self.service=service;self.run(0x620493,(origin,));self.service=None
        return dict(index=index,additional_origin=list(offset),result=result)

def native_projection(width,height,x,y):
    # Current authored bitmap/text point construction, PC canvas contraction,
    # then renderer's centered640x480 foreground scale. Every step is binary32.
    scale=min(f32(width/640),f32(height/480))
    px=f32(320+x);py=f32(240-y)
    px=f32(320+f32(f32(px-320)*f32(480/1200)))
    py=f32(240+f32(f32(py-240)*f32(480/1200)))
    return [f32(f32(f32(width-f32(640*scale))*.5)+f32(px*scale)),
            f32(f32(f32(height-f32(480*scale))*.5)+f32(py*scale))]

def main():
    p=PE(EXE,EXE_SHA);image=p.data;assert sha(image)==SHA
    assets=json.loads((OUT/'evidence.json').read_text())['assets']
    dimensions=[(640,480),(1440,1080),(1920,1200),(1920,1080),(2560,1080),(3840,1080),(5120,1440),(1919,1079)]
    raster=[];button_rows=[];grid_rows=[];projection=[];pointer=[];visits=0;service_names=set()
    points={(0,0),(-960,600),(960,-600)}
    for pane in assets['panes']:
        for b in pane['buttons']:
            top,left,bottom,right=b['bounds'];points.update(((left,top),(right,bottom)))
        for t in pane['texts']:
            top,left,bottom,right=t['bounds'];points.update(((left,top),(right,bottom)))
    for pi,pane in enumerate(assets['panes']):
        for bi,button in enumerate(pane['buttons']):
            o=Oracle(image);r=o.button(button)
            assert r['retained_draw']==button['bounds']
            expected=button['bounds'].copy();expected[0]-=r['hit_top_offset'];assert r['hit']==expected
            assert r['runtime_flags']==r['source_flags']&3
            button_rows.append(dict(pane=pi,button=bi,**r));visits+=o.visits;service_names.update(o.services)
        for gi,grid in enumerate(pane['grids']):
            for i in range(grid['count']):
                o=Oracle(image);r=o.grid(grid,i)
                assert r['result']==[grid['position'][0],grid['position'][1]+grid['step_xy'][1]*i]
                grid_rows.append(dict(pane=pi,grid=gi,**r));visits+=o.visits;service_names.update(o.services)
    max_native_delta=0
    for width,height in dimensions:
        o=Oracle(image);raster.append(o.raster(width,height))
        for x,y in sorted(points):
            row=o.project(x,y);native=native_projection(width,height,x,y)
            delta=max(abs(a-b) for a,b in zip(row['result'][:2],native))
            max_native_delta=max(max_native_delta,delta)
            projection.append(dict(width=width,height=height,**row,native_derived=native,delta=delta))
        for x,y in ((0,0),(width//2,height//2),(width-1,height-1),(width//4,height//3)):
            pointer.append(dict(width=width,height=height,**o.pointer(x,y)))
        visits+=o.visits;service_names.update(o.services)
    owners=(('raster',0x65e191,0x65e22f),('raster_fields',0x664979,0x664a18),
        ('projection',0x61d05f,0x61d162),('rectangle_projection',0x61d162,0x61d1f8),
        ('projection_distance',0x6096a0,0x6096a7),('mouse_inverse',0x61d620,0x61d67e),
        ('layout',0x610a44,0x610b4e),('pane_layout',0x6107df,0x610a44),
        ('button_initializer',0x63f85b,0x63f9b7),('button_hit',0x63fff8,0x64001d),
        ('bounds_copy',0x6114e0,0x6114f2),('bounds_set',0x6116d2,0x6116e4),
        ('animated_bounds',0x611703,0x61174b),('float_bounds',0x61174b,0x6117e0),
        ('grid_origin',0x620493,0x6205b6),('skin_children',0x642910,0x642bb4),
        ('translate_bounds',0x47708e,0x4770ac),('bitmap_initialize',0x61c3fb,0x61c5dc),
        ('button_draw',0x63f9b7,0x63fce5),('button_frame',0x63ff21,0x63ff8e),
        ('header_builder',0x60fd41,0x60ff73),('subtitle_builder',0x60ff73,0x6101a4))
    source_spans={name:p.span(a,b) for name,a,b in owners}
    report=dict(source_sha256=SHA,map_sha256=assets['source']['sha256'],
        tool_sha256=sha(Path(__file__).read_bytes()),source_spans=source_spans,
        raster_cases=raster,buttons=button_rows,grids=grid_rows,projections=projection,pointers=pointer,
        instructions=visits,services=sorted(service_names),max_current_native_binary32_delta=max_native_delta,
        floating_environment=dict(x87_control='027F',mxcsr='00001F80',projection_distance=0,widget_depth=0),
        inspected_native_source_pins=[dict(path=f,sha256=sha((ROOT/f).read_bytes())) for f in
            ('src/ui/frontend_vista.cpp','src/ui/frontend_painter.cpp','include/halo2/frontend.hpp','include/halo2/frontend_canvas.hpp')],
        limits=__doc__)
    (OUT/'geometry-original.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(raster_cases=len(raster),button_cases=len(button_rows),grid_cases=len(grid_rows),
        projection_cases=len(projection),pointer_cases=len(pointer),instructions=visits,
        max_current_native_binary32_delta=max_native_delta,report_sha256=sha((OUT/'geometry-original.json').read_bytes()))))

if __name__=='__main__':main()
