#!/usr/bin/env python3
"""Qualify12FCB0/4057F0 against unchanged .text and D3D CPU instructions."""
import argparse
import copy
import hashlib
import inspect
import json
import os
from pathlib import Path
import random
import re
import subprocess
import sys
from types import SimpleNamespace

ROOT=Path(__file__).resolve().parents[1]
for seed in sorted(ROOT.glob('.local/*/python')):
    if(seed/'capstone').is_dir():sys.path.insert(0,str(seed));break
import validate_native_tu15_prt_helpers as physical
from validate_native_array_growth_tu15 import Xbe,NEW_DEFAULT,NEW_PIN,_Code,complete_body
from native_selected_operand_binding import require_selected_identity
from native_resource_busy_replacements import canonical_body,install_native_resource_busy,prefix,installed_entries

IMAGES={0x12fcb0:(83,'1df96e347163f4151ccb82bf996d1aea2abae0e06e9c425b1525e2b450b4861a'),
    0x4057f0:(130,'26b21d4d8c3d411324b2a5c9f96dff30ef8103fe860c412947cdd57bf6cd6e8b')}
SOURCE='src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
BODY_PINS={0x12fcb0:'7be8374150d17cb592e03074ccdcff4797511a24359bde3f0923ec49c28bf535',
    0x4057f0:'ed41100ceb15f8ada64661a57e88be9e6db9c948b3e47d63d0c54edcabe32e4c'}
RESOLVER_PIN='cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61'
REGIONS=((0x411000,4096),(0x4f2000,4096),(0x61000000,8192),(0x62000000,8192))
COPY_ROUTES={'death':('src/gameplay/units/death_cycle_runtime_tu15_generated.cpp',('12fcb0','4057f0')),
    'combat':('src/gameplay/units/source_combat_action_tu15_generated.cpp',('4057f0',))}
IO=(0x12fcb0,0x12fcb4,0x12fcba,0x12fcc5,0x12fcd6,0x12fce0,0x12fceb,0x12fcf5,0x12fce8,0x12fd00,
    0x4057f0,0x405800,0x405801,0x405809,0x40581b,0x405825,0x40583a,0x405841,0x405849,0x405850,
    0x405853,0x405854,0x405856,0x405861,0x405869,0x40586a,0x40586d,0x405870,0x405871)


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class OriginalCodeView:
    """Original sections in one executable view; no instruction is changed."""
    def __init__(self,image):
        self.image=image;first=image.section('.text').va;d3d=image.section('D3D');last=d3d.va+len(image.section_bytes(d3d));flat=bytearray(last-first)
        self.sections=[]
        for section in image.sections:
            raw=image.section_bytes(section)
            if first<=section.va and section.va+len(raw)<=last:
                flat[section.va-first:section.va-first+len(raw)]=raw
                self.sections.append(dict(name=section.name,address=section.va,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest()))
        self.raw=bytes(flat);self.view=SimpleNamespace(name='.text',va=first,vsize=len(flat),rsize=len(flat),raw=0,flags=22)
    def section(self,name):return self.view if name=='.text'else self.image.section(name)
    def section_bytes(self,section):return self.raw if section is self.view else self.image.section_bytes(section)
    def read(self,address,size):return self.image.read(address,size)


def physical_owner(view,rows,scratch):
    # PRT's unchanged instruction/FP observer is reused. A local, authenticated
    # assembly-layout extension avoids emitting a second ELF section over the
    # SDK global411568 already contained in the original D3D mapping. Data
    # setup remains in the observer's declared banks; executed code is intact.
    original=inspect.getsource(physical.physical_original)
    old="for index, (_, size) in enumerate(regions):\n        assembly += f'.section .bank{index},\"aw\"\\n.space {size}\\n'"
    new="for index, (base, size) in enumerate(regions):\n        if not(section.va<=base and base+size<=section.va+section.vsize):\n            assembly += f'.section .bank{index},\"aw\"\\n.space {size}\\n'"
    assert original.count(old)==1;modified=original.replace(old,new)
    old="for index, (base, _) in enumerate(regions))"
    new="for index, (base, bank_size) in enumerate(regions)if not(section.va<=base and base+bank_size<=section.va+section.vsize))"
    assert modified.count(old)==1;modified=modified.replace(old,new)
    scope=dict(physical.__dict__);exec(modified,scope)
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    original_metadata=common.metadata
    def metadata(image):
        result=original_metadata(image);code=_Code(image,'.text')
        for site in result['sites']:
            ins=code.at(site['instruction'])
            if ins.mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            if ins.mnemonic=='test':
                for operand in site['operands']:operand.update(read=True,write=False)
        return result
    common.metadata=metadata
    try:
        proof,receipt=scope['physical_original'](view,rows,scratch,entries=tuple(IMAGES),regions=REGIONS,
            trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in IO],trace_extended=True,preserve_x87_prefix=True)
    finally:common.metadata=original_metadata
    receipt.update(original_mapping=view.sections,mapping_view_sha256=hashlib.sha256(view.raw).hexdigest(),
        assembly_layout_extension_sha256=hashlib.sha256(modified.encode()).hexdigest(),original_helper_sha256=hashlib.sha256(original.encode()).hexdigest(),
        sdk_global_bank_inside_original_mapping=True,all_executed_original_owner_bytes_unchanged=True)
    return proof,receipt


