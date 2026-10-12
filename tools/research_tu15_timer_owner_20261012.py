#!/usr/bin/env python3
"""Authenticate complete owned timer149/150 CFGs and retain missing owners.

Only static original instructions and actual export metadata are read. No
timer return, scheduler state, time, guest writes or physics result is supplied.
The reference is modified Complex4627/xemu, not physical Xbox execution.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--kernel',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
 a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 report=json.loads((a.kernel/'report.json').read_text());boot=a.kernel.parent
 v=json.loads((boot/'validation.json').read_text())
 assert v['kernel_report_sha256']==sha(a.kernel/'report.json') and v['original_resources_unchanged']
 assert report['tool_sha256']==sha(ROOT/'tools/capture_tu15_timer_exports_20261012.py')
 assert not v['title_executed'] and not v['guest_writes'] and not v['code_patches']
 metadata=(a.kernel/'export-metadata.bin').read_bytes();assert sha(a.kernel/'export-metadata.bin')==report['metadata_sha256']
 table=metadata[-report['export_count']*4:]
 exports={report['kernel_base']+struct.unpack_from('<I',table,i*4)[0]:report['export_ordinal_base']+i for i in range(report['export_count'])}
 dis=Cs(CS_ARCH_X86,CS_MODE_32);dis.detail=True;result={};all_calls=[]
 for row in report['timer_exports']:
  path=a.kernel/(row['name']+'-window.bin');assert sha(path)==row['sha256'];raw=path.read_bytes();start=row['address']
  todo=[start];sites={};calls=[]
  while todo:
   p=todo.pop()
   if p in sites:continue
   assert start<=p<start+len(raw)
   i=next(dis.disasm(raw[p-start:],p,count=1));sites[p]=i
   if i.mnemonic=='ret':continue
   if i.mnemonic=='call':
    assert i.operands[0].type==2
    target=i.operands[0].imm
    calls.append(dict(instruction=p,target=target,export_ordinal=exports.get(target),return_address=p+i.size))
   if i.mnemonic.startswith('j'):
    assert i.operands[0].type==2;todo.append(i.operands[0].imm)
    if i.mnemonic=='jmp':continue
   todo.append(p+i.size)
  end=max(p+i.size for p,i in sites.items());contiguous=list(dis.disasm(raw[:end-start],start))
  assert {i.address for i in contiguous}==set(sites),'Complete function includes an untraced instruction'
  lines=[f'{p:08x} {i.bytes.hex():24s} {i.mnemonic} {i.op_str}' for p,i in sorted(sites.items())]
  (a.output/(row['name']+'.asm')).write_text('\n'.join(lines)+'\n')
  (a.output/(row['name']+'.bin')).write_bytes(raw[:end-start])
  result[str(row['ordinal'])]=dict(address=start,bytes=end-start,instruction_count=len(sites),
   sha256=sha(a.output/(row['name']+'.bin')),calls=calls,
   source_instructions={f'{p:08x}':dict(raw=i.bytes.hex(),mnemonic=i.mnemonic,operand=i.op_str) for p,i in sorted(sites.items())})
  all_calls+=calls
 owned={row['address'] for row in report['timer_exports']}
 missing=sorted({c['target'] for c in all_calls}-owned)
 # The actual TU1.5 caller's signed relative request, decoded without a
 # default clock value. Scheduling ownership and expiry remain unresolved.
 due=struct.unpack('<q',struct.pack('<II',0xb5659000,0xffffffcd))[0]
 r=dict(schema=1,tool_sha256=sha(__file__),kernel_report_sha256=sha(a.kernel/'report.json'),
  boot_validation_sha256=sha(boot/'validation.json'),parity_target='English TU1.5 only',
  reference='owned modified Complex4627 in xemu0.8.136',complete_functions=result,
  missing_callees=[dict(address=p,export_ordinal=exports.get(p)) for p in missing],
  actual_title_call=dict(instruction=0x2e0096,encoded_ordinal=149,timer=0x56ede0,dpc=0x56edc0,
   due_time_signed_100ns=due,frame_bytes=20),
  title_executed=False,kernel_functions_executed=False,physical_xbox_proof=False,
  timer_results_supplied=[],scheduler_state_supplied=False,live_physics_parity=False)
 (a.output/'report.json').write_text(json.dumps(r,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(functions={k:(v['bytes'],v['instruction_count']) for k,v in result.items()},missing=r['missing_callees'],due_time_100ns=due)))
if __name__=='__main__':main()
