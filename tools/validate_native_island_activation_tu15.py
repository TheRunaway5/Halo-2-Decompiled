#!/usr/bin/env python3
"""Unchanged TU1.5 activation/notification/reference-prefix qualification.

Default callbacks execute their authentic RET4 body. Other callbacks and array
expansion stop at the actual owner entry; they supply no result. All private
compiler products, original ELF/banks and fixtures share run_test.py scratch.
"""
import argparse,copy,hashlib,json,os,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_geometry_tu15 as reference
ROOT=reference.ROOT
BASE,WORLD,ISLAND,BODY=0x1000000,0x1000100,0x1000500,0x1001800
ACTIVE,INACTIVE,WL,BODIES,BL,LISTENER=BASE+0x1000,BASE+0x1100,BASE+0x1200,BASE+0x1300,BASE+0x2000,BASE+0x2800
DEFAULT_TABLE,CUSTOM_TABLE=0x41d958,BASE+0x3800
ENTRIES=(0x316d40,0x3165d0,0x315cb0,0x322660,0x283450,0x2e4d60)
DRIVER=r'''
struct OwnerStop{};
int main(int argc,char**argv){try{
 check(argc==2,"Original activation fixture required");std::ifstream input(argv[1]);const auto f=J::parse(input);check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("body_sites")==J::array({45,57,18,98,37,1}),"Activation source authority changed");std::size_t cases{},reads{},writes{};
 for(const auto&test:f.at("cases")){
  const auto name=test.at("name").get<std::string>();Memory before;for(const auto&b:test.at("banks"))before.banks.emplace(b.at("address"),bytes(b.at("before")));const auto fault=test.value("fault",J::object());before.phase=fault.value("phase",std::string{});before.ordinal=fault.value("ordinal",0u);
  for(bool reading:{false,true}){if(reading&&before.phase=="after_write")continue;auto m=before;m.logging=reading;m.raw_logging=!reading&&(before.phase=="before_read"||before.phase=="before_write");std::vector<SourcePhysicsContactRecordStore>stores;unsigned observed{};SourcePhysicsContactRecordObserver observer;if(!reading&&!m.raw_logging)observer=[&](const auto&s){stores.push_back(s);if(before.phase=="after_write"&&observed++==before.ordinal)throw Prefix{};};
   NativeTu15PhysicsFrameFp fp{test.at("before_fp")[0],test.at("before_fp")[1],test.at("before_fp")[2]};NativeTu15IslandActivationStats stats;NativeTu15IslandActivationServices services;J required=nullptr;
   const auto stop=[&](std::uint32_t owner,std::uint32_t receiver,const std::vector<std::uint32_t>&args){required=J{{"owner",owner},{"receiver",receiver},{"arguments",args}};throw OwnerStop{};};
   services.grow_array=[&](auto header,auto stride,auto&){stop(0x2e3250,0,{header.identity,stride});};
   services.island_activated=[&](auto listener,auto owner,auto island,auto&){stop(owner.identity,listener.identity,{island.identity});};
   services.body_activated=[&](auto listener,auto owner,auto body,auto&){stop(owner.identity,listener.identity,{body.identity});};
   services.destroy_body=[&](auto body,auto owner,auto disposing,auto&){stop(owner.identity,body.identity,{disposing});};
   Host host;bool prefix{},owner_stop{},predicate{};try{const auto entry=test.at("entry").get<std::uint32_t>();const NativeTu15PhysicsWorld world{test.at("world")};const NativeTu15PhysicsIsland island{test.at("island")};const NativeTu15PhysicsBody body{test.at("body")};switch(entry){
    case 0x316d40:native_tu15_activate_physics_island(m,world,island,fp,services,observer,&stats);break;
    case 0x3165d0:native_tu15_remove_deferred_physics_body(m,world,body,fp,services,observer,&stats);break;
    case 0x315cb0:native_tu15_activate_physics_body(m,body,fp,services,observer,&stats);break;
    case 0x322660:native_tu15_notify_island_activation(m,world,island,fp,services,observer,&stats);break;
    case 0x283450:predicate=native_tu15_reserved_body_reference(m,body);break;
    default:throw std::runtime_error("Unexpected actual activation entry");}
   }catch(const Prefix&){prefix=true;}catch(const OwnerStop&){owner_stop=true;}host.verify();
   check(prefix==!before.phase.empty(),name+" activation fault prefix differs");check(owner_stop==(!test.at("boundary").is_null()&&test.at("boundary").at("phase")=="required_owner"),name+" activation required-owner stop differs");
   if(owner_stop){const auto&boundary=test.at("boundary");check(required.at("owner")==boundary.at("owner")&&required.at("arguments")==boundary.at("arguments"),name+" actual activation service arguments differ");if(required.at("owner")!=0x2e3250)check(required.at("receiver")==boundary.at("receiver"),name+" actual activation receiver differs");}
   if(reading){J expected=J::array();for(const auto&r:test.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" ordered activation read differs at "+std::to_string(n)+" native="+(n<m.reads.size()?m.reads[n].dump():"end")+" original="+(n<expected.size()?expected[n].dump():"end"));}reads+=m.reads.size();}
   else{if(m.raw_logging)stores=m.raw;const auto&expected=test.at("writes");check(stores.size()==expected.size(),name+" ordered activation store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=expected[n];check((m.raw_logging||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered activation store differs at "+std::to_string(n));}writes+=stores.size();}
   for(const auto&b:test.at("banks"))check(m.banks.at(b.at("address"))==bytes(b.at("after")),name+" retained activation bytes differ");const auto&after=test.at("after_fp");check(fp.control_word==after[0]&&fp.status==after[1]&&fp.mxcsr==after[2],name+" activation FP differs");
   if(test.at("entry")==0x283450&&test.at("completed").get<bool>())check(predicate==((test.at("boundary_gp")[0].get<std::uint32_t>()&255u)!=0),name+" reserved-reference result differs");
  }++cases;
 }
 std::cout<<cases<<" physical activation cases/prefixes, "<<reads<<" ordered reads "<<writes<<" stores passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
def metadata(image):
 code=reference._Code(image,'.text');regs={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];hashes={}
 for entry in ENTRIES:
  body=reference.complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment;b,idx=i.reg_name(op.mem.base),i.reg_name(op.mem.index);ops.append(dict(base=regs[b]if b else-1,index=regs[idx]if idx else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,bytes=i.bytes.hex(),operands=ops))
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[45,57,18,98,37,1];return dict(sites=sites,body_sites=counts,listing_sha256=hashes,stop=reference.STOP)
def cases(image):
 out=[]
 def make(name,entry=0x316d40,active=1,inactive=3,index=1,capacity=8,wp=(),bp=(),body_count=2,deferred=(),refs=4,body_world=True,activated=False,body=BODY,enabled=1,reserved_bytes=0x1000):
  raw=bytearray(0x4000);globals=bytearray(0x1000)
  def put(p,v,n=4):raw[p-BASE:p-BASE+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
  put(WORLD+8,ACTIVE);put(WORLD+0xc,active);put(WORLD+0x10,capacity);put(WORLD+0x14,INACTIVE);put(WORLD+0x18,inactive)
  put(WORLD+0x78,WL);put(WORLD+0x7c,len(wp));put(ISLAND+0x24,index);put(ISLAND+0x29,activated,1);put(ISLAND+0x30,1,1);put(ISLAND+0x34,0x12345678);put(ISLAND+0x38,0xabcdef01);put(ISLAND+0x3c,BODIES);put(ISLAND+0x40,body_count)
  for k in range(8):put(INACTIVE+k*4,ISLAND if k==index else BASE+0x600+k*0x80);put(ACTIVE+k*4,BASE+0x3000+k*0x20)
  for k in range(10):put(LISTENER+k*0x10,DEFAULT_TABLE)
  put(CUSTOM_TABLE+4,0x316790);put(CUSTOM_TABLE+8,0x3167a0);put(CUSTOM_TABLE,0x316790)
  required=None
  for k,v in enumerate(wp):
   target=LISTENER+k*0x10 if v else 0;put(WL+k*4,target)
   if v==2:put(target,CUSTOM_TABLE);required=dict(owner=0x316790,argument_count=1)
  for k in range(max(0,min(body_count,3))):
   target=BODY+k*0x100;put(BODIES+k*4,target);put(target+8,WORLD if body_world else 0);put(target+0x44,ISLAND);put(target+6,refs,2);put(target,CUSTOM_TABLE);put(target+0x70,BL+k*0x100);put(target+0x74,len(bp))
   for j,v in enumerate(bp):
    listener=LISTENER+(j+5)*0x10 if v else 0;put(BL+k*0x100+j*4,listener)
    if v==2:put(listener,CUSTOM_TABLE);required=dict(owner=0x3167a0,argument_count=1)
  put(WORLD+0x108,BASE+0x3500);put(WORLD+0x10c,len(deferred))
  for k,v in enumerate(deferred):put(BASE+0x3500+k*4,BODY if v else BODY+0x100)
  globals[0x588]=enabled;struct.pack_into('<I',globals,0x5ac,reserved_bytes)
  if entry in(0x316d40,0x315cb0)and active==(capacity&0x7fffffff)and not activated:required=dict(owner=0x2e3250,argument_count=2)
  if refs==1 and 1 in deferred:required=dict(owner=0x316790,argument_count=1)
  n=len(out);depth=(0,1,3,8)[n%4];live=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)];gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=reference.SP+(n%4)*4;gp[1]=WORLD if entry in(0x316d40,0x3165d0)else body;args=[ISLAND]if entry==0x316d40 else[body]if entry in(0x3165d0,0x283450)else[WORLD,ISLAND]if entry==0x322660 else[]
  c=dict(id=n,name=name,entry=entry,world=WORLD,island=ISLAND,body=body,gp=gp,arguments=args,banks=[dict(address=BASE,before=raw.hex()),dict(address=0x41d000,before=image.read(0x41d000,4096).hex()),dict(address=0x556000,before=globals.hex())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*13%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=live,alignment_fault=False)
  if required:c['required_owner']=required
  out.append(c)
 patterns=((),(0,),(1,),(0,1,0,1),(1,0,1,0),(1,1,1))
 for active in(0,1,3):
  for inactive in(1,3):
   for index in range(inactive):
    for k in range(6):make(f'island_{active}_{inactive}_{index}_{k}',active=active,inactive=inactive,index=index,wp=patterns[k],bp=patterns[(k+2)%6])
 for wi,wp in enumerate(patterns):
  for bi,bp in enumerate(patterns):
   for count in(0,1,3):make(f'notify_{wi}_{bi}_{count}',entry=0x322660,wp=wp,bp=bp,body_count=count)
 for entry in(0x3165d0,0x315cb0):
  for pattern in((),(0,),(1,),(1,1),(1,0,1),(0,1,0,1)):
   for refs in(2,4,0):make(f'deferred_{entry:x}_{len(out)}',entry=entry,deferred=pattern,refs=refs)
 make('duplicate_skip',entry=0x3165d0,deferred=(1,1),refs=8)
 for active in(0,1,3):make('growth_'+str(active),active=active,capacity=active)
 make('world_callback_prefix',entry=0x322660,wp=(2,));make('body_callback_prefix',entry=0x322660,bp=(2,))
 make('destroy_prefix',entry=0x3165d0,deferred=(1,),refs=1)
 make('null_world',entry=0x315cb0,body_world=False);make('already_active',entry=0x315cb0,activated=True);make('unindexed',entry=0x315cb0,index=0xffffffff)
 for enabled in(0,1,0x80):
  for size in(0,0x1000,0x7fffffff,0xffffffff):
   for body in(0,0x7fffffff,0x80060fff,0x80061000,0x80061001,0x80062000,0xffffffff):make(f'reserved_{enabled}_{size:x}_{body:x}',entry=0x283450,body=body,enabled=enabled,reserved_bytes=size)
 make('fault_seed_island',wp=(0,1,0,1),bp=(1,0,1));make('fault_seed_deferred',entry=0x3165d0,deferred=(1,1,0,1),refs=8);make('fault_seed_notify',entry=0x322660,wp=(0,1,0,1),bp=(1,0,1))
 return out
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta));(p/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{p/'text.bin'}"
.section .bank,"aw"
.space 16384
.section .table,"aw"
.space 4096
.section .globals,"aw"
.space 4096
.section .stack,"aw"
.space 65536
.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
 fninit
 int3
 jmp _start
''');(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} .bank 0x{BASE:x}:{{*(.bank)}} .table 0x41d000:{{*(.table)}} .globals 0x556000:{{*(.globals)}} .stack 0x{reference.STACK:x}:{{*(.stack)}} .stop 0x{reference.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
 observer=(ROOT/'tools/observe_native_geometry_tu15.cpp').read_text();old='std::vector<std::byte>stack(12);const std::array<std::uint32_t,3>args{stop,c.at("matrix"),c.at("source")};for(unsigned n=0;n<3;++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(args[n]>>(k*8u));';new='std::vector<std::uint32_t>args{stop};for(const auto&value:c.at("arguments"))args.push_back(value);std::vector<std::byte>stack(args.size()*4);for(unsigned n=0;n<args.size();++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(args[n]>>(k*8u));';assert observer.count(old)==1;observer=observer.replace(old,new)
 old='if(pc==stop){finished=true;break;}const auto it=sites.find(pc);';new='''if(pc==stop){finished=true;break;}if(c.contains("required_owner")&&pc==c.at("required_owner").at("owner")){J arguments=J::array();for(unsigned k=0;k<c.at("required_owner").at("argument_count").get<unsigned>();++k){const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u+k*4u,4);std::uint32_t v;std::memcpy(&v,raw.data(),4);arguments.push_back(v);}boundary=J{{"phase","required_owner"},{"owner",pc},{"receiver",std::uint32_t(regs.rcx)},{"arguments",arguments}};break;}const auto it=sites.find(pc);''';assert observer.count(old)==1;observer=observer.replace(old,new);(p/'observer.cpp').write_text(observer)
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(p/'observer.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(command,text=True,capture_output=True);assert r.returncode==0,r.stderr[-3000:]
 return p
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=ROOT/'src/physics/native_tu15_island_activation.cpp';header=ROOT/'include/halo2/native_tu15_island_activation.hpp';files=[source,header,ROOT/'tests/native_tu15_shape_bounds_tests.cpp',ROOT/'tools/observe_native_geometry_tu15.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):reference.sha(f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[])
 def save():(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 save()
 try:
  assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);meta=metadata(image);owned=prepare(image,meta,scratch);rows=reference.physical(owned,cases(image),'complete');prefix=[];excluded=[]
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n,event in enumerate(events):
     if phase=='before_write'and event['instruction']in(0x316d6e,0x316d8d,0x31663a):
      excluded.append(dict(seed=seed['name'],instruction=event['instruction'],address=event['address'],reason='Original probe stops before retiring the RMW instruction; this read/write storage API has already observed its data read before a rejected write. Native public state remains separately bounded; atomic read-observer rejection is open.'));continue
     c=copy.deepcopy(seed);c.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(c)
  rows+=reference.physical(owned,prefix,'prefix');fixture=scratch/'activation.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')))
  project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'test.cpp').write_text('#include "halo2/native_tu15_island_activation.hpp"\n'+prefix+DRIVER);shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={};needle='const auto new_index=m.word(world.identity+0xcu)-1u;';assert text.count(needle)==1;variants['wrong_index']=text.replace(needle,'const auto new_index=m.word(world.identity+0xcu);');needle='value=m.word(current+cursor*4u+4u)';assert text.count(needle)==1;variants['wrong_compaction']=text.replace(needle,'value=m.word(current+cursor*4u)');needle='if(less(body.identity,first))return false;';assert text.count(needle)==1;variants['wrong_range']=text.replace(needle,'if(!less(first,body.identity))return false;')
  for name,body in variants.items():(project/(name+'.cpp')).write_text(body)
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(s in r.stderr for s in("activation store differs","retained activation bytes differ","activation store count differs","reserved-reference result differs","ordered activation read differs")),(r.returncode,r.stdout,r.stderr)\nprint("Unchanged original rejects activation mutation:",r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeIslandActivation LANGUAGES CXX)\ninclude(CTest)\n';cmake+=f'add_library(driver OBJECT test.cpp)\ntarget_include_directories(driver PRIVATE "{ROOT}/include")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for name in('native',*variants):
   cmake+=f'add_executable(activation_{name} $<TARGET_OBJECTS:driver> {name}.cpp)\ntarget_include_directories(activation_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(activation_{name} PRIVATE cxx_std_20)\n';command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:activation_{name}> "{fixture}"';cmake+=f'add_test(NAME activation_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','activation_native',*sum((['--target','activation_'+n]for n in variants),[]),'--filter','^activation_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(reference.sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),required_owner_stops=sum(bool(c.get('required_owner'))and not c.get('fault')for c in rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),source_snapshot_unchanged=unchanged,excluded_rmw_prewrite_observer_prefixes=excluded,complete_fault_contract_qualified=False,negative_controls=list(variants),image_sha256=reference.NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],live_activation_proven=False,remaining=['Genuine native growth/destructor/custom-listener composition','Callback-mutable cohort cases','Full live network/object wake integration','Unmasked platform exceptions and arbitrary control-frame aliases'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save()
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
