#!/usr/bin/env python3
"""Physical unchanged TU1.5 copy, predicate and compaction qualification.

The original303800 is executed inside original304130. No predicate callback,
calculated source outcome, instruction patch or register-result adapter exists.
"""
import argparse,copy,hashlib,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,PROBE,SECOND,OUTPUT,SETTINGS,HEADER,RECORDS=0x1000000,0x1000100,0x1000200,0x1000300,0x1000400,0x1000700,0x1000800
CONSTANT,STACK,SP,STOP=0x41d000,0x2000000,0x200f000,0x3000000
BACKEND='unchanged-tu15-contact-records-physical-operands-fp-v1'
ENTRIES=(0x303ba0,0x303800,0x304130)
LIFETIME=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
ORIGINAL_INPUT_PINS={'gemini-column0':'44bcd66f527fbfd21e3ed3799975758f441b6cdd11cb499ab25e5d89b5f6166a','gemini-column1':'bf29f112f1ba582e86c70a91dbf4d7dcbbdb27b94b8bf00c1f160b47078c911f','elongation-crates':'ed7dbc646f209755a538cfa5a4ad18a9f93dd4eaac9601bf1948617ad2579c87'}
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(p,j):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def metadata(image):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites={};hashes={};counts=[]
 for entry in ENTRIES:
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment;access=op.access;assert access&(CS_AC_READ|CS_AC_WRITE)
    # Capstone marks FSTP m32 READ|WRITE. The genuine instruction publishes
    # ST0 to its destination and pops; it does not read that destination.
    if i.mnemonic=='fstp':assert pc in(0x3042b4,0x3042c1)and op.size==4;access=CS_AC_WRITE
    base=i.reg_name(op.mem.base);index=i.reg_name(op.mem.index);ops.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites[pc]=dict(instruction=pc,bytes=i.bytes.hex(),operands=ops)
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[29,60,142]
 return dict(sites=list(sites.values()),stop=STOP,body_sites=counts,listing_sha256=hashes)
