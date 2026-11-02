#!/usr/bin/env python3
"""Execute original127CF0 and2216D0 numeric outputs with their unchanged callees.

Typed envelope and ducking observations are collected at rooted instruction
boundaries. All pool/metadata/class/clock inputs remain declared component
data; no voice/device availability or callee return is supplied.
"""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 if a.output.exists():p.error('Preserve previous original references')
 source=Path('tests/fixtures/loop_combined_publication_tu15.json');base=json.loads(source.read_text())
 assert base['original_xbe_sha256']==NEW_PIN and base['reference_backend'].startswith('unchanged original Linux i386 CPU')
 x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
 code=_Code(x,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000);cpu.mem_map(0x1000000,0x40000)
 for section in x.sections:cpu.mem_write(section.va,x.section_bytes(section))
 def write(records):
  for at,data in records:cpu.mem_write(at,bytes.fromhex(data))
 def word(at):return struct.unpack('<I',cpu.mem_read(at,4))[0]
 def put(at,value):cpu.mem_write(at,struct.pack('<I',value&0xffffffff))
 def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
 def words(at,n):return [word(at+i*4) for i in range(n)]
 allowed=set();routines={};roots={0x127cf0,0x2216d0};instructions={};context={}
 def routine(entry):
  if entry not in routines:routines[entry]=set(_trace(code,entry,roots).code);allowed.update(routines[entry])
  return routines[entry]
 routine(0x127cf0);routine(0x2216d0)
 assert 0x127d2d in routine(0x127cf0) and 0x127d3b in routine(0x127cf0)
 stop=0x103f000;stack=base['stack'];output=base['output_address']
 def observe(_,pc,size,__):
  if pc==stop:context['complete']=True;cpu.emu_stop();return
  if pc not in allowed:
   caller=code.at(context['previous']);assert caller.mnemonic=='call' and caller.operands[0].imm==pc
   routine(pc);assert pc in allowed
  assert bytes(cpu.mem_read(pc,size))==x.read(pc,size);instructions[f'{pc:08x}']=x.read(pc,size).hex();context['previous']=pc
  if pc==0x127d2d:context['envelopes'].append(cpu.reg_read(R.UC_X86_REG_EAX)&0xffffffff)
  if pc==0x127d3b:context['ducked'].append(cpu.reg_read(R.UC_X86_REG_EAX)&0xffffffff)
 cpu.hook_add(UC_HOOK_CODE,observe)
 gp_regs=[getattr(R,'UC_X86_REG_'+s) for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
 def run(entry,gp,control,mxcsr):
  context.clear();context.update(envelopes=[],ducked=[])
  for reg,value in zip(gp_regs,gp):cpu.reg_write(reg,value)
  for reg,value in ((R.UC_X86_REG_FPCW,control),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),
   (R.UC_X86_REG_MXCSR,mxcsr),(R.UC_X86_REG_EFLAGS,2)):cpu.reg_write(reg,value)
  cpu.emu_start(entry,0,count=12000);assert context.get('complete')
 write(base['memory']);parents=word(word(0x50ebe8)+0x44);definition=word(word(0x4f0414)+8)
 scales=word(word(0x52d664)+0xc);classes=word(word(word(0x4f0414)+24)+4)
 def metadata():return dict(class_lower=word(classes+0x20),class_upper=word(classes+0x24),scale_lower=word(scales),scale_upper=word(scales+4))
 combined=[];accepted=[]
 for index,row in enumerate(base['cases']):
  write(base['memory']);write(row['memory']);run(0x2216d0,row['gp'],row['control'],row['mxcsr'])
  assert bytes(cpu.mem_read(output,80)).hex()==row['output']
  if not context['envelopes']:continue
  assert len(context['envelopes'])==4 and len(set(context['envelopes']))==1 and len(set(context['ducked']))==1
  combined.append(dict(metadata=metadata(),amplitudes=words(parents+0xc4,4),control=word(parents+0x10),parent_gain=word(parents+0x14),
   ducked_class_gain=context['ducked'][0],expected=words(output+0x28,4),fp_control=row['control'],mxcsr=row['mxcsr'],physical_parent_case=index))
  accepted.append(row)
 scalar=[]
 for n in range(768):
  row=accepted[n%len(accepted)];write(base['memory']);write(row['memory'])
  values=[-100.,-64.,-24.,-12.,-6.,-.001,0.,3.]
  fields=[bits(values[(n//(2**i)+i)%len(values)]) for i in range(4)]
  for at,value in zip((classes+0x20,classes+0x24,scales,scales+4),fields):put(at,value)
  fraction=bits([0.,1.,.25,.5,.75,-1.,2.,.000001][n%8]);offset=bits([-6.,0.,3.][n%3])
  put(stack,stop);put(stack+4,offset);put(stack+8,fraction)
  gp=[0]*8;gp[4]=stack;gp[6]=definition;rounding=(n//8)%4;cw=0x027f|(rounding<<10);csr=0x1f80|(rounding<<13)
  run(0x127cf0,gp,cw,csr);assert len(context['envelopes'])==1 and len(context['ducked'])==1
  scalar.append(dict(metadata=metadata(),control=fraction,offset=offset,expected=context['envelopes'][0],
   ducked_class_gain=context['ducked'][0],expected_total=cpu.reg_read(R.UC_X86_REG_EAX)&0xffffffff,
   fp_control=cw,mxcsr=csr,memory=[[at,bytes(cpu.mem_read(at,4)).hex()] for at in (classes+0x20,classes+0x24,scales,scales+4)],
   base_case=n%len(accepted)))
 report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,publication_reference_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
  source_instruction_patches=[],substituted_calls=[],every_executed_site_has_rooted_provenance=True,original_sites=instructions,
  combined=combined,scalar=scalar,stack=stack,output=output,definition=definition,accepted_base_cases=accepted,memory=base['memory'],
  voice_or_device_verified=False)
 raw=json.dumps(report,separators=(',',':'))+'\n';assert len(raw.encode())<1024*1024
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(raw)
 print(json.dumps(dict(scalar_cases=len(scalar),combined_cases=len(combined),rooted_sites=len(instructions),bytes=len(raw))))


if __name__=='__main__':main()
