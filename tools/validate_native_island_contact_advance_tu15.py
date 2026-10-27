#!/usr/bin/env python3
"""Unchanged original3205B0 closed cases/genuine virtual-entry/fault prefixes.

No motion or collision outcome is supplied. Use run_test.py ownership; all raw
ELF/bank/fixture/build products remain disposable. Actual MCC owner identities
come from retained before inputs, original instructions from pinned TU1.5.
"""
import argparse,bisect,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import validate_native_island_agent_phase_tu15 as phase
BASE,ISLAND,WORLD,ENTRY,COLL_A,COLL_B,BODY_A,BODY_B,MOTION_A,MOTION_B,CONFIG,OUTPUT,HEADER,CONTACTS=0x1000000,0x1000100,0x1000300,0x1000600,0x1000700,0x1000800,0x1000900,0x1000a00,0x1000b00,0x1000c00,0x1000d00,0x1000e00,0x1000f00,0x1001000

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def inputs(image,output):
 capture=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-04-gemini-column0/original/required-failure-inputs';d=json.loads((capture/'capture.json').read_text());regions=sorted(d['regions'],key=lambda r:r['address']);addresses=[r['address']for r in regions];slices=[]
 def read(p,n):
  region=regions[bisect.bisect_right(addresses,p)-1];offset=p-region['address'];assert offset>=0 and offset+n<=region['size'];path=capture/region['before_file']
  with path.open('rb')as f:f.seek(offset);raw=f.read(n)
  assert len(raw)==n;slices.append(dict(address=p,bytes=n,source_file=str(path.relative_to(ROOT)),offset=offset,sha256=hashlib.sha256(raw).hexdigest()));return raw
 def word(p):return struct.unpack('<I',read(p,4))[0]
 world=word(0x52be48);island=word(word(world+8));entry=word(island+0x68);motion_tables=[]
 for offset in(0,4):
  motion=word(word(word(entry+offset)+0x10)+0x3c);motion_tables.append(word(motion))
 motion_tables=list(dict.fromkeys(motion_tables));motion_owners={struct.unpack('<I',image.read(p+0x74,4))[0]for p in motion_tables};agent_table=0x41ca9c;agent_owner=struct.unpack('<I',image.read(agent_table+0x10,4))[0];provenance=dict(capture=str(capture.relative_to(ROOT)),capture_sha256=sha(capture/'capture.json'),motion_tables=motion_tables,motion_owners=sorted(motion_owners),agent_table=agent_table,agent_owner=agent_owner,slices=slices,before_inputs_only=True,declared_component_storage=True,no_outcomes_supplied=True);(output/'MCC-owner-provenance.json').write_text(json.dumps(provenance,indent=2)+'\n');return motion_tables,motion_owners,agent_table,agent_owner

