#!/usr/bin/env python3
"""Qualify native contact entity notification against unchanged physical TU1.5.

Real default callbacks execute; custom owners stop at their genuine entry.
No listener result, memory postimage or guest register result is supplied.
Compiler products and operand banks belong to run_test.py scratch.
"""
import argparse, copy, json, os, shutil, struct, subprocess, sys
from pathlib import Path
from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_activation_tu15 as activation
import validate_native_geometry_tu15 as reference
ROOT=reference.ROOT
BASE,EVENT,FIRST,SECOND,ROOT1,ROOT2,BODY1,BODY2=0x1000000,0x1000100,0x1000400,0x1000600,0x1000800,0x1000a00,0x1000c00,0x1000e00
PROP1,PROP2,POOL,ROWS=BASE+0x1000,BASE+0x1100,BASE+0x1200,BASE+0x1400
ENTRIES={0x281e10:98,0x1d7530:20,0x1d75b0:20}
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
  hashes[hex(entry)]=reference.hashlib.sha256(''.join(listing).encode()).hexdigest()
 return dict(sites=sites,body_sites=list(ENTRIES.values()),listing_sha256=hashes,stop=reference.STOP)
def cases(image):
 rows=[]
 def make(kinds=(1,1),parents=(0,0),enabled=(0,0),subject=0,properties=0,seed=False):
  raw=bytearray((n*19+7)&255 for n in range(0x4000));glob=bytearray(0x1000)
  def put(p,v,width=4):raw[p-BASE:p-BASE+width]=(v&((1<<(width*8))-1)).to_bytes(width,'little')
  put(EVENT+4,FIRST);put(EVENT+8,SECOND);put(EVENT+16,subject)
  for payload,root,body,kind,parent in((FIRST,ROOT1,BODY1,kinds[0],parents[0]),(SECOND,ROOT2,BODY2,kinds[1],parents[1])):
   put(payload+12,root if parent else 0);put(root+12,0)
   put((root if parent else payload)+24,kind);put((root if parent else payload)+32,body)
  for body,props,datum,slot in((BODY1,PROP1,0xaaaa0001,3),(BODY2,PROP2,0xbbbb0002,7)):
   put(body+0x30,props);put(body+0x34,4 if properties<2 else 0xffffffff)
   for n,(key,value) in enumerate(((0x1999,123),(0x2002,slot),(0x2001,datum),(0x2001,0xdeadbeef))):put(props+n*8,key if properties!=1 or key!=0x2001 else 0x1998);put(props+n*8+4,value)
  put(POOL+0x44,ROWS);put(ROWS+4,0,1);put(ROWS+160+4,enabled[0],1);put(ROWS+320+4,enabled[1],1);struct.pack_into('<I',glob,0xe5c,POOL)
  n=len(rows);depth=(0,1,3,8)[n%4];live=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=reference.SP
  row=dict(id=n,name=('fault_seed_'if seed else'notify_')+str(n),entry=0x281e10,world=0,island=0,body=0,event=EVENT,arguments=[EVENT],gp=gp,before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n%4)<<8)|(n*7%64),0x1f80|((n%4)<<13)|((0,0x40,0x8000,0x8040)[n%4])],live_x87=live,banks=[dict(address=BASE,before=raw.hex()),dict(address=0x52b000,before=glob.hex())],alignment_fault=False)
  if kinds==(1,1)and properties==0 and(enabled[0]if subject==BODY1 else enabled[1])&1:row['required_owner']=dict(owner=0x1d8a20,argument_count=5)
  rows.append(row)
 for repeat in range(4):
  for kinds in((0,0),(0,1),(1,0),(1,1)):
   for subject in(0,BODY1,BODY2):
    for enabled in((0,0),(1,1),(1,0),(0,1)):make(kinds=kinds,parents=(repeat&1,(repeat>>1)&1),subject=subject,enabled=enabled)
 for properties in(1,2):make(properties=properties,enabled=(1,1))
 make(enabled=(1,1),subject=BODY1,parents=(1,1),seed=True)
 return rows
