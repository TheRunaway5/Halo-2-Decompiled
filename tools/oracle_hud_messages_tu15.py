#!/usr/bin/env python3
"""Execute unchanged TU1.5 HUD queue submission and message fade instructions.

The queue, UTF16 inputs, clock and HUD durations are declared ABI inputs.
No original instruction, text result, replacement slot or math result is supplied.
Run through tools/run_test.py; retain only the bounded fixture/report.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *
from oracle_multiplayer_hud_positions import Retail, STACK, STOP
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from xbe import Xbe


def bits(x):
    return struct.unpack('<I',struct.pack('<f',x))[0]


class Original(Retail):
    def __init__(self):
        image=Xbe(NEW_DEFAULT)
        assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
        self.g=g=Uc(UC_ARCH_X86,UC_MODE_32)
        g.mem_map(0x10000,0x600000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(0x1000000,0x400000);g.mem_map(0x2000000,0x10000);g.mem_map(STOP,0x1000)
        self.next=0x1000000;self.pool=self.alloc(bytes(0x13a8));self.clock=self.alloc(bytes(16))
        self.text=self.alloc(bytes(1024));self.definition=self.alloc(bytes(0x84));self.sorted=self.alloc(bytes(0x220))
        self.w(0x50eecc,self.pool);self.w(0x51df54,self.clock)
        self.image=image

    def submit(self,tick,text,view=0):
        encoded=text.encode('utf-16-le')
        self.g.mem_write(self.text,encoded+bytes(1024-len(encoded)))
        self.w(self.clock+8,tick)
        self.run(0x25aaf1,argv=[self.text],registers=[(UC_X86_REG_EAX,view)])

    def bank(self,view=0):
        raw=bytes(self.g.mem_read(self.pool+view*0x4e0,0x220))
        self.g.mem_write(self.sorted,raw)
        self.run(0x32b560,argv=[self.sorted,4,0x88,0x25ac7d])
        return dict(rows_hex=raw.hex(),sorted_hex=bytes(self.g.mem_read(self.sorted,0x220)).hex(),
                    next_sequence=self.read(self.pool+0x1385,'<B')[0])

    def fade(self,age,hold,fade,opacity):
        self.w(self.definition+0x58,bits(hold));self.w(self.definition+0x5c,bits(fade))
        frame=STACK-0x100;self.w(frame-0x44,self.definition)
        self.w(frame-0x60,bits(opacity));self.w(frame-0x34,bits(opacity))
        self.prepare(registers=[(UC_X86_REG_EBP,frame),(UC_X86_REG_XMM1,bits(age))])
        self.g.emu_start(0x25b2c5,0x25b326,count=200000)
        assert self.g.reg_read(UC_X86_REG_EIP)==0x25b326
        return self.r(frame-0x60)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    assert os.environ.get('HALO2_TEST_SCRATCH'),'Run through tools/run_test.py'
    original=Original();submissions=[]
    inputs=[(0,'',0),(0,'Teleporter blocked',0),(1,'second',0),(1,'third',0),
        (2,'fourth',0),(2,'fifth',0),(3,'\u00e9\u03a9\U0001f680',0),(4,'x'*90,0),
        (5,'another viewport',3),(6,'NONE recipient',0xffffffff)]
    inputs += [(10+i,'sequence '+str(i),0) for i in range(258)]
    for index,(tick,text,view) in enumerate(inputs):
        original.submit(tick,text,view)
        if tick<20 or tick>260:
            submissions.append(dict(input=index,tick=tick,text=text,view=view,
                bank=original.bank(),view3=original.bank(3)))
    fades=[]
    for hold,fade in ((3.,1.),(0.,1.),(1.,0.),(.5,.25)):
        for opacity in (1.,.375):
            for age in (0.,hold,hold+.01,hold+fade*.25,hold+fade*.5,hold+fade,hold+fade+1):
                fades.append(dict(age_bits=bits(age),hold_bits=bits(hold),fade_bits=bits(fade),
                    opacity_bits=bits(opacity),alpha_bits=original.fade(age,hold,fade,opacity)))
    report=dict(original_sha256=NEW_PIN,queue_entry='0025aaf1',queue_slot_entry='0025abde',
        fade_range=['0025b2c5','0025b326'],unmodified_math_entry='0037cde8',
        supplied_math_results=[],original_instruction_changes=[],
        color_bytes=original.image.read(0x4751e4,28).hex(),
        fade_exponent_bytes=original.image.read(0x46a378,8).hex(),
        inputs=[dict(tick=tick,view=view,utf16=list(struct.unpack('<'+'H'*(len(text.encode('utf-16-le'))//2),
            text.encode('utf-16-le')))) for tick,text,view in inputs],submissions=submissions,fades=fades)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(queue_inputs=len(inputs),retained_queue_cases=len(submissions),fade_cases=len(fades),
        fixture_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())))


if __name__=='__main__':main()
