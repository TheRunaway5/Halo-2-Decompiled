#!/usr/bin/env python3
"""Qualify native BC6C0/DC4C0 against unchanged TU1.5 instructions.

Constructed object/response graphs are declared component inputs. Complete
original bodies execute without replaced callees. Numeric postimages are test
expectations only; native execution receives ordinary BEFORE data and ABI.
"""
import argparse
import ctypes
import gzip
import hashlib
import json
import os
from pathlib import Path
import signal
import struct
import subprocess
import sys
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from original_audio_cpu import OriginalAudioCpu

BANK,SIZE,STACK=0x2100000,65536,0x210f800
POOL,HEADERS,BODY= BANK+0x100,BANK+0x1000,BANK+0x2000
COUNT,ACTIVE,CACHED,STATES=BANK+0xc000,BANK+0xc010,BANK+0xc020,BANK+0xc030
INFO,SECTIONS,RESPONSES=BANK+0xd000,BANK+0xd200,BANK+0xd800
GLOBAL,RETURN=0x4ecbd0,0x3000000
NAMES=('eax','ecx','edx','ebx','esp','ebp','esi','edi')
ALIASES=dict(zip(NAMES,('rax','rcx','rdx','rbx','rsp','rbp','rsi','rdi')))
REGISTERS=[getattr(R,'UC_X86_REG_'+name.upper()) for name in NAMES]
SOURCES=('include/halo2/native_tu15_object_regions.hpp','include/halo2/native_tu15_damage_region_eligibility.hpp',
 'src/gameplay/objects/source_object_regions_native_tu15_internal.hpp',
 'src/gameplay/objects/source_damage_region_eligibility_native_tu15_internal.hpp',
 'tests/native_tu15_damage_region_eligibility_tests.cpp')


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def prepare(case):
    seed=case['seed'];data=bytearray((n*37+seed)&255 for n in range(SIZE));writes=[]
    def put(address,value,width=4):
        value&=(1<<(width*8))-1;raw=value.to_bytes(width,'little')
        writes.append([address,raw.hex()])
        if address==GLOBAL:return
        assert BANK<=address and address+width<=BANK+SIZE
        data[address-BANK:address-BANK+width]=raw
    extent=case.get('extent',40);offset=case.get('offset',512);object=case.get('object',0xb17a0003)
    put(GLOBAL,POOL);put(POOL+0x44,HEADERS);put(HEADERS+3*12+8,BODY)
    put(BODY+0x118,extent,2);put(BODY+0x11a,offset,2)
    put(INFO+0xc0,SECTIONS);section=case.get('section',0);response=case.get('response',0)
    put((SECTIONS+section*0x38+16)&0xffffffff,RESPONSES)
    authored=(RESPONSES+response*0x50)&0xffffffff
    put(authored+32,case.get('requested',1),2);put(authored+34,case.get('region',0),2)
    # These are declared ordinary state bytes, never expected query outputs.
    count=int(extent/10);base=(BODY+offset)&0xffffffff;region=case.get('region',0)
    if 0<=region<count:
        put((base+count*2+region*8+1)&0xffffffff,case.get('state',0),1)
    gp=[(0x61834a79*(i+1)+seed*0x19660d)&0xffffffff for i in range(8)];gp[4]=STACK
    if case['entry']==0xbc6c0:
        gp[0]=object;gp[6]=case.get('count_output',COUNT);gp[7]=case.get('active_output',ACTIVE);gp[3]=case.get('cached_output',CACHED)
        put(STACK+4,case.get('states_output',STATES))
    else:
        gp[0]=section;gp[1]=response;gp[2]=case.get('info',INFO);put(STACK+4,object)
        if case.get('bad_states'):
            # The actual descriptor offset is signed16; redirect its body
            # lookup to an explicitly unmapped object for the query fault.
            put(HEADERS+3*12+8,0xb0000000)
    if 'body' in case:put(HEADERS+3*12+8,case['body'])
    put(STACK,RETURN)
    flags=0x202|(seed&0x8d5)
    return data,writes,gp,flags


