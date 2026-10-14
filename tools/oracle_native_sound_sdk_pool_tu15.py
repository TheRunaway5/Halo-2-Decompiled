#!/usr/bin/env python3
"""Validate actual native pool capacities against observed original kernel instructions.

Small allocation sizing runs the unchanged original allocation prefix; every
actual native small header is then consumed by the whole unchanged original
capacity getter on the host i386 CPU. Large requests independently execute the
original MM page-count arithmetic, without supplying physical PTEs or results.
The owned Complex4627/xemu kernel is identified explicitly. Kernel free lists,
physical availability, failure status, raw metadata and retail hardware parity
are not established by these capacity-contract checks.
"""
import argparse,json,hashlib,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_sandbox_first_person_hardware_tu15 import execute

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
class Code:
 def __init__(self,banks):self.banks=banks;self.decoder=Cs(CS_ARCH_X86,CS_MODE_32);self.decoder.detail=True;self.cache={}
 def at(self,pc):
  if pc not in self.cache:
   b=next(b for b in self.banks if b['address']<=pc< b['address']+len(b['bytes']));self.cache[pc]=next(self.decoder.disasm(b['bytes'][pc-b['address']:],pc,count=1))
  return self.cache[pc]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--native',type=Path,required=True);p.add_argument('--original',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--hardware-limit',type=int);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 native=json.loads((a.native/'report.json').read_text());reference=json.loads((a.original/'report.json').read_text());assert native['original_sha256']==reference['original_sha256']=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
 assert reference['source_factory_returned_normally'] and not any(reference[k]for k in('original_code_patches','original_memory_writes','original_register_writes','supplied_callee_returns'))
 assert not native['fixture_callee_results_used'] and native['actual_original_full_call_matches']==5 and native['native_allocations_after_abort']==0
 assert sha(reference['platform']['bios_path'])==reference['platform']['bios_sha256'] and sha(reference['platform']['emulator_path'])==reference['platform']['emulator_sha256']
 raw=(a.original/'kernel-pool-code.bin').read_bytes();k=reference['kernel_code'];assert hashlib.sha256(raw).hexdigest()==k['sha256'];banks=[dict(address=k['address'],bytes=raw)]
 for row in reference['memory_calls']:
  raw=(a.original/(row['kind']+'.bin')).read_bytes();assert hashlib.sha256(raw).hexdigest()==row['sha256'];banks.append(dict(address=row['address'],bytes=raw))
 code=Code(banks);entry=k['allocation_entry'];branch=code.at(entry+0xf);assert branch.mnemonic=='jbe';small_start=branch.operands[0].imm;small_stop=code.at(small_start+3).address+code.at(small_start+3).size;large_stop=branch.address+branch.size
 assert code.at(small_start).mnemonic=='lea'and code.at(small_start+3).mnemonic=='shr'
 core=next(b['address']for b in banks if b['address']==next(r['address']for r in reference['memory_calls']if r['kind']=='mm_allocate_core'));page_start=core+0x28;page_stop=core+0x3e
 assert code.at(page_start).mnemonic=='mov'and code.at(page_stop).mnemonic=='mov'
 machine=Uc(UC_ARCH_X86,UC_MODE_32);pages=set()
 for b in banks:pages.update(range(b['address']&~4095,(b['address']+len(b['bytes'])+4095)&~4095,4096))
 for page in sorted(pages):machine.mem_map(page,4096)
 for b in banks:machine.mem_write(b['address'],b['bytes'])
 stack=0x62000000;sp=stack+0x80;machine.mem_map(stack,4096);trace=[];stops=set()
 def hook(_,pc,size,user):
  if pc in stops:machine.emu_stop();return
  trace.append(pc);assert bytes(machine.mem_read(pc,size))==bytes(code.at(pc).bytes)
 machine.hook_add(UC_HOOK_CODE,hook)
 allocation_proofs=[];hardware=[];getter_banks=[dict(address=k['address'],hex=banks[0]['bytes'].hex(),sha256=k['sha256'])]
 for identity,c in enumerate(native['cases'],1):
  n=c['requested'];machine.mem_write(sp,struct.pack('<3I',0x30000000,n,0x626f5344));machine.reg_write(R.UC_X86_REG_ESP,sp);machine.reg_write(R.UC_X86_REG_EFLAGS,2);trace.clear();stops={small_stop,large_stop};machine.emu_start(entry,0,count=100)
  pc=machine.reg_read(R.UC_X86_REG_EIP);small=pc==small_stop;assert small==(n<=4056)
  if small:
   units=machine.reg_read(R.UC_X86_REG_EBX);header=bytes.fromhex(c['prefix8_hex']);assert len(header)==8 and header[3]==units and c['backing_bytes']==units*32 and c['client_offset']==8
   expected=units*32-8;allocation_proofs.append(dict(id=identity,requested=n,kind='small',original_units=units,instructions=list(trace)))
   # Execute the whole original getter on actual native metadata. No expected
   # capacity/callee result is supplied to original execution.
   pointer=c['pointer'];page=pointer&~4095
   if page not in pages:machine.mem_map(page,4096);pages.add(page)
   machine.mem_write(pointer-8,header);machine.mem_write(sp,struct.pack('<2I',0x30000000,pointer));machine.reg_write(R.UC_X86_REG_ESP,sp);machine.reg_write(R.UC_X86_REG_EFLAGS,2);trace.clear();stops={0x30000000}
   machine.mem_map(0x30000000,4096)if 0x30000000 not in pages else None;pages.add(0x30000000)
   machine.emu_start(k['capacity_entry'],0x30000000,count=32);assert machine.reg_read(R.UC_X86_REG_EIP)==0x30000000 and machine.reg_read(R.UC_X86_REG_EAX)==c['capacity_query_result']==expected
   abi=dict(gp=[0,0,0,0,sp,0,0,0],eflags=2,mxcsr=0x1f80,x87_control_word=0x27f,x87_status=0,x87_tag=65535,x87_values=[],xmm=['0'*32]*8,fs_base=0)
   hardware.append(dict(id=identity,entry=k['capacity_entry'],exit_entry=0x30000000,return_address=0x30000000,entry_abi=abi,initial_banks=[dict(address=b['address'],hex=b['hex'])for b in getter_banks]+[dict(address=pointer-8,hex=header.hex()),dict(address=sp,hex=struct.pack('<2I',0x30000000,pointer).hex())],exit={},original_instructions=list(trace),declared_cpu_inputs=[],native_capacity=c['capacity_query_result']))
  else:
   machine.mem_write(stack+0x14,struct.pack('<I',n));machine.reg_write(R.UC_X86_REG_EBP,stack);machine.reg_write(R.UC_X86_REG_EFLAGS,2);trace.clear();stops={page_stop};machine.emu_start(page_start,page_stop,count=32)
   count=machine.reg_read(R.UC_X86_REG_ESI);expected=count*4096;assert c['client_offset']==0 and c['pointer']%4096==0 and c['backing_bytes']==expected
   allocation_proofs.append(dict(id=identity,requested=n,kind='page',original_pages=count,instructions=list(trace)))
  assert c['client_capacity']==c['capacity_query_result']==expected and expected>=n
 selected=hardware[:a.hardware_limit]if a.hardware_limit else hardware;assert selected
 paired=dict(address_map={},instruction_map={f'{pc:08x}':pc for c in selected for pc in c['original_instructions']},entries={f'{c["entry"]:08x}':c['entry']for c in selected})
 raw=execute(dict(image='1.5',cases=selected),paired,(a.output/'hardware').resolve(),code_override=code,additional_original_code=getter_banks);compared=[]
 for before,after in zip(selected,raw['cases']):
  errors=[]
  if after['exit']['gp'][0]!=before['native_capacity']:errors.append('capacity')
  if after['exit']['gp'][4]!=sp+8:errors.append('stdcall_stack')
  if after['instructions']!=before['original_instructions']:errors.append('unicorn_hardware_trace')
  if any(b['hex']!=e['hex']for b,e in zip(before['initial_banks'],after['final_banks'])):errors.append('retained_memory')
  compared.append(dict(id=after['id'],status='pass'if not errors else'fail',errors=errors,instructions=len(after['instructions']),retained_bytes=sum(len(bytes.fromhex(b['hex']))for b in before['initial_banks'])))
 (a.output/'original-results.json').write_text(json.dumps(raw,separators=(',',':'))+'\n')
 report=dict(scope=__doc__,status='pass'if all(c['status']=='pass'for c in compared)else'fail',original_sha256=reference['original_sha256'],platform=reference['platform'],kernel_code=k,native_cases=len(native['cases']),small_native_cases=len(hardware),large_native_cases=len(native['cases'])-len(hardware),actual_original_full_call_matches=native['actual_original_full_call_matches'],allocation_capacity_calculations=allocation_proofs,hardware_cases=compared,
 original_instruction_patches=raw['original_instruction_patches'],native_expected_data_used=raw['native_expected_data_used'],fixture_callee_results_used=False,byte_masks_used=False,kernel_volatile_clobber_or_native_instruction_trace_equality_claimed=False,physical_availability_free_lists_raw_metadata_or_retail_hardware_verified=False,source_fs_not_consumed_by_this_integer_capacity_scope=True,
 tool_sha256=sha(__file__),native_capture_sha256=sha(a.native/'report.json'),original_results_sha256=sha(a.output/'original-results.json'),reference_capture_sha256={p.name:sha(p)for p in a.original.glob('*')if p.is_file()})
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(status=report['status'],native_cases=report['native_cases'],hardware_small_cases=len(compared),hardware_instructions=sum(c['instructions']for c in compared),large_calculation_cases=report['large_native_cases'],actual_original_calls=report['actual_original_full_call_matches'],byte_masks_used=False)))
 return int(report['status']!='pass')
if __name__=='__main__':raise SystemExit(main())
