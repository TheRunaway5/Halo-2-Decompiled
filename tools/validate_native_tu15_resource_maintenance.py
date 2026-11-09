#!/usr/bin/env python3
"""Unchanged-i386 maintenance controller proof; open child families stay explicit."""
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
import native_resource_maintenance_replacements as binding
from native_selected_operand_binding import require_selected_identity
from specialize_native_operands import leaf_body
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
SOURCE='src/physics/physics_live_support_tu15_generated.cpp'
AI='src/campaign/ai/source_ai_frame_tu15_generated.cpp'
REGIONS=((0x4ef000,4096),(0x4f0000,8192),(0x51d000,8192),(0x61000000,65536),(0x62000000,8192))
ENTRIES=(0x1264e0,0x126480,0x16ea60,0x16e8a0)
PAGE_ENTRIES=(0x126060,0x13fe50,0x13fe00,0x140310,0x32a660,0x16e6a0,0x16e770,0x16e860)
PROOF_FILES=('include/halo2/native_tu15_resource_maintenance_kernel.hpp','src/physics/runtime/native_tu15_resource_maintenance.hpp','tools/native_resource_maintenance_replacements.py','tools/validate_native_tu15_resource_maintenance.py','tests/native_tu15_resource_maintenance_tests.cpp')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def corpus():
    rng=random.Random(0x1264e0);rows=[]
    for n in range(96):
        mode=n%12;banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];stack=0x62001000+n%16
        pool=0x61000100;records=0x61001000;bitmap=0x61002000;inactive_pool=0x61003000;inactive_rows=0x61004000;inactive_bitmap=0x61005000;resources=0x61006000
        gp=[rng.getrandbits(32)for _ in range(8)];request_modes=[1]if mode in(3,6,7,9,11)else[2]if mode in(4,10)else[1,2,0]
        if mode==2:request_modes=[]
        count={5:2,6:8,7:7}.get(mode,0)
        if mode==7:request_modes=[1,1,2]
        if mode==11:records=stack-24;request_modes=[2];count=1;gp[3]=0x1234
        def put(at,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
            raise ValueError('Maintenance fixture escaped bank '+hex(at))
        def get(at):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-4:return struct.unpack_from('<I',bank,at-base)[0]
            raise ValueError('Maintenance fixture lookup escaped bank')
        put(0x4f041c,pool);put(0x4f0418,inactive_pool);put(0x4ef1c0,26 if mode==1 else 25);put(0x51df21,0 if mode==8 else 1,1)
        for p,stride,extent,storage,bits in ((pool,12,len(request_modes),records,bitmap),(inactive_pool,16,count,inactive_rows,inactive_bitmap)):
            for offset,value in ((0x20,16),(0x24,stride),(0x34,extent),(0x38,extent),(0x3c,extent),(0x44,storage),(0x48,bits)):put(p+offset,value)
            put(bits,(1<<extent)-1);put(p+0x29,0 if mode==0 and p==pool else 1,1);put(p+0x2a,0,1)
        for index in range(count):put(inactive_rows+index*16,0x1234,2);put(inactive_rows+index*16+4,0,1)
        for index,priority in enumerate(request_modes):
            put(records+index*12,0x1234+index,2);put(records+index*12+4,priority);put(records+index*12+8,resources+index*20)
            put(resources+index*20+12,0xffffffff if mode in(9,10)else 0x12340000+index)
        put(stack,0x63000000);put(stack+4,0x10203040);put(stack+8,0x50607080);gp[4]=stack
        values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')*2
        rows.append(dict(id=n,name=f'mode{mode}-{n}',entry=0x1264e0,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
            before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64),0x1f80|((n%4)<<13)|(0x8040 if n%8>=4 else 0)],
            live_x87=list(values[:(0,1,4,8)[n%4]]),matrix=get(stack+4),source=get(stack+8),max_steps=10000,stop_before_entries=[0x126060,0x1260c0],
            banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    return rows


def page_inputs(image,rows):
    from validate_native_physical_allocation_abi_tu15 import physical_inputs
    banks,provenance=physical_inputs(image);owner=int.from_bytes(banks[0x4f2d34],'little')
    def captured_word(at):return next(int.from_bytes(raw[at-base:at-base+4],'little')for base,raw in banks.items()if base<=at<=base+len(raw)-4)
    if owner!=0x1017d000 or captured_word(owner+0x30)!=7889 or captured_word(owner+0x3c)!=0xffffffff or captured_word(owner+0x40)!=0xffffffff:
        raise ValueError('Genuine physical owner initial span changed')
    if captured_word(owner+0x44)!=0x7fffffff or captured_word(owner+0x48)!=0x7fffffff:raise ValueError('Genuine physical owner policy budget changed')
    added=[]
    for source in rows:
        if not source['name'].startswith(('mode9-','mode10-')):continue
        for size in(4096,0x10000000):
            row=copy.deepcopy(source);row.update(id=len(rows)+len(added),name=source['name']+f'-genuine-page-{size:x}',stop_before_entries=[0x1260c0],max_steps=50000)
            # These complete normal page fixtures enter with DF clear. A separately
            # retained original-fault diagnostic covers the reverse-clear
            # input that faults at140277.
            row['eflags']&=~0x400
            combined={b['address']:bytes.fromhex(b['before'])for b in row['banks']};combined.update(banks);combined[0x474000]=image.read(0x474000,4096)
            # This real page owner needs its authored4KiB private frame.
            old_stack=row['gp'][4];new_stack=0x62004000+(old_stack&15);row['gp'][4]=new_stack
            combined[0x62000000]=bytes(32768)
            def put(at,value,width=4):
                candidates=[base for base,raw in combined.items()if base<=at<=base+len(raw)-width]
                if len(candidates)!=1:raise ValueError('Ambiguous page composition backing '+hex(at))
                base=candidates[0];raw=bytearray(combined[base]);raw[at-base:at-base+width]=value.to_bytes(width,'little');combined[base]=bytes(raw)
            put(new_stack,0x63000000);put(new_stack+4,row['matrix']);put(new_stack+8,row['source'])
            put(0x4f0424,owner);put(0x61006004,size)
            # This declared empty inactive-datum array is now used by the
            # genuine specific allocator, not only its empty-count query.
            # Unoccupied datum salt words must therefore actually be zero.
            for index in range(16):put(0x61004000+index*16,0,2)
            put(0x61003040,0x8000,2)
            row['banks']=[dict(address=base,before=raw.hex())for base,raw in sorted(combined.items())]
            added.append(row)
    provenance.update(normal_success_request_bytes=4096,oversized_span_request_bytes=0x10000000,
        failure_scope='Oversized physical span search against7889 total pages; original7FFFFFFF policy budgets remain unchanged.',
        page_call_direction_flag_scope='DF0 normal page calls; parent resident/entry fixtures separately retain both DF values. DF1 original-fault input is outside normal page qualification.',
        resource_inputs='Declared queued request, inactive-datum pool, resource byte extent and real-owner binding; allocator computes every result.')
    return rows+added,provenance


def page_bodies(image,scratch,primary):
    from validate_native_physical_allocation_abi_tu15 import recovered_body
    from native_resource_page_replacements import canonical_body as resource_page_body,PIN as RESOURCE_PAGE_PIN
    from native_datum_specific_replacements import canonical_body as specific_body,BODY_PINS as SPECIFIC_PINS
    from native_datum_initialize_replacements import canonical_body as initialize_body,BODY_PINS as INITIALIZE_PINS
    from native_page_allocate_replacements import canonical_body as page_allocate_body
    from native_page_window_replacements import canonical_body as window_body,BODY_PIN as WINDOW_PIN
    from native_page_retire_replacements import canonical_body as retire_body,BODY_PINS as RETIRE_PINS
    from native_effect_datum_allocate_replacements import canonical_body as allocate_body,PINS as ALLOCATE_PINS
    from native_stack_probe_replacements import canonical_body as stack_body,LITERAL_PIN as STACK_PIN,SELECTED_PIN as STACK_SELECTED_PIN
    directory=scratch/'page-reference';directory.mkdir();recovered_body(image,directory)
    source=(directory/'physical-reference.cpp').read_text();children={}
    for entry in PAGE_ENTRIES:
        name=f'{entry:x}';body=leaf_body(primary if entry==0x126060 else source,name)[0]
        if entry in(0x126060,0x13fe50,0x13fe00,0x140310,0x16e6a0,0x32a660,0x16e770,0x16e860):
            strip,pins={0x13fe50:(page_allocate_body,('ce273066496b0ee1694f6ead6056091fa8c5bc1090bc56767f40c52552b57963',)),0x16e770:(specific_body,SPECIFIC_PINS),0x16e860:(initialize_body,INITIALIZE_PINS),0x126060:(resource_page_body,(RESOURCE_PAGE_PIN,)),0x13fe00:(window_body,(WINDOW_PIN,)),0x140310:(retire_body,RETIRE_PINS),0x16e6a0:(allocate_body,ALLOCATE_PINS),0x32a660:(stack_body,(STACK_PIN,STACK_SELECTED_PIN))}[entry]
            body=strip(body)
            if hashlib.sha256(body.encode()).hexdigest()not in pins:raise ValueError('Complete page child canonical body drift '+name)
        children[name]=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)
    return children,{'original_xbe_sha256':PIN,'generator_receipt_sha256':sha(directory/'physical-reference.json'),
        'child_body_sha256':{name:hashlib.sha256(body.encode()).hexdigest()for name,body in children.items()}}