def cases():
    rows=[]
    def add(name,entry=0xdc4c0,**values):rows.append(dict(name=name,entry=entry,seed=(len(rows)*17+91)&255,**values))
    for extent in (-32768,-101,-10,-9,-1,0,1,9,10,11,39,40,32767):
        for region in (-1,0,32767):add(f'extent_{extent}_region_{region}',extent=extent,region=region)
    for region in (-32768,-1,0,3,4,32767):
        for current in (-128,-1,0,4,127):
            for requested in (-32768,-128,-1,0,1,4,127,32767):
                add(f'region_{region}_state_{current}_requested_{requested}',region=region,state=current,requested=requested)
    for section,response in ((1,1),(0x40000000,0x20000000),(0x80000000,0x40000000)):
        add(f'wrapped_section_{section:x}_response_{response:x}',section=section,response=response)
    add('ignored_object_salt',object=0x00000003)
    add('negative_descriptor_offset',offset=-512,state=-1,requested=0)
    add('zero_region_count_still_queries',extent=0,region=-1)
    for name,values in (
        ('invalid_info',dict(info=0xb0000000)),('invalid_object',dict(object=0xffffffff)),
        ('null_body',dict(body=0)),('unmapped_body',dict(body=0xb0000000))):add(name,**values)
    for extent in (-32768,-11,-10,-9,-1,0,1,9,10,11,40,32767):
        for mask in range(8):
            add(f'region_query_extent_{extent}_optional_{mask}',entry=0xbc6c0,extent=extent,
                active_output=ACTIVE if mask&1 else 0,cached_output=CACHED if mask&2 else 0,states_output=STATES if mask&4 else 0)
    for name,values in (
        ('all_outputs_alias',dict(active_output=COUNT,cached_output=COUNT,states_output=COUNT)),
        ('active_alias_count',dict(active_output=COUNT)),('cache_alias_count',dict(cached_output=COUNT)),
        ('raw_extent_alias_descriptor',dict(count_output=BODY+0x118)),
        ('raw_extent_alias_state_argument',dict(count_output=STACK+4)),
        ('count_alias_saved_ebp',dict(count_output=STACK-4)),
        ('active_alias_saved_ebp',dict(active_output=STACK-4)),
        ('cache_alias_global',dict(cached_output=GLOBAL)),
        ('count_write_fault',dict(count_output=0xb0000000)),
        ('active_write_fault',dict(active_output=0xb0000000)),
        ('cached_write_fault',dict(cached_output=0xb0000000)),
        ('states_write_fault',dict(states_output=0xb0000000)),
        ('body_read_fault',dict(body=0xb0000000)),
        ('slot_read_fault',dict(object=0xffffffff))):add('query_'+name,entry=0xbc6c0,**values)
    return rows


