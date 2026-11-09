#!/usr/bin/env python3
"""Physical TU1.5 cached color selection versus the native semantic consumer.

Original17E460,BCDF0,BCFB0,3DDC0 execute unchanged on declared cache-hit pools.
No callee, allocation, query or collision outcome is supplied. This qualifies
color fields/parameter write-back in the selected cached/no-probe domain, not
an ABI adapter, render-cache allocator, cold probe or a complete game effect.
Run under tools/run_test.py; executable and compiler products are disposable.
"""
import argparse,ctypes,hashlib,json,os,random,shutil,signal,struct,subprocess,sys
from pathlib import Path
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,pack
from weapon_reference_versions import images
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
BASE=0x1000000
POOL,ROWS,WEAPON,UNIT,RENDER_POOL,RECORDS,EFFECT,PARAMS=[BASE+n*0x1000 for n in range(8)]
OWNER,ROOT_OWNER,TOKEN=0x80010002,0x80020003,0x9abc0002

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2)+'\n')
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 _,image,pin=next(x for x in images()if x[0]=='TU1.5');assert pin==PIN
 code=_Code(image,'.text');owners=(0x17e460,0xbcdf0,0xbcfb0,0x3ddc0);allowed=set();listings={}
 for owner in owners:
  body=complete_body(code,owner);allowed.update(body.code);listing=''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(body.code));listings[hex(owner)]=dict(sites=len(body.code),listing_sha256=hashlib.sha256(listing.encode()).hexdigest())
 rows=[];visited=set();steps=0;rng=random.Random(0x17e460);report={'passed':False,'scope':__doc__,'image_sha256':PIN,'original_owners':listings,'original_instructions_changed':False,'original_services_intercepted':0,'helper_sha256':sha(Path(__file__)),'retained_large_artifacts':[],'results':[]}
 try:
  bank_bytes=0x1100000
  assert BASE<=STACK-512<STACK+64<=BASE+bank_bytes
  for mode in range(4):
   cw=0x27f|(mode<<10);mx=0x1f80|(mode<<13)
   with OriginalAudioCpu(image,scratch/f'original-{mode}.elf',cw,mx,source_bank_bytes=bank_bytes)as host:
    host.trace(6,host.debug_offset+7*8,0)
    for number in range(20):
     kind=('cached-parameters','root-sample','root-not-ready','invalid-object','no-parameters')[number%5]
     bank=bytearray(rng.randbytes(0x10000));flags=rng.getrandbits(32)&~8
     if kind=='cached-parameters':flags|=8
     initial=[rng.getrandbits(32),rng.getrandbits(32)];parameter_colors=[rng.getrandbits(32),rng.getrandbits(32)];diffuse,lightmap=rng.getrandbits(32),rng.getrandbits(32)
     if number<3:
      lightmap,diffuse=((5128504,9538419),(4409165,6841679),(3748910,10261124))[number]
     present=kind!='no-parameters';ready=kind in('cached-parameters','root-sample','no-parameters');probe=kind in('cached-parameters','root-sample','no-parameters')
     def word(at,value,width=4):bank[at-BASE:at-BASE+width]=int(value).to_bytes(width,'little')
     word(POOL+0x24,12);word(POOL+0x38,4);word(POOL+0x44,ROWS)
     for index,salt,body,typ in((2,0x8001,WEAPON,2),(3,0x8002,UNIT,0)):
      word(ROWS+index*12,salt,2);word(ROWS+index*12+3,typ,1);word(ROWS+index*12+8,body)
     word(WEAPON+0x14,ROOT_OWNER);word(UNIT+0x14,0xffffffff);word(UNIT+0xcc,TOKEN)
     word(RENDER_POOL+0x24,256);word(RENDER_POOL+0x38,3);word(RENDER_POOL+0x44,RECORDS)
     record=RECORDS+2*256;word(record,TOKEN>>16,2);word(record+2,int(ready),1);word(record+4,ROOT_OWNER);word(record+0x28,diffuse);word(record+0x2c,lightmap)
     word(EFFECT+0x104,initial[0]);word(EFFECT+0x108,initial[1]);word(PARAMS,flags);word(PARAMS+0x54,parameter_colors[0]);word(PARAMS+0x58,parameter_colors[1]);word(PARAMS+0x5c,0)
     host.write(BASE,bank);host.write(0x4ecbd0,pack([POOL]));host.write(0x515f14,pack([RENDER_POOL]));host.write(STACK-512,rng.randbytes(576));host.write(STACK,pack([STOP,PARAMS if present else 0,int(probe)]))
     host.prepare(0x17e460,STACK,{'rax':0xffffffff if kind=='invalid-object'else OWNER,'rcx':EFFECT});regs=host.registers();pc_trace=[]
     for _ in range(600):
      pc=int(regs.rip)
      if pc==STOP:break
      assert pc in allowed,('Unowned original child',kind,hex(pc))
      ins=code.at(pc);assert host.read(pc,ins.size)==image.read(pc,ins.size),'Changed original bytes'
      pc_trace.append(pc);visited.add(pc);host.trace(9);_,status=os.waitpid(host.pid,0);assert os.WIFSTOPPED(status)and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,hex(pc),status);regs=host.registers();steps+=1
     else:raise AssertionError('Original color routine did not return')
     assert regs.rip==STOP and regs.rsp==STACK+12
     after=host.read(BASE,len(bank));read=lambda p:int.from_bytes(after[p-BASE:p-BASE+4],'little');expected=[read(EFFECT+0x104),read(EFFECT+0x108)];expected_flags=read(PARAMS);expected_parameters=[read(PARAMS+0x54),read(PARAMS+0x58)]
     only_expected=bytearray(bank)
     for at,value in((EFFECT+0x104,expected[0]),(EFFECT+0x108,expected[1]),(PARAMS,expected_flags),(PARAMS+0x54,expected_parameters[0]),(PARAMS+0x58,expected_parameters[1])):only_expected[at-BASE:at-BASE+4]=pack([value])
     assert after==only_expected,'Original unexpectedly changed a cache/owner word'
     cached=ready and kind!='invalid-object';cache=[diffuse,lightmap]if cached else None
     rows.append(dict(name=f'{kind}-{mode}-{number}',flags=flags,initial=initial,parameter_colors=parameter_colors,parameters_present=present,probe=probe,cache=cache,expected=expected,expected_flags=expected_flags,expected_parameter_colors=expected_parameters,expected_queries=0 if present and flags&8 else 1,expected_completed=bool((present and flags&8)or cached),rounding=mode,original_trace_sha256=hashlib.sha256(pack(pc_trace)).hexdigest()))
  fixture=scratch/'original-colors.json';save(fixture,{'image_sha256':PIN,'cases':rows,'scope':__doc__})
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in ('src/rendering/effects/source_effect_lighting_colors_tu15.cpp','tests/source_effect_lighting_colors_tu15_tests.cpp'):shutil.copyfile(ROOT/name,project/Path(name).name)
  kernel=project/'source_effect_lighting_colors_tu15.cpp'
  (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(colors LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\nadd_executable(colors source_effect_lighting_colors_tu15.cpp source_effect_lighting_colors_tu15_tests.cpp)\ntarget_include_directories(colors PRIVATE include)\ntarget_compile_features(colors PRIVATE cxx_std_20)\ntarget_compile_options(colors PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(colors PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME colors COMMAND colors "'+str(fixture)+'")\n')
  report.update(cases=len(rows),physical_steps=steps,visited_sites=len(visited),fixture_sha256=sha(fixture),original_other_heap_bytes_unchanged=True,compared_native_fields='effect A/B, request flags and cached A/B; FP environment preserved. Not raw ABI/private-stack or complete cache/probe parity.',source_hashes={n:sha(ROOT/n)for n in ('include/halo2/source_effect_lighting_colors_tu15.hpp','src/rendering/effects/source_effect_lighting_colors_tu15.cpp','tests/source_effect_lighting_colors_tu15_tests.cpp')})
  def run(label,compiler,expect):
   command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','colors','--filter','^colors$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   with(a.output/(label+'.log')).open('w')as f:r=subprocess.run(command,stdout=f,stderr=subprocess.STDOUT)
   data=json.loads((a.output/label/'report.json').read_text());ok=r.returncode==0 and data['passed'];report['results'].append({'label':label,'compiler':compiler,'exit_code':r.returncode,'passed':ok,'expected_pass':expect});save(a.output/'report.json',report);assert ok==expect,(label,'Unexpected validator result')
  clean=kernel.read_text()
  for compiler in('g++','clang++'):
   run(compiler,compiler,True)
   mutants=(('swapped','colors.a=sample->lightmap_rgb;colors.b=sample->diffuse_rgb;','colors.a=sample->diffuse_rgb;colors.b=sample->lightmap_rgb;'),('alpha','colors.a=sample->lightmap_rgb;','colors.a=sample->lightmap_rgb|0xff000000u;'),('writeback','parameters->flags|=8u;','parameters->flags|=0u;'))
   for name,old,new in mutants:
    assert clean.count(old)==1;kernel.write_text(clean.replace(old,new));run(compiler+'-'+name,compiler,False);kernel.write_text(clean)
  report['passed']=True;save(a.output/'report.json',report);print(json.dumps({k:report[k]for k in('passed','cases','physical_steps','visited_sites')}),flush=True)
 except BaseException as e:report['error']=str(e);save(a.output/'report.json',report);raise
 return 0
if __name__=='__main__':raise SystemExit(main())