def reference_bodies(scratch):
    source=(ROOT/SOURCE).read_text();rows=[]
    for entry,pin in BODY_PINS.items():
        body=canonical_body(re.search(r'static void run_'+f'{entry:x}'+r'\(WorldState &s\) \{.*?\n\}',source,re.S)[0],f'{entry:x}')
        assert hashlib.sha256(body.encode()).hexdigest()==pin;require_selected_identity(source,body,(RESOLVER_PIN,))
        rows.append(re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body))
    text='static void run_4057f0(WorldState &s);\nstatic void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){if(target!=0x4057f0u)throw std::runtime_error("Unexpected busy reference child");run_4057f0(s);}\n'+'\n'.join(rows)
    (scratch/'resource_busy_reference.inc').write_text(text+'\n');return hashlib.sha256(text.encode()).hexdigest()


def copy_routes(scratch,require_binding):
    output=[];qualified={};drift_checks=0
    for route,(path,entries)in COPY_ROUTES.items():
        source=(ROOT/path).read_text();bound=install_native_resource_busy(source)
        if require_binding:assert bound==source,route+' busy prefixes missing'
        assert install_native_resource_busy(bound)==bound and installed_entries(bound)==sorted(entries)
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
        block='namespace '+route+' {\n'+resolver+'\nstatic void run_4057f0(WorldState &s);\nstatic void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){if(target!=0x4057f0u)throw std::runtime_error("Unexpected copy busy child");run_4057f0(s);}\n'
        hashes={}
        for entry in entries:
            body=re.search(r'static void run_'+entry+r'\(WorldState &s\) \{.*?\n\}',bound,re.S)[0];hashes[entry]=hashlib.sha256(body.encode()).hexdigest()
            block+=body.replace(prefix(entry),prefix(entry)+'    ++resource_busy_source_fallbacks;\n',1)+'\n'
            changed=body.replace('4,0);','2,0);',1);assert changed!=body
            try:install_native_resource_busy(bound.replace(body,changed,1))
            except ValueError:drift_checks+=1
            else:raise AssertionError('Changed busy body admitted '+route+entry)
        output.append(block+'}\n')
        try:install_native_resource_busy(bound.replace(resolver,resolver.replace('switch(p){','switch(p){/* drift */',1),1))
        except ValueError:drift_checks+=1
        else:raise AssertionError('Changed busy resolver admitted '+route)
        qualified[route]=dict(source=path,bodies=hashes)
    (scratch/'resource_busy_copy_routes.inc').write_text('\n'.join(output));return qualified,drift_checks


