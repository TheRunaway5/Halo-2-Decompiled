#!/usr/bin/env python3
"""Compare boot publication and complete TU1.5 audio-helper branches with original CPU.

The boot publication interval is from the explicitly identified owned Complex4627
kernel, not asserted physical retail code. SMBus status/response/prior and altered
AV-pack/UserSettings values are declared test inputs. Query output is independently
computed by compiled primary code. No original callee return is inserted. Native
startup does not acquire a boot mode, construct a device or produce PCM here.
"""
import argparse,json,struct,hashlib
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_source_sound_startup_tu15 import replay,initial_abi,differences
from oracle_sandbox_first_person_hardware_tu15 import execute
from oracle_smg_firing_1_1 import Xbe
from generate_source_sound_sdk_tu15 import Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(p.read_text())
class KernelCode:
 def __init__(self,b):self.bank=b;self.decoder=Cs(CS_ARCH_X86,CS_MODE_32);self.decoder.detail=True;self.cache={}
 def at(self,pc):
  if pc not in self.cache:self.cache[pc]=next(self.decoder.disasm(self.bank['bytes'][pc-self.bank['address']:],pc,count=1))
  return self.cache[pc]
def primary_query(before,after,entry,user,writes):
 machine=Uc(UC_ARCH_X86,UC_MODE_32);banks=[dict(address=r['address'],data=bytes.fromhex(r['hex']))for r in before['regions']]
 for b in entry['banks']:banks.append(dict(address=b['address'],data=b['data']))
 pages=set();private=0x30040000;halt=0x30050000;sp=private+0x800
 for b in banks:pages.update(range(b['address']&~4095,(b['address']+len(b['data'])+4095)&~4095,4096))
 assert private not in pages and halt not in pages;pages.update((private,halt))
 for page in sorted(pages):machine.mem_map(page,4096)
 for b in banks:machine.mem_write(b['address'],b['data'])
 machine.mem_write(entry['entries']['EEPROMBacking']+96,user)
 source_sp=before['abi']['gp'][4];frame=struct.unpack('<6I',bytes(machine.mem_read(source_sp,24)));assert frame[0]==0x37dab2 and frame[1]==9 and frame[4:]==(4,0)
 machine.mem_write(sp,struct.pack('<6I',halt,*frame[1:]));machine.reg_write(R.UC_X86_REG_ESP,sp);ordered=[];outputs=((frame[2],frame[2]+4),(frame[3],frame[3]+4))
 def observe(_,access,address,size,value,opaque):
  if any(lo<=address and address+size<=hi for lo,hi in outputs):ordered.append(dict(instruction=0x2e101c,address=address,before_hex=bytes(machine.mem_read(address,size)).hex(),after_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
 machine.hook_add(UC_HOOK_MEM_WRITE,observe);machine.emu_start(entry['entries']['primary_query'],halt,count=100000)
 errors=[]
 if machine.reg_read(R.UC_X86_REG_EIP)!=halt or machine.reg_read(R.UC_X86_REG_ESP)!=sp+24:errors.append('primary_return')
 if machine.reg_read(R.UC_X86_REG_EAX)!=after['abi']['gp'][0]:errors.append('query_status')
 if ordered!=writes:errors.append('ordered_query_writes')
 for region in after['regions']:
  if bytes(machine.mem_read(region['address'],region['size'])).hex()!=region['hex']:errors.append(dict(kind='all_caller_memory',address=region['address']))
 return dict(status='pass'if not errors else'fail',errors=errors,ordered_writes=len(ordered),retained_bytes=sum(r['size']for r in after['regions']))
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in('native','original','primary','eeprom','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);native=load(a.native/'report.json');reference=load(a.original/'report.json');assert native['original_sha256']==reference['original_sha256']==NEW_PIN and sha(NEW_DEFAULT)==NEW_PIN
 assert native['reference_or_modified_inputs_test_only']and not native['ordinary_live_native_boot_mode_bound']and not native['native_smc_transport_implemented']and not native['mode_callee_result_supplied']
 for key in('original_code_patches','original_memory_writes','original_register_writes','supplied_callee_returns'):assert not reference[key]
 image=Xbe(NEW_DEFAULT);code=Code(image);cases=[];fragments=[];ends=[];queries=[]
 primary=load(a.primary/'manifest.json');primary['banks']=[dict(b,data=(a.primary/b['file']).read_bytes())for b in primary['banks']]
 for b in primary['banks']:assert len(b['data'])==b['size']and sha(a.primary/b['file'])==b['sha256']
 eeprom=a.eeprom.read_bytes();assert len(eeprom)==256;actual=eeprom[96:192]
 for row in native['helper_cases']:
  directory=a.native/row['directory'];user=bytes.fromhex(row['user_section_hex']);assert len(user)==96 and int.from_bytes(user[56:60],'little')==row['declared_audio_flags']
  assert row['actual_owned_settings']==(user==actual)
  before=load(directory/'query-before.json');after=load(directory/'query-after.json');query=primary_query(before,after,primary,user,row['query_writes']);query.update(id=row['id'],mode=row['mode'],checksum_valid=row['checksum_valid']);queries.append(query)
  for first,last in(('before.json','query-before.json'),('query-after.json','after.json')):
   before=load(directory/first);after=load(directory/last);case,fragment=replay(before,after,len(cases)+1,code)
   for pc in case['original_instructions']:
    ins=code.at(pc);bank=next(r for r in before['regions']if r['address']<=pc and pc+ins.size<=r['address']+r['size']);assert bytes.fromhex(bank['hex'])[pc-bank['address']:pc-bank['address']+ins.size]==image.read(pc,ins.size)
   assert not fragment['unicorn_errors'];case['native_case']=row['id'];cases.append(case);fragments.append(fragment);ends.append(after)
 paired=dict(address_map={},instruction_map={f'{pc:08x}':pc for c in cases for pc in c['original_instructions']},entries={f'{c["entry"]:08x}':c['entry']for c in cases})
 result=execute(dict(image='1.5',cases=cases),paired,(a.output/'helper-hardware').resolve(),code_override=code);compared=[]
 for c,end,f in zip(result['cases'],ends,fragments):
  errors=differences(c['exit'],initial_abi(end['abi']))
  for region,bank in zip(end['regions'],c['final_banks']):
   assert region['address']==bank['address']
   if region['hex']!=bank['hex']:errors.append(dict(kind='all_retained_memory',address=region['address']))
  if c['instructions']!=f['instructions']or c['instructions']!=end['instructions']:errors.append('original_unicorn_native_trace')
  compared.append(dict(id=c['id'],status='pass'if not errors else'fail',errors=errors,instructions=len(c['instructions']),retained_bytes=f['retained_bytes']))
 (a.output/'helper-original-results.json').write_text(json.dumps(result,separators=(',',':'))+'\n')
 # Extract an unchanged original code window covering both publication branches.
 section=next(b for b in reference['kernel_executable_sections']if b['address']<=0x80054f49<b['address']+b['bytes']);payload=(a.original/section['file']).read_bytes();assert sha(a.original/section['file'])==section['sha256']
 start,stop=0x80054f49,0x80054f58;window=payload[start-section['address']:stop-section['address']+4];bank=dict(address=start,bytes=window,hex=window.hex(),sha256=hashlib.sha256(window).hexdigest());kernel=KernelCode(bank)
 assert window[:15].hex()=='85c07c0b8b45f883e007a3ecaf0380'
 mode_address=reference['boot_mode_export']['address'];assert mode_address==0x8003afec
 machine=Uc(UC_ARCH_X86,UC_MODE_32);machine.mem_map(start&~4095,4096);machine.mem_map(mode_address&~4095,4096);machine.mem_map(0x30020000,4096);machine.mem_write(start,window);trace=[];machine.hook_add(UC_HOOK_CODE,lambda _,pc,size,opaque:trace.append(pc));publication=[]
 for row in native['producer_cases']:
  sp=0x30020800;machine.mem_write(sp,struct.pack('<I',row['response']));machine.mem_write(mode_address,struct.pack('<I',row['prior']));machine.reg_write(R.UC_X86_REG_EAX,row['status']);machine.reg_write(R.UC_X86_REG_EBP,sp+8);machine.reg_write(R.UC_X86_REG_EFLAGS,0x202);trace.clear();machine.emu_start(start,stop,count=8);assert machine.reg_read(R.UC_X86_REG_EIP)==stop
  abi=dict(gp=[row['status'],0,0,0,sp+64,sp+8,0,0],eflags=0x202,mxcsr=0x1f80,x87_control_word=0x27f,x87_status=0,x87_tag=65535,x87_values=[],xmm=['0'*32]*8,fs_base=0)
  publication.append(dict(id=len(publication)+1,entry=start,exit_entry=stop,return_address=0x30000000,entry_abi=abi,initial_banks=[dict(address=start,hex=window.hex()),dict(address=mode_address,hex=struct.pack('<I',row['prior']).hex()),dict(address=sp,hex=struct.pack('<I',row['response']).hex())],exit={},original_instructions=list(trace),declared_cpu_inputs=['Actual or declared prior, signed status and SMC output inputs; no transport result is generated here']))
 paired=dict(address_map={},instruction_map={f'{pc:08x}':pc for c in publication for pc in c['original_instructions']},entries={f'{start:08x}':start})
 boot=execute(dict(image='1.5',cases=publication),paired,(a.output/'boot-hardware').resolve(),code_override=kernel,additional_original_code=[bank]);boot_compared=[]
 for row,c,before in zip(native['producer_cases'],boot['cases'],publication):
  errors=[];value=int.from_bytes(bytes.fromhex(c['final_banks'][1]['hex']),'little')
  if value!=row['native_result']:errors.append('publication_value')
  if c['instructions']!=before['original_instructions']:errors.append('original_cpu_trace')
  if c['final_banks'][0]!=before['initial_banks'][0]or c['final_banks'][2]!=before['initial_banks'][2]:errors.append('unmodified_input_banks')
  boot_compared.append(dict(id=c['id'],status='pass'if not errors else'fail',errors=errors,instructions=len(c['instructions'])))
 (a.output/'boot-original-results.json').write_text(json.dumps(boot,separators=(',',':'))+'\n')
 all_cases=compared+queries+boot_compared;report=dict(scope=__doc__,status='pass'if all(c['status']=='pass'for c in all_cases)else'fail',original_sha256=NEW_PIN,platform=reference['platform'],whole_helper_cases=len(native['helper_cases']),source_cpu_cases=compared,primary_query_cases=queries,boot_publication_cases=boot_compared,mode6_operand='AND WORD [EBP-2],0 clears high16',native_instruction_trace_compared=True,byte_masks_used=False,fixture_callee_results_used=False,mode_callee_result_supplied=False,ordinary_live_native_boot_mode_bound=False,native_smc_transport_implemented=False,
 original_instruction_patches=result['original_instruction_patches']+boot['original_instruction_patches'],native_expected_data_used=result['native_expected_data_used']or boot['native_expected_data_used'],tool_sha256=sha(__file__),reference_capture_sha256={p.name:sha(p)for p in a.original.glob('*')if p.is_file()},native_capture_sha256={str(p.relative_to(a.native)):sha(p)for p in a.native.rglob('*.json')},primary_manifest_sha256=sha(a.primary/'manifest.json'),original_results_sha256={p.name:sha(p)for p in a.output.glob('*results.json')})
 report['boot_publication_native_instruction_trace_or_kernel_clobber_equality_claimed']=False
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(status=report['status'],whole_helpers=report['whole_helper_cases'],source_cpu_intervals=len(compared),source_instruction_events=sum(c['instructions']for c in compared),primary_queries=len(queries),boot_publication_cases=len(boot_compared),boot_instruction_events=sum(c['instructions']for c in boot_compared),ordinary_native_mode_bound=False)))
 return int(report['status']!='pass')
if __name__=='__main__':raise SystemExit(main())
