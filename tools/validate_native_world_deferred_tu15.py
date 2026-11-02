#!/usr/bin/env python3
"""Physical unchanged TU1.5 deferred queue/merge-monitor qualification.

Nested owners stop at genuine entries; no callback outcome is supplied.
The authenticated RET4 leaf is executed as the exact no-op destructor.
All reference/compiler output belongs to run_test.py scratch.
"""
import argparse,copy,hashlib,json,os,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_activation_tu15 as activation
import validate_native_geometry_tu15 as reference
ROOT=reference.ROOT
BASE,WORLD,ISLAND,BODY,QUEUE,PAIRS,TABLE,UPDATE=0x1000000,0x1000100,0x1000500,0x1001800,0x1002800,0x1002c00,0x1003000,0x1001c00
ENTRIES={0x319750:42,0x316e90:44,0x319cd0:45,0x316e10:46,0x282bc0:5,0x282be0:9,0x149b90:36,0x283450:37,0x2e4d60:1}
def metadata(image):
 code=reference._Code(image,'.text');names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];hashes={}
 for entry,count in ENTRIES.items():
  body=reference.complete_body(code,entry);assert len(body.code)==count;listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment
    b,x=i.reg_name(op.mem.base),i.reg_name(op.mem.index)
    ops.append(dict(base=names.get(b,-1),index=names.get(x,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 return dict(sites=sites,body_sites=list(ENTRIES.values()),listing_sha256=hashes,stop=reference.STOP)
def cases(image):
 rows=[]
 def make(entry,count=3,refs=3,callback=False,custom_destroy=False,allocation_total=0,prior_allocation=0,seed=False):
  raw=bytearray((n*7+3)&255 for n in range(0x4000));heaps=bytearray(0x1000);monitor=bytearray((n*13+5)&255 for n in range(0x1000));globals=bytearray(0x1000)
  def put(p,v,size=4):raw[p-BASE:p-BASE+size]=(v&((1<<(size*8))-1)).to_bytes(size,'little')
  for offset in (0x24,0x10c,0xf4,0x100):put(WORLD+offset,count)
  for offset in (0x108,0xf0,0xfc):put(WORLD+offset,QUEUE)
  put(WORLD+0x20,PAIRS);put(TABLE,0x316790 if custom_destroy else 0x2e4d60)
  for n in range(5):
   body=BODY+n*0x80;update=UPDATE+n*0x80;put(body,TABLE);put(body+6,refs,2);put(body+0x44,ISLAND if callback else 0);put(update,TABLE);put(update+6,refs,2);put(update+0xc,body)
   put(QUEUE+n*4,update if entry==0x316e10 else body);put(PAIRS+n*8,body);put(PAIRS+n*8+4,body if not callback else BODY+(n+1)%5*0x80)
  if entry==0x319750:
   for n in range(5):put(BODY+n*0x80+0x44,ISLAND+n*0x20 if callback else ISLAND)
  put(ISLAND+0x29,1,1)
  if allocation_total:
   struct.pack_into('<I',heaps,0x70,BASE+0x3400);put(BASE+0x3418,BASE+0x3500);put(BASE+0x341c,8);put(BASE+0x3420,1);put(BASE+0x3504,allocation_total)
  struct.pack_into('<I',monitor,0xfd0,prior_allocation)
  n=len(rows);depth=(0,1,3,8)[n%4];live=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[1]=WORLD;gp[4]=reference.SP
  row=dict(id=n,name=('fault_seed_'if seed else'queue_')+f'{entry:x}_{n}',entry=entry,world=WORLD,island=ISLAND,body=BODY,result=reference.SP-1,arguments=[ISLAND]if entry in(0x282bc0,0x282be0)else[],gp=gp,before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n%4)<<8)|(n*7%64),0x1f80|((n%4)<<13)|((0,0x40,0x8000,0x8040)[n%4])],live_x87=live,banks=[dict(address=BASE,before=raw.hex()),dict(address=0x41d000,before=image.read(0x41d000,0x1000).hex()),dict(address=0x556000,before=globals.hex()),dict(address=0x486000,before=heaps.hex()),dict(address=0x56d000,before=monitor.hex())],alignment_fault=False)
  if count in(1,3)and callback and entry in(0x319750,0x316e90,0x319cd0,0x316e10):row['required_owner']=dict(owner={0x319750:0x3190d0,0x316e90:0x316cd0,0x319cd0:0x3198a0,0x316e10:0x316aa0}[entry],argument_count=1 if entry==0x316e90 else 2)
  elif count in(1,3)and refs==1 and custom_destroy and entry!=0x319750:row['required_owner']=dict(owner=0x316790,argument_count=1)
  rows.append(row)
 for entry in(0x319750,0x316e90,0x319cd0,0x316e10):
  for repeat in range(3):
   for count in(0,1,3,0xffffffff,0x80000000):
    for refs in(0,1,2,0xffff):make(entry,count,refs)
  for count in(1,3):make(entry,count,callback=True)
  if entry!=0x319750:make(entry,1,refs=1,custom_destroy=True)
  make(entry,3,refs=1,seed=True)
 for entry in(0x282bc0,0x282be0):
  for total in(0,1,0x7fffffff,0xffffffff):
   for previous in(0,1,0xffffffff):make(entry,allocation_total=total,prior_allocation=previous,seed=(total==1 and previous==0))
 return rows
