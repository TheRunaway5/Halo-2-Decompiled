#!/usr/bin/env python3
"""Whole physical2749F0 create/find/link, real pressure and retirement callbacks."""
import argparse,copy,gzip,hashlib,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
import oracle_native_campaign_ai_perception_group_actor_tu15 as actor_inputs
from oracle_native_campaign_ai_perception_group_actor_tu15 import BASE,ACTORS,PROPERTIES,GROUPS,OBJECTS,AROWS,PROWS,GROWS,HEADERS,BODY,PARENT,CLUMP,BITS
sha=common.sha
CLUMPS,GROUP_BITS,CLUMP_BITS,GAME,AI,CLOCK,STATE=BASE+0x500,BASE+0xe100,BASE+0xe200,BASE+0xf000,BASE+0xf100,BASE+0xf300,BASE+0xf400
def intern_banks(fixture):
    # Each value is still the exact complete physically read bank, including
    # unchanged bytes. Deduplicate bytes; do not derive output from stores.
    payloads={}
    for case in fixture['cases']:
        for bank in case['banks']:
            for phase in ('before','after'):
                raw=bank.pop(phase);digest=hashlib.sha256(bytes.fromhex(raw)).hexdigest()
                assert digest not in payloads or payloads[digest]==raw
                payloads[digest]=raw;bank[phase+'_ref']=digest
    fixture['bank_format']='physical-complete-bank-intern-v1';fixture['bank_data']=payloads
    return fixture
def prepare(image,meta,scratch):
    owned=scratch/'physical-original';owned.mkdir();assembly='.code32\n';linker='ENTRY(_start)\nSECTIONS{'
    for i,section in enumerate(image.sections):
        source=owned/f'original{i}.bin';raw=image.section_bytes(section);source.write_bytes(raw);name='original'+str(i);assembly+=f'.section .{name},"'+('ax'if section.name=='.text'else'aw')+f'"\n.incbin "{source}"\n';
        if len(raw)<section.vsize:assembly+=f'.space {section.vsize-len(raw)}\n'
        linker+=f'.{name} 0x{section.va:x}:{{*(.{name})}} '
    for name,address,size in [('bank',BASE,0x10000),('stack',geometry.STACK,65536)]:assembly+=f'.section .{name},"aw"\n.space {size}\n';linker+=f'.{name} 0x{address:x}:{{*(.{name})}} '
    assembly+='.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n';linker+=f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n';(owned/'reference.s').write_text(assembly);(owned/'link.ld').write_text(linker);(owned/'metadata.json').write_text(json.dumps(meta))
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_perception_group_link_tu15.cpp'),'-o',str(owned/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert r.returncode==0,r.stderr
    return owned
