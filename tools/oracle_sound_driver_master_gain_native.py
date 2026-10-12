#!/usr/bin/env python3
"""Unchanged complete master gains on real allocated driver prefix/raw ABI inputs."""
import argparse
import json
import struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe,_Code,sha,save,encoded
from oracle_sound_startup_native import AUDIO_ARENA,AUDIO_CAPACITY

PAIR=ROOT/'analysis/multiplayer/integration/sound-driver-master-gain-native/paired'
PAIR_PIN='dec64b3042c91f34913b2622f8aa37e4ffad93549cb58affb3f3e31a11433a13'
DRIVER=ROOT/'analysis/multiplayer/integration/sound-driver-startup-native/measured/report.json'
DRIVER_PIN='12751ad5e29ef686a654884baa716d6b9446cf489ccf5194d91d7d3eb5845f24'
RESET=ROOT/'analysis/multiplayer/integration/grenade-sound-pool/measured/report.json'
RESET_PIN='9c587bcde2735fcccdb74baba633fa8104bad1a49b7fe6d5a4530b4a8ebb1f36'
STACK,STOP=0x02008000,0x03000000


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    for path,pin in ((PAIR/'report.json',PAIR_PIN),(DRIVER,DRIVER_PIN),(RESET,RESET_PIN)):
        assert sha(path)==pin
    pair=json.loads((PAIR/'report.json').read_text());parent=json.loads(DRIVER.read_text())
    reset=json.loads(RESET.read_text())['final_state']['sound_owner_context']
    input_state=int(reset['owner_state_pointer'],16);input_address=input_state+0x228
    actual_input=bytes.fromhex(reset['owner_state244_hex'])
    assert len(actual_input)==0x244
    words=list(struct.unpack('<6I',actual_input[0x228:0x240]))
    assert words==[0x3f800000]*4+[0x1f40,0]
    vectors=[('actual_reset',words)]
    for value in (0,0x80000000,0x3e800000,0x3f000000,0x3f800000,0x40000000,0xbf800000,
                  1,0x00800000,0x7f800000,0xff800000,0x7fc00001,0xffc00001,0x7f800001,0xff800001):
        vectors.append((f'uniform_{value:08x}',[value]*4+words[4:]))
    vectors.extend([('mixed_gain_slots',[0x3f000000,0,0x3e800000,0x40000000,0x11223344,0x3f400000]),
                    ('fractional_conversion',[0x3f19999a,0x3dcccccd,0x3e99999a,0x80000000,0xffffffff,0x7fc00001])])
    images=[]
    for side,path in enumerate((OLD,NEW)):
        assert sha(path)==PINS[str(path)];image=Xbe(path);code=_Code(image,'.text')
        owner=pair['owners'][0]
        entry=int(owner['old'if side==0 else'u5'],16)
        globals_={int(q['old_va'],16):int(q['old_va'if side==0 else'new_va'],16)for q in owner['memory']}
        output_base=globals_[0x477544];constants=[image.read(globals_[q],4).hex()for q in (0x465030,0x465258,0x465090)]
        driver_case=parent['cases'][side*4];driver=int(driver_case['after']['driver_address'],16)
        regions=[dict(address=f'{AUDIO_ARENA:08x}',raw_hex=driver_case['output_arena_hex'],owner='Actual original driver allocation prefix; device not admitted'),
                 dict(address=f'{input_state:08x}',raw_hex=actual_input.hex(),owner='Actual original incomplete looping-sound map-reset state'),
                 dict(address=f'{output_base:08x}',raw_hex=image.read(output_base,0x30).hex(),owner='Declared original image master-output globals; no runtime initialization inferred')]
        driver_offset=driver-AUDIO_ARENA
        assert struct.unpack_from('<h',bytes.fromhex(regions[0]['raw_hex']),driver_offset+6)[0]==0
        pcs={}
        for row in pair['owners']:
            left=(PAIR/'instructions'/f'{row["old"]}-old.asm').read_text().splitlines()
            right=(PAIR/'instructions'/f'{row["old"]}-u5.asm').read_text().splitlines()
            for a,b in zip(left,right,strict=True):pcs[int((a if side==0 else b).split()[0],16)]=int(a.split()[0],16)
        cases=[];executed=set()
        for name,values in vectors:
            for cw in (0x027f,0x067f,0x0a7f,0x0e7f):
                g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
                for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
                g.mem_map(AUDIO_ARENA,AUDIO_CAPACITY);g.mem_map(0x02000000,0x10000);g.mem_map(STOP,0x1000)
                for page in range(input_state&~0xfff,(input_state+0x244+0xfff)&~0xfff,0x1000):g.mem_map(page,0x1000)
                for region in regions:g.mem_write(int(region['address'],16),bytes.fromhex(region['raw_hex']))
                g.mem_write(globals_[0x526474],struct.pack('<I',driver))
                raw=struct.pack('<6I',*values);g.mem_write(input_address,raw)
                before=[dict(address=q['address'],raw_hex=bytes(g.mem_read(int(q['address'],16),len(bytes.fromhex(q['raw_hex'])))).hex())for q in regions]
                g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(UC_X86_REG_ESP,STACK)
                g.reg_write(UC_X86_REG_EAX,input_address);g.reg_write(UC_X86_REG_FPCW,cw);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
                stores=[];local=set()
                def instruction(_g,address,size,user):
                    assert address in pcs,hex(address);local.add(address)
                def write(_g,kind,address,size,value,user):
                    if not any(int(q['address'],16)<=address and address+size<=int(q['address'],16)+len(bytes.fromhex(q['raw_hex']))for q in regions):return
                    stores.append(dict(instruction=f'{pcs[g.reg_read(UC_X86_REG_EIP)]:08x}',address=f'{address:08x}',bytes=size,
                        before=bytes(g.mem_read(address,size)).hex(),after=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
                g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,write)
                g.emu_start(entry,STOP,count=10000);assert g.reg_read(UC_X86_REG_EIP)==STOP
                after=[dict(address=q['address'],raw_hex=bytes(g.mem_read(int(q['address'],16),len(bytes.fromhex(q['raw_hex'])))).hex())for q in regions]
                cases.append(dict(name=f'{name}_{cw:04x}',input_address=f'{input_address:08x}',x87_cw=f'{cw:04x}',mxcsr='00001f80',
                    input24_hex=raw.hex(),actual_source_reset_input=name=='actual_reset',
                    explicit_input24_control=name!='actual_reset',before=before,after=after,stores=stores,returned=True))
                executed.update(local)
        listing=args.output/'instructions'/f'{"old"if side==0 else"u5"}-executed.asm'
        rows=[code.at(pc)for pc in sorted(executed)]
        for row in rows:assert bytes(row.bytes)==image.read(row.address,len(row.bytes))
        save(listing,''.join(f'{row.address:08x} {row.bytes.hex()} {row.mnemonic} {row.op_str}\n'for row in rows).encode())
        images.append(dict(image=str(path),image_sha256=sha(path),driver=f'{driver:08x}',output_globals=f'{output_base:08x}',
            constants=constants,cases=cases,listing_sha256=sha(listing),executed_instruction_rows=len(rows)))
    report=dict(schema='original-sound-driver-master-gain-v1',oracle_sha256=sha(__file__),paired_sha256=PAIR_PIN,
        driver_allocation_report_sha256=DRIVER_PIN,looping_reset_report_sha256=RESET_PIN,images=images,
        instruction_patches=[],supplied_source_returns=[],
        limits=['Explicit composition of real driver allocation prefix and looping reset backing; no complete source startup or map activation inferred.',
                'Actual static voice count remains zero; positive voice flag loop has static body evidence only. No stream, device, source admission, playback or native grenade completion supplied.',
                'Direct ABI input24 and x87 rounding controls are declared; no audio or gameplay success results are chosen.'])
    save(args.output/'report.json',encoded(report))
    print(json.dumps(dict(report_sha256=sha(args.output/'report.json'),cases=sum(len(q['cases'])for q in images),stores=sum(len(c['stores'])for q in images for c in q['cases']))))


if __name__=='__main__':main()
