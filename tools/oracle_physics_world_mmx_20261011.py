#!/usr/bin/env python3
"""Measure the complete owned index-repair body and an active-MMX prefix.

Declared boundary-word inputs test signed comparison, 16-bit wrapping, zero
iterations, x87 TOP/live values and the physical alias after all eight writes.
These generated component inputs are not an Xbox map or gameplay replay.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_physics_world_event_runtime_20261011 import captured_abi


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def measure(image, entry, instruction_map, image_name, ordinal, edge, count, top, partial):
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(entry&~4095,0x2000)
    original=image.read(entry,0xf9);g.mem_write(entry,original)
    g.mem_map(0x6000000,0x4000);g.mem_map(0x3200000,0x1000);g.mem_map(0x9000000,0x1000)
    sp=0x3200108;body=0x6001000;fresh=0x6003000;stop=0x9000000
    words=(0,1,2,0x7fff,0x8000,0x8001,0xfffe,0xffff)
    data=struct.pack('<'+'H'*(8*max(count,1)),*(words[(edge+i*3)%8] for i in range(8*max(count,1))))
    new=struct.pack('<8H',*(words[(edge+i)%8] for i in range(8)))
    stack=bytearray((i*29+edge*17+count)&255 for i in range(256))
    stack[128:144]=struct.pack('<4I',stop,body,count,fresh)
    banks=[dict(address=body,hex=data.hex()),dict(address=fresh,hex=new.hex()),dict(address=sp-128,hex=stack.hex())]
    for bank in banks:g.mem_write(bank['address'],bytes.fromhex(bank['hex']))
    gp=(0x12345678,0x87654321,0x80008000,0x7fff0001,sp,0xabcdef00,0x01020304,0xaabbccdd)
    for name,value in zip(('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI'),gp):g.reg_write(getattr(R,'UC_X86_REG_'+name),value)
    for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),(i+1)*0x102030405060708090a0b0c0d0e0f00)
    for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_FP{i}'),(0x8000000000000000+i*0x1020304050607,0x3fff+i))
    depth=(edge+count)%3;tag=0xffff
    for i in range(depth):tag&=~(3<<(((top+i)&7)*2))
    for name,value in (('EFLAGS',0x246),('FPCW',0x27f),('MXCSR',0x1f80),('FPTAG',tag),('FPSW',(top<<11)|0x4500)):
        g.reg_write(getattr(R,'UC_X86_REG_'+name),value)
    initial=captured_abi(g);trace=[];writes=[];exit_address=None
    def hook(_g,p,_n,_u):
        nonlocal exit_address
        canonical=instruction_map.get(p,p)
        if p==stop or (partial and canonical==0x2e632a):exit_address=p;g.emu_stop();return
        assert p in instruction_map,hex(p)
        trace.append(canonical)
    def write(_g,_a,p,n,value,_u):
        site=g.reg_read(R.UC_X86_REG_EIP)
        writes.append(dict(instruction=instruction_map[site],address=p,before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
    g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
    g.emu_start(entry,0,count=10000);assert exit_address is not None
    assert bytes(g.mem_read(entry,len(original)))==original
    final=captured_abi(g)
    if partial:
        assert all(((final['x87_tag']>>(2*i))&3)!=3 for i in range(8)) and (final['x87_status']&0x3800)==0 and len(final['x87_values'])==8
        # Unicorn2.1.4 reclassifies tags and retains old physical upper words.
        # Retain its raw observations; only low64 payloads are CPU expectations.
        # Intel SDM and the separate host probe establish upper words/tags.
        final['active_mmx']=dict(known_mask=255,physical=final['x87_values'])
        final['x87_values']=[]
    else:assert final['x87_tag']==0xffff and not final['x87_values']
    return dict(id=ordinal,image=image_name,canonical_entry=0x2e6240,entry_abi=initial,
        declared_input=dict(edge_rotation=edge,body_count=count,initial_top=top,initial_depth=depth),
        initial_banks=banks,final_banks=[dict(address=b['address'],hex=bytes(g.mem_read(b['address'],len(b['hex'])//2)).hex()) for b in banks],
        instructions=trace,writes=writes,exit=final,stop_before=0x2e632a if partial else 0,complete=not partial)


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--correspondence',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    pair=json.loads(a.correspondence.read_text());assert pair['disc_1_1_sha256']==PIN and pair['later_owned_xbe_sha256']==NEW_PIN
    old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(old_path)==PIN and sha(NEW_DEFAULT)==NEW_PIN
    old=Xbe(old_path);new=Xbe(NEW_DEFAULT);row=next(r for r in pair['routines'] if r['disc_1_1_entry']=='002e6240')
    assert row['instructions']==71 and row['complete_paired_body']
    old_sites={int(p,16):int(p,16) for p in pair['instruction_map']}
    new_sites={q:int(p,16) for p,q in pair['instruction_map'].items() if int(p,16) in {x for x in old_sites if 0x2e6240<=x<=0x2e6336}}
    cases=[]
    for image,entry,sites,name in ((old,0x2e6240,old_sites,'1.1'),(new,pair['entries']['002e6240'],new_sites,'1.5')):
        for edge in range(8):
            for count in (0,1,3,9):
                for top in (0,3,7):cases.append(measure(image,entry,sites,name,len(cases),edge,count,top,False))
            for top in (0,3,7):cases.append(measure(image,entry,sites,name,len(cases),edge,1,top,True))
    assert len(cases)==240
    for left,right in zip(cases[:120],cases[120:]):
        assert all(left[k]==right[k] for k in ('entry_abi','initial_banks','final_banks','instructions','writes','exit','complete','stop_before'))
    report=dict(schema=1,tool_sha256=sha(__file__),correspondence_sha256=sha(a.correspondence),original_sha256=PIN,later_original_sha256=NEW_PIN,
        cases=cases,paired_image_results_equal=True,live_gameplay_proof=False,original_xbox_cpu_executed=False,
        active_alias_upper_words_and_classified_tags_are_emulator_diagnostics=True,
        active_mmx_payload_comparison_bits=64,
        mmx_isa_authority='Intel SDM Volume1 section9.6.2 and Volume3A table11-2; upper wordsFFFF and valid tags are checked separately against host CPU',
        scope='Declared component boundary-word inputs; original owned complete body and active-MMX prefix. Native implementation is not used to generate expectations.')
    a.output.mkdir(parents=True,exist_ok=False);p=a.output/'fixture.json';p.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),fixture_sha256=sha(p))))


if __name__=='__main__':main()