def inputs(image):
    rows=[];seeds=actor_inputs.inputs(image)
    def make(seed,name=None,existing=False,missing_clump=False,group_refuse=False,pressure=0,property_slots=None,duplicate=False,attached_match=False,already=False):
        source=copy.deepcopy(seed);banks={b['address']:bytearray.fromhex(b['before'])for b in source['banks']};banks[0x4ea000]=bytearray([0x79]*4096);banks[0x4f3000]=bytearray([0x83]*4096);banks[0x51d000]=bytearray([0x89]*4096);banks[0x47d000]=bytearray(image.read(0x47d000,4096));banks[0x474000]=bytearray(image.read(0x474000,4096));banks[0x44abec]=bytearray(image.read(0x44abec,12))
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(8*w))-1)).to_bytes(w,'little');return
            raise ValueError((hex(p),w))
        def get(p,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):return int.from_bytes(data[p-first:p-first+w],'little')
            raise ValueError(hex(p))
        # The top caller's target object is a real admitted unit. Its actor
        # team field and the campaign tables are independent retained inputs.
        for p,v in [(0x50eef8,CLUMPS),(CLUMPS+0x44,CLUMP),(CLUMPS+0x24,0x50),(CLUMPS+0x20,4),(CLUMPS+0x38,2),(CLUMPS+0x48,CLUMP_BITS),(CLUMP_BITS,3),(GROUPS+0x24,0xc4),(GROUPS+0x20,4),(GROUPS+0x34,0),(GROUPS+0x38,0),(GROUPS+0x3c,0),(GROUPS+0x48,GROUP_BITS),(GROUP_BITS,0),(CLUMP+0x14,0xe3000000 if existing else 0xffffffff),(0x4f3218,GAME),(GAME+8,1),(0x5020c4,AI),(0x51df54,CLOCK),(CLOCK+8,1000),(CLOCK+4,0x3d088889),(0x5020a8,STATE),(0x4eabcc,7),(0x4eabd4,8 if already else 6),(BODY+0x12c,0xe1000000),(BODY+0x13c,0xffffffff)]:put(p,v)
        put(GROUPS+0x40,0x8000,2);put(HEADERS+12,0xea00,2);put(HEADERS+15,0,1);put(OBJECTS+0x38,16);put(OBJECTS+0x24,12);put(BODY+0xaa,12 if attached_match else 0,1);put(BODY+0x10a,0,1);put(BODY+0x138,2,2);put(AROWS+0x24,2,2);put(CLUMP+0x12,3,2);put(CLUMP+0x24,4 if group_refuse else 0,2);put(STATE+1,0,1)
        for i in range(8):put(AI+0xc4+i*4,0)
        for i in range(4):put(AROWS+i*0x888+0x7c,0xffffffff if missing_clump else 0xe4000000)
        # The standalone actor matrix's full labels did not necessarily
        # exhaust append capacity. Here the actual header/bitmap does.
        if get(BITS)==0xffff:put(PROPERTIES+0x38,16);put(PROPERTIES+0x3c,16)
        if get(PROPERTIES+0x20)<8:
            put(PROPERTIES+0x38,0);put(PROPERTIES+0x3c,0);put(BITS,0)
        if property_slots is not None:
            for p,v in [(PROPERTIES+0x20,property_slots),(PROPERTIES+0x34,0),(PROPERTIES+0x38,0),(PROPERTIES+0x3c,0),(BITS,0)]:put(p,v)
            for i in range(4):put(AROWS+i*0x888+0x58,0xffffffff)
        if existing:
            for p,v in [(GROWS+8,0xea000002 if attached_match else 0xea000001),(GROWS+0x14,0xffffffff),(GROWS+0x18,0xe2000000)]:put(p,v)
            put(GROWS+0x22,1 if attached_match else 0,1);put(GROWS+0x1c,0xe7000000)
            owners=[0xe1000000,0xe1000001,0xe1000000]if duplicate else [0xe1000000]
            for i,owner in enumerate(owners):put(PROWS+i*0x3c+4,owner);put(PROWS+i*0x3c+0x34,0xe2000000+i+1 if i+1<len(owners)else 0xffffffff)
            if attached_match:put(BODY+0x134,0);put(BODY+0x13a,0x400,2);put(BODY+0x404,0xe7000000)
        if pressure:
            for p,v in [(GROUPS+0x20,1),(GROUPS+0x34,1),(GROUPS+0x38,1),(GROUPS+0x3c,1),(GROUP_BITS,1),(CLUMP+0x50+0x14,0xe3000000),(GROWS+8,0xea000005),(GROWS+0x14,0xffffffff),(GROWS+0x18,0xe2000000 if pressure>=2 else 0xffffffff)]:put(p,v)
            put(CLUMP+0x24,2,2);put(CLUMP+0x50+0x24,0,2);put(CLUMP+0x50+0x26,0,2);put(CLUMP+0x50+0x1c,1,1);put(STATE+1,1,1);put(GROWS+0xc,0,2)
            if pressure>=2:
                actor=AROWS+0x888;put(CLUMP+0x18,0xe1000000);put(AROWS+0x80,0xffffffff);put(AROWS+0x58,0xffffffff);put(actor+0x58,0xe2000000);put(actor+0x7c,0xe4000001);put(actor+0x190,0 if pressure==3 else 0xffff,2);put(actor+7,0,1)
                for off in (0x338,0x32c,0x344,0x368,0x7e0,0x3b4):put(actor+off,0xffffffff)
                for off in (0x722,0x4ac,0x688,0x6a0,0x6b0):put(actor+off,0,2)
                for p,v in [(PROPERTIES+0x34,1),(PROPERTIES+0x38,1),(PROPERTIES+0x20,16),(PROPERTIES+0x3c,1),(BITS,1),(PROWS+4,0xe1000001),(PROWS+8,0xe3000000),(PROWS+0x14,0xffffffff),(PROWS+0x2c,0xffffffff),(PROWS+0x30,0xffffffff),(PROWS+0x34,0xffffffff),(PROWS+0x38,0xffffffff)]:put(p,v)
                if pressure==3:
                    put(actor+0x90,27,2);put(actor+0xa0,0xffffffff);put(actor+0xb4,0xe2000000)
                    table_entry=0x47b478+27*4;descriptor=int.from_bytes(image.read(table_entry,4),'little');banks[table_entry]=bytearray(image.read(table_entry,4));banks[descriptor+0x28]=bytearray(image.read(descriptor+0x28,4));assert int.from_bytes(banks[descriptor+0x28],'little')==0x1bda70
        n=len(rows);source.update(id=n,name=(name or source['name'])+'_'+str(n),entry=0x2749f0,object=0xea000001,actor=0xe1000000,priority=(0,1,3,0xffff)[n%4],matrix=0xe1000000,source=(0,1,3,0xffff)[n%4],banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())]);source['gp'][0]=0xea000001;rows.append(source)
    for seed in seeds:
        if seed['name'].startswith(('fault_seed','actor_head_alias','empty_clump_group_alias')):continue
        make(seed)
    seed=seeds[0]
    for existing in (False,True):
        for missing in (False,True):make(seed,name='missing_or_existing',existing=existing,missing_clump=missing)
    make(seed,name='existing_last_actor_match',existing=True,duplicate=True);make(seed,name='existing_attachment_owner',existing=True,attached_match=True);make(seed,name='class_zero_already_stamped',already=True);make(seed,name='classification_threshold_refusal',group_refuse=True)
    for pressure in (1,2,3):make(seed,name='real_pressure_retirement',pressure=pressure)
    fanout=next(s for s in seeds if s['name'].startswith('group_fanout')and int.from_bytes(bytes.fromhex(next(b['before']for b in s['banks']if b['address']==BASE))[CLUMP-BASE+0x18:CLUMP-BASE+0x1c],'little')==0xe1000000 and 'fault'not in s['name'])
    for slots in (0,1,2):
        copy_seed=copy.deepcopy(fanout);data=bytearray.fromhex(next(b['before']for b in copy_seed['banks']if b['address']==BASE));struct.pack_into('<I',data,AROWS-BASE+0x80,0xe1000001);struct.pack_into('<I',data,AROWS-BASE+0x888+0x80,0xe1000002);struct.pack_into('<I',data,AROWS-BASE+2*0x888+0x80,0xffffffff)
        next(b for b in copy_seed['banks']if b['address']==BASE)['before']=data.hex();make(copy_seed,name='genuine_partial_property_refusal',property_slots=slots)
    make(seed,name='fault_seed_create');make(seed,name='fault_seed_pressure_callback',pressure=3);make(seed,name='fault_seed_existing',existing=True,duplicate=True)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
    common.ENTRIES=(0x2749f0,0x274450,0x274e30,0x273b70,0x274490,0x274200,0x274190,0x274020,0x2660f0,0x274670,0x2745e0,0x274750,0xbcdf0,0xbbe00,0x1e7810,0x160a60,0xc1e90,0x29a990,0xc0ca0,0x16e6a0,0x16e8a0,0x274800,0x2748e0,0x274970,0x1ea400,0x299aa0,0x1bda70,0x1c8e90,0x1c75b0,0x1c9750);meta=common.metadata(image)
    for site in meta['sites']:
        # F3 also prefixes scalar SSE. Only actual repeated string stores
        # suppress their operand when ECX is zero.
        if site['bytes'][:4] in ('f3aa','f3ab'):site['repeat_count_register']=1
        if site['instruction']==0x16e6c5:assert site['bytes']=='852c8b';site['operands'][0]['read']=True
        if site['instruction']==0x2742b1:assert site['bytes']=='85048e';site['operands'][0]['read']=True
        if site['instruction']==0x2747eb:assert site['bytes']=='d95b28';site['operands'][0]['read']=False;site['operands'][0]['write']=True
    owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in (r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];fixture=intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Whole perception link physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(p)for p in uncovered],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],private_numeric_fault_transport_closed=False,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_perception_group_link_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_perception_group_link_tu15.cpp'),'tools/oracle_native_campaign_ai_perception_group_actor_tu15.py':sha(ROOT/'tools/oracle_native_campaign_ai_perception_group_actor_tu15.py')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
