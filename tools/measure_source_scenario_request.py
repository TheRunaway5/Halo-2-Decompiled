#!/usr/bin/env python3
"""Unchanged TU1.1/u5 D6660 and all real request/math/policy callees.

Only declared ABI buffers, raw authored cache rows, explicit game-option
fields and labeled branch controls are supplied. No constructor runs and no
call result or instruction is replaced. Callee FP status is not a byte-owner
claim; the original COMISS boundary is recorded separately from Euler.
"""
import argparse
import json
import random
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT, OLD, NEW, PINS, Xbe, _Code, sha
from inspect_multiplayer import Cache

REQUEST, GAME, PLACEMENT, PALETTE, ROWS = (0x02000000, 0x02001000, 0x02002000, 0x02004000, 0x02005000)
STACK, STOP = 0x02408000, 0x03000000
PAIR = ROOT/'analysis/damage-shields-goal-20261011/overlay-pass/registration/scenario-request/paired-complete'
PAIR_PIN = '2b035c0e6d293b1f7e7b69635b37d661104c49e4f936164a735c8800facd8a05'
MAP = ROOT/'assets/disc/maps/lockout.map'
MAP_PIN = '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args(); out=args.output.resolve(); out.mkdir(parents=True,exist_ok=False)
    assert sha(PAIR/'report.json')==PAIR_PIN and sha(MAP)==MAP_PIN
    pair=json.loads((PAIR/'report.json').read_text()); cache=Cache(MAP)
    source=Xbe(OLD); bank=struct.unpack('<13I',source.read(0x46fad0,52))
    layouts=[struct.unpack('<hhh',source.read(p+0xa,6)) for p in bank]
    specs=[]
    for typ,(placement_offset,palette_offset,stride) in enumerate(layouts):
        if placement_offset==-1 or palette_offset==-1:continue
        rows=cache.block(cache.scenario+placement_offset,stride)
        palettes=cache.block(cache.scenario+palette_offset,40)
        palette_address=cache.scenario+cache.magic+palette_offset
        palette_bytes=bytes(cache.data[cache.scenario+palette_offset:cache.scenario+palette_offset+8])
        for ordinal,p in enumerate(rows):
            regions=[(p+cache.magic,bytes(cache.data[p:p+stride])),(palette_address,palette_bytes)]
            if palettes:regions.append((palettes[0]+cache.magic,bytes(cache.data[palettes[0]:palettes[0]+40*len(palettes)])))
            specs.append(dict(name=f'authored_{typ}_{ordinal}',type=typ,ordinal=ordinal,force=1,
                placement=p+cache.magic,palette=palette_address,regions=regions,authored=True))
    module=cache.block(cache.scenario+0x328,0x4c)[1]
    baseline=bytearray(cache.data[module:module+0x4c])+bytearray(0x5c-0x4c)
    def control(name,typ=11,ordinal=123,force=0,changes=(),count=2,definition=0xe6f80583,extent=None,type_null=False):
        row=baseline.copy()
        row[0x2e]=typ
        for at,raw in changes:row[at:at+len(raw)]=raw
        palette=struct.pack('<II',count&0xffffffff,ROWS)
        choices=bytearray(80);struct.pack_into('<I',choices,4,definition);struct.pack_into('<I',choices,44,definition)
        specs.append(dict(name=name,type=typ,ordinal=ordinal,force=force,placement=PLACEMENT,palette=PALETTE,
            regions=[(PLACEMENT,bytes(row)),(PALETTE,palette),(ROWS,bytes(choices))],authored=False,
            extent=extent,type_null=type_null))
    def dword(value):return struct.pack('<I',value&0xffffffff)
    for typ in range(13):control(f'type_{typ}',typ)
    for extent in (0,0x4b,0x4c,0xffff):control(f'extent_{extent:x}',extent=extent)
    control('null_type',type_null=True)
    for value in (-32768,-2,-1,0,1,2,32767):
        control(f'palette_index_{value}',changes=[(0,struct.pack('<h',value))])
    for count in (0,1,2,0xffffffff,0x80000000):
        control(f'palette_count_{count:x}',count=count,changes=[(0,struct.pack('<h',0))])
    for definition in (0xffffffff,0xe6f80583):
        for flags in (0,8,0x40,0x100,0x140):
            for force in (0,1,255):
                control(f'flags_{definition:x}_{flags:x}_{force}',force=force,definition=definition,changes=[(4,dword(flags))])
    for typ in (0,4,11,43,255):
        for force in (0,1):control(f'raw_type_{typ}_{force}',force=force,changes=[(0x2e,bytes([typ])),(4,dword(0))])
    for policy in (0,1,2,3,127,128,255):
        for src in (0,1,2,128,255):
            for origin in (-1,0):control(f'policy_{policy}_{src}_{origin}',changes=[
                (0x30,bytes([policy])),(0x2f,bytes([src])),(0x2c,struct.pack('<h',origin))])
    for bits in (0,0x80000000,0xbf800000,0x3f800000,0x40000000,0x7f800000,0xff800000):
        control(f'scale_{bits:x}',changes=[(0x20,dword(bits))])
    for engine in range(12):
        for mask in (0,1,2,4,8,16,32,64,0x7f,0x80,0xffff):
            control(f'game_{engine}_{mask:x}',typ=6,changes=[(0x5a,struct.pack('<H',mask)),(4,dword(0x40))])
            specs[-1]['engine']=engine
    for value in range(256):
        colors=struct.pack('<4I',value*0x010101,0xff000000|value*0x010101,
                           (value<<16)|((255-value)<<8)|value,value<<8)
        control(f'color_{value}',changes=[(0x3c,colors)])
    rng=random.Random(0xd6660)
    for i in range(64):
        control(f'euler_{i}',changes=[(0x14,struct.pack('<3f',*(rng.uniform(-100,100) for _ in range(3))))])
    lines=['SCENARIO_REQUEST_V1','D '+f'{next(k for k,v in cache.tags.items() if v["group"]=="scnr"):08x}']; images=[]
    for side,path in enumerate((OLD,NEW)):
        assert sha(path)==PINS[str(path)]
        x=Xbe(path); code=_Code(x,'.text'); pcs={}; entries={}; executed=set()
        for owner in pair['owners']:
            old=owner['old_entry']; actual=owner['old_entry' if side==0 else 'u5_entry'];entries[int(old,16)]=int(actual,16)
            left=(PAIR/'instructions'/f'{old}-old.asm').read_text().splitlines()
            right=(PAIR/'instructions'/f'{old}-{("old","u5")[side]}.asm').read_text().splitlines()
            for a,b in zip(left,right,strict=True):pcs[int(b.split()[0],16)]=int(a.split()[0],16)
        one_at,zero_at,factor_at,forward_at,up_at,bank_at,game_slot=(
            (0x465030,0x465048,0x465024,0x46fc48,0x46fc50,0x46fad0,0x4edf18),
            (0x46a0d0,0x46a0e8,0x46a0c4,0x474d08,0x474d10,0x474b90,0x4f3218))[side]
        pointers=struct.unpack('<13I',x.read(bank_at,52))
        constants=[x.read(one_at,4),x.read(int.from_bytes(x.read(forward_at,4),'little'),12),
                   x.read(int.from_bytes(x.read(up_at,4),'little'),12),x.read(zero_at,4),x.read(factor_at,4)]
        lines.append('I '+str(side)+' '+PINS[str(path)]+' '+' '.join(v.hex() for v in constants))
        for typ,p in enumerate(pointers):
            actual_layout=struct.unpack('<hhh',x.read(p+0xa,6));assert actual_layout==layouts[typ]
            lines.append(f'T {typ} {p:08x} {x.read(p+0xa,6).hex()}')
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
        for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
        g.mem_map(REQUEST,0x500000);g.mem_map(STOP,4096)
        scnr=next(t for t in cache.tags.values() if t['group']=='scnr')
        scnr_address=scnr['offset']+cache.magic
        start=scnr_address&~4095;end=(scnr_address+scnr['size']+4095)&~4095
        g.mem_map(start,end-start)
        cases=[];current=None
        def hook(guest,pc,n,_):
            canonical=pcs.get(pc)
            assert canonical is not None,(current['name'],hex(pc))
            assert bytes(guest.mem_read(pc,n))==x.read(pc,n)==code.at(pc).bytes
            executed.add(pc)
            if canonical==0xd6777:current['comparison_mxcsr']=guest.reg_read(UC_X86_REG_MXCSR)
            call_regs={0xd673b:(4,),0xd677c:(0,1,2),0xd6783:(2,),0xd67c6:(0,1)}
            if canonical in call_regs:
                regs=[0]*6
                ids=(UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI)
                for i in call_regs[canonical]:regs[i]=guest.reg_read(ids[i])
                args=list(struct.unpack('<3I',guest.mem_read(guest.reg_read(UC_X86_REG_ESP),12))) if canonical==0xd673b else []
                entry={0xd673b:0xb8820,0xd677c:0x11f680,0xd6783:0xd7070,0xd67c6:0x133320}[canonical]
                current['calls'].append([canonical,entry,sum(1<<i for i in call_regs[canonical]),regs,args])
        def write(guest,kind,at,n,value,_):
            if REQUEST<=at and at+n<=REQUEST+0xc4:
                current['stores'].append([pcs[guest.reg_read(UC_X86_REG_EIP)],at,n,
                    bytes(guest.mem_read(at,n)).hex(),(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()])
        g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
        for spec in specs:
            current=dict(name=spec['name'],calls=[],stores=[],comparison_mxcsr=0x1f80)
            for p in pointers:g.mem_write(p+0xe,x.read(p+0xe,2))
            g.mem_write(bank_at+spec['type']*4,x.read(bank_at+spec['type']*4,4))
            if spec.get('type_null'):g.mem_write(bank_at+spec['type']*4,b'\0'*4)
            if spec.get('extent') is not None:g.mem_write(pointers[spec['type']]+0xe,struct.pack('<H',spec['extent']))
            # These explicit controls edit metadata fields, never executable instructions.
            game=bytearray(0x184);struct.pack_into('<I',game,8,2);struct.pack_into('<I',game,0x180,spec.get('engine',1))
            g.mem_write(game_slot,struct.pack('<I',GAME))
            prior=bytes((i*37+11)&255 for i in range(0xc4))
            regions=[(REQUEST,prior),(GAME,bytes(game))]+spec['regions']
            for at,data in regions:g.mem_write(at,data)
            for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):g.reg_write(reg,0)
            for reg,value in ((UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_EAX,REQUEST),
                (UC_X86_REG_EDI,spec['placement']),(UC_X86_REG_ECX,spec['palette']),
                (UC_X86_REG_FPCW,0x027f),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff),(UC_X86_REG_MXCSR,0x1f80)):
                g.reg_write(reg,value)
            g.mem_write(STACK,struct.pack('<4I',STOP,spec['type'],spec['ordinal']&0xffffffff,spec['force']))
            g.emu_start(entries[0xd6660],STOP,count=10000)
            assert g.reg_read(UC_X86_REG_EIP)==STOP and g.reg_read(UC_X86_REG_ESP)==STACK+16
            returned=g.reg_read(UC_X86_REG_EAX)&255
            fixture_extent=spec['extent'] if spec.get('extent') is not None else -1
            lines.append('N '+spec['name']+f' {spec["type"]} {spec["ordinal"]} {spec["force"]} {spec["placement"]:08x} {spec["palette"]:08x} {returned} {current["comparison_mxcsr"]:08x} '+str(fixture_extent)+f' {int(bool(spec.get("type_null")))}')
            for at,data in regions:lines.append(f'B {at:08x} {data.hex()}')
            for pc,entry,mask,regs,arguments in current['calls']:
                lines.append(f'C {pc:08x} {entry:08x} {mask:x} '+ ' '.join(f'{v:08x}' for v in regs)+f' {len(arguments)} '+ ' '.join(f'{v:08x}' for v in arguments))
            for pc,at,n,b,a in current['stores']:lines.append(f'S {pc:08x} {at:08x} {n} {b} {a}')
            after=bytes(g.mem_read(REQUEST,0xc4));lines.append('A '+after.hex());lines.append('E')
            current.update(returned_al=returned,request_hex=after.hex(),authored=spec['authored'],
                returned_mxcsr=g.reg_read(UC_X86_REG_MXCSR),returned_x87_status=g.reg_read(UC_X86_REG_FPSW))
            cases.append(current)
        # Every executed instruction remains byte-identical at the end too.
        for pc in executed:assert bytes(g.mem_read(pc,len(code.at(pc).bytes)))==x.read(pc,len(code.at(pc).bytes))
        listing=out/f'image{side}-executed.asm'
        listing.write_text(''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n' for pc in sorted(executed)))
        images.append(dict(image_sha256=sha(path),cases=cases,executed_instructions=len(executed),listing_sha256=sha(listing)))
    assert [{k:c[k] for k in ('name','calls','stores','returned_al','request_hex','comparison_mxcsr')} for c in images[0]['cases']]==[
        {k:c[k] for k in ('name','calls','stores','returned_al','request_hex','comparison_mxcsr')} for c in images[1]['cases']]
    (out/'fixtures.tsv').write_text('\n'.join(lines)+'\n')
    report=dict(schema='source-scenario-request-v1',tool_sha256=sha(__file__),paired_sha256=PAIR_PIN,map_sha256=sha(MAP),
        scenario_datum=next(k for k,v in cache.tags.items() if v['group']=='scnr'),scenario_address=f'{scnr_address:08x}',
        fixtures_sha256=sha(out/'fixtures.tsv'),images=images,cases_per_image=len(specs),
        authored_per_image=sum(s['authored'] for s in specs),instruction_patches=[],supplied_call_returns=[],
        limits=['Request bytes, defined AL, calls and ordered request stores only; no object creation/physics/world admission.',
                'Nearest math controls027F/1F80. Returned callee arithmetic FP status is recorded but outside byte-binding comparison.',
                'Raw authored cache rows plus explicit game-type2/engine controls; no live current game-options capture.',
                'Type-null and placement-extent controls alter only labeled immutable metadata fields, not instructions.'])
    (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases_per_image=len(specs),authored_per_image=report['authored_per_image'],fixture_sha256=report['fixtures_sha256'])))

if __name__=='__main__':main()