def prepare(image,meta,scratch):
 p=activation.prepare(image,meta,scratch)
 asm=(p/'reference.s').read_text();asm=asm.replace('.section .stack,"aw"','.section .components,"aw"\n.space 4096\n.section .stack,"aw"');(p/'reference.s').write_text(asm)
 linker=(p/'link.ld').read_text().replace('.stack 0x','.components 0x52b000:{*(.components)} .stack 0x');(p/'link.ld').write_text(linker)
 observer=(p/'observer.cpp').read_text();needle='{"receiver",std::uint32_t(regs.rcx)},{"arguments",arguments}';assert observer.count(needle)==1;observer=observer.replace(needle,'{"receiver",std::uint32_t(regs.rcx)},{"event",std::uint32_t(regs.rsi)},{"arguments",arguments}');(p/'observer.cpp').write_text(observer)
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(p/'observer.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(command,text=True,capture_output=True);assert r.returncode==0,r.stderr[-3000:]
 return p
def driver():
 text=activation.DRIVER.replace('J::array({45,57,18,98,37,1})','J::array({98,20,20})')
 begin=text.index('NativeTu15IslandActivationStats stats;');end=text.index('   Host host;',begin)
 text=text[:begin]+'''NativeTu15ContactEntityNotificationStats stats;NativeTu15ContactEntityNotificationServices services;J required=nullptr;
   services.process_contacts=[&](auto event,auto component,auto own,auto other,auto own_slot,auto other_slot,auto&){required=J{{"owner",0x1d8a20},{"receiver",component},{"event",event.identity},{"arguments",J::array({component,own,other,own_slot,other_slot})}};throw OwnerStop{};};
'''+text[end:]
 begin=text.index('switch(entry){');end=text.index('   }catch(const Prefix&)',begin)
 text=text[:begin]+'''native_tu15_notify_contact_entities(m,{test.at("event")},fp,services,&stats);
'''+text[end:]
 start=text.index('   if(test.at("entry")==0x283450');end=text.index('\n',start);text=text[:start]+text[end:]
 needle='   if(reading){J expected=';assert text.count(needle)==1;text=text.replace(needle,'   if(owner_stop)check(required.at("event")==test.at("boundary").at("event"),name+" genuine contact processor implicit event input differs");\n'+needle)
 return text.replace('activation','contact entity')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 source=ROOT/'src/physics/native_tu15_contact_entity_notification.cpp';files=[source,ROOT/'include/halo2/native_tu15_contact_entity_notification.hpp',ROOT/'tools/validate_native_island_activation_tu15.py',ROOT/'tools/validate_native_geometry_tu15.py',ROOT/'tests/native_tu15_shape_bounds_tests.cpp',ROOT/'tools/observe_native_geometry_tu15.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):reference.sha(f)for f in files};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);meta=metadata(image);owned=prepare(image,meta,scratch);rows=reference.physical(owned,cases(image),'complete');faults=[]
  for seed in [c for c in rows if c['name'].startswith('fault_seed')]:
   for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n,event in enumerate(events):
     c=copy.deepcopy(seed);c.update(id=len(rows)+len(faults),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));faults.append(c)
  rows+=reference.physical(owned,faults,'prefix');fixture=scratch/'notification.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')))
  project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_contact_entity_notification.hpp"\n'+prefix+driver());shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={}
  for name,needle,replacement in (('wrong_class','payload+24u)==1u','payload+24u)==0u'),('wrong_subject','subject==first','subject!=first'),('wrong_slot_key','second,0x2002u','second,0x2001u')):
   assert text.count(needle)==1,(name,text.count(needle));variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.returncode,r.stdout,r.stderr)\nprint("Original rejects mutation:",r.stderr.strip()[:300])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeContactEntityNotification LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'+f'target_include_directories(driver PRIVATE "{ROOT}/include")\n'
  for name in ('native',*variants):
   cmake+=f'add_executable(notification_{name} $<TARGET_OBJECTS:driver> {name}.cpp)\ntarget_include_directories(notification_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(notification_{name} PRIVATE cxx_std_20)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:notification_{name}> "{fixture}"';cmake+=f'add_test(NAME notification_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in ('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','notification_native',*sum((['--target','notification_'+n]for n in variants),[]),'--filter','^notification_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(reference.sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),required_owner_stops=sum(bool(c.get('required_owner'))and not c.get('fault')for c in rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),source_snapshot_unchanged=unchanged,negative_controls=list(variants),image_sha256=reference.NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],remaining=['Genuine1D8A20 complete return composition','Full visible multiplayer binding','Unmasked platform exceptions'])
 except BaseException as error:report['error']=str(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
