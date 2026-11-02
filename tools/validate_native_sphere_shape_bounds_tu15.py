#!/usr/bin/env python3
"""Native sphere bounds versus the unchanged20-site English TU1.5 owner.

All original operands, aliases, FP modes and failure prefixes are independent
inputs. Native arithmetic never receives original outputs or estimate bits.
All generated output belongs to run_test.py scratch.
"""
import argparse,copy,hashlib,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_activation_tu15 as activation
import validate_native_geometry_tu15 as reference
ROOT=reference.ROOT
BASE,SHAPE,MATRIX,OUTPUT=0x1000000,0x1000100,0x1000200,0x1000300
DRIVER=r'''
int main(int argc,char**argv){try{
 check(argc==2,"Original sphere fixture required");std::ifstream input(argv[1]);const auto f=J::parse(input);
 check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("body_sites")==20,"Sphere source authority changed");std::size_t cases{},reads{},writes{};
 for(const auto&test:f.at("cases")){
  const auto name=test.at("name").get<std::string>();Memory before;for(const auto&b:test.at("banks"))before.banks.emplace(b.at("address"),bytes(b.at("before")));const auto fault=test.value("fault",J::object());before.phase=fault.value("phase",std::string{});before.ordinal=fault.value("ordinal",0u);
  for(bool reading:{false,true}){if(reading&&before.phase=="after_write")continue;auto m=before;m.logging=reading;m.raw_logging=!reading&&(before.phase=="before_read"||before.phase=="before_write");std::vector<SourcePhysicsContactRecordStore>stores;unsigned observed{};SourcePhysicsContactRecordObserver observer;if(!reading&&!m.raw_logging)observer=[&](const auto&s){stores.push_back(s);if(before.phase=="after_write"&&observed++==before.ordinal)throw Prefix{};};
   NativeTu15PhysicsFrameFp fp{test.at("before_fp")[0],test.at("before_fp")[1],test.at("before_fp")[2]};NativeTu15SphereBoundsStats stats;Host host;bool stopped{},alignment{};
   try{native_tu15_sphere_shape_bounds(m,{test.at("shape")},{test.at("matrix")},test.at("margin"),{test.at("output")},fp,test.at("live_x87").size(),observer,&stats);}
   catch(const Prefix&){stopped=true;}catch(const std::runtime_error&e){alignment=std::string(e.what())=="Native TU1.5 sphere bounds requires aligned vector storage";if(!alignment)throw;}
   host.verify();check(stopped==!before.phase.empty()&&alignment==test.value("alignment_fault",false),name+" sphere prefix/alignment differs");
   if(reading){J expected=J::array();for(const auto&r:test.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));check(m.reads==expected,name+" ordered sphere reads differ");reads+=m.reads.size();}
   else{if(m.raw_logging)stores=m.raw;const auto&expected=test.at("writes");check(stores.size()==expected.size(),name+" sphere store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=expected[n];check((m.raw_logging||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" sphere store differs");}writes+=stores.size();}
   for(const auto&b:test.at("banks"))check(m.banks.at(b.at("address"))==bytes(b.at("after")),name+" retained sphere bytes differ");const auto&after=test.at("after_fp");check(fp.control_word==after[0]&&fp.status==after[1]&&fp.mxcsr==after[2],name+" sphere FP differs native="+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+after.dump());
  }++cases;
 }std::cout<<cases<<" physical sphere cases/prefixes, "<<reads<<" ordered reads "<<writes<<" stores passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
def metadata(image):
 code=reference._Code(image,'.text');body=reference.complete_body(code,0x2e8d90);assert len(body.code)==20;names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];listing=[]
 for pc in sorted(body.code):
  i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
  for op in i.operands:
   if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
   assert not op.mem.segment;b,x=i.reg_name(op.mem.base),i.reg_name(op.mem.index);access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access
   ops.append(dict(base=names.get(b,-1),index=names.get(x,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
  sites.append(dict(instruction=pc,operands=ops))
 return dict(sites=sites,body_sites=20,listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),stop=reference.STOP)
def cases(image):
 rows=[]
 def make(name,radius=0x3f000000,margin=0x3dcccccd,center=None,output=OUTPUT,matrix=MATRIX,fp_mode=None,alignment=False):
  n=len(rows);rng=random.Random(18813+n);raw=bytearray(rng.getrandbits(8)for _ in range(0x4000));struct.pack_into('<I',raw,SHAPE-BASE+12,radius)
  center=center or[0x3f800000,0xc0000000,0x40400000,0x3f800000]
  for lane,value in enumerate(center):struct.pack_into('<I',raw,matrix-BASE+48+lane*4,value)
  depth=(0,1,3,7)[n%4];live=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[1]=SHAPE;gp[4]=reference.SP
  precision,x87round,sseround,denorm=fp_mode or((0,2,3)[n%3],n%4,(n//4)%4,(0,0x40,0x8000,0x8040)[n%4])
  rows.append(dict(id=n,name=name,entry=0x2e8d90,shape=SHAPE,matrix=matrix,margin=margin,output=output,arguments=[matrix,margin,output],gp=gp,before_fp=[0x7f|(precision<<8)|(x87round<<10),((n%8)<<11)|((n%4)<<8)|(n*7%64),0x1f80|(sseround<<13)|denorm|(n*11%64)],live_x87=live,banks=[dict(address=BASE,before=raw.hex()),dict(address=0x41d000,before=image.read(0x41d000,0x1000).hex()),dict(address=0x556000,before=bytes(0x1000).hex())],alignment_fault=alignment))
 for precision in(0,2,3):
  for xr in range(4):
   for sr in range(4):
    for d in(0,0x40,0x8000,0x8040):make(f'controls_{precision}_{xr}_{sr}_{d}',fp_mode=(precision,xr,sr,d))
 values=(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff854321)
 for value in values:
  make('radius_'+hex(value),radius=value);make('margin_'+hex(value),margin=value)
  for lane in range(4):
   center=[0x3f800000,0xc0000000,0x40400000,0x3f800000];center[lane]=value;make(f'center_{lane}_{value:x}',center=center)
 for a,b in((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
  make('paired_radius_margin_'+hex(a)+hex(b),radius=a,margin=b)
  make('paired_center_'+hex(a)+hex(b),radius=a,center=[b,b,b,b])
 for output in(SHAPE,MATRIX,MATRIX+16,MATRIX+32,MATRIX+48,MATRIX+64):make('alias_'+hex(output),output=output)
 for radius,margin in((0x3f800000,0xbf800000),(0x3f800001,0xbf800000),(0x4b800000,0x3f800000),(0x3eaaaaab,0xbeaaaaaa)):
  make('rounding_'+hex(radius)+hex(margin),radius=radius,margin=margin)
 make('fault_seed_plain');make('fault_seed_center_alias',output=MATRIX+48)
 make('alignment_matrix',matrix=MATRIX+4,alignment=True);make('alignment_output',output=OUTPUT+4,alignment=True)
 return rows
def prepare(image,meta,scratch):
 owned=activation.prepare(image,meta,scratch);p=owned/'observer.cpp';text=p.read_text()
 old='check(fp.cwd==entry_fp.cwd&&fp.swd==entry_fp.swd&&fp.ftw==entry_fp.ftw&&!std::memcmp(fp.st_space,entry_fp.st_space,sizeof(fp.st_space)),"Original geometry unexpectedly changed live x87 values/status");'
 new='check(fp.cwd==entry_fp.cwd&&((fp.swd^entry_fp.swd)&0x3800u)==0&&fp.ftw==entry_fp.ftw,"Original sphere changed incoming x87 control/TOP/tags");for(unsigned n=0;n<depth;++n)check(!std::memcmp(reinterpret_cast<const std::byte*>(fp.st_space)+n*16,reinterpret_cast<const std::byte*>(entry_fp.st_space)+n*16,10),"Original sphere changed an incoming live x87 payload");'
 assert text.count(old)==1;text=text.replace(old,new);p.write_text(text)
 subprocess.run(['c++','-std=c++20','-O2',str(p),'-o',str(owned/'observer')],check=True,capture_output=True)
 return owned
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=ROOT/'src/physics/native_tu15_sphere_shape_bounds.cpp';files=[source,ROOT/'include/halo2/native_tu15_sphere_shape_bounds.hpp',ROOT/'tools/validate_native_island_activation_tu15.py',ROOT/'tools/validate_native_geometry_tu15.py',ROOT/'tests/native_tu15_shape_bounds_tests.cpp',ROOT/'tools/observe_native_geometry_tu15.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):reference.sha(f)for f in files};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);meta=metadata(image);owned=prepare(image,meta,scratch);all_cases=cases(image);excluded_alignment=[c for c in all_cases if c['name']=='alignment_matrix'];rows=reference.physical(owned,[c for c in all_cases if c['name']!='alignment_matrix'],'complete');faults=[];excluded_faults=[]
  for seed in[c for c in rows if c['name'].startswith('fault_seed')]:
   for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n,event in enumerate(events):
     if phase=='before_read'and event['instruction']in(0x2e8d9f,0x2e8da2):
      excluded_faults.append(dict(seed=seed['name'],instruction=event['instruction'],reason='Original holds a private x87 value at this rejected read; transient private payload ownership remains open.'));continue
     c=copy.deepcopy(seed);c.update(id=len(rows)+len(faults),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));faults.append(c)
  rows+=reference.physical(owned,faults,'prefix');fixture=scratch/'sphere.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,body_sites=20,cases=rows),separators=(',',':')));project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_sphere_shape_bounds.hpp"\n'+prefix+DRIVER);shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={}
  for name,needle,replacement in(('wrong_lower','subtract(lower_center,expansion)','add(lower_center,expansion)'),('stale_upper','const auto upper_center=m.vector(transform.identity+48u);','const auto upper_center=lower_center;'),('wrong_radius','m.word(shape.identity+12u)','m.word(shape.identity+8u)')):
   assert text.count(needle)==1,(name,text.count(needle));variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.returncode,r.stdout,r.stderr)\nprint("Original rejects sphere mutation:",r.stderr.strip()[:300])\n');cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeSphereBounds LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'+f'target_include_directories(driver PRIVATE "{ROOT}/include")\n'
  for name in('native',*variants):
   cmake+=f'add_executable(sphere_{name} $<TARGET_OBJECTS:driver> {name}.cpp)\ntarget_include_directories(sphere_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(sphere_{name} PRIVATE cxx_std_20)\ntarget_compile_options(sphere_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:sphere_{name}> "{fixture}"';cmake+=f'add_test(NAME sphere_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','sphere_native',*sum((['--target','sphere_'+n]for n in variants),[]),'--filter','^sphere_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(reference.sha(ROOT/f)==v for f,v in pins.items()),compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),body_sites=20,listing_sha256=meta['listing_sha256'],ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),negative_controls=list(variants),excluded_private_x87_read_prefixes=excluded_faults,excluded_private_x87_alignment_cases=[c['name']for c in excluded_alignment],image_sha256=reference.NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],remaining=['Full live world activation','Unmasked platform traps and private transient x87 payloads'])
 except BaseException as error:report['error']=str(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
