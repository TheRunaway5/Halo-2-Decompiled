#!/usr/bin/env python3
"""All-known native shape bounds chains against unchanged original TU1.5.

One disposable physical ELF serves complete chain and read/store/observer
prefixes. Native services recursively invoke native known children, with actual
stack-derived transformed scratch lifetime; no child outcome is supplied.
"""
import argparse,copy,difflib,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import validate_native_geometry_tu15 as geometry
BASE,FORWARD,TRANSFORMED,CONVEX,MOTION,MATRIX,OUTPUT,BODY,VELOCITY,OBJECT=0x1000000,0x1000100,0x1000200,0x1000300,0x1000400,0x1000500,0x1000600,0x1000800,0x1000a00,0x1000b00
CENTERED=False
ENTRIES=(0x306690,0x306760,0x2e8870,0x2e9600,0x2e5e80)
TABLES={0x461e98:0x2e9600,0x4614b0:0x2e8870,0x462090:0x306760,0x41c3d0:0x2e5e80}
CONSTANTS=((0x461950,32),*[(p,0x1c)for p in TABLES])
PINS={'src/physics/native_tu15_shape_bounds.cpp':'117d1459b133857217fe6ea61551e2fd7e061f3e698faf404db01300a558b217','include/halo2/native_tu15_shape_bounds.hpp':'af451b0c502a0b4dd2ec8cb962a3005378c4fffdf1867eafaed0bc6dacb3d2b4'}
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def metadata(image):
 code=_Code(image,'.text');names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];hashes={}
 for entry in(*ENTRIES,0x2e44d0,*((0x1d64c0,0x307e30)if CENTERED else())):
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment;access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access;assert access&(CS_AC_READ|CS_AC_WRITE);b,idx=i.reg_name(op.mem.base),i.reg_name(op.mem.index);ops.append(dict(base=names[b]if b else-1,index=names[idx]if idx else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,bytes=i.bytes.hex(),operands=ops))
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==([48,13,25,3,48,40,61,15]if CENTERED else[48,13,25,3,48,40]);return dict(sites=sites,stop=geometry.STOP,body_sites=counts,listing_sha256=hashes)
def cases(image):
 out=[]
 def make(entry,name,changes=(),destination=OUTPUT,matrix=MATRIX,mode=2,rounding=None,denorm=None):
  n=len(out);rng=random.Random(971+n);raw=bytearray().join(struct.pack('<f',rng.uniform(-4,4))for _ in range(0x4000//4));sp=geometry.SP+(n%4)*4
  def put(p,v):struct.pack_into('<I',raw,p-BASE,v)
  for p,v in((FORWARD,0x461e98),(FORWARD+0xc,TRANSFORMED),(TRANSFORMED,0x4614b0),(TRANSFORMED+0xc,CONVEX),(CONVEX,0x462090),(MOTION,0x41c3d0),(MOTION+0x30,FORWARD),(MOTION+0x34,BODY),(BODY+8,OBJECT),(BODY+0x3c,VELOCITY),(OBJECT+0x150,0x3f800000)):
   put(p,v)
  if mode==0:put(MOTION+0x34,0)
  elif mode==1:put(BODY+8,0)
  for p,bits in changes:put(p,bits)
  shape={0x306690:CONVEX+16,0x306760:CONVEX,0x2e8870:TRANSFORMED,0x2e9600:FORWARD,0x2e5e80:MOTION}[entry];margin=(0,0x80000000,0x3dcccccd,0xbe800000)[n%4];banks={BASE:raw}
  for p,size in CONSTANTS:banks[p]=bytearray(image.read(p,size))
  if entry in(0x2e8870,0x2e9600,0x2e5e80):
   transformed_sp=sp if entry!=0x2e5e80 else((sp-4)&~15)-0x20;scratch=((transformed_sp-4)&~15)-0x40;banks[scratch]=bytearray(rng.getrandbits(8)for _ in range(64))
   if matrix==-1:matrix=scratch;bank=banks[scratch];bank[:]=raw[MATRIX-BASE:MATRIX-BASE+64]
  depth=(0,1,3,7)[n%4];live=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)];gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[1]=shape;gp[4]=sp;args=[matrix,shape,margin,destination]if entry==0x306690 else[matrix,margin,destination];r=n%4 if rounding is None else rounding;d=(0,0x40,0x8000,0x8040)[n%4]if denorm is None else denorm
  out.append(dict(id=n,name=name,entry=entry,shape=shape,transform=matrix,margin_bits=margin,output=destination,gp=gp,arguments=args,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*13%64),0x1f80|(r<<13)|d|(n*7%64)],live_x87=live,alignment_fault=False))
 for entry in ENTRIES:
  for r in range(4):
   for d in(0,0x40,0x8000,0x8040):make(entry,f'finite_{entry:x}_{r}_{d:x}',rounding=r,denorm=d)
  for first,second in((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
   for lane in range(4):
    for a,b in((MATRIX+lane*4,MATRIX+16+lane*4),(MATRIX+32+lane*4,CONVEX+16+8),(MATRIX+48+lane*4,VELOCITY+0x40+lane*4)):
     make(entry,f'paired_nan_{entry:x}_{first:x}_{lane}_{a:x}',changes=((a,first),(b,second)))
  for destination in(MATRIX,MATRIX+16,MATRIX+32,MATRIX+48,CONVEX+16,VELOCITY+0x30,VELOCITY+0x40):make(entry,f'alias_{entry:x}_{destination:x}',destination=destination)
  make(entry,f'fault_seed_{entry:x}');make(entry,f'fault_seed_alias_{entry:x}',destination=MATRIX+32)
  make(entry,f'alignment_{entry:x}',matrix=MATRIX+4);out[-1]['alignment_fault']=True
 for entry in ENTRIES[1:]:
  for left,right in((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
   make(entry,f'radius_margin_pair_{entry:x}_{left:x}_{right:x}',changes=((CONVEX+12,right),));out[-1]['margin_bits']=left;out[-1]['arguments'][1]=left
 for entry in ENTRIES[2:]:make(entry,f'scratch_matrix_alias_{entry:x}',matrix=-1)
 for mode in(0,1):make(0x2e5e80,f'motion_null_path_{mode}',mode=mode)
 for value in(0x7fc12345,0x7f812345,0x80000000,1,0xff800000):make(0x2e5e80,f'motion_minmax_{value:x}',changes=((VELOCITY+0x40,value),(VELOCITY+0x44,value),(VELOCITY+0x48,value),(VELOCITY+0x4c,value)))
 # Explicit non-associative float boundary for the actual projected extent.
 changes=[(MATRIX+n*16+lane*4,0)for n in range(4)for lane in range(4)]
 changes +=[(MATRIX+lane*4,0x4c000000)for lane in range(4)]+[(MATRIX+16+lane*4,0x40000000)for lane in range(4)]
 changes +=[(CONVEX+16+lane*4,0x3f800000 if lane<2 else 0)for lane in range(4)]+[(CONVEX+32+lane*4,0)for lane in range(4)]
 make(0x306690,'extent_addition_association_boundary',changes=changes,rounding=0,denorm=0);out[-1]['margin_bits']=0xcc000000;out[-1]['arguments'][2]=0xcc000000
 if CENTERED:
  original=list(out)
  for seed in original:
   if seed['entry']not in(0x306690,0x306760,0x2e5e80):continue
   c=copy.deepcopy(seed);c['id']=len(out);c['name']='centered_'+c['name']
   if c['entry']==0x306690:c.update(entry=0x1d64c0,center=CONVEX+32);c['arguments']=[c['transform'],c['shape'],CONVEX+32,c['margin_bits'],c['output']]
   elif c['entry']==0x306760:c['entry']=0x307e30
   else:
    bank=next(b for b in c['banks']if b['address']==BASE);raw=bytearray.fromhex(bank['before']);struct.pack_into('<I',raw,MOTION+0x30-BASE,CONVEX);bank['before']=raw.hex()
   if c['entry']==0x2e5e80:
    private=[b for b in c['banks']if geometry.STACK<=b['address']<geometry.STACK+0x10000]
    if any(b['address']<=c['transform']<b['address']+len(bytes.fromhex(b['before']))for b in private):continue
    c['banks']=[b for b in c['banks']if b not in private]
   if c['entry']!=0x1d64c0:
    bank=next(b for b in c['banks']if b['address']==BASE);raw=bytearray.fromhex(bank['before']);struct.pack_into('<I',raw,CONVEX-BASE,0x461fe0);bank['before']=raw.hex()
   out.append(c)
 return out

def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta));assembly=[f'.code32\n.section .original,"ax"\n.incbin "{p/"text.bin"}"', '.section .bank,"aw"\n.balign 16\n.space 16384', '.section .stack,"aw"\n.space 65536','.section .stop,"ax"\nint3','.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start'];sections=[f'.original 0x{section.va:x}:{{*(.original)}}',f'.bank 0x{BASE:x}:{{*(.bank)}}',f'.stack 0x{geometry.STACK:x}:{{*(.stack)}}',f'.stop 0x{geometry.STOP:x}:{{*(.stop)}}','.bridge 0x70000000:{*(.bridge)}']
 for n,(address,size)in enumerate(CONSTANTS):
  data=p/f'constant{n}.bin';data.write_bytes(image.read(address,size));assembly.append(f'.section .constant{n},"aw"\n.incbin "{data}"');sections.append(f'.constant{n} 0x{address:x}:{{*(.constant{n})}}')
 (p/'reference.s').write_text('\n'.join(assembly));(p/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n');observer=(ROOT/'tools/observe_native_geometry_tu15.cpp').read_text();old='std::vector<std::byte>stack(12);const std::array<std::uint32_t,3>args{stop,c.at("matrix"),c.at("source")};for(unsigned n=0;n<3;++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(args[n]>>(k*8u));';new='std::vector<std::uint32_t>args{stop};for(const auto&value:c.at("arguments"))args.push_back(value);std::vector<std::byte>stack(args.size()*4);for(unsigned n=0;n<args.size();++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(args[n]>>(k*8u));';assert observer.count(old)==1;observer=observer.replace(old,new);old='check(fp.cwd==entry_fp.cwd&&fp.swd==entry_fp.swd&&fp.ftw==entry_fp.ftw&&!std::memcmp(fp.st_space,entry_fp.st_space,sizeof(fp.st_space)),"Original geometry unexpectedly changed live x87 values/status");';new='check(fp.cwd==entry_fp.cwd&&(fp.swd&0x3800)==(entry_fp.swd&0x3800)&&fp.ftw==entry_fp.ftw,"Original shape did not preserve balanced live x87 ownership");for(unsigned n=0;n<depth;++n)check(!std::memcmp(reinterpret_cast<const std::byte*>(fp.st_space)+n*16,reinterpret_cast<const std::byte*>(entry_fp.st_space)+n*16,10),"Original shape altered a genuine incoming x87 value");';assert observer.count(old)==1;observer=observer.replace(old,new);(p/'observer.cpp').write_text(observer)
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(p/'observer.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(command,capture_output=True,text=True);assert r.returncode==0,r.stderr
 return p

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--centered',action='store_true');a=p.parse_args();global CENTERED,CONSTANTS;CENTERED=a.centered
 if CENTERED:CONSTANTS=(*CONSTANTS,(0x461fe0,0x1c));PINS.update({'src/physics/native_tu15_centered_shape_bounds.cpp':'f87bb2fbe3b84a8b97ac25e0a381e65c6de597372ace8bcd00394370b6660014','include/halo2/native_tu15_centered_shape_bounds.hpp':'217a37e0e976a8f92f7c3162f06c24d05612e8e455146c2a868d5b32af765563'})
 a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;assert all(sha(ROOT/p)==v for p,v in PINS.items());image=Xbe(NEW_DEFAULT);meta=metadata(image);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,cases(image),'complete');prefix=[];excluded_radius_faults=[]
 for seed in(c for c in rows if c['name'].startswith(('fault_seed','centered_fault_seed'))):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for index,event in enumerate(events):
    if phase=='before_read'and event['instruction']in(0x306768,0x307e38):excluded_radius_faults.append(dict(seed=seed['name'],instruction=event['instruction'],reason='actual margin remains a private live x87 intermediate; typed FP API does not transport that value on failure'));continue
    c=copy.deepcopy(seed);c.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(index),fault=dict(phase=phase,ordinal=index));prefix.append(c)
 rows+=geometry.physical(owned,prefix,'prefix');fixture=scratch/'shape.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')));project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_shape_bounds.cpp';text=source.read_text();(project/'native.cpp').write_text(text);assert text.count('maximum(add(upper,delta),upper)')==1;(project/'mutated.cpp').write_text(text.replace('maximum(add(upper,delta),upper)','maximum(upper,add(upper,delta))'));assert text.count('const auto total=add(add(x_extent,y_extent),z);')==1;(project/'association.cpp').write_text(text.replace('const auto total=add(add(x_extent,y_extent),z);','const auto total=add(x_extent,add(y_extent,z));'));assert text.count('const auto second_translation=m.vector(transform.identity+48u);')==1;(project/'translation.cpp').write_text(text.replace('const auto second_translation=m.vector(transform.identity+48u);','const auto second_translation=first_translation;'))
 if CENTERED:
  centered=(ROOT/'src/physics/native_tu15_centered_shape_bounds.cpp').read_text();assert centered.count('const auto dimensions=m.vector(half_extents);')==1 and centered.count('const auto local_center=m.vector(center);')==1;(project/'centered.cpp').write_text(centered.replace('const auto dimensions=m.vector(half_extents);','const auto cached_center=m.vector(center);const auto dimensions=m.vector(half_extents);').replace('const auto local_center=m.vector(center);','const auto local_center=cached_center;'))
 (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(s in r.stderr for s in("shape store differs","retained shape bytes differ","shape FP differs","ordered shape read differs")),(r.returncode,r.stdout,r.stderr)\nprint("Physical original rejects private arithmetic/order mutation:",r.stderr.strip())\n');cmake='cmake_minimum_required(VERSION 3.20)\nproject(shape_bounds LANGUAGES CXX)\ninclude(CTest)\n'
 cmake+=f'add_library(shape_driver OBJECT "{ROOT}/tests/native_tu15_shape_bounds_tests.cpp")\ntarget_include_directories(shape_driver PRIVATE "{ROOT}/include")\ntarget_compile_features(shape_driver PRIVATE cxx_std_20)\ntarget_compile_options(shape_driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
 if CENTERED:cmake+='target_compile_definitions(shape_driver PRIVATE HALO2_SHAPE_CENTERED=1)\n'
 variants=[('native_shape','native.cpp'),('mutated_shape','mutated.cpp'),('association_shape','association.cpp'),('translation_shape','translation.cpp')]
 if CENTERED:variants.append(('centered_shape','native.cpp'))
 for name,cpp in variants:
  centered_source=f' "{project/"centered.cpp"if name=="centered_shape"else ROOT/"src/physics/native_tu15_centered_shape_bounds.cpp"}"'if CENTERED else''
  cmake+=f'add_executable({name} $<TARGET_OBJECTS:shape_driver> "{project/cpp}" "{ROOT}/src/physics/native_tu15_geometry.cpp"{centered_source})\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';cmd=f'$<TARGET_FILE:{name}> "{fixture}"'if name=='native_shape'else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{name}> "{fixture}"';cmake+=f'add_test(NAME {name} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {cmd})\n'
 (project/'CMakeLists.txt').write_text(cmake);results=[]
 for compiler in('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target',name]for name,_ in variants),[]),'--filter','^(native|mutated|association|translation|centered)_shape$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 files=[source,ROOT/'include/halo2/native_tu15_shape_bounds.hpp',ROOT/'src/physics/native_tu15_geometry.cpp',ROOT/'include/halo2/native_tu15_geometry.hpp',ROOT/'tests/native_tu15_shape_bounds_tests.cpp',Path(__file__)];files.extend(ROOT/p for p in PINS if p not in {str(f.relative_to(ROOT))for f in files});temporary=[p for p in scratch.rglob('*')if p.is_file()];report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_sha256=NEW_PIN,body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),read_fault_prefixes=sum(c.get('fault',{}).get('phase')=='before_read'for c in rows),rejected_store_prefixes=sum(c.get('fault',{}).get('phase')=='before_write'for c in rows),completed_observer_prefixes=sum(c.get('fault',{}).get('phase')=='after_write'for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),native_child_outcomes_supplied=False,negative_controls=[name for name,_ in variants if name!='native_shape'],source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in temporary),retained_large_artifacts=[],excluded_radius_read_faults=excluded_radius_faults,excluded=['unmasked native FP traps','full live world activation','source radius-read transient x87/private exception owner'if CENTERED else'actual307E30 centered child not yet included']);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