def metadata(image,boundaries):
 code=_Code(image,'.text');body=complete_body(code,0x3205b0);assert len(body.code)==437;names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];listing=[]
 for pc in sorted(body.code):
  i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
  for op in i.operands:
   if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
   assert not op.mem.segment;access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access;assert access&(CS_AC_READ|CS_AC_WRITE);b,idx=i.reg_name(op.mem.base),i.reg_name(op.mem.index);ops.append(dict(base=names[b]if b else-1,index=names[idx]if idx else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
  sites.append(dict(instruction=pc,operands=ops))
 return dict(sites=sites,body_sites=437,listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),regions=[],boundaries=sorted(boundaries))
def cases(image,tables,agent_table):
 out=[]
 def make(name,a=0,b=0,tolerance=0x3f800000,mode=2,count=0,time=0,table_index=0):
  banks={BASE:bytearray(0x2000),0x46a0cc:bytearray(image.read(0x46a0cc,4)),agent_table:bytearray(image.read(agent_table,0x14))}
  for p in tables:banks[p]=bytearray(image.read(p,0x78))
  def put(p,v):
   lo=next(lo for lo,data in banks.items()if lo<=p and p+4<=lo+len(data));banks[lo][p-lo:p-lo+4]=(v&0xffffffff).to_bytes(4,'little')
  for p,v in((ISLAND+0x20,WORLD),(WORLD+0x254,WORLD+0x280),(WORLD+0x24c,tolerance),(WORLD+0x28c,mode),(ENTRY,COLL_A),(ENTRY+4,COLL_B),(ENTRY+8,BASE+0x1500),(ENTRY+0xc,7),(BASE+0x1500,agent_table),(COLL_A+0x10,BODY_A),(COLL_B+0x10,BODY_B),(BODY_A+0x3c,MOTION_A),(BODY_B+0x3c,MOTION_B),(MOTION_A,tables[table_index%len(tables)]),(MOTION_B,tables[(table_index+1)%len(tables)]),(MOTION_A+0x7c,a),(MOTION_B+0x7c,b),(CONFIG+0x10,0x12345678),(OUTPUT,HEADER),(HEADER,CONTACTS),(HEADER+4,count),(CONTACTS+12,0x40000000)):
   put(p,v)
  n=len(out);out.append(dict(name=name,entry=0x3205b0,receiver=ISLAND,arguments=[CONFIG,ENTRY,OUTPUT,time],island=ISLAND,configuration=CONFIG,agent_entry=ENTRY,output=OUTPUT,time_bits=time,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[((0x7f,0x27f,0x37f)[n%3]&~0xc00)|((n%4)<<10),((n%8)<<11)|((n%3==0)*0x453f),(0x1f80,0x5fa5,0xffc0)[n%3]],stop_pc=phase.STOP,boundary=None))
 for value in(0,0x80000000,1,0x80000001,0x3f800000,0x40000000,0xbf800000,0xff800000,0x7f800000,0x7fc12345,0x7f812345):
  for mode in(0,2):
   for count in(0,1,0xffffffff):make(f'equal_{value:x}_{mode}_{count:x}',a=value,b=value,mode=mode,count=count)
 for a,b in((0,0x3f800000),(0x40000000,0),(0x7fc12345,0),(0,0x7f812345),(0xbf800000,0xc0000000)):
  for table in range(len(tables)):make(f'actual_motion_{a:x}_{b:x}_{table}',a=a,b=b,table_index=table)
 for rounding in range(4):
  for precision in(0,2,3):
   make(f'closed_rounding_{rounding}_{precision}',a=0x3f800001,b=0x3f800001);out[-1]['before_fp'][0]=0x7f|(precision<<8)|(rounding<<10)
 make('fault_seed_closed',a=0,b=0);make('fault_seed_negative',a=0,b=0,mode=0);make('fault_seed_motion',a=0,b=0x40000000)
 return out

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);tables,motion_owners,agent_table,agent_owner=inputs(image,a.output);meta=metadata(image,motion_owners|{agent_owner})
 def boundary(case,actual):
  target=actual['actual_stop']
  if target==phase.STOP:return None
  args=actual['boundary_stack'];assert target in motion_owners|{agent_owner};return dict(kind='motion'if target in motion_owners else'process',owner=target,receiver=actual['boundary_gp'][1],arguments=args[1:3]if target in motion_owners else args[1:5])
 phase.boundary=boundary;rows=phase.physical(image,meta,cases(image,tables,agent_table),scratch);faults=[]
 for seed in(c for c in rows if c['name'].startswith('fault_seed')):
  for category in('reads','writes'):
   seen=set();occurrences={}
   for event in seed[category]:
    size=event['bytes']if category=='reads'else len(event['written'])//2;key=event['address'],size;occurrences[key]=occurrences.get(key,0)+1
    if event['instruction']in seen:continue
    seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=size,occurrence=occurrences[key],access='read'if category=='reads'else'write'));faults.append(c)
 rows+=phase.physical(image,meta,faults,scratch);fixture=scratch/'advance.json';fixture.write_text(json.dumps(dict(original_sha256=NEW_PIN,body_sites=437,cases=rows),separators=(',',':')));project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_island_contact_advance.cpp';text=source.read_text();(project/source.name).write_text(text);assert text.count('multiply(m.word(0x46a0ccu))')==4;(project/'mutated.cpp').write_text(text.replace('multiply(m.word(0x46a0ccu))','multiply(m.word(0x46a0ccu)|0x00800000u)'));(project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(s in r.stderr for s in ("contact advance dependency ABI differs","contact advance FP differs","contact advance store differs","contact advance read order differs")),(r.returncode,r.stdout,r.stderr)\nprint("Source half-interval mutation rejected:",r.stderr.strip())\n');cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_advance LANGUAGES CXX)\ninclude(CTest)\n'
 for name,cpp in(('native_advance_original',source.name),('native_advance_mutation','mutated.cpp')):
  cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_island_contact_advance_tests.cpp" "{project/cpp}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=f'$<TARGET_FILE:{name}> "{fixture}"'if name.endswith('original')else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{name}> "{fixture}"';cmake+=f'add_test(NAME {name} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
 (project/'CMakeLists.txt').write_text(cmake);results=[]
 for compiler in('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_advance_original','--target','native_advance_mutation','--filter','^native_advance_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 files=[source,ROOT/'include/halo2/native_tu15_island_contact_advance.hpp',ROOT/'tests/native_tu15_island_contact_advance_tests.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(project/f.name if f==source else f)for f in files};temporary=[f for f in scratch.rglob('*')if f.is_file()];report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_sha256=NEW_PIN,body_sites=437,listing_sha256=meta['listing_sha256'],original_cases=len(rows),complete_original_calls=sum(c['actual_stop']==phase.STOP for c in rows),genuine_owner_entry_prefixes=sum(c['boundary']is not None for c in rows),read_fault_prefixes=sum(c.get('fault',{}).get('access')=='read'for c in rows),store_fault_prefixes=sum(c.get('fault',{}).get('access')=='write'for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),source_hashes=pins,current_sources_match_snapshot=all(sha(ROOT/p)==v for p,v in pins.items()),fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=sum(f.stat().st_blocks*512 for f in temporary),retained_large_artifacts=[],excluded=['genuine motion/agent owner returns','actual positive bisection completion','full live world integration','source private stack/GP postimages','unmasked FP traps']);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
