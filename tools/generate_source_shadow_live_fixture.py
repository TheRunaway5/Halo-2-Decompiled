#!/usr/bin/env python3
"""Freeze literal live335E0..32893 input/output banks; no guest advancement.

Original31F50 is also executed unchanged over the actual owner records.
4BAF0 and3E9C0 receive corroborated literal call inputs; their observed outputs
remain independent assertions. No observed return becomes a service hook.
"""
from pathlib import Path
import argparse,collections,hashlib,json,re,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from differential_source_model_fade import live_observations
from differential_source_local_player_priority import live as priority_live
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def h(raw,at=0):return struct.unpack_from('<H',raw,at)[0]
def n(raw,at=0):return struct.unpack_from('<I',raw,at)[0]
def main():
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--binary',type=Path,default=ROOT/'build-mp-lightmap/source_shadow_live_tests');args=a.parse_args()
 directory=ROOT/'analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl'
 capture=json.loads((directory/'capture.json').read_text());phases={p['name']:p for p in capture['phases']};sup=json.loads((directory/'supplement.json').read_text());cin=json.loads((directory/'cinematic-supplement.json').read_text());uniform=json.loads((directory/'uniform-capture.json').read_text())
 verified=0
 for ph in capture['phases']:
  assert ph['status']['running']==False and ph['clock_tick']==12475 and ph['view_index']==0
  for f in ph['files']:
   raw=(directory/f['file']).read_bytes();assert len(raw)==f['size'] and hashlib.sha256(raw).hexdigest()==f['sha256'];verified+=1
 for f in sup['files']:
  raw=(directory/f['file']).read_bytes();assert len(raw)==f['size'] and hashlib.sha256(raw).hexdigest()==f['sha256'];verified+=1
 for ph in uniform['phases']:
  assert not ph['status']['running'] and ph['clock_tick']==12475 and ph['frame']==111186
  for f in ph['files']:
   raw=(directory/f['file']).read_bytes();assert len(raw)==f['size'] and hashlib.sha256(raw).hexdigest()==f['sha256'];verified+=1
 assert cin['registers_unchanged'] and cin['byte5']==0 and not cin['after']['running']
 raw=(directory/cin['file']).read_bytes();assert hashlib.sha256(raw).hexdigest()==cin['sha256'];verified+=1
 assert (directory/'original335E0-before.bin').read_bytes()==(directory/'original335E0-after.bin').read_bytes()
 def data(phase,name):return (directory/f'{phase}-{name}.bin').read_bytes()
 phase='33400-entry';bank=data(phase,'object-handles');fade={int(o['owner'],16):o['fade_outputs'][1]for o in capture['fade_observations']};priority={int(o['owner'],16):o['priority']for o in capture['priority_observations']}
 owners=[];files={f['file']:f for f in phases[phase]['files']}
 for f in phases[phase]['files']:
  prefix=phase+'-object-'
  if not re.fullmatch(re.escape(prefix)+r'[0-9A-F]{8}\.bin',f['file']):continue
  handle=int(f['file'][len(prefix):-4],16);slot=handle&65535;header=bank[slot*12:(slot+1)*12];body=(directory/f['file']).read_bytes();definition=data(phase,f'definition-{handle:08X}')
  assert h(header)==handle>>16 and n(header,8)==int(f['address'],16) and len(body)==h(header,6)
  has_camo=body[0xaa]==0 or header[3]<2
  owners.append([handle,n(body,0x14),n(body,0xc),n(body,0x10),n(body,4),h(definition,0x18),body[0xaa],header[2],header[3],body[0x10a],n(body,0xbc),*words(body[0x30:0x40]),int(has_camo),n(body,0x2b0)if has_camo else 0,int(handle in fade),fade.get(handle,0),priority.get(handle,0xffffffff)])
 assert len(owners)==202 and all(len(o)==20 for o in owners)
 clock=data(phase,'clock');view=data(phase,'view-context');queue=data(phase,'main-object-queue');qcount=h(queue,4);assert qcount==41
 ids=words(data(phase,'main-queue-handles'));flags=struct.unpack('<'+'H'*qcount,data(phase,'main-queue-flags'))
 scratch=data(phase,'candidate-scratch');row=[3,phases[phase]['mode'],len(owners),qcount,n(clock,8),n(clock,4),*[n(view,i)for i in [0xa0,0xac,0xb8,0xc4]],n(view,0x150)]
 row += [v for o in owners for v in o]+[v for pair in zip(ids,flags)for v in pair]+[scratch[i*20+3]for i in range(32)]
 before=data('4A7C0-before-history','histories');cache_count=n(before,0x480);assert cache_count==1;row += [cache_count]+words(before[:36*cache_count])
 state=data('32893-after-part-admission','shadow-camera');lighting=data('32410-cached-opacity','cached-lighting');root=phases['32350-entry']['owner'];fade_word=phases['32350-entry']['fade_word'];cpu=(directory/'32893-source-cpu-draw-state.bin').read_bytes()
 row += [n(cpu,0x34),cin['byte5'],root,*words(lighting[0xc:0x18]),n(lighting,0x1c),fade_word,(n(cpu,0x7a80)>>10)&1]
 fade_provenance,fade_calls=live_observations();assert len(fade_calls)==len(fade)==2
 row += [len(fade_calls)];expected=[len(fade_calls)]
 for inputs,observed in fade_calls:
  identity=inputs[4];assert identity in fade and observed[1]==fade[identity]
  row += [len(inputs),*inputs,*observed]
  expected += [identity,*observed]
 x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
 priority_input,priority_provenance=priority_live(x);assert len(priority)==1 and priority_input[2]in priority
 priority_owner=priority_input[2];observed_priority=priority[priority_owner]
 row += [1,len(priority_input),*priority_input,observed_priority]
 expected += [1,priority_owner,observed_priority]
 expected += [phases['4A7C0-before-history']['candidate_count']]+words(data('4A7C0-before-history','candidates')[:20])
 expected += [phases['33632-after-selection']['candidate_count']]+words(data('33632-after-selection','selected-candidates')[:20])
 after=data('33632-after-selection','histories');expected += [n(after,0x480)]+words(after[:36])
 #32350 flow actually passed3D7F0, parentNONE and3D7C0, with local shadowfalse.
 stackphase=phases['32410-cached-opacity'];stack=data(stackphase['name'],'stack');stackbase=int(next(f['address']for f in stackphase['files']if f['file'].endswith('-stack.bin')),16)
 expected += [1,n(stack,stackphase['fields']['ESP']+0x14-stackbase),0,0]
 # Untouched31F50 traversal over actual virtual owner/header records.
 assert (directory/'uniform-producer-code-before.bin').read_bytes()==(directory/'uniform-producer-code-after.bin').read_bytes()==x.read(0x4a1e0,0x340)+x.read(0x48ee0,0x580)
 u=Uc(UC_ARCH_X86,UC_MODE_32);pages=set()
 def put(addr,raw):
  for p in range(addr&~4095,(addr+len(raw)+4095)&~4095,4096):
   if p not in pages:u.mem_map(p,4096);pages.add(p)
  u.mem_write(addr,raw)
 for s in x.sections:put(s.va,x.section_bytes(s))
 for f in phases[phase]['files']:
  if f['file'].startswith((phase+'-object-',)) or f['file'] in [phase+'-object-header.bin',phase+'-object-handles.bin']:
   put(int(f['address'],16),(directory/f['file']).read_bytes())
 put(0x4e0300,pack([int(files[phase+'-object-header.bin']['address'],16)]))
 STOP=0x3000000;TEMP=0x2100000;STACK=0x2200000;put(STOP,b'\xf4');put(TEMP,bytes(0x100));put(STACK-4096,bytes(8192))
 original_owner=next(o for o in owners if o[0]==root);put(TEMP,pack(original_owner[11:15]+words(lighting[0xc:0x18])+[original_owner[14],original_owner[14]]))
 visited=[];code={}
 def observe(g,pc,size,_):
  if pc==STOP:g.emu_stop();return
  assert bytes(g.mem_read(pc,size))==x.read(pc,size),(hex(pc),'instruction drift');code[pc]=size
  if pc==0x31f99:visited.extend(words(g.mem_read(g.reg_read(UC_X86_REG_EDI)+0x30,16)))
 u.hook_add(UC_HOOK_CODE,observe);put(STACK,pack([STOP,root,TEMP,TEMP+16,TEMP+28,TEMP+32]));u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_FPCW,0x23f);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,0x1fb0);u.emu_start(0x31f50,STOP,count=100000)
 assert u.reg_read(UC_X86_REG_EIP)==STOP and len(visited)%4==0;expected += [len(visited)//4]+visited
 hierarchy_code_count=len(code)
 # Complete unchanged3E9C0/155760/BAF80 consumes the existing actual202-owner
 # bank, not a reconstructed owner-lookup service. Later observer/local-user
 # raw inputs are independently corroborated in priority_provenance.
 priority_directory=directory.parent/'priority-metadata-3290A'
 priority_manifest=json.loads((priority_directory/'capture.json').read_text());priority_files={f['file']:f for f in priority_manifest['files']}
 for name in ['local-slot.bin','players-global-pointer.bin','players-global.bin','observer-bank.bin','observer-special-byte.bin']:
  record=priority_files[name];raw=(priority_directory/name).read_bytes();assert len(raw)==record['size'] and hashlib.sha256(raw).hexdigest()==record['sha256'];put(int(record['address'],16),raw)
 priority_code_before=set(code);put(STACK,pack([STOP]));u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_ESI,priority_owner);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_MXCSR,0x1fb0)
 u.emu_start(0x3e9c0,STOP,count=100000);assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==STACK+4
 original_priority_eax=u.reg_read(UC_X86_REG_EAX);assert original_priority_eax&255==observed_priority
 observed_priority_event=next(o for o in capture['priority_observations']if int(o['owner'],16)==priority_owner)
 observed_eax=int(re.search(r'EAX=([0-9a-fA-F]+)',observed_priority_event['exit_registers']).group(1),16);assert original_priority_eax==observed_eax
 priority_code_count=len(set(code)-priority_code_before)
 priority_functions={f'{addr:08X}':{'size':size,'sha256':hashlib.sha256(x.read(addr,size)).hexdigest()}for addr,size in [(0x3e9c0,0x65),(0x155760,0x60),(0xbaf80,0x28)]}
 finalphase=phases['32893-after-part-admission'];stack=data(finalphase['name'],'stack');stackbase=int(next(f['address']for f in finalphase['files']if f['file'].endswith('-stack.bin')),16);esp=finalphase['fields']['ESP']
 sv=lambda offset:n(stack,esp+offset-stackbase)
 # Literal32350/C1A80 output locations from original disassembly, no native math.
 frame_words=[*words(state[:12]),*words(state[0x28:0x34]),*words(state[0x34:0x40]),*[sv(i)for i in [0x3c,0x40,0x44]],*[sv(i)for i in [0x98,0x9c,0xa0]],*[sv(i)for i in [0x54,0x58,0x5c]],*[sv(i)for i in [0x60,0x64,0x68]],sv(0x24),sv(0x20),n(state,0xa8),n(state,0xb4),n(state,0xb8),sv(0x94),n(state,0xb0),n(state,0x74)]
 assert len(frame_words)==29;expected += [1]+frame_words
 expected += words(data('3290A-uniform-output','projection-c31-39')[:24*4])+words(data('3290A-uniform-output','constants-c40-48'))
 fixture=ROOT/'tests/fixtures/retail_source_shadow_live.tsv';temporary=fixture.with_suffix('.tmp');temporary.write_text('# Actual335E0..32893 OpenGL/llvmpipe copied-session phase; complete4BAF0/3E9C0 inputs corroborated with later3290A metadata and independent observed outputs. No math/lookup/return hooks.\n'+' '.join(f'{v:x}'for v in row)+' | '+' '.join(f'{v:x}'for v in expected)+'\n');temporary.replace(fixture)
 actual=words(b'')
 output=subprocess.check_output([str(args.binary.resolve()),'--probe'],input=' '.join(f'{v:x}'for v in row)+'\n',text=True);actual=[int(v,16)for v in output.split()]
 diffs=[{'word':i,'native':f'{a:08X}','source':f'{b:08X}'}for i,(a,b)in enumerate(zip(actual,expected))if a!=b]
 report={'backend':'SourceBackendOpenGL, llvmpipe; actual source CPU phase, not physicalXbox or retainedframe20 GPU','source_executable_sha256':PIN,'capture_manifest_sha256':hashlib.sha256((directory/'capture.json').read_bytes()).hexdigest(),'uniform_manifest_sha256':hashlib.sha256((directory/'uniform-capture.json').read_bytes()).hexdigest(),'verified_capture_files':verified,'owners':len(owners),'literal_collector_entries':qcount,'source_tick':n(clock,8),'source_frame':n(cpu,0x34),'source_view':phases[phase]['view_index'],'actual_observed_fades':capture['fade_observations'] and {f'{k:08X}':v for k,v in fade.items()},'actual_observed_priorities':{f'{k:08X}':v for k,v in priority.items()},'reconstructed_model_fade_calls':len(fade_calls),'fade_call_provenance':fade_provenance,'fade_output_expectations':'Both bytes are independently observed source outputs; live test computes source_model_fade, checks both, then binds only its computed second byte into candidates/rootfade. Unobserved outputs remain absent.','candidate_count':phases['4A7C0-before-history']['candidate_count'],'selected_count':phases['33632-after-selection']['candidate_count'],'hierarchy_owner_count':len(visited)//4,'original31F50_executed_instruction_addresses':hierarchy_code_count,'original3E9C0_executed_instruction_addresses':priority_code_count,'unchanged_priority_functions':priority_functions,'reconstructed_priority_calls':1,'priority_call_provenance':priority_provenance,'priority_output_expectations':'AL is an independently captured result; unchanged3E9C0/155760/BAF80 replay against actual202owners matches it. Live native test computes source_local_player_priority, asserts its AL independently, and supplies only the computed byte to candidates.','output_words':len(expected),'native_words':len(actual),'differences':diffs,'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'boundaries':['Two4BAF0 calls use actual observed metrics and corroborated complete later3290A metadata; temporal perfield provenance and Chief unusedflag30 change are explicit. Complete producer logic is source_model_fade; no simultaneous allinput export is claimed.3E9C0 uses actual prior33400root/header/view inputs plus corroborated later3290A observer/local-user metadata. Original observed AL remains independent; no simultaneous observer export is claimed.','Collector/owner registration/PVS/history are actual source phase inputs; production chronology not integrated.','NativeSourceShadowFrame compared to actual32350/C1A80 frame fields. c31..36/c40..48 compare actual original4A1E0/48EE0 output at3290A, with actual featurebit10. No uniform math/upload service hooks.','Owncopiedsession paused3290A after bounded original advancement; fixture generation performs no further guest mutation or advance. Original31F50 executes only in an independent Unicorn clone.']}
 (ROOT/'analysis/multiplayer-shading/shadow-live-original.json').write_text(json.dumps(report,indent=2)+'\n');fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));assert len(actual)==len(expected)and not diffs
if __name__=='__main__':main()