def original(image,work,rows):
    code=_Code(image,'.text');bodies={entry:complete_body(code,entry) for entry in (0xbc6c0,0xdc4c0)}
    sites={pc:bytes(code.at(pc).bytes).hex() for body in bodies.values() for pc in body.code}
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000);u.mem_map(BANK,SIZE)
    # Ordinary declared caller return page; stop before its first instruction.
    # This is already mapped by the independent physical ELF caller.
    u.mem_map(RETURN,4096)
    for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
    current={}
    def instruction(_,pc,size,__):
        if pc==RETURN:u.emu_stop();return
        assert pc in sites and bytes(u.mem_read(pc,size)).hex()==sites[pc]
        current['pc']=pc;current['trace'].append(pc)
    def access(_,kind,address,size,value,__):
        if not (BANK<=address and address+size<=BANK+SIZE or address==GLOBAL and size==4):return
        current['accesses'].append(dict(pc=current['pc'],kind='write' if kind==17 else 'read',address=address,size=size))
        if kind==17:current['stores'].append([current['pc'],address,size,value&((1<<(size*8))-1)])
    def invalid(_,kind,address,size,value,__):
        current['fault']=dict(pc=current['pc'],kind='write' if kind==20 else 'read',address=address,size=size)
        return False
    u.hook_add(UC_HOOK_CODE,instruction);u.hook_add(UC_HOOK_MEM_READ,access);u.hook_add(UC_HOOK_MEM_WRITE,access);u.hook_add(UC_HOOK_MEM_INVALID,invalid)
    results=[]
    for row in rows:
        data,writes,gp,flags=prepare(row);u.mem_write(BANK,bytes(data));u.mem_write(GLOBAL,struct.pack('<I',POOL))
        for reg,value in zip(REGISTERS,gp):u.reg_write(reg,value)
        u.reg_write(R.UC_X86_REG_EFLAGS,flags);current.clear();current.update(trace=[],accesses=[],stores=[],fault=None)
        try:u.emu_start(row['entry'],0,count=160)
        except UcError:
            if not current['fault']:raise
        after_gp=[u.reg_read(reg)&0xffffffff for reg in REGISTERS]
        assert current['fault'] or u.reg_read(R.UC_X86_REG_EIP)==RETURN
        # Construct expected persistent bytes from committed original stores
        # only. Any extra original mutation fails the before/after comparison.
        expected=bytearray(data);global_after=struct.pack('<I',POOL)
        for _,address,width,value in current['stores']:
            raw=value.to_bytes(width,'little')
            if address==GLOBAL:global_after=raw
            else:expected[address-BANK:address-BANK+width]=raw
        assert bytes(expected)==bytes(u.mem_read(BANK,SIZE)) and global_after==bytes(u.mem_read(GLOBAL,4))
        fault=current['fault'];mask=0x8c5 if row['entry']==0xbc6c0 else 0x8d5
        if fault and fault['pc']==0xdc4cf:mask=0x801 # IMUL defines only CF/OF.
        result=dict(input=row,before_writes=writes,before_gp=gp,before_eflags=flags,
            after_gp=after_gp,after_eflags=u.reg_read(R.UC_X86_REG_EFLAGS),defined_flags_mask=mask,
            trace=current['trace'],accesses=current['accesses'],stores=current['stores'],fault=fault)
        results.append(result)
    # Independently exhaust the original integer division slice. No memory
    # operand, callee or count formula supplies its observed result.
    raw=bytearray(b'REG15V01'+struct.pack('<I',65536));division_sites={}
    for word in range(65536):
        u.reg_write(R.UC_X86_REG_EDX,word if word<32768 else word|0xffff0000)
        u.reg_write(R.UC_X86_REG_EIP,0xbc6e7);u.reg_write(R.UC_X86_REG_EFLAGS,0x202)
        current.clear();current.update(trace=[],accesses=[],stores=[],fault=None)
        # Stop before TEST EDI; the exact six-instruction arithmetic slice.
        u.emu_start(0xbc6e7,0xbc6f8,count=6)
        assert u.reg_read(R.UC_X86_REG_EIP)==0xbc6f8 and not current['accesses']
        value=u.reg_read(R.UC_X86_REG_EAX)&0xffffffff;raw.extend(struct.pack('<I',value))
        for pc in current['trace']:division_sites[pc]=sites[pc]
    return dict(schema='native-region-eligibility-tu15-v1',original_xbe_sha256=NEW_PIN,bank=BANK,size=SIZE,
        global_pointer=GLOBAL,global_before=POOL,return_address=RETURN,cases=results,source_sites=sites,
        division_sites=division_sites,division_cases=65536,original_code_patches=[],supplied_callee_results=[]),bytes(raw),code


