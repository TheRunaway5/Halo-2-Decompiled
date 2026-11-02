#!/usr/bin/env python3
"""Qualify native2E12A0(flags=0) for absent registry entries against TU1.5.

One actual selected destruction cleanup BEFORE supplies its own object and
registry fields. Declared valid empty/colliding tables extend the numerical
domain. Registered entries stop at original2E32E0, before any storage result;
native code refuses them without mutation. No original callee is replaced.
"""
import argparse,bisect,ctypes,hashlib,json,os,shutil,struct,subprocess,sys,tempfile
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from original_audio_cpu import OriginalAudioCpu
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

ENTRY,REGISTERED,RETURN,STACK=0x2e12a0,0x2e32e0,0x3000000,0x100f000
HEADER='include/halo2/native_tu15_base_lock.hpp'
DRIVER='tests/native_tu15_base_lock_tests.cpp'

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def save(path,data):path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(data,indent=2)+'\n')

def numeric_cases(capture):
    manifest=json.loads((capture/'capture-native-before.json').read_text())
    assert manifest['schema']=='native-tu15-contact-manager-typed-before-v1'
    assert not manifest['source_memory_mutated'] and not manifest['physics_result_supplied']
    regions=sorted(manifest['regions'],key=lambda x:x['address']);starts=[r['address']for r in regions]
    def get(p,n=4):
        region=regions[bisect.bisect_right(starts,p)-1];assert region['address']<=p and p+n<=region['address']+region['size']
        with (capture/region['before_file']).open('rb')as stream:stream.seek(p-region['address']);raw=stream.read(n)
        assert len(raw)==n;return int.from_bytes(raw,'little')
    object=manifest['input']['manager']+0x10;registry=get(0x48c93c);keys=get(registry+8);mask=get(registry+16)
    assert 0<=mask<=0xffff and not mask&(mask+1)
    before=get(object+5,1);actual=dict(name='actual_destruction_cleanup_registry',object=object,registry=registry,
        keys_address=keys,mask=mask,keys=[get(keys+n*4)for n in range(mask+1)],before_byte=before,declared_table=False)
    rows=[actual]
    for mask in(0,1,3,15,31):
        for count in sorted({0,min(mask,1),min(mask,4)}):
            for present in(False,True):
                table=[0]*(mask+1);slot=((object>>4)*0x9e3779b1)&mask
                for n in range(count):
                    key=object+(mask+1)*16*(n+1);assert (((key>>4)*0x9e3779b1)&mask)==slot
                    table[(slot+n)&mask]=key
                if present:table[(slot+count)&mask]=object
                rows.append(dict(name=f'declared_mask{mask}_collisions{count}_present{int(present)}',
                    object=object,registry=registry,keys_address=0x21000000,mask=mask,keys=table,
                    before_byte=(0,0x7f,0x80,0xff)[len(rows)%4],declared_table=True))
    return rows,dict(capture_manifest_sha256=sha(capture/'capture-native-before.json'),
        selected_input=manifest['input'],captured_bank_bytes=manifest['bank_bytes'],
        actual_input=actual,raw_banks_retained=False)

def fields(row):
    return [(0x48c93c,struct.pack('<I',row['registry'])),(row['registry']+8,struct.pack('<I',row['keys_address'])),
        (row['registry']+16,struct.pack('<I',row['mask'])),
        (row['keys_address'],struct.pack('<'+'I'*len(row['keys']),*row['keys'])),
        (row['object']+5,bytes([row['before_byte']]))]

def observe(image,code,row):
    regions=fields(row);u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
    u.mem_map(0x1000000,0x10000);u.mem_map(RETURN,4096)
    pages=sorted({p&~4095 for first,raw in regions if first>=0x4000000 for p in range(first,first+len(raw))})
    for p in pages:u.mem_map(p,4096)
    for p,raw in regions:u.mem_write(p,raw)
    u.mem_write(STACK,struct.pack('<II',RETURN,0));u.reg_write(R.UC_X86_REG_ESP,STACK);u.reg_write(R.UC_X86_REG_ECX,row['object'])
    out=dict(reads=[],writes=[],trace=[]);current={}
    def inside(p,n):return any(a<=p and p+n<=a+len(raw)for a,raw in regions)
    def instruction(_,pc,size,__):
        if pc in(RETURN,REGISTERED):out['stop']=pc;u.emu_stop();return
        i=code.at(pc);assert bytes(u.mem_read(pc,size))==image.read(pc,size) and i.size==size
        assert pc in range(0x2e12a0,0x2e12c9) or pc in range(0x2e11e0,0x2e126b) or pc in range(0x2e33f0,0x2e3424) or pc in range(0x2e3350,0x2e3363)
        current['pc']=pc;out['trace'].append(pc)
    def access(_,kind,p,n,value,__):
        if 0x1000000<=p and p+n<=0x1010000:return
        assert inside(p,n),(hex(current['pc']),hex(p),n)
        if kind==16:out['reads'].append([p,n])
        else:
            assert n==1 and p==row['object']+5
            out['writes'].append(dict(instruction=current['pc'],address=p,before=int.from_bytes(u.mem_read(p,n),'little'),written=value))
    u.hook_add(UC_HOOK_CODE,instruction);u.hook_add(UC_HOOK_MEM_READ,access);u.hook_add(UC_HOOK_MEM_WRITE,access)
    u.emu_start(ENTRY,0,count=1024);assert out.get('stop')in(RETURN,REGISTERED)
    out['registered']=out['stop']==REGISTERED;out['after_byte']=u.mem_read(row['object']+5,1)[0]
    for p,raw in regions:
        expected=bytes([out['after_byte']])if p==row['object']+5 else raw
        assert bytes(u.mem_read(p,len(raw)))==expected
    return dict(row,**out)

