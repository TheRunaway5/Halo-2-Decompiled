#!/usr/bin/env python3
"""Unchanged physical island agent phases, contact-buffer lifetime and minimum.

All original instructions remain unchanged. Real allocator/agent owners stop
at their genuine entries; no result is supplied. Native kernels use the same
retained packets and bank inputs. All raw/compiler products are disposable.
"""
import argparse,bisect,copy,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,HEADER,ISLAND,WORLD,ALLOCATOR,ENTRIES,AGENT,CONTACTS,CONFIG,ARENA=0x1000000,0x1000100,0x1000300,0x1000500,0x1000800,0x1000900,0x1001000,0x1002000,0x1004000,0x1005000
STACK,SP,STOP,FS=0x2000000,0x200f000,0x3000000,0x4000000
CLEAR_HEADER,CLEAR_OUTPUT,SET_HEADER,SET_OUTPUT=SP-0x34,SP-0x50,SP-0x20,SP-0x2c
BANKS=((BASE,0x6000),(BASE+0xe000,0x100),(0x41ca9c,0x18),(0x41d01c,0x18),(0x41f064,4),(0x41f260,0x20),(0x45f4cc,0x2c),(0x46a0e8,4),(0x48c938,4),(0x48c974,8))
CONSTANT_BANKS={0x41ca9c,0x41d01c,0x41f064,0x41f260,0x45f4cc,0x46a0e8}
BOUNDARIES=(0x2e10a0,0x2e1110,0x149e00,0x2f0f10,0x304350,0x3230c0,0x3205b0)
BACKEND='unchanged-tu15-island-agent-phases-buffer-minimum-physical-v1'
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def metadata(image):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
 for entry in(0x321d20,0x321a70,0x321180,0x321220,0x320510):
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   ins=code.at(pc);listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n');operands=[]
   for op in ins.operands:
    if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
    if op.mem.segment:assert ins.reg_name(op.mem.segment)=='fs';continue
    access=CS_AC_WRITE if ins.mnemonic in('fst','fstp')else op.access;assert access&(CS_AC_READ|CS_AC_WRITE)
    base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index);operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=operands,timestamp=ins.mnemonic=='rdtsc'))
  listings[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[97,205,51,55,63],counts
 return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[],boundaries=list(BOUNDARIES))
def observer_source():
 text=(ROOT/'tools/observe_native_constraint_step_tu15.cpp').read_text()
 text=text.replace('estimates=J::array();user_regs_struct','estimates=J::array(),timestamps=J::array();user_regs_struct')
 text=text.replace('if(pc==stop)break;','if(pc==stop||std::find(m.at("boundaries").begin(),m.at("boundaries").end(),pc)!=m.at("boundaries").end())break;')
 text=text.replace('  check(std::uint32_t(regs.rip)==stop,"Original pose stop not reached");','  check(std::uint32_t(regs.rip)==stop||std::find(m.at("boundaries").begin(),m.at("boundaries").end(),std::uint32_t(regs.rip))!=m.at("boundaries").end(),"Original pose stop not reached");')
 needle='    for(auto&a:pending){a["written"]=hex(memory(child.pid,a.at("address"),a.at("bytes")));'
 assert text.count(needle)==1
 text=text.replace(needle,'    if(site->second.value("timestamp",false)){user_regs_struct ticks{};request(PTRACE_GETREGS,child.pid,nullptr,&ticks);timestamps.push_back(J{{"instruction",pc},{"value",(std::uint64_t(std::uint32_t(ticks.rdx))<<32)|std::uint32_t(ticks.rax)}}); }\n'+needle)
 text=text.replace('{"banks",banks}}.dump()', '{"banks",banks},{"actual_stop",std::uint32_t(regs.rip)},{"timestamps",timestamps},{"extended_result",hex(std::vector<std::byte>(reinterpret_cast<const std::byte*>(fp.st_space),reinterpret_cast<const std::byte*>(fp.st_space)+10))}}.dump()')
 return text.replace('#include <array>','#include <array>\n#include <algorithm>')