def prepare(image,meta,scratch):
 p=activation.prepare(image,meta,scratch);source=p/'reference.s';t=source.read_text();t+='\n.section .heapglobals,"aw"\n.space 4096\n.section .monitor,"aw"\n.space 4096\n';source.write_text(t);link=p/'link.ld';t=link.read_text();assert t.count('.bridge 0x70000000:')==1;t=t.replace('.bridge 0x70000000:','.heapglobals 0x486000:{*(.heapglobals)} .monitor 0x56d000:{*(.monitor)} .bridge 0x70000000:');link.write_text(t)
 subprocess.run(['as','--32',str(source),'-o',str(p/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(link),str(p/'reference.o'),'-o',str(p/'reference')],check=True,capture_output=True);return p
def driver():
 text=activation.DRIVER.replace('J::array({45,57,18,98,37,1})','J::array({42,44,45,46,5,9,36,37,1})');begin=text.index('NativeTu15IslandActivationStats stats;');end=text.index('   Host host;',begin)
 text=text[:begin]+'''NativeTu15WorldDeferredStats stats;NativeTu15WorldDeferredServices services;J required=nullptr;
   const auto stop=[&](std::uint32_t owner,std::uint32_t receiver,J args){required=J{{"owner",owner},{"receiver",receiver},{"arguments",args}};throw OwnerStop{};};
   services.merge_islands=[&](auto world,auto first,auto second,auto&){stop(0x3190d0,world.identity,J::array({first.identity,second.identity}));};
   services.deactivate_island=[&](auto world,auto island,auto&){stop(0x316cd0,world.identity,J::array({island.identity}));};
   services.remove_body=[&](auto world,auto out,auto body,auto&){stop(0x3198a0,world.identity,J::array({out.identity,body.identity}));};
   services.remove_update=[&](auto world,auto out,auto update,auto&){stop(0x316aa0,world.identity,J::array({out.identity,update.identity}));};
   services.destroy_reference=[&](auto body,auto owner,auto disposing,auto&){if(owner.identity==0x2e4d60)return;stop(owner.identity,body.identity,J::array({disposing}));};
'''+text[end:];begin=text.index('switch(entry){');end=text.index('   }catch(const Prefix&)',begin)
 text=text[:begin]+'''switch(entry){
    case 0x319750:native_tu15_drain_world_operations(m,world,fp,services,observer,&stats);break;
    case 0x316e90:native_tu15_drain_world_added_actions(m,world,fp,services,observer,&stats);break;
    case 0x319cd0:native_tu15_drain_world_deletes(m,world,{test.at("result")},fp,services,observer,&stats);break;
    case 0x316e10:native_tu15_drain_world_updates(m,world,{test.at("result")},fp,services,observer,&stats);break;
    case 0x282bc0:native_tu15_begin_world_merge_monitor(m,island,observer);break;
    case 0x282be0:native_tu15_end_world_merge_monitor(m,island,observer);break;
    default:throw std::runtime_error("Unexpected actual deferred entry");}
'''+text[end:];start=text.index('   if(test.at("entry")==0x283450');end=text.index('\n',start);return (text[:start]+text[end:]).replace('activation','deferred')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=ROOT/'src/physics/native_tu15_world_deferred.cpp';files=[source,ROOT/'include/halo2/native_tu15_world_deferred.hpp',ROOT/'src/physics/native_tu15_island_activation.cpp',ROOT/'src/physics/native_tu15_island_collision.cpp',ROOT/'tools/validate_native_island_activation_tu15.py',ROOT/'tools/validate_native_geometry_tu15.py',ROOT/'tests/native_tu15_shape_bounds_tests.cpp',ROOT/'tools/observe_native_geometry_tu15.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):reference.sha(f)for f in files};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);meta=metadata(image);owned=prepare(image,meta,scratch);rows=reference.physical(owned,cases(image),'complete');faults=[];excluded=[]
  for seed in[c for c in rows if c['name'].startswith('fault_seed')]:
   for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n,event in enumerate(events):
     if phase=='before_write'and event['instruction']in(0x316ec9,0x319d08,0x316e4b):excluded.append(dict(seed=seed['name'],instruction=event['instruction'],reason='Original observer stops before the RMW retirement; read/write storage has observed the input read before rejected write.'));continue
     c=copy.deepcopy(seed);c.update(id=len(rows)+len(faults),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));faults.append(c)
  rows+=reference.physical(owned,faults,'prefix');fixture=scratch/'deferred.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')));project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_world_deferred.hpp"\n'+prefix+driver());shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={}
  for name,needle,replacement in(('wrong_decrement','(previous-1u)&0xffffu','(previous+1u)&0xffffu'),('wrong_count_clear','0x3197a6u,world.identity+0x24u,0','0x3197a6u,world.identity+0x24u,1'),('wrong_monitor_delta','if(after-before)','if(after==before)')):
   assert text.count(needle)==1,(name,text.count(needle));variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.returncode,r.stdout,r.stderr)\nprint("Original rejects deferred mutation:",r.stderr.strip()[:300])\n');cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeWorldDeferred LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'+f'target_include_directories(driver PRIVATE "{ROOT}/include")\n'
  for name in('native',*variants):
   cmake+=f'add_executable(deferred_{name} $<TARGET_OBJECTS:driver> {name}.cpp "{ROOT}/src/physics/native_tu15_island_activation.cpp" "{ROOT}/src/physics/native_tu15_island_collision.cpp")\ntarget_include_directories(deferred_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(deferred_{name} PRIVATE cxx_std_20)\n';command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:deferred_{name}> "{fixture}"';cmake+=f'add_test(NAME deferred_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','deferred_native',*sum((['--target','deferred_'+n]for n in variants),[]),'--filter','^deferred_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(reference.sha(ROOT/f)==v for f,v in pins.items()),compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),required_owner_stops=sum(bool(c.get('required_owner'))and not c.get('fault')for c in rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),negative_controls=list(variants),excluded_rmw_prewrite_observer_prefixes=excluded,complete_fault_contract_qualified=False,image_sha256=reference.NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],remaining=['Genuine native merge/remove/custom destructor return composition','Callback-mutable queue cohorts','Full live multiplayer binding','RMW prewrite read-observer atomicity and unmasked exceptions'])
 except BaseException as error:report['error']=str(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
