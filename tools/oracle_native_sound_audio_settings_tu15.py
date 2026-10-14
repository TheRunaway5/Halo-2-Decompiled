#!/usr/bin/env python3
"""Compare actual native audio settings and following SDK CPU with independent primary code.

The checksum/query expression is independently compiled from pinned primary
source. Only the actual non-key UserSettings resource (or a declared checksum
negative input) is provided. Original SDK CPU intervals execute unchanged;
boot SMC mode, a device result and PCM are never supplied.
"""
import argparse,json,struct,bisect
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_source_sound_startup_tu15 import sha,replay,initial_abi,differences
from generate_source_sound_sdk_tu15 import Code
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_sandbox_first_person_hardware_tu15 import execute

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--primary',type=Path,required=True);p.add_argument('--eeprom',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 root=json.loads((a.capture/'report.json').read_text());assert root['original_sha256']==NEW_PIN and not root['reference_boot_mode_used_as_native_input']
 primary=json.loads((a.primary/'manifest.json').read_text());actual=a.eeprom.read_bytes();assert len(actual)==256;user=actual[96:192];assert primary['user_offset']==96 and primary['user_bytes']==96
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=Code(image);cases=[];fragments=[];ends=[];queries=[]
 for condition in('valid','invalid'):
  directory=a.capture/condition;evidence=json.loads((directory/'report.json').read_text());assert evidence['checksum_valid']==(condition=='valid')and not evidence['fixture_callee_results_used']and not evidence['boot_mode_value_supplied']and not evidence['source_device_or_ready_supplied']
  assert evidence['native_audio_queries']==evidence['unavailable_boot_mode_requests']==1 and evidence['native_allocations_after_abort']==evidence['native_locks_after_abort']==0 and evidence['partial_source_state_faulted']
  assert len(evidence['native_platform_calls'])==6;start='before.json'
  for number in range(1,8):
   stop=f'call-{number}-before.json'if number<=6 else'mode-before.json';before=json.loads((directory/start).read_text());after=json.loads((directory/stop).read_text());identity=len(cases)+1;case,fragment=replay(before,after,identity,code)
   for pc in case['original_instructions']:
    i=code.at(pc);r=next(r for r in before['regions']if r['address']<=pc and pc+i.size<=r['address']+r['size']);assert bytes.fromhex(r['hex'])[pc-r['address']:pc-r['address']+i.size]==image.read(pc,i.size)
   cases.append(case);fragment['condition']=condition;fragments.append(fragment);ends.append(after);start=f'call-{number}-after.json'
  # Fresh independently compiled primary query over actual caller memory.
  before=json.loads((directory/'call-6-before.json').read_text());after=json.loads((directory/'call-6-after.json').read_text());machine=Uc(UC_ARCH_X86,UC_MODE_32);pages=set()
  retained=before['regions'];banks=[dict(address=r['address'],data=bytes.fromhex(r['hex']))for r in retained]
  for b in primary['banks']:
   raw=(a.primary/b['file']).read_bytes();assert len(raw)==b['size']and sha(a.primary/b['file'])==b['sha256'];banks.append(dict(address=b['address'],data=raw))
  private=0x30020000;call_sp=private+0x800;halt=0x30030000
  for b in banks:pages.update(range(b['address']&~4095,(b['address']+len(b['data'])+4095)&~4095,4096))
  assert private not in pages and halt not in pages;pages.update((private,halt))
  for page in sorted(pages):machine.mem_map(page,4096)
  for b in banks:machine.mem_write(b['address'],b['data'])
  input_user=user if condition=='valid'else bytes([user[0]^1])+user[1:];machine.mem_write(primary['entries']['EEPROMBacking']+96,input_user)
  sp=before['abi']['gp'][4];raw=bytes(machine.mem_read(sp,24));frame=struct.unpack('<6I',raw);assert frame[0]==0x37dab2 and frame[1]==9 and frame[4:]==(4,0)
  machine.mem_write(call_sp,struct.pack('<6I',halt,*frame[1:]));machine.reg_write(R.UC_X86_REG_ESP,call_sp);writes=[];outputs=((frame[2],frame[2]+4),(frame[3],frame[3]+4))
  def write(_,access,address,size,value,user):
   if any(lo<=address and address+size<=hi for lo,hi in outputs):writes.append(dict(instruction=0x2e101c,address=address,before_hex=bytes(machine.mem_read(address,size)).hex(),after_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
  machine.hook_add(UC_HOOK_MEM_WRITE,write);machine.emu_start(primary['entries']['primary_query'],halt,count=100000)
  errors=[]
  if machine.reg_read(R.UC_X86_REG_EIP)!=halt or machine.reg_read(R.UC_X86_REG_ESP)!=call_sp+24:errors.append('primary_return')
  if machine.reg_read(R.UC_X86_REG_EAX)!=evidence['query_status']:errors.append('query_status')
  if writes!=evidence['query_writes']:errors.append('ordered_query_writes')
  for region in after['regions']:
   if bytes(machine.mem_read(region['address'],region['size'])).hex()!=region['hex']:errors.append(dict(kind='exact_caller_memory',address=region['address']))
  queries.append(dict(condition=condition,status='pass'if not errors else'fail',errors=errors,actual_query_status=machine.reg_read(R.UC_X86_REG_EAX),ordered_output_writes=len(writes),all_caller_retained_bytes=sum(r['size']for r in retained),null_result_length=True,primary_audio_offset=56))
 paired=dict(address_map={},instruction_map={f'{pc:08x}':pc for c in cases for pc in c['original_instructions']},entries={f'{c["entry"]:08x}':c['entry']for c in cases})
 raw=execute(dict(image='1.5',cases=cases),paired,(a.output/'hardware').resolve(),code_override=code);compared=[]
 for c,end,f in zip(raw['cases'],ends,fragments):
  expected=initial_abi(end['abi']);errors=differences(c['exit'],expected)
  for region,bank in zip(end['regions'],c['final_banks']):
   assert region['address']==bank['address']
   if region['hex']!=bank['hex']:errors.append(dict(kind='exact_memory',address=region['address']))
  if c['instructions']!=f['instructions']:errors.append(dict(kind='unicorn_original_trace'))
  if c['instructions']!=end['instructions']:errors.append(dict(kind='native_trace'))
  compared.append(dict(id=c['id'],condition=f['condition'],status='pass'if not errors else'fail',errors=errors,instructions=len(c['instructions']),retained_bytes=f['retained_bytes'],raw_fp_encodings=[dict(field=k,host=c['exit'][k],native=expected[k])for k in('x87_control_word','x87_tag')if c['exit'][k]!=expected[k]]))
 (a.output/'original-results.json').write_text(json.dumps(raw,separators=(',',':'))+'\n')
 report=dict(scope=__doc__,status='pass'if all(c['status']=='pass'for c in compared+queries)else'fail',original_sha256=NEW_PIN,source_cpu_cases=compared,primary_query_cases=queries,fragments=fragments,
 actual_non_key_resource_sha256=__import__('hashlib').sha256(user).hexdigest(),primary_manifest_sha256=sha(a.primary/'manifest.json'),tool_sha256=sha(Path(__file__)),original_results_sha256=sha(a.output/'original-results.json'),
 original_instruction_patches=raw['original_instruction_patches'],native_expected_data_used=raw['native_expected_data_used'],byte_masks_used=False,fixture_callee_results_used=False,boot_mode_value_supplied=False,kernel_query_clobber_or_physical_pcm_equality_claimed=False,
 capture_sha256={str(p.relative_to(a.capture)):sha(p)for p in a.capture.rglob('*.json')})
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(status=report['status'],source_cpu_intervals=len(compared),source_instruction_events=sum(c['instructions']for c in compared),primary_query_cases=len(queries),primary_output_writes=sum(c['ordered_output_writes']for c in queries),byte_masks_used=False,boot_mode_supplied=False)))
 return int(report['status']!='pass')
if __name__=='__main__':raise SystemExit(main())
