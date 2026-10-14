#!/usr/bin/env python3
"""Complete original16A06A/16B44A section1 with literal authored clip records.

Only169F3B/16B31B clip resolution is supplied, from named original jmad data
or explicit missing/boundary records. Original event4 scan, conversion and
all branching execute unchanged. This does not prove graph lookup or a view.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_weapon_barrel_timing_tu15 import Original as TimingOriginal
from oracle_weapon_barrel_timing_tu15 import TIME,STACK,STOP,pack,u32
from weapon_reference_versions import images
from inspect_multiplayer import Cache
from unicorn.x86_const import *
CLIP,EVENTS=0x1020000,0x1021000

class Original(TimingOriginal):
    def __init__(self,label,image):
        super().__init__(label,image)
        self.entry,self.lookup,self.constant=(0x16a06a,0x169f3b,0x4650dc)if label=='TU1.1'else(0x16b44a,0x16b31b,0x46a17c)
        self.present=False
    def audit(self,g,address,size,_):
        super().audit(g,address,size,_)
        if address!=self.lookup:return
        sp=g.reg_read(UC_X86_REG_ESP)
        assert u32(g.mem_read(sp+4,4))==0x08000071 and u32(g.mem_read(sp+8,4))==0
        ret=u32(g.mem_read(sp,4))
        g.reg_write(UC_X86_REG_EAX,CLIP if self.present else 0)
        g.reg_write(UC_X86_REG_ESP,sp+12);g.reg_write(UC_X86_REG_EIP,ret)
    def call_timing(self,clip,events,rate):
        g=self.g;self.present=clip is not None
        if clip is not None:
            g.mem_write(CLIP,clip);g.mem_write(CLIP+0x50,pack([EVENTS]))
            if events:g.mem_write(EVENTS,events)
        g.mem_write(TIME+2,struct.pack('<h',rate))
        g.mem_write(STACK,pack([STOP,0x08000071,1]));g.reg_write(UC_X86_REG_ESP,STACK)
        g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        g.emu_start(self.entry,STOP,count=10000)
        assert g.reg_read(UC_X86_REG_EIP)==STOP and g.reg_read(UC_X86_REG_ESP)==STACK+12
        return g.reg_read(UC_X86_REG_EAX)&65535

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--map',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
    originals=[Original(label,image)for label,image,_ in images()if label in ('TU1.1','TU1.5')]
    scales=[u32(o.image.read(o.constant,4))for o in originals];assert scales[0]==scales[1]
    c=Cache(a.map);records=[];seen=set()
    for tag in c.tags.values():
        if tag['group']!='weap':continue
        for record in c.block(tag['offset']+0x2a8,16):
            graph=c.tags.get(c.u(record+12))
            if not graph or graph['offset']is None:continue
            for index,offset in enumerate(c.block(graph['offset']+0x2c,0x6c)):
                if (graph['datum'],index)in seen:continue
                seen.add((graph['datum'],index))
                clip=bytes(c.data[offset:offset+0x6c]);events=b''.join(bytes(c.data[e:e+4])for e in c.block(offset+0x4c,4))
                records.append(dict(graph=graph['name'],index=index,clip_hex=clip.hex(),events_hex=events.hex()))
    # Explicit state-boundary controls, never claimed to be authored clips.
    for frames in (-1,0,1,10,32767):
        for event in (-1,0,1,5,10,32767):
            clip=bytearray(0x6c);struct.pack_into('<h',clip,0x14,frames);struct.pack_into('<I',clip,0x4c,1)
            records.append(dict(graph='declared ABI boundary',index=len(records),clip_hex=clip.hex(),events_hex=struct.pack('<hh',4,event).hex()))
    records.append(dict(graph='declared missing clip',index=0,clip_hex=None,events_hex=''))
    lines=['# present frames first_event4 tick_rate original_animation_seconds result16'];count=0
    for record in records:
        clip=bytes.fromhex(record['clip_hex'])if record['clip_hex']is not None else None
        events=bytes.fromhex(record['events_hex']);frames=struct.unpack_from('<h',clip,0x14)[0]if clip else 0
        event4=next((frame for kind,frame in struct.iter_unpack('<hh',events)if kind==4),-1)
        for rate in (1,24,30,60,120):
            outputs=[o.call_timing(clip,events,rate)for o in originals];assert outputs[0]==outputs[1]
            words=[int(clip is not None),frames&65535,event4&65535,rate,scales[0],outputs[0]]
            lines.append(' '.join(f'{v:08x}'for v in words));count+=1
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text('\n'.join(lines)+'\n')
    report=dict(scope=__doc__,records=records,cases=count,instruction_patches=[],
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
        map_sha256=hashlib.sha256(a.map.read_bytes()).hexdigest(),
        images=[dict(label=o.label,sha256=hashlib.sha256(o.image.data).hexdigest(),
            reached_instruction_bytes={f'{p:08x}':raw for p,raw in sorted(o.instructions.items())})for o in originals])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=count,authored_clips=len(seen))),flush=True)
if __name__=='__main__':main()