def physical(image,code,rows,scratch):
    pages=sorted({p&~4095 for row in rows for first,raw in fields(row) if first>=0x4000000 for p in range(first,first+len(raw))})
    host=OriginalAudioCpu(image,scratch/'base-lock-original32.elf',0x27f,0x1fa0,[(p,4096)for p in pages]);steps=0
    aliases={'eax':'rax','ecx':'rcx','edx':'rdx','ebx':'rbx','esp':'rsp','ebp':'rbp','esi':'rsi','edi':'rdi'}
    try:
        host.trace(6,host.debug_offset+7*8,0)
        for row in rows:
            regions=fields(row)
            for p,raw in regions:host.write(p,raw)
            host.write(STACK,struct.pack('<II',RETURN,0));host.prepare(ENTRY,STACK,dict(rcx=row['object']));regs=host.registers();trace=[];reads=[];writes=[]
            for _ in range(1024):
                pc=regs.rip
                if pc in(RETURN,REGISTERED):break
                i=code.at(pc);assert host.read(pc,i.size)==i.bytes;trace.append(pc);pending=[]
                def register(name):return getattr(regs,aliases[name])&0xffffffff if name else 0
                for op in i.operands:
                    if op.type!=3 or i.mnemonic=='lea':continue
                    m=op.mem;p=(register(i.reg_name(m.base))+register(i.reg_name(m.index))*m.scale+m.disp)&0xffffffff
                    if 0x1000000<=p and p+op.size<=0x1010000:continue
                    assert any(a<=p and p+op.size<=a+len(raw)for a,raw in regions)
                    if op.access&1:reads.append([p,op.size])
                    if op.access&2:pending.append((p,op.size,host.read(p,op.size)))
                host.trace(9);regs=host.wait_stop()
                for p,n,before in pending:writes.append(dict(instruction=pc,address=p,before=int.from_bytes(before,'little'),written=int.from_bytes(host.read(p,n),'little')))
            else:raise RuntimeError('Original physical lookup did not reach its declared boundary')
            assert regs.rip==row['stop'] and trace==row['trace'] and reads==row['reads'] and writes==row['writes'],row['name']
            assert host.read(row['object']+5,1)[0]==row['after_byte'];steps+=len(trace)
    finally:host.close()
    return dict(passed=True,cases=len(rows),original_instructions=steps,backend='Physical Linux i386 compatibility; unchanged instructions and PTRACE_SINGLESTEP')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--reference',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
    assert not a.reference.exists();pins={n:sha(ROOT/n)for n in(HEADER,DRIVER,'tools/qualify_native_base_lock_release_tu15.py')}
    report=dict(passed=False,scope=__doc__,source_sha256=pins,retained_large_artifacts=[])
    try:
        image=Xbe(NEW_DEFAULT);assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(image,'.text');rows,provenance=numeric_cases(a.capture)
        rows=[observe(image,code,row)for row in rows]
        with tempfile.TemporaryDirectory(prefix='base-lock-',dir=scratch)as owned:
            owned=Path(owned);physical_report=physical(image,code,rows,owned)
            reference=dict(original_xbe_sha256=NEW_PIN,source_provenance=provenance,cases=rows,original_code_patches=[],supplied_callee_results=[])
            save(a.reference,reference);project=owned/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/DRIVER,project/'driver.cpp')
            cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeBaseLock LANGUAGES CXX)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
enable_testing()
add_executable(base_lock driver.cpp)
target_compile_features(base_lock PRIVATE cxx_std_20)
target_include_directories(base_lock PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(base_lock PRIVATE glm::glm nlohmann_json::nlohmann_json)
add_test(NAME base_lock COMMAND base_lock "'''+str(a.reference.resolve())+'''")
''';(project/'CMakeLists.txt').write_text(cmake)
            commands=[];results=[]
            def run(label,compiler):
                command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--target','base_lock','--filter','^base_lock$','--jobs','1','--test-jobs','1','--scratch-root',str(owned/'builds'),'--output',str(a.output/label),'--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
                commands.append(command);result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT);results.append(dict(label=label,returncode=result.returncode));return result.returncode
            for n,compiler in enumerate(a.compilers):assert run('compiler-'+str(n),compiler)==0
            target=project/HEADER;text=target.read_text();controls=[('hash','0x9e3779b1u','0x9e3779b3u'),('stride','keys+slot*4u','keys+slot*8u'),('clear_bit','std::byte{0x7f}','std::byte{0xff}'),('registered','if(native_tu15_base_lock_registered(memory,object))','if(false && native_tu15_base_lock_registered(memory,object))')]
            for label,before,after in controls:
                assert text.count(before)==1;target.write_text(text.replace(before,after));assert run('reject-'+label,a.compilers[0])!=0;target.write_text(text)
            assert all(sha(ROOT/n)==digest for n,digest in pins.items())
            report.update(passed=True,original_xbe_sha256=NEW_PIN,physical=physical_report,reference_sha256=sha(a.reference),
                source_provenance=provenance,cases=len(rows),missing_cases=sum(not r['registered']for r in rows),
                registered_refusals=sum(r['registered']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),
                ordered_stores=sum(len(r['writes'])for r in rows),commands=commands,results=results,negative_controls=[x[0]for x in controls],
                original_code_patches=[],supplied_callee_results=[],full_contact_manager_or_gameplay_acceptance=False)
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:save(a.output/'report.json',report)
    print(json.dumps({k:report[k]for k in('passed','cases','missing_cases','registered_refusals','ordered_reads','ordered_stores')}),flush=True)

if __name__=='__main__':main()
