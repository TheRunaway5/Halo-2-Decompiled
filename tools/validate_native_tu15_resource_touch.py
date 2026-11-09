#!/usr/bin/env python3
"""Qualify whole126200 transfers using unchanged owners and real entry stops."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
import native_resource_touch_replacements as binding
from specialize_native_operands import leaf_body
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
SOURCES=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_actor_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp')
REGIONS=((0x41b000,4096),(0x474000,4096),(0x4f0000,8192),(0x51d000,8192),(0x56e000,4096),(0x61000000,65536),(0x62000000,8192))
ENTRIES=(0x126200,0x126190,0x16e6a0,0x33b590)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def corpus():
    rng=random.Random(0x126200);rows=[]
    for n in range(160):
        mode=n%20;priority=(0,1,0x80,0xff)[(n//20)%4];banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        stack=0x62001000+n%16;resource=0x61000100;pool=0x61001000;records=0x61002000;bitmap=0x61003000
        queue=0x61004000;queue_descriptor=0x61005000;queue_records=0x61006000;cell=0x61007000
        handle=0x12340000;enabled=0 if mode==0 else 1;current=100;previous=99;threshold=50;deadline=200
        count=2;capacity=2;first_free=0;bits=3;owner0=0xdeadbeef;owner1=0xcafebabe
        gp=[rng.getrandbits(32)for _ in range(8)]
        if mode in (2,4,13,15):previous=0
        if mode==3:previous=101
        if mode==4:
            variant=(n//20)%4;handle=(0x1234ffff,0x80008000,0x00010001,0x7fffffff)[variant]
            queue_records=(0x61006014-20-(handle&0xffff)*24)&0xffffffff
            current=(0xffffffff,0,0x80000000,0x7fffffff)[variant]
            previous=(0x7fffffff,0xffffffff,0,0x80000001)[variant]
            threshold=(0,0xffffffff,0x80000000,0x7fffffff)[variant]
        if mode in (5,6,7,8,9,10,11,12,14,16,17,18,19):handle=0xffffffff
        if mode==5:owner1=resource
        if mode==7:deadline=50
        if mode==8:deadline=100
        if mode==9:bits=1;first_free=1
        if mode==10:capacity=3;first_free=2
        if mode==11:count=0;capacity=1;first_free=0;bits=0;records=stack-8
        if mode==12:stack=0x51df25;gp[3]=n&1
        if mode==13:stack=0x474d88;gp[6]=1
        if mode==14:resource=stack-16;gp[3]=0xffffffff
        if mode==15:queue_records=(stack-12-20)&0xffffffff;gp[7]=0
        if mode==16:stack=0x56e1f4;gp[6]=50
        if mode==17:cell=stack-4;gp[3]=0xffffffff if n&1 else 100;deadline=50
        if mode==18:deadline=0xffffffff;cell=0x41b774
        if mode==19:deadline=0;cell=stack-8;gp[6]=0xffffffff
        def put(at,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
            raise ValueError('Resource fixture escaped bank '+hex(at))
        def get(at):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-4:return struct.unpack_from('<I',bank,at-base)[0]
            raise ValueError('Resource fixture lookup escaped bank '+hex(at))
        put(0x51df21,enabled,1);put(resource+12,handle);put(0x4f041c,pool);put(0x4f0424,queue)
        for offset,value in ((0x20,capacity),(0x24,12),(0x34,first_free),(0x38,count),(0x3c,count),(0x40,0x1234),(0x44,records),(0x48,bitmap)):put(pool+offset,value)
        put(bitmap,bits)
        if mode!=11:
            put(records,0x1234,2);put(records+8,owner0);put(records+12,0x1235,2);put(records+20,owner1)
        put(0x474d80,threshold);put(0x474d8c,threshold);put(queue+0x64,queue_descriptor);put(queue_descriptor+0x44,queue_records);put(queue+0x38,current)
        put((queue_records+(handle&0xffff)*24+20)&0xffffffff if handle!=0xffffffff else queue_records+20,previous)
        put(cell,100);put(0x41b774,cell);put(0x56e1ec,deadline)
        put(stack,0x63000000);put(stack+4,0x10203040);put(stack+8,0x50607080)
        gp[0]=resource;gp[1]=(gp[1]&0xffffff00)|priority;gp[4]=stack
        values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')*2
        flags=0x202|(rng.getrandbits(12)&0xcd5)
        # With DF set, this deliberately overlapping constructor also destroys
        # its own return word. Keep this normal-transfer witness forward;
        # ordinary backed allocations below still cover both DF directions.
        if mode==11:flags&=~0x400
        rows.append(dict(id=n,name=f'mode{mode}-priority{priority:x}-{n}',entry=0x126200,gp=gp,eflags=flags,
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64),0x1f80|((n%4)<<13)|(0x8040 if n%8>=4 else 0)],
            live_x87=list(values[:(0,1,4,8)[n%4]]),matrix=get(stack+4),source=get(stack+8),max_steps=5000,stop_before_entries=[0x1264e0],
            banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    for index,target in ((5,0x126190),(6,0x16e6a0),(7,0x33b590)):
        for source in rows[index::20][:4]:
            row=copy.deepcopy(source);row.update(id=len(rows),name=source['name']+f'-stop-{target:x}',stop_before_entries=[0x1264e0,target]);rows.append(row)
    return rows


def sources(scratch,required):
    snapshots={p:(ROOT/p).read_text()for p in SOURCES};rendered=[];rejected=0
    primary=snapshots[SOURCES[0]];children=[]
    from native_resource_record_search_replacements import canonical_body as search_body,PINS as SEARCH_PINS
    from native_effect_datum_allocate_replacements import canonical_body as datum_body,PINS as DATUM_PINS
    from native_millisecond_clock_replacements import canonical_body as clock_body,PINS as CLOCK_PINS
    for entry,strip,pins in (('126190',search_body,SEARCH_PINS),('16e6a0',datum_body,DATUM_PINS),('33b590',clock_body,CLOCK_PINS)):
        original=strip(leaf_body(primary,entry)[0])
        if hashlib.sha256(original.encode()).hexdigest()not in pins:raise ValueError('Complete actual child source changed '+entry)
        children.append(original)
    rendered.append('namespace retained {\n'+'\n'.join(children)+'\n}')
    for name,path in zip(('primary','actor','frame'),SOURCES,strict=True):
        source=snapshots[path];native=binding.replace_resource_touch(source)
        if required and source!=native:raise ValueError('Production126200 binding missing '+path)
        if binding.replace_resource_touch(native)!=native:raise ValueError('Resource-touch binding is not idempotent')
        variants=[source,native]
        raw_parent=binding.canonical_body(leaf_body(source,'126200')[0])
        if 'selected('in raw_parent:
            literal=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',raw_parent)
            literal_source=source.replace(leaf_body(source,'126200')[0],literal,1)
            literal_native=binding.replace_resource_touch(literal_source)
            if binding.replace_resource_touch(literal_native)!=literal_native:raise ValueError('Literal identity binding is not idempotent')
            variants.extend((literal_source,literal_native))
        for victim in variants:
            target=leaf_body(victim,'126200')[0]
            for old,new in (('0x7530u','0x7531u'),('0x474d74u','0x474d78u')):
                changed=victim.replace(target,target.replace(old,new,1),1)
                if changed==victim:raise ValueError('Missing whole-body drift witness')
                try:binding.replace_resource_touch(changed)
                except ValueError:rejected+=1
                else:raise ValueError('Resource-touch accepted changed complete body')
        resolver=next((line for line in source.splitlines()if line.startswith('std::uint32_t selected(')),'')
        dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        for old,new in (('run_126200(s);','run_126190(s);'),('s.required(site,target,tail);','s.required(site+1u,target,tail);')):
            changed=source.replace(dispatcher,dispatcher.replace(old,new,1),1)
            if changed==source:raise ValueError('Missing resource dispatcher drift witness')
            try:binding.replace_resource_touch(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Resource-touch accepted changed child dispatcher')
        if 'selected('in leaf_body(source,'126200')[0]:
            for old,new in (('switch(p)','p^=1u;switch(p)'),('case 0x56e1ecu:return 0x56e1ecu;','case 0x56e1ecu:return 0x56e1f0u;')):
                changed=source.replace(resolver,resolver.replace(old,new,1),1)
                if changed==source:raise ValueError('Missing resource operand resolver witness')
                try:binding.replace_resource_touch(changed)
                except ValueError:rejected+=1
                else:raise ValueError('Resource-touch accepted changed complete resolver')
        parent=leaf_body(native,'126200')[0]
        parts=['namespace '+name+' {',resolver]
        for entry in ('126190','16e6a0','33b590'):
            parts.append(f'static void run_{entry}(WorldState&s){{before_owner(s,0x{entry}u);retained::run_{entry}(s);}}')
        parts.append('static void run_1264e0(WorldState&s){before_owner(s,0x1264e0u);throw std::runtime_error("Unqualified maintenance owner");}')
        parts.append('static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){switch(target){case 0x126190u:run_126190(s);break;case 0x16e6a0u:run_16e6a0(s);break;case 0x33b590u:run_33b590(s);break;case 0x1264e0u:run_1264e0(s);break;default:throw std::runtime_error("Unknown resource owner");}}')
        parts.extend((parent,'}'));rendered.append('\n'.join(parts))
    text='\n'.join(rendered);(scratch/'resource_touch_bodies.inc').write_text(text)
    return rejected,{path:hashlib.sha256(value.encode()).hexdigest()for path,value in snapshots.items()},hashlib.sha256(text.encode()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--compiler',default='c++');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file():return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=args.require_binding,scratch_directory=str(scratch),retained_large_artifacts=[])
    try:
        rejected,pins,rendered_pin=sources(scratch,args.require_binding);binary=scratch/'resource-touch'
        command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_resource_touch_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        result=subprocess.run(command,capture_output=True,text=True,timeout=120)
        if result.returncode:raise ValueError('Resource-touch compile failed '+result.stderr[-4000:])
        from oracle_smg_firing_1_1 import Xbe
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        old_metadata=common.metadata
        def metadata(image):
            result=old_metadata(image);code=common._Code(image,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in ('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                # Capstone reports no access for these genuine TEST memory
                # operands. Authenticate complete opcodes before restoring the
                # actual bitmap reads to the independent observer metadata.
                if site['instruction']in(0x1261c1,0x16e6c5):
                    expected={0x1261c1:'852c8e',0x16e6c5:'852c8b'}[site['instruction']]
                    if site['bytes']!=expected or len(site['operands'])!=1:raise ValueError('Original bitmap TEST metadata changed')
                    site['operands'][0].update(read=True,write=False)
            return result
        common.metadata=metadata;rows=corpus()
        trace=[dict(entry=target,arguments=0,gp=True,xmm=True)for target in (0x126190,0x16e6a0,0x33b590)]
        physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=ENTRIES,regions=REGIONS,trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)
        prefixes=[]
        for row,case in zip(rows[:20],physical['cases'][:20],strict=True):
            for ordinal,event in enumerate(case['reads']):
                if 0x126200<=event['instruction']<=0x1262ad:
                    value=copy.deepcopy(row);value.update(id=len(rows)+len(prefixes),name=row['name']+'-before-read-'+str(ordinal),fault=dict(phase='before_read',ordinal=ordinal));prefixes.append(value)
        directory=scratch/'prefixes';directory.mkdir();observed_prefixes,prefix_proof=physical_original(Xbe(args.xbe),prefixes,directory,entries=ENTRIES,regions=REGIONS,trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)
        cases=[]
        for row,original in zip(rows+prefixes,physical['cases']+observed_prefixes['cases'],strict=True):
            value=copy.deepcopy(row);value.update({key:original[key]for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes','calls')})
            value['stop_kind']='before_read'if 'fault'in row else'return'if original['stopped_at']==0x63000000 else'child'
            value['expected_pc']=original['boundary']['instruction']if 'fault'in row else original['stopped_at']if value['stop_kind']=='child'else max(pc for pc in original['executed_sites']if pc in (0x126276,0x1262ad))
            if not original['live_x87_unchanged']:raise ValueError('Original resource path changed live x87')
            for bank,after in zip(value['banks'],original['banks'],strict=True):bank['after']=after['after']
            for event in value['reads']+value['writes']:event.pop('instruction')
            cases.append(value)
        fixture=scratch/'resource-touch.json';fixture.write_text(json.dumps(dict(cases=cases),separators=(',',':')))
        results=[]
        for forced in (False,True):
            env=dict(os.environ);env.pop('HALO2_REFERENCE_RESOURCE_TOUCH',None)
            if forced:env['HALO2_REFERENCE_RESOURCE_TOUCH']='1'
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=env,timeout=90)
            if result.returncode:raise ValueError('Resource-touch comparison failed '+result.stderr[-6500:])
            results.append(json.loads(result.stdout))
        sites={int(pc,16)for pc in re.findall(r's\.services\.instruction\((0x[0-9a-f]+)u\)',binding.canonical_body(leaf_body((ROOT/SOURCES[0]).read_text(),'126200')[0]))}
        executed={pc for c in physical['cases']for pc in c['executed_sites']if pc in sites}
        if sites-executed:raise ValueError('Original parent sites unexecuted '+str(sorted(sites-executed)))
        files=('include/halo2/native_tu15_resource_touch_kernel.hpp','src/physics/runtime/native_tu15_resource_touch.hpp','tools/native_resource_touch_replacements.py','tools/validate_native_tu15_resource_touch.py','tests/native_tu15_resource_touch_tests.cpp')
        report.update(passed=True,native=results[0],forced_reference=results[1],physical_original=proof,physical_read_prefixes=prefix_proof,
            full_return_cases=sum(c['stop_kind']=='return'for c in cases),genuine_child_entry_cases=sum(c['stop_kind']=='child'for c in cases),before_parent_read_cases=len(prefixes),
            authentication_mutations_rejected=rejected,all_parent_sites=len(sites),executed_parent_sites=len(executed),source_sha256={**pins,**{name:sha(ROOT/name)for name in files}},rendered_source_sha256=rendered_pin,compiler_command=command,
            compared_bank_bytes=3*sum(sum(len(bank['before'])//2 for bank in row['banks'])for row in cases),fixture_sha256=sha(fixture),
            retained_before_parent_read_abi_qualified=True,ordered_read_write_addresses_widths_verified=True,child_read_cursor_pcs_qualified=False,store_exception_state_qualified=False,observer_exceptions_qualified=False,
            supplied_child_outcomes=False,composed_child_scope='Complete pinned canonical126190/16E6A0/33B590 algorithms with real backed banks;1264E0 stops at genuine entry. Production child-route integration is separately qualified.',
            clock_scope='Static backed cells and observation-only595 protocol; no clock provider or timestamp outcome supplied.',whole1264e0_closed=False,physical_page_faults_qualified=False,
            original_bitmap_read_metadata_repairs={'1261c1':'852c8e','16e6c5':'852c8b'})
        print(json.dumps(results[0]))
    except BaseException as error:report['error']=str(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
