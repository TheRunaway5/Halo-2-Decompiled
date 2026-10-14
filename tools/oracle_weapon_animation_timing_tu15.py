#!/usr/bin/env python3
"""Complete unchanged16A06A/16B44A across all animation timing sections.

The literal authored jmad records are inputs. Clip resolution is the sole
supplied service; event4/event0 scan, branch selection and conversion execute
unchanged in both original updates. No graph/view availability is inferred.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from oracle_weapon_vent_timing_tu15 import Original as Base,CLIP,EVENTS
from oracle_weapon_barrel_timing_tu15 import TIME,STACK,STOP,pack
from weapon_reference_versions import images
from unicorn.x86_const import *


class Original(Base):
    def call(self,clip,events,section,rate):
        self.present=clip is not None;g=self.g
        if clip is not None:
            g.mem_write(CLIP,clip);g.mem_write(CLIP+0x50,pack([EVENTS]))
            if events:g.mem_write(EVENTS,events)
        g.mem_write(TIME+2,struct.pack('<h',rate))
        g.mem_write(STACK,pack([STOP,0x08000071,section]))
        g.reg_write(UC_X86_REG_ESP,STACK)
        g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        g.reg_write(UC_X86_REG_FPSW,0);g.reg_write(UC_X86_REG_FPTAG,0xffff)
        g.emu_start(self.entry,STOP,count=10000)
        assert g.reg_read(UC_X86_REG_EIP)==STOP and g.reg_read(UC_X86_REG_ESP)==STACK+12
        return g.reg_read(UC_X86_REG_EAX)&65535


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clips',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args()
    report=json.loads(args.clips.read_text())
    originals=[Original(label,image) for label,image,_ in images() if label in ('TU1.1','TU1.5')]
    scales=[struct.unpack('<I',o.image.read(o.constant,4))[0] for o in originals]
    assert scales[0]==scales[1]
    records=[record for record in report['records'] if not record['graph'].startswith('declared')]
    authored=len(records)
    for frames in (-1,0,1,10,32767):
        for event4,event0 in ((-1,-1),(0,0),(1,1),(5,5),(10,10),(32767,32767),(-2,-2),(1,0),(0,1)):
            clip=bytearray(0x6c);struct.pack_into('<h',clip,0x14,frames)
            struct.pack_into('<I',clip,0x4c,2)
            events=struct.pack('<4h',4,event4,0,event0)
            records.append(dict(graph='declared ABI boundary',clip_hex=clip.hex(),events_hex=events.hex()))
    records.append(dict(graph='declared missing clip',clip_hex=None,events_hex=''))
    lines=['# present frames first_event4 first_event0 section tick_rate animation_seconds result16'];count=0
    for record in records:
        clip=bytes.fromhex(record['clip_hex']) if record['clip_hex'] is not None else None
        events=bytes.fromhex(record['events_hex'])
        frames=struct.unpack_from('<h',clip,0x14)[0] if clip else 0
        event=lambda kind:next((frame for k,frame in struct.iter_unpack('<hh',events) if k==kind),-1)
        for section in (-1,0,1,2,3,4,32767):
            for rate in (1,24,30,60,120):
                results=[o.call(clip,events,section,rate) for o in originals]
                assert results[0]==results[1],(record,section,rate,results)
                words=[int(clip is not None),frames&65535,event(4)&65535,event(0)&65535,
                    section&65535,rate,scales[0],results[0]]
                lines.append(' '.join(f'{value:08x}' for value in words));count+=1
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text('\n'.join(lines)+'\n')
    result=dict(scope=__doc__,cases=count,authored_clips=authored,records=records,instruction_patches=[],
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        clip_report_sha256=hashlib.sha256(args.clips.read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        images=[dict(label=o.label,sha256=hashlib.sha256(o.image.data).hexdigest(),
            reached_instruction_bytes={f'{address:08x}':raw for address,raw in sorted(o.instructions.items())}) for o in originals])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=count,authored_clips=authored)),flush=True)


if __name__=='__main__':main()
