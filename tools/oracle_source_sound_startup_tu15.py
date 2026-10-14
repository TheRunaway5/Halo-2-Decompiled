#!/usr/bin/env python3
"""Replay only original CPU intervals between genuine TU1.5 sound I/O calls.

Every interval starts from real stopped original state and ends before an I/O
entry or at the actual caller. Original kernel/device/DSP calls are not run on
the host and no return is supplied to original execution. Whole original calls
and I/O results were independently observed without guest mutations in xemu.
"""
import argparse,bisect,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_sandbox_first_person_hardware_tu15 import execute

GP=[getattr(R,'UC_X86_REG_'+n)for n in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def layout(s):return [(r['address'],r['size'],r['writable'])for r in s['regions']]
def initial_abi(a):
 return dict(gp=a['gp'],eflags=a['eflags'],mxcsr=a['mxcsr'],x87_control_word=a['x87_control'],x87_status=a['x87_status'],x87_tag=a['x87_tag'],x87_values=a['x87_logical_hex'][:a['x87_depth']],xmm=[f'{sum(v<<(32*k)for k,v in enumerate(lanes)):032x}'for lanes in a['xmm']],fs_base=a.get('fs_base',0))
def differences(actual,expected):
 errors=[]
 for key in ('gp','mxcsr','x87_status','x87_values','xmm'):
  if actual[key]!=expected[key]:errors.append(dict(kind=key,original=actual[key],observed=expected[key]))
 # Full raw xemu/host encodings are retained separately. This ABI carries
 # logical x87 values/depth and defined control bits, not reserved CW bits or
 # a stale target-protocol classification of an occupied physical register.
 occupied=lambda tag:sum(((((tag>>(i*2))&3)!=3)<<i)for i in range(8))
 if occupied(actual['x87_tag'])!=occupied(expected['x87_tag']):errors.append(dict(kind='x87_occupancy'))
 if (actual['x87_control_word']&0x1f3f)!=(expected['x87_control_word']&0x1f3f):errors.append(dict(kind='defined_x87_control'))
 if (actual['eflags']&0xcd5)!=(expected['eflags']&0xcd5):errors.append(dict(kind='defined_flags',original=actual['eflags'],observed=expected['eflags']))
 return errors

def replay(before,after,identity,code):
 assert layout(before)==layout(after),'Original CPU interval changed retained storage ownership'
 machine=Uc(UC_ARCH_X86,UC_MODE_32);pages=set();starts=[r['address']for r in before['regions']]
 for r in before['regions']:pages.update(range(r['address']&~4095,(r['address']+r['size']+4095)&~4095,4096))
 runs=[]
 for p in sorted(pages):
  if runs and p==runs[-1][1]:runs[-1][1]+=4096
  else:runs.append([p,p+4096])
 for p,end in runs:machine.mem_map(p,end-p)
 for r in before['regions']:machine.mem_write(r['address'],bytes.fromhex(r['hex']))
 a=before['abi']
 if a.get('fs_base'):
  gdt=0x30010000;assert gdt not in pages
  def segment(base,access):return struct.pack('<HHBBBB',0xffff,base&65535,(base>>16)&255,access,0xcf,(base>>24)&255)
  machine.mem_map(gdt,4096);machine.mem_write(gdt+8,segment(0,0x9b)+segment(0,0x93)+segment(a['fs_base'],0x93));machine.reg_write(R.UC_X86_REG_GDTR,(0,gdt,4095,0))
  for register,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):machine.reg_write(register,value)
  assert machine.reg_read(R.UC_X86_REG_FS_BASE)==a['fs_base']
 top=(a['x87_status']>>11)&7
 for i,raw in enumerate(a['x87_logical_hex'][:a['x87_depth']]):machine.reg_write(R.UC_X86_REG_FP0+((i+top)&7),struct.unpack('<QH',bytes.fromhex(raw)))
 machine.reg_write(R.UC_X86_REG_FPCW,a['x87_control']);machine.reg_write(R.UC_X86_REG_FPSW,a['x87_status']);machine.reg_write(R.UC_X86_REG_FPTAG,a['x87_tag']);machine.reg_write(R.UC_X86_REG_MXCSR,a['mxcsr'])
 for register,value in zip(GP,a['gp']):machine.reg_write(register,value)
 for i,lanes in enumerate(a['xmm']):machine.reg_write(R.UC_X86_REG_XMM0+i,sum(v<<(32*k)for k,v in enumerate(lanes)))
 machine.reg_write(R.UC_X86_REG_EFLAGS,a['eflags'])
 trace=[];touched=set()
 def instruction(_,pc,size,user):
  assert code.inside(pc),f'CPU interval executed external owner {pc:08x}'
  i=code.at(pc);assert a.get('fs_base')or all(not(op.type==3 and op.mem.segment and i.reg_name(op.mem.segment)=='fs')for op in i.operands),'Actual FS base needs its original producer'
  trace.append(pc)
 def memory(_,access,address,size,value,user):
  i=bisect.bisect_right(starts,address)-1;assert i>=0 and address+size<=starts[i]+before['regions'][i]['size'],f'Unretained original operand {address:08x}+{size:x}'
  touched.update(range(address&~4095,(address+size+4095)&~4095,4096))
 machine.hook_add(UC_HOOK_CODE,instruction);machine.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,memory)
 # The I/O target is a stopped boundary, not executed original SDK code.
 stop=after['pc'];stop_page=stop&~4095
 if stop_page not in pages:
  machine.mem_map(stop_page,4096)
 machine.emu_start(before['pc'],stop,count=1000000)
 assert machine.reg_read(R.UC_X86_REG_EIP)==stop
 result=dict(gp=[machine.reg_read(r)for r in GP],eflags=machine.reg_read(R.UC_X86_REG_EFLAGS),mxcsr=machine.reg_read(R.UC_X86_REG_MXCSR),x87_control_word=machine.reg_read(R.UC_X86_REG_FPCW),x87_status=machine.reg_read(R.UC_X86_REG_FPSW),x87_tag=machine.reg_read(R.UC_X86_REG_FPTAG))
 t=(result['x87_status']>>11)&7
 result['x87_values']=[struct.pack('<QH',*machine.reg_read(R.UC_X86_REG_FP0+((i+t)&7))).hex()for i in range(after['abi']['x87_depth'])]
 result['xmm']=[f'{machine.reg_read(R.UC_X86_REG_XMM0+i):032x}'for i in range(8)]
 errors=differences(result,initial_abi(after['abi']))
 for r in after['regions']:
  expected=bytes.fromhex(r['hex']);actual=bytes(machine.mem_read(r['address'],r['size']))
  if actual!=expected:
   index=next(i for i,(a,b)in enumerate(zip(actual,expected))if a!=b);errors.append(dict(kind='memory',address=r['address']+index))
 hardware=dict(id=identity,entry=before['pc'],exit_entry=stop,return_address=0x30000000,entry_abi=initial_abi(before['abi']),initial_banks=[dict(address=r['address'],hex=r['hex'])for r in before['regions']],exit={},original_instructions=trace,declared_cpu_inputs=[])
 return hardware,dict(id=identity,entry=before['pc'],boundary=stop,instructions=trace,unicorn_errors=errors,read_write_pages=len(touched),retained_bytes=sum(r['size']for r in before['regions']))

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 captured=json.loads((a.capture/'report.json').read_text());assert captured['original_sha256']==NEW_PIN and captured['original_cold_boot']
 assert not any(captured[k]for k in ('original_code_patches','original_memory_writes','original_register_writes','supplied_callee_returns'))
 cases=[];fragments=[];expected={};owners=[]
 for name in ('environment','mapping'):
  directory=a.capture/name;report=json.loads((directory/'report.json').read_text());assert report['original_returned_normally'] and report['original_sha256']==NEW_PIN
  before=json.loads((directory/'before.json').read_text());first_id=len(cases)+1
  for r in before['regions']:
   if r['label']=='.text':assert bytes.fromhex(r['hex'])==image.read(r['address'],r['size']),'Original .text bytes were modified'
  sequence=[]
  for call in report['calls']:
   row=json.loads((directory/f'call-{call["id"]:02}.json').read_text())
   assert row['entry']==row['before']['pc'] and row['after']['pc']==row['return_pc']
   sequence.append((before,row['before']));before=row['after']
  sequence.append((before,json.loads((directory/'after.json').read_text())))
  for before,after in sequence:
   identity=len(cases)+1;case,fragment=replay(before,after,identity,code);cases.append(case);fragment['owner']=name;fragments.append(fragment);expected[identity]=(after,sequence[0][0]['abi']['gp'][4])
  owners.append(dict(owner=name,entry=report['entry'],original_io_calls=len(report['calls']),fragment_ids=list(range(first_id,len(cases)+1))))
 paired=dict(address_map={},instruction_map={f'{pc:08x}':pc for c in cases for pc in c['original_instructions']},entries={f'{c["entry"]:08x}':c['entry']for c in cases})
 hardware=execute(dict(image='1.5',cases=cases),paired,(a.output/'hardware').resolve())
 compared=[]
 for c in hardware['cases']:
  end,parent_sp=expected[c['id']];observed_abi=initial_abi(end['abi']);errors=differences(c['exit'],observed_abi)
  encoding=[dict(field=k,host=c['exit'][k],xemu=observed_abi[k])for k in ('x87_control_word','x87_tag')if c['exit'][k]!=observed_abi[k]]
  written_live=set()
  for w in c['writes']:
   written_live.update(p for p in range(w['address'],w['address']+len(w['written'])//2)if p>=end['abi']['gp'][4])
  residue=[]
  for r,b in zip(end['regions'],c['final_banks']):
   assert r['address']==b['address'];observed=bytes.fromhex(r['hex']);actual=bytes.fromhex(b['hex'])
   changed=[r['address']+i for i,(x,y)in enumerate(zip(actual,observed))if x!=y]
   if not changed:continue
   if not r['label'].startswith('stack'):
    errors.append(dict(kind='persistent_memory',address=changed[0],bytes=len(changed)))
   else:
    owned=[p for p in changed if p>=parent_sp or p in written_live]
    if owned:errors.append(dict(kind='source_written_live_or_caller_stack',address=owned[0],bytes=len(owned)))
    residue.append(dict(address=r['address'],differences=changed,scope='Unwritten/dead private stack residue; both complete buffers remain retained. Its producer is not reconstructed by this CPU interval.'))
  fragment=fragments[c['id']-1]
  if c['instructions']!=fragment['instructions']:errors.append(dict(kind='trace'))
  compared.append(dict(id=c['id'],status='pass'if not errors else'fail',errors=errors,original_instructions=len(c['instructions']),retained_bytes=fragment['retained_bytes'],raw_fp_encoding_differences=encoding,unowned_private_stack_residue=residue))
 (a.output/'original-results.json').write_text(json.dumps(hardware,separators=(',',':'))+'\n')
 report=dict(scope=__doc__,status='pass'if all(c['status']=='pass'for c in compared)else'fail',original_sha256=NEW_PIN,tool_sha256=sha(Path(__file__)),capture=str(a.capture.resolve()),owners=owners,fragments=fragments,hardware_cases=compared,
  original_code_patches=hardware['original_instruction_patches'],native_expected_data_used=hardware['native_expected_data_used'],original_callee_returns_supplied=False,native_device_dsp_or_pcm_verified=False,full_xemu_private_stack_and_raw_fp_encoding_equality=False,
  acceptance='Exact native CPU comparisons use every host-original computed bank and complete logical ABI. Genuine xemu caller results, persistent banks and source-written live stack agree; raw FP encodings and unowned private stack residue remain explicit diagnostics.',
  original_results_path=str((a.output/'original-results.json').resolve()),original_results_sha256=sha(a.output/'original-results.json'),capture_sha256={str(p.relative_to(a.capture)):sha(p)for p in a.capture.rglob('*.json')})
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(status=report['status'],whole_original_calls=len(owners),original_io_calls=sum(o['original_io_calls']for o in owners),cpu_fragments=len(compared),original_instructions=sum(c['original_instructions']for c in compared))))
 return int(report['status']!='pass')
if __name__=='__main__':raise SystemExit(main())
