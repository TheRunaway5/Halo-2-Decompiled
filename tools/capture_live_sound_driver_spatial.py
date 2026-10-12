"""Observe a genuine original220E00 call and its actual SDK handoffs in GDB.

The original game executes every callee normally. Hardware breakpoints observe
SetPosition, SetI3DL2Source and SetFilter; no source instruction, guest memory,
register, method result or return address is changed. Run at original220E00
with H2_SOUND_DRIVER_SPATIAL_OUTPUT naming a new directory. At its actual caller
return, finish_driver_capture() saves the retained driver state and SDK inputs.
This is original handoff/state evidence, not native or audible parity.
"""
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import gdb

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

image_path=ROOT/'assets/disc/default.xbe'
image_sha256=hashlib.sha256(image_path.read_bytes()).hexdigest()
assert image_sha256=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image=Xbe(image_path);inferior=gdb.selected_inferior()
output=Path(os.environ['H2_SOUND_DRIVER_SPATIAL_OUTPUT']);output.mkdir(parents=True,exist_ok=False)
def read(address,size):return bytes(inferior.read_memory(address,size))
def word(address):return struct.unpack('<I',read(address,4))[0]
registers={name:int(gdb.parse_and_eval('$'+name))&0xffffffff for name in
           ('eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags','fctrl','fstat','ftag','mxcsr')}
assert registers['eip']==0x220e00
return_pc,voice_index=struct.unpack('<Ii',read(registers['esp'],8))
driver=word(0x51ebe4);voice=driver+0x1a18+voice_index*0x3c
request=read(registers['edi'],36);buffer=word(voice+0x34)
before=read(driver,0x2ad4)
filter_controls=read(0x470084,8)
(output/'driver-before.bin').write_bytes(before)
calls=[];foreign_calls=[]


class OriginalSdkCall(gdb.Breakpoint):
    def __init__(self,name,entry,caller,words,payload_size=0):
        super().__init__(f'*{entry:#x}',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
        self.label=name;self.entry=entry;self.caller=caller;self.words=words;self.payload_size=payload_size
    def stop(self):
        sp=int(gdb.parse_and_eval('$esp'))&0xffffffff
        caller=word(sp)
        if caller!=self.caller:
            foreign_calls.append(dict(method=self.label,caller=f'{caller:08x}'))
            return False
        arguments=list(struct.unpack('<'+'I'*self.words,read(sp+4,self.words*4)))
        assert arguments[0]==buffer
        opcode=read(self.entry,8);assert opcode==image.read(self.entry,8)
        row=dict(method=self.label,entry=f'{self.entry:08x}',caller=f'{caller:08x}',
                 words=[f'{v:08x}'for v in arguments],entry_original8_hex=opcode.hex())
        if self.payload_size:row['payload_hex']=read(arguments[1],self.payload_size).hex()
        row['stack_address']=f'{sp:08x}'
        calls.append(row)
        # Each complete source caller contains one call at this site and no
        # loop. Disable after that invocation's first hardware observation;
        # repeated breakpoint notifications at the same guest PC are not
        # evidence of an additional executed SDK call.
        self.enabled=False
        return False


def install_driver_observers():
    # The entry breakpoint must be disabled first: three SDK observers plus
    # the actual caller-return breakpoint use the four hardware slots.
    return [OriginalSdkCall('SetPosition',0x37c668,0x220ea6,5),
            OriginalSdkCall('SetI3DL2Source',0x37c6e5,0x221427,3,36),
            OriginalSdkCall('SetFilter',0x37b68b,0x22147c,2,24)]


def finish_driver_capture():
    pc=int(gdb.parse_and_eval('$eip'))&0xffffffff
    sp=int(gdb.parse_and_eval('$esp'))&0xffffffff
    assert pc==return_pc and sp==registers['esp']+8
    assert word(0x51ebe4)==driver
    after=read(driver,0x2ad4);(output/'driver-after.bin').write_bytes(after)
    assert read(0x470084,8)==filter_controls
    request_words=struct.unpack('<9I',request)
    positions=[r for r in calls if r['method']=='SetPosition']
    for row in positions:
        assert [int(v,16)for v in row['words']]==[buffer,request_words[1],request_words[3],request_words[2],1]
    differences=[dict(offset=f'{i:04x}',before=a,after=b)for i,(a,b)in enumerate(zip(before,after))if a!=b]
    report=dict(boundary=__doc__,image_sha256=image_sha256,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        registers=registers,request_address=f'{registers["edi"]:08x}',request_hex=request.hex(),
        voice_index=voice_index,driver_address=f'{driver:08x}',voice_address=f'{voice:08x}',buffer_identity=f'{buffer:08x}',
        return_pc=f'{return_pc:08x}',actual_return_pc=f'{pc:08x}',actual_return_esp=f'{sp:08x}',
        before_sha256=hashlib.sha256(before).hexdigest(),after_sha256=hashlib.sha256(after).hexdigest(),
        filter_controls_address='00470084',filter_controls_hex=filter_controls.hex(),
        driver_byte_differences=differences,sdk_calls=calls,foreign_sdk_calls=foreign_calls,
        original_position_axis_permutation_confirmed=bool(positions),
        instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],supplied_callee_returns=[])
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Original driver call returned normally: voice={voice_index}, SDK calls={[r["method"]for r in calls]}, {len(differences)} changed driver bytes')


gdb.set_convenience_variable('audio_driver_spatial_return',return_pc)
print(f'Original driver entry: voice={voice_index}, buffer={buffer:08x}, request={request.hex()}')
