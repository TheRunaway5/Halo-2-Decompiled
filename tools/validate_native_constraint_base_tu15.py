#!/usr/bin/env python3
"""Physical unchanged329720 six-argument native base-row construction.

Declared retained data only; no builder cursor/Jacobian or physics outcome is
supplied. Products remain under run_test.py ownership and disposable CMake.
"""
import argparse,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import validate_native_geometry_tu15 as geometry
BASE,POINTS,RUNTIME,FRAME,INPUT,BUILDER,JACOBIAN,SCHEMA,FIRST,SECOND=0x1000000,0x1001000,0x1002000,0x1003000,0x1000400,0x1004000,0x1005000,0x1007000,0x1000100,0x1000200
CONSTANTS=((0x41f118,4),(0x41f64c,16),(0x461960,16),(0x46a0d0,4),(0x46a0e8,4),(0x46a178,32))
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def metadata(image):
 code=_Code(image,'.text');body=complete_body(code,0x329720);assert len(body.code)==921;names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];listing=[]
 for pc in sorted(body.code):
  i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
  for op in i.operands:
   if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
   assert not op.mem.segment;access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access;assert access&(CS_AC_READ|CS_AC_WRITE);b,idx=i.reg_name(op.mem.base),i.reg_name(op.mem.index);ops.append(dict(base=names[b]if b else-1,index=names[idx]if idx else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
  sites.append(dict(instruction=pc,bytes=i.bytes.hex(),operands=ops))
 return dict(sites=sites,stop=geometry.STOP,body_sites=921,listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest())
def cases(image):
 out=[]
 def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
 def make(name,count=1,positive=False,coupled=False,flag=0,normal_kind=0,basis_parallel=False,kind=1,spread=1.0,changes=()):
  n=len(out);raw=bytearray(0x10000)
  def word(p,v):struct.pack_into('<I',raw,p-BASE,v&0xffffffff)
  def floats(p,values):
   for k,v in enumerate(values):word(p+4*k,bits(v))
  word(INPUT,FIRST);word(INPUT+4,SECOND);word(BUILDER,JACOBIAN);word(BUILDER+4,SCHEMA)
  for body,mass in((FIRST,.5),(SECOND,.25)):
   word(body+0xc,kind);floats(body+0x30,(1,2,3,mass));floats(body+0x40,(0,0,0,0));floats(body+0x50,(1,0,0,0));floats(body+0x60,(0,1,0,0));floats(body+0x70,(0,0,1,0))
  floats(INPUT+0xc,(1.,.1,0.,.8,.5));word(FRAME,flag);floats(FRAME+4,(0.,.1,0.,-.2,0.,.05,.5));floats(FRAME+0x20,(0,0,1,0)if basis_parallel else(1,0,0,0));floats(FRAME+0x30,(0,1,0,0))
  for k in range(min(count,8)):
   floats(POINTS+k*32,(spread*k,.25,0.,-.1));normal=(0.,0.,1.,0.)
   if normal_kind==1:normal=(1.,0.,0.,0.)if k%2==0 else(0.,1.,0.,0.)
   elif normal_kind==2:normal=(1.,0.,0.,0.)if k%2==0 else(-1.,0.,0.,0.)
   elif normal_kind==3:normal=(0.,0.,0.,0.)
   floats(POINTS+k*32+16,normal);floats(RUNTIME+k*28,(1. if positive else 0.,0.,1. if positive else 0.,0.,0.,.02));word(RUNTIME+k*28+0x18,1 if coupled and k>0 else 0)
  for p,v in changes:word(p,v)
  # Retain only actual data ownership. The physical reference maps the wider
  # address bank, while every compared byte belongs to these declared spans.
  spans=((FIRST,128),(SECOND,128),(INPUT,32),(POINTS,512),(RUNTIME,512),(FRAME,512),(BUILDER,128),(JACOBIAN-64,1088),(SCHEMA,512))
  banks=[dict(address=p,before=raw[p-BASE:p-BASE+size].hex())for p,size in spans];banks.extend(dict(address=p,before=image.read(p,size).hex())for p,size in CONSTANTS);gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+(n%4)*4;live=[]if n%2==0 else[struct.pack('<QH',0xabcdef1234567890,0x3fff).hex()];out.append(dict(id=n,name=name,entry=0x329720,gp=gp,arguments=[POINTS,count,RUNTIME,FRAME,INPUT,BUILDER],banks=sorted(banks,key=lambda b:b['address']),before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*13%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=live,alignment_fault=False))
 for count in(0,1,2,3,4,0xffffffff):
  for kind in(0,1):make(f'normal_count_{count:x}_kind_{kind}',count=count,kind=kind)
 for count in(1,2,3,4):
  for normal in range(4):
   for flag in(0,4):make(f'aggregate_{count}_{normal}_{flag}',count=count,positive=True,normal_kind=normal,flag=flag,kind=count%2,basis_parallel=bool(normal%2))
 for count in(2,3,4):make(f'coupled_{count}',count=count,positive=True,coupled=True)
 make('radius_early_pair_exit',count=3,positive=True,flag=4,spread=0.)
 for rounding in range(4):
  for precision in(0,2,3):
   make(f'control_{rounding}_{precision}',count=2,positive=True,coupled=True,normal_kind=1);out[-1]['before_fp'][0]=0x7f|(precision<<8)|(rounding<<10);out[-1]['before_fp'][2]=0x1f80|(rounding<<13)
 for rounding in range(4):
  for denormal in(0,0x40,0x8000,0x8040):
   make(f'simd_control_{rounding}_{denormal:x}',count=3,positive=True,coupled=True,normal_kind=2,changes=((FIRST+0x30,1),(SECOND+0x34,0x80000001)));out[-1]['before_fp'][2]=0x1f80|(rounding<<13)|denormal
 for label,address in(('points',POINTS),('frame',FRAME),('first_body',FIRST)):
  make('alias_jacobian_'+label,changes=((BUILDER,address),))
 make('alias_schema_frame',changes=((BUILDER+4,FRAME),))
 make('alias_both_bodies',count=3,positive=True,coupled=True,normal_kind=1,changes=((INPUT+4,FIRST),))
 make('alias_runtime_points');out[-1]['arguments'][2]=POINTS
 for first_nan,second_nan in((0x7fc00021,0x7fc00341),(0x7fa00021,0x7fa00341),(0xffc00021,0x7fc00341)):
  for field in(0x30,0x34,0x3c,0x40):make(f'paired_nan_{field:x}_{first_nan:x}',changes=((FIRST+field,first_nan),(SECOND+field,second_nan)))
 for lane in range(4):
  make(f'paired_nan_normal_prefix_{lane}',changes=((POINTS+16+lane*4,0x7fa00100+lane),(FIRST+0x40+lane*4,0x7fc00200+lane)))
  # A genuine completed store boundary stops before a NaN direction could
  # enter the original's unconstrained retained-axis repair loop.
  out[-1]['fault']=dict(phase='after_write',ordinal=13)
 for direction in range(3):
  values=((FIRST+0x50,bits(1e10)),(FIRST+0x54,bits(-.000003)),(FIRST+0x60,bits(-1e10)),(FIRST+0x64,bits(.000007)),(FIRST+0x70,bits(.3)),(FIRST+0x74,bits(.2)),(POINTS,bits(1.)),(POINTS+4,0),(POINTS+8,bits(1.)),(POINTS+16,0),(POINTS+20,bits(1.)),(POINTS+24,bits(1.)))
  make(f'basis_association_{direction}',count=3,kind=0,normal_kind=direction,changes=values)
 make('bias_input_bound',changes=((INPUT+0x10,bits(-1.)),))
 make('bias_zero_threshold',changes=((POINTS+12,0),(RUNTIME+0x14,0),(RUNTIME+4,0),(INPUT+0x10,0)))
 make('repair_smallest_z',positive=True,basis_parallel=True,changes=tuple((p,bits(v))for p,v in((POINTS+16,.7),(POINTS+20,.7),(POINTS+24,.1),(FRAME+0x20,.7),(FRAME+0x24,.7),(FRAME+0x28,.1))))
 make('signed_large_negative',count=0x80000001)
 for count in(0x7fffffff,0x80000000):
  make(f'signed_admission_{count:x}_prefix',count=count);out[-1]['fault']=dict(phase='after_write',ordinal=6)
 for name,kwargs in(('normal',{}),('coupled',dict(count=2,coupled=True)),('aggregate',dict(count=2,positive=True,normal_kind=1,flag=0)),('repair',dict(count=2,positive=True,basis_parallel=True,flag=4))):make('fault_seed_'+name,**kwargs)
 return out

def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta));assembly=[f'.code32\n.section .original,"ax"\n.incbin "{p/"text.bin"}"','.section .bank,"aw"\n.balign16\n.space65536','.section .stack,"aw"\n.space65536','.section .stop,"ax"\nint3','.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start'];sections=[f'.original 0x{section.va:x}:{{*(.original)}}',f'.bank 0x{BASE:x}:{{*(.bank)}}',f'.stack 0x{geometry.STACK:x}:{{*(.stack)}}',f'.stop 0x{geometry.STOP:x}:{{*(.stop)}}','.bridge 0x70000000:{*(.bridge)}']
 for n,(address,size)in enumerate(CONSTANTS):
  file=p/f'constant{n}.bin';file.write_bytes(image.read(address,size));assembly.append(f'.section .constant{n},"aw"\n.incbin "{file}"');sections.append(f'.constant{n} 0x{address:x}:{{*(.constant{n})}}')
 (p/'reference.s').write_text('\n'.join(assembly).replace('.balign16','.balign 16').replace('.space65536','.space 65536'));(p/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n');observer=(ROOT/'tools/observe_native_geometry_tu15.cpp').read_text();old='std::vector<std::byte>stack(12);const std::array<std::uint32_t,3>args{stop,c.at("matrix"),c.at("source")};for(unsigned n=0;n<3;++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(args[n]>>(k*8u));';new='std::vector<std::uint32_t>args{stop};for(const auto&value:c.at("arguments"))args.push_back(value);std::vector<std::byte>stack(args.size()*4);for(unsigned n=0;n<args.size();++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(args[n]>>(k*8u));';assert observer.count(old)==1;observer=observer.replace(old,new).replace('steps<1000','steps<20000');old='check(fp.cwd==entry_fp.cwd&&fp.swd==entry_fp.swd&&fp.ftw==entry_fp.ftw&&!std::memcmp(fp.st_space,entry_fp.st_space,sizeof(fp.st_space)),"Original geometry unexpectedly changed live x87 values/status");';new='const unsigned private_depth=(((entry_fp.swd>>11u)&7u)-((fp.swd>>11u)&7u))&7u;check(private_depth+depth<=8,"Original base private depth exceeds hardware capacity");if(finished)check(!private_depth&&fp.ftw==entry_fp.ftw,"Original base did not balance private stack");for(unsigned n=0;n<depth;++n)check(!std::memcmp(reinterpret_cast<const std::byte*>(fp.st_space)+(n+private_depth)*16,reinterpret_cast<const std::byte*>(entry_fp.st_space)+n*16,10),"Original base changed a genuine incoming value");';assert observer.count(old)==1;observer=observer.replace(old,new).replace('{"live_x87_unchanged",true}', '{"live_x87_unchanged",true},{"private_x87_depth",private_depth}');(p/'observer.cpp').write_text(observer)
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(p/'observer.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(command,capture_output=True,text=True);assert r.returncode==0,r.stderr
 return p

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,cases(image),'complete');prefix=[]
 for seed in(c for c in rows if c['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for index in range(len(events)):
    c=copy.deepcopy(seed);c.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(index),fault=dict(phase=phase,ordinal=index));prefix.append(c)
 rows+=geometry.physical(owned,prefix,'prefix');executed=set().union(*(set(c['executed_sites'])for c in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in executed];assert not uncovered,('Actual source sites still unexercised',list(map(hex,uncovered)));fixture=scratch/'base.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,body_sites=921,cases=rows),separators=(',',':')));project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_constraint_base.cpp';text=source.read_text();(project/'native.cpp').write_text(text);variants={}
 for name,old,new in(('metadata','0x11801u','0x11800u'),('cross','return sub(low,high);','return sub(high,low);'),('rsqrt','scale=scalar_mul(scale,correction);','scale=scale;'),('association','return add(add(x_term,y_term),z_term);','return add(x_term,add(y_term,z_term));')):
  assert text.count(old)==(2 if name=='association'else 1);variants[name]=text.replace(old,new);(project/(name+'.cpp')).write_text(variants[name])
 (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(s in r.stderr for s in("base ordered store differs","base retained bytes differ","base complete FP differs","base read differs")),(r.returncode,r.stdout,r.stderr)\nprint("Physical original rejects private arithmetic/order mutation:",r.stderr.strip())\n');cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_base LANGUAGES CXX)
include(CTest)
add_library(base_driver OBJECT "{ROOT}/tests/native_tu15_constraint_base_tests.cpp")
target_include_directories(base_driver PRIVATE "{ROOT}/include")
target_compile_features(base_driver PRIVATE cxx_std_20)
target_compile_options(base_driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
 for name in('native',*variants):
  target=name+'_base';cmake+=f'add_executable({target} $<TARGET_OBJECTS:base_driver> "{project/(name+".cpp")}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';cmd=f'$<TARGET_FILE:{target}> "{fixture}"'if name=='native'else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}"';cmake+=f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {cmd})\n'
 (project/'CMakeLists.txt').write_text(cmake);results=[]
 for compiler in('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target',name+'_base']for name in('native',*variants)),[]),'--filter','_base$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 files=[source,ROOT/'include/halo2/native_tu15_constraint_base.hpp',ROOT/'include/halo2/native_tu15_constraint_producer.hpp',ROOT/'tests/native_tu15_constraint_base_tests.cpp',Path(__file__)];temporary=[p for p in scratch.rglob('*')if p.is_file()];report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_sha256=NEW_PIN,body_sites=921,executed_source_sites=len(executed),uncovered_source_sites=uncovered,listing_sha256=meta['listing_sha256'],original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),read_fault_prefixes=sum(c.get('fault',{}).get('phase')=='before_read'for c in rows),rejected_store_prefixes=sum(c.get('fault',{}).get('phase')=='before_write'for c in rows),completed_observer_prefixes=sum(c.get('fault',{}).get('phase')=='after_write'for c in rows),faults_with_private_x87_intermediates=sum(bool(c.get('private_x87_depth',0))for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),negative_controls=list(variants),source_hashes={str(p.relative_to(ROOT)):sha(project/'native.cpp'if p==source else p)for p in files},owned_scratch=str(scratch),fixture_bytes=fixture.stat().st_size,temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in temporary),retained_large_artifacts=[],remaining=['Private live x87 temporary payload on exceptional return is not transported by this typed API; controls/status and borrowed inputs are checked','Unmasked FP hardware/platform exception owner','Caller GP/private control-frame aliases are outside ordinary immutable six-value API','Actual producer wrapper/full native composition and live activation']);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