def physical(image,work,reference,code):
    # Full original instructions on physical i386 for every declared ABI case.
    host=OriginalAudioCpu(image,work/'original32.elf',0x27f,0x1f80,source_bank_bytes=0x1110000)
    visited=set();steps=0
    try:
        host.trace(6,host.debug_offset+7*8,0)
        for row in reference['cases']:
            data,_,_,_=prepare(row['input']);host.write(BANK,data);host.write(GLOBAL,struct.pack('<I',POOL))
            host.prepare(row['input']['entry'],STACK,dict(zip(ALIASES.values(),row['before_gp'])))
            regs=host.registers();regs.eflags=row['before_eflags'];host.trace(13,0,ctypes.addressof(regs))
            trace=[];accesses=[];stores=[];fault=None
            for _ in range(160):
                pc=regs.rip
                if pc==RETURN:break
                assert pc in {int(p) for p in reference['source_sites']}
                i=code.at(pc);assert host.read(pc,i.size)==i.bytes
                trace.append(pc);visited.add(pc);pending=[]
                def register(name):return getattr(regs,ALIASES[name])&0xffffffff if name else 0
                def operand(address,size,write):
                    valid=(BANK<=address and address+size<=BANK+SIZE) or (address==GLOBAL and size==4)
                    if not valid:return
                    accesses.append(dict(pc=pc,kind='write' if write else 'read',address=address,size=size))
                    if write:pending.append((address,size))
                if i.mnemonic!='lea':
                    for op in i.operands:
                        if op.type!=3:continue
                        address=(register(i.reg_name(op.mem.base))+register(i.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&0xffffffff
                        assert op.access in (1,2)
                        operand(address,op.size,op.access==2)
                if i.mnemonic in ('push','call'):operand((regs.rsp-4)&0xffffffff,4,True)
                if i.mnemonic in ('pop','ret'):operand(regs.rsp&0xffffffff,4,False)
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers()
                assert os.WIFSTOPPED(status)
                if os.WSTOPSIG(status)==signal.SIGSEGV:
                    assert row['fault'] and pc==row['fault']['pc'];fault=row['fault'];break
                assert os.WSTOPSIG(status)==signal.SIGTRAP
                for address,size in pending:stores.append([pc,address,size,int.from_bytes(host.read(address,size),'little')])
            else:raise AssertionError('Physical original exceeded bounded complete CFG')
            mask=row['defined_flags_mask'];assert trace==row['trace'],(row['input']['name'],'trace')
            assert accesses==row['accesses'] and stores==row['stores'],(row['input']['name'],'ordered fields/stores')
            assert [getattr(regs,name)&0xffffffff for name in ALIASES.values()]==row['after_gp'],(row['input']['name'],'GP')
            assert regs.eflags&mask==row['after_eflags']&mask,(row['input']['name'],'defined flags',hex(regs.eflags),hex(row['after_eflags']))
            assert fault==row['fault'],(row['input']['name'],'fault boundary',fault,row['fault']);steps+=len(trace)
    finally:host.close()
    return dict(passed=True,cases=len(reference['cases']),instructions=steps,instruction_sites=len(visited),
                source_instruction_sites=len(reference['source_sites']),unvisited_sites=sorted(set(map(int,reference['source_sites']))-visited),
                fault_prefixes=sum(r['fault'] is not None for r in reference['cases']),original_code_patches=[],supplied_callee_results=[])


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output.mkdir(parents=True,exist_ok=False);work=scratch/('eligibility-'+args.output.name);work.mkdir()
    report=dict(passed=False,scope=__doc__,retained_large_artifacts=[],original_gp_frame_captured=False)
    try:
        image=Xbe(NEW_DEFAULT);assert sha(NEW_DEFAULT)==NEW_PIN
        ref,vectors,code=original(image,work,cases());report['physical']=physical(image,work,ref,code)
        fixture=work/'original.json';fixture.write_text(json.dumps(ref,separators=(',',':'))+'\n')
        (work/'counts.bin').write_bytes(vectors)
        report['reference_sha256']=sha(fixture);report['count_vectors_sha256']=sha(work/'counts.bin')
        # Small required numeric references, no complete raw memory bank.
        for name,data in [('original.json.gz',fixture.read_bytes()),('counts.bin.gz',vectors)]:
            packed=gzip.compress(data,mtime=0);assert len(packed)<1024*1024
            (args.output/name).write_bytes(packed)
        project=work/'project';project.mkdir()
        closure=set(SOURCES)
        closure.update(str(p.relative_to(ROOT)) for p in (ROOT/'include').rglob('*') if p.is_file())
        closure.update(str(p.relative_to(ROOT)) for p in (ROOT/'src').rglob('*.hpp'))
        pins={name:sha(ROOT/name) for name in sorted(closure)}
        for name in sorted(closure):
            path=project/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes((ROOT/name).read_bytes())
        # Freeze the compatibility support headers too; no engine archive or
        # full-game source is borrowed by this focused component build.
        cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeRegionEligibility LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
'''
        (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True,timeout=120)\nd=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not d["passed"] and d.get("error") in ("Native region query/eligibility differs from original","Signed extent division differs from unchanged original"),(r.returncode,d)\nprint("Original region contract rejects mutant")\n')
        variants={'native':None}
        for name,path,old,new in (
            ('wrong_divisor','include/halo2/native_tu15_object_regions.hpp','std::int32_t(extent)/10','std::int32_t(extent)/8'),
            ('missing_extent_store','include/halo2/native_tu15_object_regions.hpp','storage.publish_extent(extent);',''),
            ('cached_count_snapshot','include/halo2/native_tu15_object_regions.hpp','base+storage.reread_count(false)','base+std::uint32_t(count)'),
            ('reject_missing_region','include/halo2/native_tu15_damage_region_eligibility.hpp','return NativeTu15DamageRegionEligibility::missing_region;','return NativeTu15DamageRegionEligibility::already_at_or_above;'),
            ('unsigned_state','include/halo2/native_tu15_damage_region_eligibility.hpp','const auto current=storage.current_state(region);','const auto current=std::uint8_t(storage.current_state(region));'),
            ('unmasked_object','src/gameplay/objects/source_object_regions_native_tu15_internal.hpp','s.flags.logic(datum&0xffffu,4)','s.flags.logic(datum,4)'),
            ('wrong_response_stride','src/gameplay/objects/source_damage_region_eligibility_native_tu15_internal.hpp','s.gp[5]=s.gp[1]*5u','s.gp[5]=s.gp[1]*4u')):
            original_text=(project/path).read_text();assert original_text.count(old)==1,(name,path)
            variant=project/'variants'/name;target=variant/path;target.parent.mkdir(parents=True,exist_ok=True)
            target.write_text(original_text.replace(old,new));variants[name]=variant
        retain=[]
        for name,variant in variants.items():
            includes=f'"{variant}/include" "{variant}/src" ' if variant else ''
            cmake+=f'add_executable(region_{name} tests/native_tu15_damage_region_eligibility_tests.cpp)\ntarget_include_directories(region_{name} PRIVATE {includes}"{project}/include" "{project}/src")\ntarget_compile_features(region_{name} PRIVATE cxx_std_20)\ntarget_compile_options(region_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(region_{name} PRIVATE nlohmann_json::nlohmann_json)\n'
            output='evidence/'+name+'.json';retain.append(output)
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if name!='native':command+=f'"{sys.executable}" "{project}/negative.py" '
            command+=f'$<TARGET_FILE:region_{name}> "{fixture}" "{work}/counts.bin" "${{CMAKE_BINARY_DIR}}/{output}"'
            cmake+=f'add_test(NAME native_regions_{name} COMMAND {command})\nset_tests_properties(native_regions_{name} PROPERTIES TIMEOUT 150)\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),*sum((['--target','region_'+name] for name in variants),[]),
                     '--filter','^native_regions_','--scratch-root',str(work/'builds'),'--output',str(args.output/compiler),
                     '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report',path] for path in retain),[])]
            result=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
        report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,source_hashes=pins,
            current_sources_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),
            original_xbe_sha256=NEW_PIN,declared_cases=len(ref['cases']),division_cases=65536,
            reference_bytes=fixture.stat().st_size,count_vector_bytes=len(vectors),negative_controls=list(variants)[1:])
    except BaseException as error:
        report['error']=str(error);raise
    finally:(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':sys.exit(main())
