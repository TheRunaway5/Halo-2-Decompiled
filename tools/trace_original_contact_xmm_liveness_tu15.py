#!/usr/bin/env python3
"""Bounded actual TU1.5 XMM3 gap-lane lifetime after contact-copy stores.

Runs the retained exact frame with unchanged original instructions, using only
hardware execute breakpoints and single stepping. No runtime or raw gate changes.
All ELF/output/heap products belong to run_test.py scratch; reports are bounded.
"""
import argparse,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
import capstone
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
GDB=r'''
set pagination off
set confirm off
set disable-randomization off
set debuginfod enabled off
run > DEBUGGEE
python
import gdb,json,struct,sys,hashlib
sys.path.insert(0,CAPSTONE_PATH)
import capstone as C
from capstone.x86 import X86_OP_REG,X86_OP_MEM,X86_OP_IMM
md=C.Cs(C.CS_ARCH_X86,C.CS_MODE_32);md.detail=True
seeds={0x30482f:0x30482b,0x3042ca:0x3042c7};root=0x1ccaca
segments=[];instructions={};total=0

def reg(n):return int(gdb.parse_and_eval('$'+n))&0xffffffff

def raw(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))

def state():return dict(gp={n:reg(n)for n in('eax','ecx','edx','ebx','esp','ebp','esi','edi')},xmm3=hex(int(gdb.parse_and_eval('$xmm3.uint128'))),cw=reg('fctrl'),sw=reg('fstat'),mxcsr=reg('mxcsr'))

def decode(pc):
 i=next(md.disasm(raw(pc,15),pc,count=1));key=hex(pc);row=dict(pc=pc,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)
 if key in instructions:assert instructions[key]==row
 else:instructions[key]=row
 return i

def x3(i,o):return o.type==X86_OP_REG and i.reg_name(o.reg)=='xmm3'

def assess(i,mask):
 # Track the original register value's lanes until a real definition removes
 # them. A new memory load kills the previous register provenance; it is not
 # a claim about all memory or later source origins of that new value.
 ops=i.operands;mn=i.mnemonic
 reads,writes=i.regs_access();names_read={i.reg_name(r)for r in reads};names_write={i.reg_name(r)for r in writes}
 if 'xmm3'not in names_read and 'xmm3'not in names_write:return mask,None
 if len(ops)>=2 and x3(i,ops[0]):
  if mn in('xorps','xorpd','pxor')and x3(i,ops[1]):return 0,dict(kind='self_zero_definition')
  if mn in('movaps','movups','movapd','movupd','movdqa','movdqu'):
   if x3(i,ops[1]):return mask,dict(kind='identity_transport')
   return 0,dict(kind='full_register_definition',source=i.op_str)
  if mn in('movd','movq'):return 0,dict(kind='zero_upper_definition',source=i.op_str)
  if mn in('movss','movsd'):
   if ops[1].type==X86_OP_MEM:return 0,dict(kind='memory_scalar_zero_upper_definition',source=i.op_str)
   if x3(i,ops[1]):return mask,dict(kind='identity_transport')
   return mask&~(1 if mn=='movss'else 3),dict(kind='partial_scalar_definition')
  if mn=='movlhps'and not x3(i,ops[1]):return mask&3,dict(kind='high_half_definition')
  if mn=='movhlps'and not x3(i,ops[1]):return mask&12,dict(kind='low_half_definition')
  if mn=='shufps'and ops[2].type==X86_OP_IMM:
   imm=ops[2].imm;nextmask=sum(((mask>>((imm>>(2*n))&3))&1)<<n for n in range(2))
   if x3(i,ops[1]):nextmask|=sum(((mask>>((imm>>(2*n))&3))&1)<<n for n in range(2,4))
   return nextmask,dict(kind='lane_shuffle_transport',immediate=imm)
 if 'xmm3'in names_read:
  readmask=1 if mn.endswith('ss')or mn in('comiss','ucomiss','movd')else 3 if mn.endswith('sd')or mn=='movq'else 15
  if mask&readmask:
   return mask,dict(kind='unqualified_tainted_read',read_lanes=readmask,source=i.op_str)
 if 'xmm3'in names_write:
  if mn.endswith('ss'):return mask&~1,dict(kind='partial_scalar_definition')
  if mn.endswith('sd'):return mask&~3,dict(kind='partial_scalar_definition')
  return mask,dict(kind='unqualified_definition',source=i.op_str)
 return mask,None

for pc in(*seeds,root):
 bp=gdb.Breakpoint('*0x%x'%pc,type=gdb.BP_HARDWARE_BREAKPOINT);bp.silent=True
while True:
 gdb.execute('continue',to_string=True);pc=reg('eip')
 if pc==root:break
 assert pc in seeds,hex(pc)
 segment=dict(after_store=seeds[pc],first_pc=pc,entry=state(),steps=0,events=[],branches={},calls=[],returns=[]);mask=8;digest=hashlib.sha256()
 for step in range(4096):
  pc=reg('eip')
  if pc==root:segment['ended']='live_at_root_return';break
  i=decode(pc);digest.update(struct.pack('<I',pc)+i.bytes);prior=mask;mask,event=assess(i,mask)
  gdb.execute('stepi',to_string=True);nextpc=reg('eip');segment['steps']+=1;total+=1
  if i.mnemonic=='call':segment['calls'].append(dict(site=pc,target=nextpc))
  elif i.mnemonic.startswith('ret'):segment['returns'].append(dict(site=pc,target=nextpc))
  elif i.mnemonic.startswith('j'):
   key=hex(pc)+'->'+hex(nextpc);segment['branches'][key]=segment['branches'].get(key,0)+1
  if event:segment['events'].append(dict(pc=pc,before_lanes=prior,after_lanes=mask,**event))
  if event and event['kind'].startswith('unqualified'):
   segment['ended']='unqualified_frontier';break
  if mask==0:
   segment['ended']='prior_register_value_killed';segment['definition']=dict(pc=pc,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str,next_pc=nextpc,after=state());break
 else:segment['ended']='step_budget_frontier'
 segment['instruction_digest']=digest.hexdigest();segment['remaining_lanes']=mask;segments.append(segment)
 assert len(segments)<=1024
 if reg('eip')==root:break
final=state();gdb.execute('dump binary memory HEAP_DUMP HEAP_FIRST HEAP_END')
result=dict(segments=segments,unique_instructions=instructions,total_stepped=total,root=final)
with open(REPORT,'w')as f:json.dump(result,f,sort_keys=True)
gdb.execute('quit')
end
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--report-output',type=Path,required=True);a=p.parse_args();a.capture=a.capture.resolve();a.report_output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(completed=False,owned_scratch=str(scratch),retained_large_artifacts=[]);(a.report_output/'report.json').write_text(json.dumps(report)+'\n')
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;capture=json.loads((a.capture/'capture.json').read_text());heap=next(r for r in capture['regions']if r['address']==0x10060000);replay=scratch/'replay';command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(a.capture),'--output',str(replay)];reference=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(replay/'output.bin').exists(),reference.stderr[-2000:]
  dump=scratch/'watched-heap.bin';results=scratch/'liveness.json';script=GDB.replace('DEBUGGEE',str(scratch/'debuggee.bin')).replace('CAPSTONE_PATH',repr(str(Path(capstone.__file__).parent.parent))).replace('HEAP_DUMP',str(dump)).replace('HEAP_FIRST',hex(heap['address'])).replace('HEAP_END',hex(heap['address']+heap['size'])).replace('REPORT',repr(str(results)));commands=scratch/'liveness.gdb';commands.write_text(script)
  debugger_command=['gdb','-q','-nx','-batch','-x',str(commands),str(replay/'original-hardware')];debugger=subprocess.run(debugger_command,capture_output=True,text=True,timeout=180);log=debugger.stdout+'\nSTDERR:\n'+debugger.stderr;assert len(log.encode())<1024*1024;(a.report_output/'debugger.log').write_text(log);assert debugger.returncode==0 and results.exists(),log[-3000:]
  observed=json.loads(results.read_text());image=Xbe(NEW_DEFAULT)
  for row in observed['unique_instructions'].values():assert image.read(row['pc'],len(bytes.fromhex(row['bytes']))).hex()==row['bytes'],'Observed instruction differs from original image'
  offset=276
  with(replay/'output.bin').open('rb')as stream:
   for r in sorted(capture['regions'],key=lambda r:r['address']):
    if r['address']==heap['address']:break
    offset+=r['size']
   stream.seek(offset);original=stream.read(heap['size'])
  assert dump.read_bytes()==original,'Single-step watched heap differs from ordinary original'
  reference_report=json.loads((replay/'report.json').read_text());abi=reference_report['original_abi'];assert observed['root']['cw']==abi['x87_control_word']and observed['root']['sw']==abi['x87_status']and observed['root']['mxcsr']==abi['mxcsr'],'Single-step observer changed actual original FP state'
  report.update(completed=True,original_sha256=NEW_PIN,capture=str(a.capture),capture_metadata_sha256=sha(a.capture/'capture.json'),source_sha256={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/replay_live_phantom_hardware_tu15.py':sha(ROOT/'tools/replay_live_phantom_hardware_tu15.py')},reference_comparison_exit=reference.returncode,debugger_exit=debugger.returncode,checked_heap_bytes=len(original),ordinary_and_watched_heap_sha256=hashlib.sha256(original).hexdigest(),original_fp_matches=True,observation=observed,replay_command=command,debugger_command=debugger_command,scope='Exact captured-frame register-lane lifetime only. New definitions kill prior register provenance; persistent-memory provenance, future frames, paired native history, and global all-caller closure are not claimed. Raw gates remain unchanged and RED.')
 finally:
  report['temporary_allocated_bytes_before_owner_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.report_output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(completed=True,segments=len(observed['segments']),stepped=observed['total_stepped'],frontiers=sum(s['ended']!='prior_register_value_killed'for s in observed['segments']))))
if __name__=='__main__':main()