def sources(scratch,required,image,page_children):
    snapshots={p:(ROOT/p).read_text()for p in(SOURCE,AI)};source=snapshots[SOURCE];native=binding.replace_resource_maintenance(source)
    if required and native!=source:raise ValueError('Maintenance production binding missing')
    if binding.replace_resource_maintenance(native)!=native:raise ValueError('Maintenance binder is not idempotent')
    rejected=0
    for victim in(source,native):
        target=leaf_body(victim,'1264e0')[0]
        for old,new in(('0x19u','0x18u'),('0x7000180u','0x7000184u'),('run_126060(s);','run_1260c0(s);')):
            changed=victim.replace(target,target.replace(old,new,1),1)
            if changed==victim:raise ValueError('Missing maintenance whole-body mutation witness')
            try:binding.replace_resource_maintenance(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Maintenance accepted complete-body drift')
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
    for old,new in(('run_1264e0(s);','run_126060(s);'),('s.required(site,target,tail);','s.required(site+1u,target,tail);')):
        changed=source.replace(dispatcher,dispatcher.replace(old,new,1),1)
        if changed==source:raise ValueError('Missing maintenance dispatcher mutation witness')
        try:binding.replace_resource_maintenance(changed)
        except ValueError:rejected+=1
        else:raise ValueError('Maintenance accepted changed complete dispatcher')
    changed=native.replace(binding.PREFIX,binding.PREFIX.replace('dispatch(v,target,site,tail);','dispatch(v,target,site+1u,tail);'),1)
    try:binding.replace_resource_maintenance(changed)
    except ValueError:rejected+=1
    else:raise ValueError('Maintenance accepted altered native prefix')
    from native_resource_record_count_replacements import canonical_body as counter_body,PIN as COUNTER_PIN
    from native_effect_slot_scan_replacements import canonical_body as next_body,PINS as NEXT_PINS,RESOLVER_PINS as NEXT_MAP
    from native_datum_delete_replacements import canonical_body as delete_body,BODY_PINS as DELETE_PINS,RESOLVER_PINS as DELETE_MAP
    children={}
    for text,entry,strip,pins,maps in ((source,'126480',counter_body,(COUNTER_PIN,),()),(snapshots[AI],'16ea60',next_body,NEXT_PINS,NEXT_MAP),(snapshots[AI],'16e8a0',delete_body,DELETE_PINS,DELETE_MAP)):
        original=strip(leaf_body(text,entry)[0])
        if hashlib.sha256(original.encode()).hexdigest()not in pins:raise ValueError('Complete actual child drift '+entry)
        require_selected_identity(text,original,maps);children[entry]=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',original)
    page_proof=None
    if page_children:
        extra,page_proof=page_bodies(image,scratch,source);children.update(extra)
    trace_targets={'126480','16ea60','16e8a0','126060'}
    parts=['namespace retained {',''.join(f'static void run_{entry}(WorldState&);'for entry in children),
        'static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){switch(target){'+''.join(f'case 0x{entry}u:run_{entry}(s);break;'for entry in children)+'default:throw std::runtime_error("Unknown real page child");}}']
    for entry,body in children.items():
        if entry in trace_targets:
            start=body.index('{')+1;body=body[:start]+f'\n    before_owner(s,0x{entry}u);'+body[start:]
        parts.append(body)
    parts.extend(('}','namespace primary {'))
    for entry in children:parts.append(f'static void run_{entry}(WorldState&s){{retained::run_{entry}(s);}}')
    for entry in(('1260c0',)if page_children else('126060','1260c0')):parts.append(f'static void run_{entry}(WorldState&s){{before_owner(s,0x{entry}u);throw std::runtime_error("Unqualified resource child return");}}')
    parts.append('static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){switch(target){'+''.join(f'case 0x{entry}u:run_{entry}(s);break;'for entry in('126480','16ea60','16e8a0','126060','1260c0'))+'default:throw std::runtime_error("Unknown maintenance owner");}}')
    parts.extend((leaf_body(native,'1264e0')[0],'}'));text='\n'.join(parts);(scratch/'resource_maintenance_bodies.inc').write_text(text)
    return {name:hashlib.sha256(value.encode()).hexdigest()for name,value in snapshots.items()},page_proof,rejected


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--require-complete',action='store_true');p.add_argument('--page-children',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file():return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=args.require_binding,scratch_directory=str(scratch),retained_large_artifacts=[])
    proof_pins={name:sha(ROOT/name)for name in PROOF_FILES}
    try:
        from oracle_smg_firing_1_1 import Xbe
        image=Xbe(args.xbe);pins,page_proof,rejected=sources(scratch,args.require_binding,image,args.page_children);binary=scratch/'maintenance'
        command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_resource_maintenance_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise ValueError('Maintenance compile failed '+result.stderr[-5000:])
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        old_metadata=common.metadata
        def metadata(image):
            result=old_metadata(image);code=common._Code(image,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                expected={0x1264b1:'852c8e',0x16ea85:'851c8e',0x1401a2:'851c88',
                    0x32a672:'8500',0x32a689:'8501',0x32a696:'8501',0x16e6c5:'852c8b'}.get(site['instruction'])
                if expected:
                    if site['bytes']!=expected or len(site['operands'])!=1:raise ValueError('Original bitmap TEST metadata drift')
                    site['operands'][0].update(read=True,write=False)
                if site['instruction']==0x140071:
                    if site['bytes']!='0f184c0464' or len(site['operands'])!=1:raise ValueError('Original advisory prefetch metadata drift')
                    site['operands'][0].update(read=False,write=False)
            return result
        common.metadata=metadata;rows=corpus();page_provenance=None
        if args.page_children:rows,page_provenance=page_inputs(image,rows)
        trace=[dict(entry=target,arguments=0,gp=True,xmm=True)for target in(0x126480,0x16ea60,0x16e8a0,0x126060)]
        entries=(*ENTRIES,*PAGE_ENTRIES)if args.page_children else ENTRIES
        regions=tuple(sorted({(bank['address'],len(bank['before'])//2)for row in rows for bank in row['banks']}))
        # A larger stack region replaces the prototype's smaller region.
        regions=tuple((base,max(size for at,size in regions if at==base))for base in sorted({base for base,_ in regions}))
        physical,proof=physical_original(image,rows,scratch,entries=entries,regions=regions,trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)
        prefixes=[]
        for row,case in zip(rows[:12],physical['cases'][:12],strict=True):
            for ordinal,event in enumerate(case['reads']):
                if 0x1264e0<=event['instruction']<=0x126607:
                    value=copy.deepcopy(row);value.update(id=len(rows)+len(prefixes),name=row['name']+'-before-read-'+str(ordinal),fault=dict(phase='before_read',ordinal=ordinal),native_only=True);prefixes.append(value)
        directory=scratch/'prefixes';directory.mkdir();pref,preproof=physical_original(image,prefixes,directory,entries=entries,regions=regions,trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)
        cases=[]
        for row,original in zip(rows+prefixes,physical['cases']+pref['cases'],strict=True):
            value=copy.deepcopy(row);value.update({key:original[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes','calls')})
            value['stop_kind']='before_read'if 'fault'in row else'return'if original['stopped_at']==0x63000000 else'child'
            value['read_instructions']=[event['instruction']for event in original['reads']]
            value['expected_pc']=original['boundary']['instruction']if 'fault'in row else 0x126607 if value['stop_kind']=='return'else original['stopped_at']
            if not original['live_x87_unchanged']:raise ValueError('Original maintenance changed live x87')
            for bank,after in zip(value['banks'],original['banks'],strict=True):bank['after']=after['after']
            for event in value['reads']+value['writes']:event.pop('instruction')
            cases.append(value)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases),separators=(',',':')));results=[]
        for forced in(False,True):
            env=dict(os.environ);env.pop('HALO2_REFERENCE_RESOURCE_MAINTENANCE',None)
            if forced:env['HALO2_REFERENCE_RESOURCE_MAINTENANCE']='1'
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=env)
            if result.returncode:raise ValueError('Maintenance comparison failed '+result.stderr[-7000:])
            results.append(json.loads(result.stdout))
        sites={int(pc,16)for pc in re.findall(r's\.services\.instruction\((0x[0-9a-f]+)u\)',binding.canonical_body(leaf_body((ROOT/SOURCE).read_text(),'1264e0')[0]))}
        executed={pc for case in physical['cases']for pc in case['executed_sites']if pc in sites};remaining=sorted(sites-executed)
        if args.require_complete and remaining:raise ValueError('Maintenance parent still has uncovered sites '+str(remaining))
        drift=[name for name,pin in proof_pins.items()if sha(ROOT/name)!=pin]
        if drift:raise ValueError('Maintenance proof source changed during run '+str(drift))
        report.update(passed=True,native=results[0],forced_reference=results[1],physical_original=proof,physical_read_prefixes=preproof,all_parent_sites=len(sites),executed_parent_sites=len(executed),unexecuted_parent_sites=remaining,authentication_mutations_rejected=rejected,
            source_sha256={**pins,**proof_pins},proof_source_drift=drift,compiler_command=command,supplied_child_outcomes=False,parent_site_coverage_complete=not remaining,whole_controller_qualified=False,
            reference_prefix_scope='Retained emulated IMUL undefined flags are not used as a prefix oracle; native prefix state compares full flags to unchanged physical instructions without masking.',
            physical_page_faults_qualified=False,real_page_and_job_children_closed=False,page_reference=page_proof,page_input_provenance=page_provenance,
            advisory_hint_metadata={'140071':'0f184c0464; PREFETCHT0 is retained in original execution/site coverage, without a scalar data-read event.'})
        print(json.dumps(results[0]))
    except BaseException as error:report['error']=str(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
