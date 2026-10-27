#!/usr/bin/env python3
"""Unchanged TU1.5 island->native bodyforce composition prefix qualification.

Pose/constraint dependency calls stop before their real entry instructions;
no result is supplied. Every fixture is also verified by physical i386 source
execution using the common owned original observer, not a lifted root.
"""
import argparse,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_motion_forces_tu15 import inputs,hardware,sha
BASE,WORLD,ISLAND,BODIES=0x1000000,0x1001000,0x1002000,0x1002200
CONFIG= WORLD+0x140
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x3000),(0x41f000,0x1000),(0x46a000,0x1000),(0x48c000,0x1000))
def fixture():
    controls=inputs();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');body=complete_body(code,0x320b70);assert len(body.code)==98
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(BASE,0x10000);g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000)
    write=lambda p,v,n=4:g.mem_write(p,(v&((1<<(n*8))-1)).to_bytes(n,'little'));read=lambda p:int.from_bytes(g.mem_read(p,4),'little')
    active=False;writes=[];boundary=None;target=STOP
    def on_write(_g,_access,p,n,value,_data):
        if not active or not any(lo<=p and p+n<=lo+size for lo,size in BANKS):return
        site=g.reg_read(R.UC_X86_REG_EIP);i=code.at(site);assert i.mnemonic=='movaps'and n==8,(hex(site),i.mnemonic,n)
        b=dict(instruction=site,address=p,before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex())
        if writes and writes[-1]['instruction']==site and writes[-1]['address']+8==p and len(writes[-1]['written'])==16:writes[-1]['before']+=b['before'];writes[-1]['written']+=b['written']
        else:writes.append(b)
    def on_code(_g,p,_size,_data):
        nonlocal boundary,target
        if not active:return
        if p==0x325170:
            boundary=dict(owner='pose',arguments=[g.reg_read(R.UC_X86_REG_ECX),p,read(g.reg_read(R.UC_X86_REG_ESP)+4)]);target=p;g.emu_stop()
        elif p==0x326200:
            boundary=dict(owner='constraints',arguments=[read(g.reg_read(R.UC_X86_REG_ESP)+4+i*4)for i in range(9)]);target=p;g.emu_stop()
    g.hook_add(UC_HOOK_MEM_WRITE,on_write);g.hook_add(UC_HOOK_CODE,on_code)
    cases=[]
    def setup(c,count,constraints=0):
        nonlocal active
        active=False;g.mem_write(BASE,bytes(0x3000));g.mem_write(STACK,bytes(0x10000))
        raw=bytes.fromhex(c['banks'][0]['before'])
        for n in range(3):g.mem_write(BASE+n*0x400,raw[:0x200]);write(BASE+n*0x400,0x41f2e0 if n%2==0 else 0x41f360)
        g.mem_write(WORLD+0x150,raw[0x200:0x208]);g.mem_write(WORLD+0xe0,raw[0x220:0x230]);write(ISLAND+0x20,WORLD)
        write(ISLAND+0x40,count);write(ISLAND+0x3c,BODIES);write(ISLAND+0x5c,constraints);write(ISLAND+0x58,BASE+0x2600)
        for n in range(3):p=BASE+0x2300+n*0x80;write(BODIES+n*4,p);write(p+0x3c,BASE+n*0x400)
        write(0x48c974,0);write(0x48c978,0)
    def run(name,c):
        nonlocal active,writes,boundary,target
        cw,sw,mx=c['before_fp'];g.reg_write(R.UC_X86_REG_ECX,ISLAND);g.reg_write(R.UC_X86_REG_ESP,SP);g.reg_write(R.UC_X86_REG_EFLAGS,2)
        g.reg_write(R.UC_X86_REG_FPCW,cw);g.reg_write(R.UC_X86_REG_FPSW,sw);g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,mx)
        write(SP,STOP);write(SP+4,CONFIG);banks=[dict(address=p,before=bytes(g.mem_read(p,n)).hex())for p,n in BANKS];writes=[];boundary=None;target=STOP;active=True
        g.emu_start(0x320b70,STOP,count=20000);active=False;assert boundary or g.reg_read(R.UC_X86_REG_EIP)==STOP
        for b,(_,n)in zip(banks,BANKS):b['after']=bytes(g.mem_read(b['address'],n)).hex()
        cases.append(dict(name=name,entry=0x320b70,receiver=ISLAND,arguments=[CONFIG],island=ISLAND,configuration=CONFIG,banks=banks,writes=writes,boundary=boundary,boundary_entry=target,
          native_force_calls=int(boundary is not None and boundary['owner']=='pose'),before_fp=c['before_fp'],unicorn_fp=[g.reg_read(R.UC_X86_REG_FPCW),g.reg_read(R.UC_X86_REG_FPSW),g.reg_read(R.UC_X86_REG_MXCSR)]))
    for n,c in enumerate(controls['cases'][:96]):setup(c,1+n%3);run('island_'+c['name'],c)
    for n in range(4):setup(controls['cases'][n],0);run(f'empty_{n}',controls['cases'][n])
    for count in (0,1,3):setup(controls['cases'][0],count,2);run(f'constraints_{count}',controls['cases'][0])
    return dict(image_sha256=NEW_PIN,island_sites=98,cases=cases,listing_sha256=hashlib.sha256(''.join(f'{code.at(p).address:x} {code.at(p).bytes.hex()} {code.at(p).mnemonic} {code.at(p).op_str}\n'for p in sorted(body.code)).encode()).hexdigest())
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=True)
    f=hardware(fixture(),scratch,BANKS);f['required_backend']='unchanged-tu15-island-unicorn-stores+physical-i386-banks-fp-v1';path=scratch/'island.json';path.write_text(json.dumps(f,separators=(',',':')))
    project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_island_integration.cpp';text=source.read_text();mutation='auto index=s.body_count(island)-1u;';assert text.count(mutation)==1;(project/'mutated.cpp').write_text(text.replace(mutation,'auto index=0u;'))
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and "real island boundary differs" in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Unchanged original rejected reversed body traversal:",r.stderr.strip())
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_island_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name,src in (('native_tu15_island_integration_tests',source),('native_tu15_island_integration_mutation',project/'mutated.cpp')):
        cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_island_integration_tests.cpp" "{src}" "{ROOT}/src/physics/native_tu15_motion_forces.cpp")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off)\n'
    cmake+=f'add_test(NAME native_island_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_island_integration_tests> "{path}")\n'
    cmake+=f'add_test(NAME native_island_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_tu15_island_integration_mutation> "{path}")\n';(project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/'cmake'),'--target','native_tu15_island_integration_tests','--target','native_tu15_island_integration_mutation','--filter','^native_island_(original_tu15|mutation_rejected)$','--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False)
    report=dict(passed=r.returncode==0,exit_code=r.returncode,original_cases=len(f['cases']),native_force_compositions=sum(c['native_force_calls']for c in f['cases']),ordered_original_vector_stores=sum(len(c['writes'])for c in f['cases']),
      physical_original_instructions=f['physical_original_instructions'],image_sha256=NEW_PIN,island_sites=98,listing_sha256=f['listing_sha256'],required_backend=f['required_backend'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,
      negative_control='Private kernel mutation traverses first body instead of source reverse traversal; unchanged original must reject actual pose boundary',retained_large_artifacts=[],
      source_hashes={str(p.relative_to(ROOT)):sha(p)for p in(source,ROOT/'include/halo2/native_tu15_island_integration.hpp',ROOT/'src/physics/native_tu15_motion_forces.cpp',ROOT/'tests/native_tu15_island_integration_tests.cpp',ROOT/'tools/validate_native_motion_forces_tu15.py',Path(__file__))},command=command,
      excluded=['residual pose/constraint/action execution','world319F90 integration','live multiplayer','profiling enabled original timing','unmasked fault contexts'])
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','native_force_compositions')}));return r.returncode
if __name__=='__main__':raise SystemExit(main())