def corpus(image):
    rng=random.Random(0x4057f0);rows=[]
    def add(entry,profile,alignment=0,alias=None):
        ordinal=len(rows);banks=[bytearray(image.read(base,size))if base<0x60000000 else bytearray(rng.randbytes(size))for base,size in REGIONS]
        stack=0x62001000+alignment;resource=0x61000100+alignment;parent=0x61000180+alignment;second=0x61000200+alignment;device=0x61000300+alignment;counter=0x61000400+alignment
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=stack
        def put(address,value,width=4):
            for(base,size),bank in zip(REGIONS,banks):
                if base<=address<=base+size-width:bank[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Busy input escaped declared bank '+hex(address))
        def get(address):return next(int.from_bytes(bank[address-base:address-base+4],'little')for(base,size),bank in zip(REGIONS,banks)if base<=address<=base+size-4)
        root_header,parent_header,second_header,follow,nested,fence,submitted,finished=profile
        if alias=='saved_header':resource=stack-4;gp[7]=0x80000
        if alias=='saved_fence':resource=stack-12
        if alias=='saved_counter':counter=stack-8
        if alias=='saved_device':device=stack-0x34;gp[7]=counter
        put(0x411568,device if fence or alias in('saved_fence','saved_counter','saved_device')else 0)
        put(device+0x30,counter);put(device+0x2c,submitted);put(counter,finished)
        put(resource,root_header);put(resource+8,fence);put(resource+0x14,parent if follow else 0)
        put(parent,parent_header);put(parent+8,fence);put(parent+0x14,second if nested else 0)
        put(second,second_header);put(second+8,fence);put(second+0x14,0)
        gp[1]=resource
        matrix=get(stack+4);source=get(stack+8)
        if entry==0x12fcb0:
            array=0x61000600;records=0x61000800;index=ordinal%3;record=records+index*40;resource=record+0x14
            flags,ready,association,global_policy=alias
            put(0x4f2d24,array);put(0x4f2d49,global_policy,1);put(array+0x44,records)
            put(record,0x8000|(flags<<16)|(ready<<24));put(record+12,association)
            put(resource,root_header);put(resource+8,fence);put(resource+0x14,parent if follow else 0)
            matrix=0x80000000|index
        put(stack,0x63000000);put(stack+4,matrix);put(stack+8,source)
        values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f',
            '00000000000000800040','00000000000000c0ff3f','00000000000000000000','0000000000000080ffbf')
        live=list(values[:(0,1,4,8)[ordinal%4]])
        row=dict(id=ordinal,name=f'{entry:x}_{ordinal}_{alias}',entry=entry,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),matrix=matrix,source=source,max_steps=256,
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10),((ordinal%8)<<11)|(ordinal&0x473f),0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)],
            live_x87=live,banks=[dict(address=base,before=bank.hex())for(base,_),bank in zip(REGIONS,banks)],fault_case=alignment in(0,1)or ordinal%17==0)
        rows.append(row)
    profiles=[(0,0,0,False,False,0,0,0),(0x80000,0,0,False,False,0,0,0),(0x50000,0,0,False,False,0,0,0),
        (0xd0000,0,0,True,False,0,0,0),(0x50000,0x80000,0,True,False,0,0,0),(0x50000,0,0,True,False,0,0,0),
        (0x50000,0x50000,0x80000,True,True,0,0,0),(0x50000,0x50000,0,True,True,0,0,0),
        (0x50000,0x50000,0,True,False,0,0,0)]
    profiles +=[(0,0,0,False,False,fence,submitted,finished)for fence,submitted,finished in((1,2,0),(1,2,1),(1,2,2),(0xffffffff,0,0xffffffff),(0x80000000,0,0x7fffffff),(0x7fffffff,0xffffffff,0x80000000),(3,1,2))]
    for profile in profiles:
        for alignment in(0,1,7,15):add(0x4057f0,profile,alignment)
    for alignment in range(16):add(0x4057f0,(0,0,0,False,False,1,7,4),alignment)
    for alias in('saved_header','saved_fence','saved_counter','saved_device'):
        for alignment in(0,1,7,15):add(0x4057f0,(0,0,0,False,False,0 if alias=='saved_header'else 1,7,4),alignment,alias)
    for policy in((1,0,0xffffffff,1),(2,0,0xffffffff,0),(2,0,0xffffffff,1),(0,0,0xffffffff,1),(0,0,17,1),(0,1,17,1)):
        for profile in(profiles[0],profiles[1],profiles[-1]):
            for alignment in(0,1,7,15):add(0x12fcb0,profile,alignment,policy)
    return rows


