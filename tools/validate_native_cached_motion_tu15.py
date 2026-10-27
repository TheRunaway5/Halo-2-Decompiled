#!/usr/bin/env python3
"""Unchanged381-site motion advance/closed quaternion and origin composition."""
import argparse,bisect,copy,hashlib,json,os,random,struct,subprocess,sys,tarfile
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import validate_native_geometry_tu15 as geometry
OWNERS={0x325810:160,0x323430:58,0x1e1010:31,0x37c5d0:69,0x2e5810:62,0x8fdb0:1}
CONSTANTS=((0x41f1d0,16),(0x46a0cc,4),(0x46a0d0,4),(0x46a0e4,4))
MOTION=0x1000000
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def metadata(image):
 code=_Code(image,'.text');pcs=set();names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];listing=[]
 for owner,count in OWNERS.items():
  body=complete_body(code,owner);assert len(body.code)==count;pcs.update(body.code)
 assert len(pcs)==381
 for pc in sorted(pcs):
  i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
  for op in i.operands:
   if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
   assert not op.mem.segment;access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access;assert access&(CS_AC_READ|CS_AC_WRITE);b,idx=i.reg_name(op.mem.base),i.reg_name(op.mem.index);ops.append(dict(base=names[b]if b else-1,index=names[idx]if idx else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
  site=dict(instruction=pc,bytes=i.bytes.hex(),operands=ops)
  if i.mnemonic=='rsqrtss':site['rsqrt']=[int(i.reg_name(i.operands[1].reg)[3:]),int(i.reg_name(i.operands[0].reg)[3:])]
  sites.append(site)
 return dict(sites=sites,stop=geometry.STOP,body_sites=OWNERS,listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest())
def cases(image):
 out=[]
 def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
 def make(name,seed=0,fraction=.5,seconds=1/30,entry=0x325810,changes=()):
  rng=random.Random(seed);raw=bytearray(256)
  for offset,bound in((0x20,1),(0x30,1),(0x70,10),(0xc0,4),(0xd0,2)):struct.pack_into('<4f',raw,offset,*[rng.uniform(-bound,bound)for _ in range(4)])
  struct.pack_into('<f',raw,0xc,.25);struct.pack_into('<f',raw,0x7c,(seed%9)*.125)
  for offset,value in changes:struct.pack_into('<I',raw,offset,value)
  n=len(out);gp=[0x5a100000+n*31+k for k in range(8)];gp[1]=MOTION;gp[4]=geometry.SP+(n%4)*4;depth=n%6 if entry==0x325810 else n%9;live=[struct.pack('<QH',0x8000000000000000+k*0x103030303,0x3fff+k).hex()for k in range(depth)];out.append(dict(id=n,name=name,entry=entry,gp=gp,arguments=[bits(fraction),bits(seconds)],banks=sorted([dict(address=MOTION,before=raw.hex()),*[dict(address=p,before=image.read(p,size).hex())for p,size in CONSTANTS]],key=lambda b:b['address']),before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*7%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*13%64)],live_x87=live,alignment_fault=False))
 for seed in range(8):
  for fraction in(0.,.25,.5,1.,-.125,1.5):make(f'closed_{seed}_{fraction}',seed,fraction)
 for control in range(9):make('closed_noop_'+str(control),control,entry=0x8fdb0)
 for precision in(0,2,3):
  for x87_round in range(4):
   for simd_round in range(4):
    for denormal in(0,0x40,0x8000,0x8040):
     make(f'control_{precision}_{x87_round}_{simd_round}_{denormal:x}',7,fraction=.3,seconds=.027,changes=((0x7c,bits(.7)),(0xc0,1),(0xd4,0x80000001)));out[-1]['before_fp']=[0x7f|(precision<<8)|(x87_round<<10),(out[-1]['before_fp'][1]&~0x3f),0x1f80|(simd_round<<13)|denormal]
 for offset in(0xc,0x20,0x2c,0x30,0x34,0x38,0x3c,0x70,0x7c,0xc0,0xcc,0xd0,0xdc):
  for value in(0,0x80000000,1,0x00800000,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
   make(f'edge_{offset:x}_{value:x}',5,changes=((offset,value),));out[-1]['before_fp'][1]&=~0x3f;out[-1]['before_fp'][2]&=~0x3f
 for argument in(0,1):
  for value in(0x80000000,1,0x00800000,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
   make(f'argument_edge_{argument}_{value:x}',3);out[-1]['arguments'][argument]=value;out[-1]['before_fp'][1]&=~0x3f;out[-1]['before_fp'][2]&=~0x3f
 for left,right in((0x30,0x34),(0x38,0x3c),(0xc0,0x70),(0xd0,0xd4),(0xd0,0x30),(0x2c,0x3c)):
  for first,second in((0x7fc00021,0xffc00341),(0x7fa00021,0xffc00341),(0x7fc00021,0xffa00341),(0x7fa00021,0xffa00341)):
   make(f'paired_nan_{left:x}_{right:x}_{first:x}_{second:x}',6,changes=((left,first),(right,second)));out[-1]['before_fp'][1]&=~0x3f;out[-1]['before_fp'][2]&=~0x3f
 make('fault_seed_normal',5,fraction=.3,seconds=.027,changes=((0x7c,bits(.7)),))
 make('fault_seed_nan',6,changes=((0x30,0x7fa00021),(0xd4,0xffa00341),(0xc0,0x7fc00567)))
 make('alignment_motion_plus4',4);out[-1]['gp'][1]+=4;out[-1]['alignment_fault']=True
 return out

def prepare(image,meta,scratch,extra_banks=()):
 owned=scratch/'physical-original';owned.mkdir();section=image.section('.text');(owned/'text.bin').write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(meta));assembly=[f'.code32\n.section .original,"ax"\n.incbin "{owned/"text.bin"}"','.section .bank,"aw"\n.balign 16\n.space 65536','.section .stack,"aw"\n.space 65536','.section .stop,"ax"\nint3','.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start'];sections=[f'.original 0x{section.va:x}:{{*(.original)}}',f'.bank 0x{MOTION:x}:{{*(.bank)}}',f'.stack 0x{geometry.STACK:x}:{{*(.stack)}}',f'.stop 0x{geometry.STOP:x}:{{*(.stop)}}','.bridge 0x70000000:{*(.bridge)}']
 for index,(address,size)in enumerate(CONSTANTS):
  path=owned/f'constant{index}.bin';path.write_bytes(image.read(address,size));assembly.append(f'.section .constant{index},"aw"\n.incbin "{path}"');sections.append(f'.constant{index} 0x{address:x}:{{*(.constant{index})}}')
 for index,(address,size)in enumerate(extra_banks):assembly.append(f'.section .actual{index},"aw"\n.space {size}');sections.append(f'.actual{index} 0x{address:x}:{{*(.actual{index})}}')
 (owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
 for command in(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_cached_motion_tu15.cpp'),'-o',str(owned/'observer')]):
  result=subprocess.run(command,text=True,capture_output=True);assert result.returncode==0,result.stderr
 return owned

def actual_inputs(image,archive_path,archive_sha,scratch,rows):
 assert archive_sha and sha(archive_path)==archive_sha;unpack=scratch/'before-input';unpack.mkdir()
 with tarfile.open(archive_path,'r:*')as archive:
  for item in archive.getmembers():
   target=(unpack/item.name).resolve();assert (target==unpack or unpack in target.parents)and(item.isfile()or item.isdir())
  archive.extractall(unpack,filter='data')
 paths=[p for p in unpack.rglob('capture.json')if json.loads(p.read_text()).get('entry')==0x3205b0];assert len(paths)==1;capture=paths[0];d=json.loads(capture.read_text());regions=sorted(d['regions'],key=lambda r:r['address']);addresses=[r['address']for r in regions];bank_hashes={}
 def read(p,n):
  r=regions[bisect.bisect_right(addresses,p)-1];assert r['address']<=p and p+n<=r['address']+r['size'];path=capture.parent/r['before_file']
  if r['address']not in bank_hashes:bank_hashes[r['address']]=dict(address=r['address'],bytes=r['size'],file=r['before_file'],sha256=sha(path))
  with path.open('rb')as stream:stream.seek(p-r['address']);raw=stream.read(n)
  assert len(raw)==n;return raw
 def word(p):return struct.unpack('<I',read(p,4))[0]
 sp=d['before']['gp'][4];config=word(sp+4);record=word(sp+8);first=word(word(word(record)+16)+60);second=word(word(word(record+4)+16)+60);assert word(word(first)+0x74)==0x325810 and word(word(second)+0x74)==0x8fdb0;seconds=word(config+16);extra=[];b=d['before'];fp=[b['x87_control_word'],b['x87_status'],b['mxcsr']];assert not b['x87_values']
 for receiver,entry in((first,0x325810),(second,0x8fdb0)):
  raw=read(receiver,256);extra.append((receiver,256))
  for fraction in((0.,.3,.5,1.)if entry==0x325810 else(0.,)):
   n=len(rows);gp=[0x5a200000+n*31+k for k in range(8)];gp[1]=receiver;gp[4]=geometry.SP+(n%4)*4;rows.append(dict(id=n,name=f'actual_before_{entry:x}_{fraction}',entry=entry,gp=gp,arguments=[struct.unpack('<I',struct.pack('<f',fraction))[0],seconds],banks=sorted([dict(address=receiver,before=raw.hex()),*[dict(address=p,before=image.read(p,size).hex())for p,size in CONSTANTS]],key=lambda b:b['address']),before_fp=fp,live_x87=[],alignment_fault=False))
 for address,size in CONSTANTS:assert read(address,size)==image.read(address,size),'Actual BEFORE constant differs from authenticated original'
 return extra,dict(archive=str(archive_path),archive_sha256=archive_sha,capture_metadata_sha256=sha(capture),before_only=True,captured_after_consumed=False,configuration=config,dynamic_motion=first,fixed_motion=second,read_bank_hashes=list(bank_hashes.values()))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--capture',type=Path);p.add_argument('--archive-sha256');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);inputs=cases(image);extra=();actual=None
 if a.capture:extra,actual=actual_inputs(image,a.capture,a.archive_sha256,scratch,inputs)
 owned=prepare(image,meta,scratch,extra);rows=geometry.physical(owned,inputs,'complete');prefix=[]
 for seed in(c for c in rows if c['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for index in range(len(events)):
    c=copy.deepcopy(seed);c.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(index),fault=dict(phase=phase,ordinal=index));prefix.append(c)
 rows+=geometry.physical(owned,prefix,'prefix');executed=set().union(*(set(c['executed_sites'])for c in rows));assert len(executed)==381;fixture=scratch/'cached-motion.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,body_sites=381,cases=rows),separators=(',',':')));project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_cached_motion.cpp';text=source.read_text();(project/'native.cpp').write_text(text);variants={}
 for name,old,new in(('rewind','negate();const auto rewind=keep();','const auto rewind=keep();'),('half','const auto scaled_angular=mul_ps(broadcast(0x3f000000u),next_angular);','const auto scaled_angular=mul_ps(broadcast(0x3e800000u),next_angular);'),('reread','angular_for_half=simd(s.angular_velocity());','angular_for_half=angular;'),('rsqrt','const auto estimate=_mm_rsqrt_ss(sum);','const auto estimate=_mm_div_ss(broadcast(0x3f800000u),_mm_sqrt_ss(sum));'),('time_spill','multiply_second();s.angular_travel();','multiply(rewind);s.angular_travel();')):
  assert text.count(old)==1;variants[name]=text.replace(old,new);(project/(name+'.cpp')).write_text(variants[name])
 (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(s in r.stderr for s in("cached motion ordered store differs","cached motion retained bytes differ","cached motion complete FP differs","cached motion read differs","native RSQRTSS estimate differs")),(r.returncode,r.stdout,r.stderr)\nprint("Physical original rejects cached-motion mutation:",r.stderr.strip())\n');cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_cached_motion LANGUAGES CXX)
include(CTest)
add_library(cached_driver OBJECT "{ROOT}/tests/native_tu15_cached_motion_tests.cpp")
add_library(cached_noop OBJECT "{ROOT}/src/physics/native_tu15_kinematic_motion.cpp")
foreach(target IN ITEMS cached_driver cached_noop)
target_include_directories(${{target}} PRIVATE "{ROOT}/include")
target_compile_features(${{target}} PRIVATE cxx_std_20)
target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
'''
 for name in('native',*variants):
  target=name+'_cached';cmake+=f'add_executable({target} $<TARGET_OBJECTS:cached_driver> $<TARGET_OBJECTS:cached_noop> "{project/(name+".cpp")}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';cmd=f'$<TARGET_FILE:{target}> "{fixture}"'if name=='native'else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}"';cmake+=f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {cmd})\n'
 (project/'CMakeLists.txt').write_text(cmake);results=[]
 for compiler in('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target',name+'_cached']for name in('native',*variants)),[]),'--filter','_cached$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 files=[source,ROOT/'include/halo2/native_tu15_cached_motion.hpp',ROOT/'src/physics/native_tu15_kinematic_motion.cpp',ROOT/'include/halo2/native_tu15_kinematic_motion.hpp',ROOT/'tests/native_tu15_cached_motion_tests.cpp',ROOT/'tools/observe_native_cached_motion_tu15.cpp',ROOT/'tools/validate_native_geometry_tu15.py',Path(__file__)];report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_sha256=NEW_PIN,body_sites={hex(p):n for p,n in OWNERS.items()},listing_sha256=meta['listing_sha256'],original_cases=len(rows),complete_original_calls=sum(c["completed"]for c in rows),executed_source_sites=len(executed),read_fault_prefixes=sum(c.get("fault",{}).get("phase")=="before_read"for c in rows),rejected_store_prefixes=sum(c.get("fault",{}).get("phase")=="before_write"for c in rows),completed_observer_prefixes=sum(c.get("fault",{}).get("phase")=="after_write"for c in rows),alignment_cases=sum(c.get("alignment_fault",False)for c in rows),native_rsqrt_observations=sum(len(c["rsqrt_estimates"])for c in rows),actual_before_inputs=actual,negative_controls=list(variants),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),source_hashes={str(p.relative_to(ROOT)):sha(project/'native.cpp'if p==source else p)for p in files},owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()),retained_large_artifacts=[],remaining=['Native3205 plus native325810 +actual-agent composition and world activation','Private transient exceptional x87 payload','Unmasked platform exceptions','Original Xbox/PentiumIII RSQRT bit identity beyond this host','World activation']);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