def inputs(image):
 cases=[];constant=image.read(CONSTANT,4096);assert constant[0x18:0x1c].hex()=='cdccccbd'
 def make(entry,name,args,destination=0,changes=(),alignment=False,depth=None,rounding=None,denorm=None,payloads=(),provenance=None):
  n=len(cases);rng=random.Random(800+n);raw=bytearray().join(struct.pack('<f',rng.uniform(-4,4))for _ in range(0x1000//4))
  def put(p,values):struct.pack_into('<'+'f'*len(values),raw,p-BASE,*values)
  put(PROBE,(0,0,0,0));put(PROBE+16,(1,0,0,0));put(SECOND,(0,0,0,0));put(SECOND+16,(0,1,0,0));put(SETTINGS+12,(0,.5))
  struct.pack_into('<I',raw,HEADER-BASE,RECORDS)
  for k in range(4):
   r=RECORDS+k*128;put(r+0x50,(-2 if k==0 else 2,0,0,0));put(r+0x60,(0,1,0,0));raw[r+0x7c-BASE]=1;raw[r+0x7d-BASE]=k
  for p,payload in payloads:raw[p-BASE:p-BASE+len(payload)]=payload
  for p,value,width in changes:raw[p-BASE:p-BASE+width]=int(value).to_bytes(width,'little')
  d=(0,1,3,6)[n%4]if depth is None else depth;sw=((n%8)<<11)|((n*0x100)&0x4700)|(n*13%64)
  live=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+(k%3)|(0x8000 if k%2 else 0)).hex()for k in range(d)]
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[1]=destination if entry==0x303ba0 else gp[1];gp[4]=SP+(n%4)*4
  rounding=n%4 if rounding is None else rounding;denorm=(0,0x40,0x8000,0x8040)[n%4]if denorm is None else denorm
  cases.append(dict(id=n,name=name,entry=entry,destination=destination,arguments=args,gp=gp,banks=[dict(address=BASE,before=raw.hex()),dict(address=CONSTANT,before=constant.hex())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|(rounding<<10),sw,0x1f80|(rounding<<13)|denorm|(n*7%64)],live_x87=live,alignment_fault=alignment,original_record_input_provenance=provenance))
 bits=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
 for k in range(16):make(0x303ba0,f'copy_controls_{k}',[PROBE],SECOND,depth=8 if k%2 else 0)
 for shift in(-64,-48,-32,-16,0,16,32,48,64,112):make(0x303ba0,f'copy_alias_{shift}',[PROBE],PROBE+shift,depth=8)
 for src,dst in((PROBE+4,SECOND),(PROBE,SECOND+4)):make(0x303ba0,f'copy_alignment_{src:x}_{dst:x}',[src],dst,alignment=True)
 make(0x303ba0,'copy_fault_seed',[PROBE],SECOND)
 make(0x303ba0,'copy_alias_fault_seed',[PROBE],PROBE+16)
 predicate=[OUTPUT,PROBE,SECOND,SETTINGS]
 for k in range(48):
  changed=()if k%3==0 else((SECOND+12,bits(-1 if k%3==1 else-.1),4),)
  make(0x303800,f'predicate_controls_{k}',predicate,changes=changed,depth=7 if k%5==0 else k%7,rounding=(k//3)%4,denorm=(0,0x40,0x8000,0x8040)[k//12])
 fields=(SECOND+12,PROBE,PROBE+4,PROBE+8,PROBE+12,SECOND,SECOND+4,SECOND+8,PROBE+16,PROBE+20,PROBE+24,PROBE+28,SECOND+16,SECOND+20,SECOND+24,SECOND+28,SETTINGS+12,SETTINGS+16)
 special=(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345)
 for p in fields:
  for value in special:make(0x303800,f'predicate_special_{p:x}_{value:x}',predicate,changes=((p,value,4),))
 for pair,(a,b)in enumerate(((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321))):
  for lane in range(4):
   for mode,(p,q)in enumerate(((PROBE+16+lane*4,SECOND+16+lane*4),(PROBE+lane*4,SECOND+lane*4),(PROBE+16+lane*4,SETTINGS+16),(PROBE+16+lane*4,SETTINGS+12))):make(0x303800,f'predicate_paired_nan_{pair}_{lane}_{mode}',predicate,changes=((p,a,4),(q,b,4)))
 make(0x303800,'predicate_association_boundary',predicate,changes=tuple((p,bits(v),4)for p,v in((PROBE+16,1),(PROBE+20,1),(PROBE+24,1),(SECOND+16,33554432),(SECOND+20,-33554432),(SECOND+24,1))))
 for args in([OUTPUT,PROBE,PROBE,SETTINGS],[PROBE,PROBE,SECOND,SETTINGS],[SECOND+12,PROBE,SECOND,SETTINGS],[OUTPUT,PROBE,SECOND,PROBE]):make(0x303800,'predicate_alias_'+str(len(cases)),args)
 for a,b in((PROBE+4,SECOND),(PROBE,SECOND+4)):make(0x303800,f'predicate_alignment_{a:x}_{b:x}',[OUTPUT,a,b,SETTINGS],alignment=True)
 make(0x303800,'predicate_fault_seed',predicate)
 make(0x303800,'predicate_early_fault_seed',predicate,changes=((SECOND+12,bits(-1),4),))
 sorting=[PROBE,HEADER,0,4,SETTINGS]
 for k in range(16):make(0x304130,f'sort_controls_{k}',sorting,rounding=k%4,denorm=(0,0x40,0x8000,0x8040)[k//4])
 for flags in((0,0,0,0),(2,255,1,0),(1,1,1,1),(1,0,1,0)):
  for positions in((-2,-1,0,1),(2,3,4,5),(-2,-3,-4,-5)):
   changes=tuple((RECORDS+k*128+0x7c,v,1)for k,v in enumerate(flags))+tuple((RECORDS+k*128+0x50,bits(v),4)for k,v in enumerate(positions))
   make(0x304130,'sort_flags_positions_'+str(len(cases)),sorting,changes=changes)
 for value in special:
  for field in(0x20,0x24):make(0x304130,f'sort_scalar_conversion_{field:x}_{value:x}',sorting,changes=((RECORDS+field,value,4),))
 for args,changes in(([PROBE,HEADER,4,4,SETTINGS],()),([PROBE,HEADER,0x7fffffff,0,SETTINGS],()),([PROBE,HEADER,0xffffffff,0,SETTINGS],((HEADER,RECORDS+128,4),)),([RECORDS+0x50,HEADER,0,4,SETTINGS],()),([PROBE,RECORDS+0x70,0,1,SETTINGS],((RECORDS+0x70,RECORDS,4),)),([PROBE,HEADER,0,4,RECORDS],((RECORDS+0x10,bits(.5),4),))):make(0x304130,'sort_alias_or_index_'+str(len(cases)),args,changes=changes)
 make(0x304130,'sort_alignment_record',sorting,changes=((HEADER,RECORDS+4,4),(RECORDS+0x80,1,1)),alignment=True)
 make(0x304130,'sort_fault_seed',[PROBE,HEADER,0,2,SETTINGS])
 make(0x304130,'sort_self_fault_seed',[PROBE,HEADER,0,1,SETTINGS],changes=((RECORDS+0x20,0x7f812345,4),(RECORDS+0x24,1,4)))
 # Genuine original-created inputs retained by the prior hardware copy-entry
 # observations. Their whole debugged heaps matched ordinary original replay.
 # Only128-byte INPUT records are relocated; no recorded result is supplied.
 # Settings and entry controls are explicit declared variations, so these do
 # not claim to reproduce an entire original caller context or a later frame.
 for name,pin in ORIGINAL_INPUT_PINS.items():
  path=LIFETIME/(name+'-original-lifetime-final')/'report.json';assert sha(path)==pin;reference=json.loads(path.read_text());assert reference['completed']and reference['image_sha256']==NEW_PIN and reference['debugger_exit']==0 and reference['checked_heap_bytes']==1077248
  events=[(n,event)for n,event in enumerate(reference['events'])if event['kind']=='source_record_copy_entry'];payloads=[bytes.fromhex(event['source_payload'])for _,event in events];assert all(len(payload)==128 for payload in payloads)
  def provenance(indices):return dict(report=str(path.relative_to(ROOT)),report_sha256=pin,events=[dict(event_index=events[n][0],original_source=events[n][1]['source'],payload_sha256=hashlib.sha256(payloads[n]).hexdigest())for n in indices],settings='Explicit declared settings; no source result/callback supplied',whole_heap_matches_plain_original_sha256=reference['plain_and_debugged_heap_sha256'])
  for n,payload in enumerate(payloads):
   prov=provenance([n]);make(0x303ba0,f'captured_{name}_copy_{n}',[PROBE],SECOND,payloads=((PROBE,payload),),provenance=prov);make(0x303ba0,f'captured_{name}_alias_copy_{n}',[PROBE],PROBE+16,payloads=((PROBE,payload),),provenance=prov)
   other=(n+1)%len(payloads);make(0x303800,f'captured_{name}_predicate_{n}',[OUTPUT,PROBE+0x50,SECOND+0x50,SETTINGS],payloads=((PROBE,payload),(SECOND,payloads[other])),provenance=provenance([n,other]))
  for n in range(0,len(payloads),4):
   indices=list(range(n,min(n+4,len(payloads))));make(0x304130,f'captured_{name}_sort_{n}',[PROBE+0x50,HEADER,0,len(indices),SETTINGS],payloads=((PROBE,payloads[n]),)+tuple((RECORDS+k*128,payloads[idx])for k,idx in enumerate(indices)),provenance=provenance(indices))
 return cases
def prepare(image,m,scratch):
 p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
 (p/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{p/'text.bin'}"
.section .constant,"aw"
.space 4096
.section .bank,"aw"
.balign 16
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
''');(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS {{ .original 0x{section.va:x}:{{*(.original)}} .constant 0x{CONSTANT:x}:{{*(.constant)}} .bank 0x{BASE:x}:{{*(.bank)}} .stack 0x{STACK:x}:{{*(.stack)}} .stop 0x{STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}} }}\n')
 for command in[['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_contact_records_tu15.cpp'),'-o',str(p/'observer')]]:
  r=subprocess.run(command,capture_output=True,text=True);assert r.returncode==0,r.stderr[-4000:]
 return p
def physical(p,rows,suffix):
 data=p/(suffix+'-inputs.json');data.write_text(json.dumps(rows,separators=(',',':')));result=p/(suffix+'-observed.json');r=subprocess.run([str(p/'observer'),str(p/'reference'),str(p/'metadata.json'),str(data),str(result)],text=True,capture_output=True,timeout=180);assert r.returncode==0,r.stdout+r.stderr;print(r.stdout.strip(),flush=True);observed=json.loads(result.read_text());assert not observed['instruction_patches']and observed['supplied_original_outcomes']is False;byid={x['id']:x for x in observed['cases']}
 for row in rows:
  actual=byid[row['id']]
  for old,new in zip(row['banks'],actual.pop('banks')):assert old['address']==new['address'];old['after']=new['after']
  row.update(actual,physical_verified=True)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[]);save(a.output/'report.json',report)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);owned=prepare(image,m,scratch);rows=physical(owned,inputs(image),'complete');prefix=[]
  for seed in[r for r in rows if 'fault_seed'in r['name']]:
   for phase,items in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n in range(len(items)):
     case=copy.deepcopy(seed);case.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(case)
  rows+=physical(owned,prefix,'prefix');fixture=scratch/'records.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],cases=rows),separators=(',',':'))+'\n')
  project=scratch/'project';(project/'include/halo2').mkdir(parents=True);(project/'src').mkdir();(project/'tests').mkdir();source=ROOT/'src/physics/native_tu15_contact_records.cpp';header=ROOT/'include/halo2/native_tu15_contact_records.hpp';test=ROOT/'tests/native_tu15_contact_records_tests.cpp';shutil.copytree(ROOT/'include',project/'include',dirs_exist_ok=True);shutil.copyfile(source,project/'src/native.cpp');shutil.copyfile(test,project/'tests/test.cpp');text=source.read_text();variants={}
  needle='add_scalar(z,yx)';assert text.count(needle)==1;variants['wrong_dot']=text.replace(needle,'add_scalar(product,add_scalar(y,z))')
  needle='if(!parity((first>>8u)&5u))return 1;';assert text.count(needle)==1;variants['skip_limit']=text.replace(needle,'if(false)return 1;')
  needle='bool committed=false;';assert text.count(needle)==1;variants['wrong_scalar']=text.replace(needle,'value^=1u;bool committed=false;')
  needle='if(!committed)asm volatile';assert text.count(needle)==1;variants['rollback_observer']=text.replace(needle,'if(true)asm volatile')
  snapshot_pins={str(p.relative_to(project)):sha(p)for p in [project/'src/native.cpp',project/'tests/test.cpp',project/'include/halo2/native_tu15_contact_records.hpp',project/'include/halo2/native_tu15_physics_frame.hpp',project/'include/halo2/physics_contact_record.hpp']}
  for name,variant in variants.items():(project/'src'/f'{name}.cpp').write_text(variant)
  (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and any(s in r.stderr for s in('retained contact bytes differ','ordered contact store differs','complete contact FP differs','ordered contact read count differs')),(r.returncode,r.stdout,r.stderr)
print('Physical original rejects private contact mutation:',r.stderr.strip())
''')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeContactRecordsTu15 LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT tests/test.cpp)
target_include_directories(driver PRIVATE include)
target_compile_features(driver PRIVATE cxx_std_20)
target_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
'''
  for name in('native',*variants):
   cmake+=f'add_executable(records_{name} $<TARGET_OBJECTS:driver> src/{name}.cpp)\ntarget_include_directories(records_{name} PRIVATE include)\ntarget_compile_features(records_{name} PRIVATE cxx_std_20)\ntarget_compile_options(records_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:records_{name}> "{fixture}"';cmake+=f'add_test(NAME records_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','records_native',*sum((['--target','records_'+name]for name in variants),[]),'--filter','^records_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  files=[source,header,test,ROOT/'include/halo2/native_tu15_physics_frame.hpp',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'tools/observe_native_contact_records_tu15.cpp',Path(__file__)];report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(rows),genuine_original_record_input_cases=sum(bool(c.get('original_record_input_provenance'))for c in rows),original_record_input_reports=ORIGINAL_INPUT_PINS,complete_original_calls=sum(c['completed']for c in rows),physical_alignment_faults=sum(c.get('alignment_fault',False)for c in rows),read_fault_prefixes=sum(c.get('fault',{}).get('phase')=='before_read'for c in rows),write_rejection_prefixes=sum(c.get('fault',{}).get('phase')=='before_write'for c in rows),completed_observer_rejection_prefixes=sum(c.get('fault',{}).get('phase')=='after_write'for c in rows),paired_nan_cases=sum('paired_nan'in c['name']for c in rows),alias_cases=sum('alias'in c['name']for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),physical_original_instructions=sum(c['original_instructions']for c in rows),image_sha256=NEW_PIN,body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],required_backend=BACKEND,fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},compiled_dependency_snapshot_sha256=snapshot_pins,compiled_snapshot_unchanged=all(sha(project/name)==pin for name,pin in snapshot_pins.items()),negative_controls=list(variants),supplied_original_outcomes=False,instruction_patches=[],live_activation_proven=False,remaining=['Actual native caller/binding qualification','Opaque record schema beyond verified fields','Full caller fault-resume/private x87 temporary owner','Unmasked FP traps and overfull source stack owner','Original Xbox CPU bit identity beyond this physical host','Protected original raw-bank and paired-native histories remain unchanged/open'])
 except BaseException as error:report['error']=str(error);raise
 finally:
  report.update(temporary_allocated_bytes_before_owner_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()));save(a.output/'report.json',report)
 print(json.dumps({k:report.get(k)for k in('passed','original_cases','ordered_original_reads','ordered_original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