def install_observations(rows,observed):
    executed=set();packets=0
    for row,case in zip(rows,observed['cases'],strict=True):
        executed.update(case['executed_sites']);row.update({key:case[key]for key in('completed','boundary_gp','boundary_eflags','boundary_xmm','after_fp','calls','reads','writes')})
        if not case['live_x87_unchanged']:raise ValueError('Physical busy owner changed live FP')
        row['expected_pc']=case['boundary']['instruction']if not case['completed']else case['executed_sites'][-1]
        # executed_sites is a set-like diagnostic, so normal RET ownership is
        # derived from actual source result/path, not its numeric maximum.
        if case['completed']:
            if row['entry']==0x12fcb0:row['expected_pc']=0x12fd00 if case['boundary_gp'][0]&255 else 0x12fce8
            else:row['expected_pc']=0x40586a if case['boundary_gp'][0]else 0x405871
        if row.get('stop_before_entries'):row['completed']=False;row['expected_pc']=case['stopped_at']
        for event in row['calls']:event.pop('arguments');packets+=1
        after={bank['address']:bank['after']for bank in case['banks']}
        for bank in row['banks']:bank['after']=after[bank['address']]
    return executed,packets


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--compiler',default='c++');p.add_argument('--binding-copies',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
        for entry,(length,pin)in IMAGES.items():assert hashlib.sha256(image.read(entry,length)).hexdigest()==pin
        reference_pin=reference_bodies(scratch);copies,drift_checks=copy_routes(scratch,args.require_binding)if(args.binding_copies or args.require_binding)else({},0)
        binding=None
        if args.require_binding:
            from specialize_native_operands import specialize,RESOURCE_BUSY_ACTUAL_OWNER_IMAGES
            assert RESOURCE_BUSY_ACTUAL_OWNER_IMAGES==IMAGES
            primary=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text();specialized,receipt=specialize(primary)
            assert receipt['native_resource_busy_actual_owner_entries']==['12fcb0','4057f0']
            context=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',primary)[0]
            for changed in(context.replace('switch(target)','switch(target+0u)',1),context.replace('s.required(site,target,tail)','s.required(site,target^1u,tail)',1)):
                assert changed!=context
                try:specialize(primary.replace(context,changed))
                except ValueError as error:assert str(error)=='Complete13FE50 dispatcher drift'
                else:raise AssertionError('Changed complete dispatcher escaped bound page-owner authentication')
            binding=dict(actual_entries=receipt['native_resource_busy_actual_owner_entries'],canonical_sha256=hashlib.sha256(primary.encode()).hexdigest(),specialized_sha256=hashlib.sha256(specialized.encode()).hexdigest(),whole_dispatch_drift_rejections=2)
        view=OriginalCodeView(image);rows=corpus(image);normal=scratch/'normal';normal.mkdir();observed,proof=physical_owner(view,rows,normal);executed,packets=install_observations(rows,observed)
        prefix_rows=[]
        for index in(0,1,2,3,20,21,24,25,32,33,36,37,64,65,66,67,80,81,84,85,88,89,92,93,96,97,112,113,132,133,156,157,164,165):
            seed=rows[index]
            for ordinal in range(len(seed['reads'])):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix_rows),name=seed['name']+f'_before_read_{ordinal}',fault=dict(phase='before_read',ordinal=ordinal),fault_case=False);prefix_rows.append(row)
        def replace_word(row,address,value):
            for bank in row['banks']:
                raw=bytearray.fromhex(bank['before'])
                if bank['address']<=address<=bank['address']+len(raw)-4:
                    raw[address-bank['address']:address-bank['address']+4]=value.to_bytes(4,'little');bank['before']=raw.hex();return
            raise ValueError('Null-boundary input escaped backing')
        for index,name,site in((64,'genuine_null_device',0x405850),(67,'genuine_null_counter',0x405854)):
            row=copy.deepcopy(rows[index]);ordinal=next(n for n,event in enumerate(row['reads'])if event['instruction']==site)
            if site==0x405850:replace_word(row,0x411568,0)
            else:replace_word(row,0x61000300+(row['gp'][4]&15)+0x30,0)
            row.pop('fault',None);row.update(id=len(rows)+len(prefix_rows),name=name,stop_before_entries=[site],fault_case=False);prefix_rows.append(row)
        prefix=scratch/'prefix';prefix.mkdir();prefix_observed,prefix_proof=physical_owner(view,prefix_rows,prefix);prefix_sites,prefix_packets=install_observations(prefix_rows,prefix_observed)
        rows+=prefix_rows;executed|=prefix_sites;packets+=prefix_packets
        code=_Code(view,'.text');expected_sites={pc for entry in IMAGES for pc in complete_body(code,entry).code}
        if executed!=expected_sites:raise ValueError('Incomplete busy owner instruction coverage')
        fixture=scratch/'resource-busy.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')))
        binary=scratch/'resource-busy';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_resource_busy_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        if copies:command+=['-DHALO2_BUSY_COPY_ROUTES']
        if args.require_binding:command+=['-DHALO2_BUSY_OWNER_DISPATCH',*[str(ROOT/'src/physics'/name)for name in(
            'native_tu15_array_allocator.cpp','native_tu15_stack_arena.cpp','native_tu15_main_heap_release.cpp','native_tu15_main_heap_allocate.cpp','native_tu15_raw_allocator.cpp','native_tu15_physical_epoch.cpp')]]
        built=subprocess.run(command,text=True,capture_output=True);report['compiler_command']=command
        if built.returncode:report['compiler_errors']=built.stderr[-6000:];sys.stderr.write(report['compiler_errors']);built.check_returncode()
        modes={}
        for route in('candidate',*copies,*(('owner',)if args.require_binding else())):
            for mode in(('native','reference','page-isolation','raw-isolation','tick-isolation','epoch-isolation','delete-isolation')if route=='owner'else('native','reference')):
                env=dict(os.environ)
                for key in('HALO2_REFERENCE_RESOURCE_BUSY','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_PARTITION_TEMPORARIES','HALO2_REFERENCE_PAGE_ALLOCATOR','HALO2_REFERENCE_RAW_ALLOCATOR','HALO2_REFERENCE_TICK_CONVERSION','HALO2_REFERENCE_PHYSICAL_EPOCHS','HALO2_REFERENCE_DATUM_DELETE'):env.pop(key,None)
                key={'reference':'HALO2_REFERENCE_RESOURCE_BUSY','page-isolation':'HALO2_REFERENCE_PAGE_ALLOCATOR','raw-isolation':'HALO2_REFERENCE_RAW_ALLOCATOR','tick-isolation':'HALO2_REFERENCE_TICK_CONVERSION','epoch-isolation':'HALO2_REFERENCE_PHYSICAL_EPOCHS','delete-isolation':'HALO2_REFERENCE_DATUM_DELETE'}.get(mode)
                if key:env[key]='1'
                result=subprocess.run([str(binary),str(fixture),route],env=env,text=True,capture_output=True,timeout=120)
                if result.returncode:report['comparison_errors']=result.stderr[-8000:];sys.stderr.write(result.stderr);result.check_returncode()
                modes[mode if route=='candidate'else route+'_'+mode]=json.loads(result.stdout)
        paths=['include/halo2/native_tu15_resource_busy_kernel.hpp','src/physics/runtime/native_tu15_resource_busy_abi.hpp','tests/native_tu15_resource_busy_tests.cpp',str(Path(__file__).relative_to(ROOT)),SOURCE,'tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp']
        paths+=['tools/native_resource_busy_replacements.py','tools/native_selected_operand_binding.py']
        if copies:paths +=[path for path,_ in COPY_ROUTES.values()]
        if args.require_binding:paths+=['src/physics/runtime/native_tu15_allocator_owner_dispatch.hpp','tools/specialize_native_operands.py','src/physics/physics_live_support_tu15_generated.cpp','tools/generate_physics_live_support_tu15.py']
        report.update(passed=True,modes=modes,production_bound=args.require_binding,binding=binding,copy_routes=copies,body_resolver_drift_rejections=drift_checks,original_images={hex(p):value for p,value in IMAGES.items()},physical_original=proof,physical_prefix_original=prefix_proof,physical_cases=len(rows),physical_normal_cases=len(rows)-len(prefix_rows),physical_read_prefix_cases=sum('fault'in row for row in prefix_rows),null_backing_instruction_stops=sum('stop_before_entries'in row for row in prefix_rows),physical_pre_io_packets=packets,all_original_sites=sorted(executed),canonical_reference_sha256=reference_pin,
            retained_memory_exception_protocol_qualified=True,physical_instruction_page_fault_atomicity_qualified=False,
            device_scope='CPU query over declared backed headers and real counter words. No GPU completion/device/outcome supplied; timeline backing ownership/freshness and physical MMIO/atomicity remain separate.',
            address_undefined_sanitizers=args.sanitizers,source_sha256={name:sha(ROOT/name)for name in paths})
        print(json.dumps(dict(passed=True,physical_cases=len(rows),physical_pre_io_packets=packets,modes=modes)));return 0
    except BaseException as error:report['error']=repr(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())