def boundary(case,actual):
 target=actual['actual_stop']
 if target==STOP:return None
 gp,args=actual['boundary_gp'],actual['boundary_stack']
 if target==0x2e10a0:return dict(kind='allocate',owner=target,receiver=gp[1],arguments=args[1:2])
 if target==0x2e1110:return dict(kind='release',owner=target,receiver=gp[1],arguments=args[1:3])
 if target==0x149e00:return dict(kind='owned',owner=target,receiver=gp[1],arguments=args[1:4])
 if target in(0x2f0f10,0x304350):return dict(kind='process',owner=target,receiver=gp[1],arguments=args[1:5])
 if target==0x3230c0:return dict(kind='finish',owner=target,receiver=gp[1],arguments=args[1:5])
 if target==0x3205b0:return dict(kind='advance',owner=target,receiver=gp[1],arguments=args[1:5])
 raise AssertionError(('Unclassified genuine owner',hex(target),case['name']))
def cases(image):
 out=[]
 def make(name,entry=0x321d20,count=0,capacity=64,remaining=0x4000,marker=ARENA+0x8000,flags=None,nonnull=False,rank=0,profile=False,changes=()):
  banks={p:bytearray(image.read(p,n)if p in CONSTANT_BANKS else bytes(n))for p,n in BANKS}
  if entry in(0x321d20,0x321a70):
   header,output=(CLEAR_HEADER,CLEAR_OUTPUT)if entry==0x321d20 else(SET_HEADER,SET_OUTPUT);banks[header]=bytearray(20);banks[output]=bytearray(8 if entry==0x321a70 else 4)
  else:header,output=HEADER,BASE+0x180
  def put(p,v,n=4):
   bank=next(lo for lo,data in banks.items()if lo<=p and p+n<=lo+len(data));banks[bank][p-bank:p-bank+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
  put(0x48c938,ALLOCATOR);put(ALLOCATOR,0x45f4cc);put(ALLOCATOR+8,ARENA);put(ALLOCATOR+0xc,remaining);put(ALLOCATOR+0x14,marker);put(ISLAND+0x20,WORLD);put(ISLAND+0x68,ENTRIES);put(ISLAND+0x6c,count);put(WORLD+0x244,0x3f000000);put(WORLD+0x248,0x3e000000);put(WORLD+0x254,BASE+0x600)
  put(0x48c974,BASE+0xe000 if profile else 0);put(0x48c978,BASE+0xe100 if profile else 0)
  put(header,CONTACTS);put(header+4,count);put(header+8,capacity|0x80000000 if flags is None else flags);put(header+0xc,ARENA);put(header+0x10,capacity);put(output,header)
  for n in range(min(count,12)):
   put(ENTRIES+n*16,BASE+0x7010+n*0x100);put(ENTRIES+n*16+4,BASE+0x8010+n*0x100);put(ENTRIES+n*16+8,AGENT if nonnull else 0);put(ENTRIES+n*16+0xc,rank)
   put(CONTACTS+n*48+12,struct.unpack('<I',struct.pack('<f',2.0-n*.25))[0])
  put(AGENT,0x41ca9c);put(AGENT+8,BASE+0x1100);put(BASE+0x1100,0x41f260)
  for p,v,n in changes:put(p,v,n)
  if entry==0x321180:receiver=header;args=[capacity]
  elif entry==0x321220:receiver=header;args=[]
  elif entry==0x320510:receiver=0;args=[]
  else:receiver=ISLAND;args=[CONFIG]
  n=len(out);out.append(dict(name=name,entry=entry,receiver=receiver,arguments=args,minimum_input=output,island=ISLAND,configuration=CONFIG,header=header,output=output,capacity=capacity,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[((0x7f,0x27f,0x37f)[n%3]&~0xc00)|((n%4)<<10),((n%8)<<11)|(0x453f if n%3==0 else 0),(0x1f80,0x5fa5,0xffc0)[n%3]],stop_pc=STOP,boundary=None))
 for capacity in(0,1,2,3,16,64,0x55555555,0x80000000,0xffffffff):
  make(f'construct_{capacity:x}',entry=0x321180,capacity=capacity)
  make(f'destroy_{capacity:x}',entry=0x321220,capacity=capacity)
 for capacity in(0,1,7,0x7fffffff):make(f'owned_release_{capacity:x}',entry=0x321220,capacity=3,flags=capacity)
 for marker in(0,ARENA):make(f'actual_stack_release_{marker:x}',entry=0x321220,marker=marker)
 for entry in(0x321d20,0x321a70):
  for count in(0,1,3,9,0xffffffff):
   for profile in(False,True):make(f'phase_{entry:x}_count_{count:x}_profile_{profile}',entry=entry,count=count,profile=profile)
  for rank in(0,1,5,0xffffffff,0xfffffffe,0x7fffffff):make(f'actual_agent_{entry:x}_{rank:x}',entry=entry,count=1,nonnull=True,rank=rank)
  make(f'actual_alloc_{entry:x}',entry=entry,remaining=0)
  make(f'fault_seed_phase_{entry:x}',entry=entry,count=3,profile=True)
  make(f'fault_seed_agent_{entry:x}',entry=entry,count=1,nonnull=True,rank=5)
  make(f'signed_loop_edge_{entry:x}',entry=entry,count=0x80000000);out[-1].update(stop_pc=0x321da1 if entry==0x321d20 else 0x321af1,fault=dict(address=ENTRIES+8,bytes=4,occurrence=1))
 for rounding in range(4):
  for precision in(0,2,3):
   for left,right in((0x3e000001,0x3f000001),(0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7f812345,0xff854321)):
    make(f'threshold_fp_{rounding}_{precision}_{left:x}_{right:x}',entry=0x321a70,count=1,nonnull=True,rank=0x1000001,changes=((WORLD+0x248,left,4),(WORLD+0x244,right,4)));out[-1]['before_fp'][0]=0x7f|(precision<<8)|(rounding<<10)
 for entry in(0x321180,0x321220):make(f'fault_seed_helper_{entry:x}',entry=entry,capacity=3)
 for count in(0,1,2,3,4,5,7,8,9,12,0xffffffff):
  for rounding in range(4):make(f'minimum_{count:x}_{rounding}',entry=0x320510,count=count)
 for count in(1,4,5,8):
  for value in(0x80000000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'minimum_edge_{count}_{value:x}',entry=0x320510,count=count,changes=((CONTACTS+12,value,4),))
 make('fault_seed_minimum',entry=0x320510,count=9)
 make('construct_alias_allocator_cursor',entry=0x321180,capacity=1,changes=((ALLOCATOR+8,HEADER,4),))
 make('destroy_alias_capacity_cursor',entry=0x321220,capacity=0,flags=0,changes=((HEADER+8,0,4),(ALLOCATOR+8,HEADER+8,4)))
 return out

def captured_mcc_cases(seeds,output):
 base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-04-gemini-column0/original/required-failure-inputs'
 capture=base/'capture.json';metadata_value=json.loads(capture.read_text());regions=sorted(metadata_value['regions'],key=lambda r:r['address']);addresses=[r['address']for r in regions];provenance={}
 def read(address,size):
  region=regions[bisect.bisect_right(addresses,address)-1];offset=address-region['address'];assert 0<=offset and offset+size<=region['size'],(hex(address),size)
  path=base/region['before_file']
  with path.open('rb')as source:source.seek(offset);data=source.read(size)
  assert len(data)==size;provenance[f'{address:x}:{size}']=dict(address=address,bytes=size,source_file=str(path.relative_to(ROOT)),source_offset=offset,input_slice_sha256=hashlib.sha256(data).hexdigest());return data
 def word(address):return struct.unpack('<I',read(address,4))[0]
 world=word(0x52be48);allocator=word(0x48c938);active=word(world+8);count=word(world+0xc);out=[]
 for index in range(min(count,2)):
  island=word(active+index*4);entries=word(island+0x68);agents=word(island+0x6c)
  assert 0<agents<=16
  for entry in(0x321d20,0x321a70):
   case=copy.deepcopy(next(c for c in seeds if c['name']==f'actual_agent_{entry:x}_0'));case.update(name=f'actual_MCC_Gemini_island_{index}_{entry:x}',receiver=island,island=island,configuration=world+0x140,arguments=[world+0x140],captured_mcc_inputs=True)
   # Replace only actual consumed fields. Unused declared synthetic banks are
   # retained as such; they are not described as a whole captured world.
   for address,size in((0x48c938,4),(0x48c974,8)):
    bank=next(b for b in case['banks']if b['address']==address);bank['before']=read(address,size).hex()
   slices={allocator:read(allocator,0x18),island:read(island,0x70),entries:read(entries,agents*16),world:read(world,0x258)}
   controller=word(world+0x254);slices[controller]=read(controller,0x14)
   for n in range(agents):
    agent=word(entries+n*16+8)
    if agent:slices[agent]=read(agent,12)
   for address,data in slices.items():
    assert not any(b['address']<=address<b['address']+len(bytes.fromhex(b['before']))or address<=b['address']<address+len(data)for b in case['banks']),('Captured slice overlaps declared bank',hex(address))
    case['banks'].append(dict(address=address,before=data.hex()))
   case['banks'].sort(key=lambda b:b['address']);out.append(case)
 report=dict(capture=str(capture.relative_to(ROOT)),capture_sha256=sha(capture),source_comparison_contract=metadata_value['comparison_contract'],actual_world=world,actual_allocator=allocator,prepared_imported_MCC_reference=True,original_executable_authority=NEW_PIN,slices=list(provenance.values()),before_inputs_only=True,no_physics_outcomes_supplied=True)
 (output/'captured-MCC-input-provenance.json').write_text(json.dumps(report,indent=2)+'\n');return out

def physical(image,m,rows,scratch):
 owned=scratch/'physical-original';owned.mkdir(exist_ok=True);observer=owned/'observer'
 if not observer.exists():
  adapted=observer_source();source=owned/'observer.cpp';source.write_text(adapted)
  subprocess.run(['c++','-std=c++20','-O2',str(source),'-o',str(observer)],check=True,capture_output=True)
 section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
 for c in rows:
  specific=dict(m);specific['regions']=[dict(address=b['address'],bytes=len(bytes.fromhex(b['before'])))for b in c['banks']];(owned/'metadata.json').write_text(json.dumps(specific,separators=(',',':')))
  sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{text}"']
  stack=bytearray(0x10000)
  for n,b in enumerate(c['banks']):
   if STACK<=b['address'] and b['address']+len(bytes.fromhex(b['before']))<=STACK+len(stack):
    off=b['address']-STACK;stack[off:off+len(bytes.fromhex(b['before']))]=bytes.fromhex(b['before']);continue
   path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
  args=c['arguments'];struct.pack_into('<'+'I'*(len(args)+1),stack,SP-STACK,STOP,*args);(owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
  assembly.append(f'.section .tls,"aw"\n.space 4096');sections.append(f'.tls 0x{FS:x} : {{ *(.tls) }}')
  cw,sw,mxcsr=c['before_fp'];env=bytearray(28);struct.pack_into('<H',env,0,cw);struct.pack_into('<H',env,4,sw);struct.pack_into('<H',env,8,0xffff);(owned/'environment.bin').write_bytes(env)
  assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 mov $123,%eax
 mov $1,%ebx
 mov $fs_descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 js setup_failure
 mov $7,%eax
 mov %ax,%fs
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${c['receiver']},%ecx
 mov ${c.get('minimum_input',0)},%eax
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long {c['entry']}
sse_control: .long {mxcsr}
fs_descriptor: .long 0,{FS},0xfffff,0x51
setup_failure:
 mov $1,%eax
 mov $125,%ebx
 int $0x80
environment: .incbin "{owned/'environment.bin'}"
''');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
  subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
  r=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'output.json'),hex(c['stop_pc'])],capture_output=True,text=True);assert r.returncode==0,(c['name'],r.stdout,r.stderr)
  observed=json.loads((owned/'output.json').read_text())
  c['actual_stop']=observed['actual_stop'];c['boundary']=boundary(c,observed)if 'fault'not in c and 'observer_fault'not in c else None
  for b,after in zip(c['banks'],observed.pop('banks')):assert b['address']==after['address'];b['after']=after['after']
  c.update(observed,physical_verified=True)
 return rows

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);inputs=cases(image);inputs+=captured_mcc_cases(inputs,args.output);complete=physical(image,meta,inputs,scratch);faults=[]
 for seed in(c for c in complete if c['name'].startswith('fault_seed')):
  seen=set();occurrences={}
  for event in seed['reads']:
   key=event['address'],event['bytes'];occurrences[key]=occurrences.get(key,0)+1
   if event['instruction']in seen:continue
   seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_read_fault_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key]));faults.append(c)
  seen=set();occurrences={}
  for event in seed['writes']:
   key=event['address'],len(event['written'])//2;occurrences[key]=occurrences.get(key,0)+1
   if event['instruction']in seen:continue
   seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_store_fault_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='write'));faults.append(c)
 observers=[];code=_Code(image,'.text')
 for seed in(c for c in complete if c['name'].startswith('fault_seed_agent_')or c['name'].startswith('fault_seed_helper_')):
  for index,event in enumerate(seed['writes']):
   c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+hex(event['instruction']),stop_pc=event['instruction']+code.at(event['instruction']).size,observer_fault=index+1);observers.append(c)
 all_cases=complete+physical(image,meta,faults+observers,scratch);fixture=scratch/'island-agent-phase.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=all_cases),separators=(',',':')));project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_island_agent_phase.cpp';text=source.read_text();(project/source.name).write_text(text);assert text.count('capacity|0x80000000u')==1;(project/'mutated.cpp').write_text(text.replace('capacity|0x80000000u','capacity|0x40000000u'));assert text.count('candidate(cursor);cursor+=48u;')==1;(project/'stride.cpp').write_text(text.replace('candidate(cursor);cursor+=48u;','candidate(cursor);cursor+=32u;'));assert text.count('multiply(m.word(world+0x248u));add(m.word(world+0x244u));')==1;(project/'threshold.cpp').write_text(text.replace('multiply(m.word(world+0x248u));add(m.word(world+0x244u));','add(m.word(world+0x244u));multiply(m.word(world+0x248u));'));(project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)\nassert r.returncode!=0 and ("ordered island agent store differs" in r.stderr or "retained island agent bytes differ" in r.stderr or "original island agent read order differs" in r.stderr or "actual extended minimum return differs" in r.stderr),(r.returncode,r.stdout,r.stderr)\nprint("Original contact lifetime rejects lost borrowed flag:",r.stderr.strip())\n');cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_island_agent_phase_validation LANGUAGES CXX)\ninclude(CTest)\n'
 for kind in('original','negative','stride','threshold'):
  target='native_island_agent_'+kind;cpp=source.name if kind=='original'else'mutated.cpp'if kind=='negative'else kind+'.cpp';cmake+=f'add_executable({target} "{ROOT}/tests/native_tu15_island_agent_phase_tests.cpp" "{project}/{cpp}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=f'$<TARGET_FILE:{target}> "{fixture}"'if kind=='original'else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}"';cmake+=f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
 (project/'CMakeLists.txt').write_text(cmake);results=[]
 for compiler in('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),'--target','native_island_agent_original','--target','native_island_agent_negative','--target','native_island_agent_stride','--target','native_island_agent_threshold','--filter','^native_island_agent_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 files=[source,ROOT/'include/halo2/native_tu15_island_agent_phase.hpp',ROOT/'tests/native_tu15_island_agent_phase_tests.cpp',ROOT/'tools/observe_native_constraint_step_tu15.cpp',Path(__file__)];hashes={str(p.relative_to(ROOT)):sha(project/p.name if p==source else p)for p in files};temporary=[p for p in scratch.rglob('*')if p.is_file()];report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,required_backend=BACKEND,image_sha256=NEW_PIN,listing_sha256=meta['listing_sha256'],body_sites=meta['body_sites'],original_cases=len(all_cases),complete_original_calls=sum(c['actual_stop']==STOP for c in complete),genuine_service_entry_prefixes=sum(c['boundary']is not None for c in complete),captured_MCC_input_cases=sum(c.get('captured_mcc_inputs',False)for c in complete),post_store_observer_prefixes=len(observers),read_fault_prefixes=sum(c.get('fault',{}).get('access')!='write'for c in all_cases if 'fault'in c),store_fault_prefixes=sum(c.get('fault',{}).get('access')=='write'for c in all_cases if 'fault'in c),ordered_original_reads=sum(len(c['reads'])for c in all_cases),ordered_original_stores=sum(len(c['writes'])for c in all_cases),original_instructions=sum(c['original_instructions']for c in all_cases),genuine_timestamp_inputs=sum(len(c['timestamps'])for c in all_cases),source_hashes=hashes,current_sources_match_snapshot=all(sha(ROOT/p)==v for p,v in hashes.items()),fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,owned_scratch=str(scratch),retained_large_artifacts=[],temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in temporary),temporary_logical_bytes_before_cleanup=sum(p.stat().st_size for p in temporary),negative_controls=['Initial borrowed contact-buffer capacity flag changed','Minimum contact record stride changed48to32','Threshold source FMUL/FADD order changed'],excluded=['actual nonconverted allocation/agent/advance owner execution','post-agent-return phase branches without full live original comparison','platform-private SEH frame/unwind postimages','unmasked FP traps','full live world integration']);(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','complete_original_calls','genuine_service_entry_prefixes')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
