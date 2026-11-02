#!/usr/bin/env python3
"""Observe unchanged original wet-return construction CPU fragment.

The224EC0 parent body authenticates225333..225452 instruction boundaries.
SDK constructor/setter calls are explicit DECLARED ABI input boundaries here;
they do not establish device startup, physical resources, playback or timing.
No original instruction or expected result is taken from native code.
"""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--parent',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 if a.output.exists():p.error('Preserve prior original reports')
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text')
 listing=(a.parent/'instructions/002205c0-u5.asm').read_text().splitlines();parent={int(v.split()[0],16):v.split()[1]for v in listing}
 assert all(pc in parent for pc in(0x225333,0x2253eb,0x2253f9,0x225426,0x225434,0x225452))
 for pc,raw in parent.items():assert image.read(pc,len(bytes.fromhex(raw))).hex()==raw
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
 for s in image.sections:cpu.mem_write(s.va,image.section_bytes(s))
 bank=0x1000000;cpu.mem_map(bank,0x50000);drivers=[bank+0x1000,bank+0x5000];stack=bank+0x4e000
 global_driver=0x52d674;read=lambda address:struct.unpack('<I',cpu.mem_read(address,4))[0]
 write=lambda address,value:cpu.mem_write(address,struct.pack('<I',value&0xffffffff))
 declared_targets={0x38759e:('create',4),0x38574f:('volume',2),0x3866c4:('bins',2),0x3857bf:('play',4)}
 current={};executed={};rows=[]
 def hook(_,pc,size,__):
  if pc==0x225452:current['complete']=True;cpu.emu_stop();return
  if pc in declared_targets:
   kind,count=declared_targets[pc];sp=cpu.reg_read(R.UC_X86_REG_ESP);args=[read(sp+4+4*i)for i in range(count)]
   call=dict(kind=kind,args=args,actions=[])
   if kind=='create':
    index=current['creates'];current['creates']+=1;call['descriptor']=list(struct.unpack('<6I',cpu.mem_read(args[1],24)))
    result=current['returns'][index];call['result']=result
    if not result&0x80000000:call['actions'].append([args[2],bank+0x20000+index*0x100])
   else:
    result=0
    if kind=='bins':
     count=read(args[1]);entries=read(args[1]+4);call['count']=count
     call['pairs']=list(struct.unpack('<'+'I'*(count*2),cpu.mem_read(entries,count*8)))
   if current['reload'] and kind in ('volume','bins'):
    # Explicit adversarial external identity mutation: the next original
    # instruction must re-read its same ESI slot after each SDK call.
    call['actions'].append([current['slot'],bank+0x30000+len(current['calls'])*0x100])
   if kind=='create':current['slot']=args[2]
   if current['switch_driver'] and kind=='volume' and not current['switched']:
    current['switched']=True;call['actions'].append([global_driver,drivers[1]])
   for address,value in call['actions']:write(address,value)
   current['calls'].append(call);cpu.reg_write(R.UC_X86_REG_EAX,result)
   cpu.reg_write(R.UC_X86_REG_ESP,sp+4+count*4 if kind!='bins' else sp+12)
   cpu.reg_write(R.UC_X86_REG_EIP,read(sp));return
  instruction=code.at(pc);assert pc in parent and instruction and image.read(pc,size)==bytes(cpu.mem_read(pc,size))
  executed[f'{pc:08x}']=image.read(pc,size).hex()
 cpu.hook_add(UC_HOOK_CODE,hook)
 profiles=[('all-created',[0]*8,False,False),('positive-status',[0x7fffffff]*8,False,False),
  ('all-failed',[0x80004005]*8,False,False),('alternating',[0,0x8007000e]*4,False,False),
  ('reloaded-identities',[0]*8,True,False),('reloaded-driver',[0]*8,True,True)]
 profiles += [('failure-at-'+str(i),[0x88780032 if j==i else 0 for j in range(8)],False,False)for i in range(8)]
 for label,returns,reload,switch in profiles:
  for n,d in enumerate(drivers):
   cpu.mem_write(d,bytes([0x5b+n])*0x2ad8);write(d+0x2ab0,bank+0x18000+n*0x100)
  write(global_driver,drivers[0]);cpu.mem_write(stack-0x100,bytes([0xa5])*0x200)
  for i,reg in enumerate(('EAX','ECX','EDX','EBX','EBP','ESI','EDI')):cpu.reg_write(getattr(R,'UC_X86_REG_'+reg),0x12340000+i*0x100)
  cpu.reg_write(R.UC_X86_REG_ESP,stack);cpu.reg_write(R.UC_X86_REG_EFLAGS,2)
  current.clear();current.update(calls=[],creates=0,returns=returns,reload=reload,switch_driver=switch,switched=False)
  cpu.emu_start(0x225333,0,count=10000);assert current.get('complete')and current['creates']==8 and cpu.reg_read(R.UC_X86_REG_ESP)==stack
  rows.append(dict(name=label,returns=returns,reload_identities=reload,switch_driver=switch,calls=current['calls'],
   final_driver=read(global_driver),final_slots=[[read(d+0x2a70+i*4)for i in range(8)]for d in drivers]))
 report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,parent_entry=0x224ec0,fragment_entry=0x225333,fragment_end=0x225452,
  parent_listing_sha256=hashlib.sha256((a.parent/'instructions/002205c0-u5.asm').read_bytes()).hexdigest(),
  executed_original_instructions=executed,source_instruction_patches=[],declared_sdk_abi_boundaries={hex(k):v for k,v in declared_targets.items()},
  declared_external_results_are_not_device_acceptance=True,device_playback_or_clock_verified=False,drivers=drivers,global_driver=global_driver,bank=bank,cases=rows,
  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),retained_large_artifacts=[])
 raw=json.dumps(report,separators=(',',':'))+'\n';assert len(raw)<1024*1024;a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(raw)
 print(json.dumps(dict(cases=len(rows),create_attempts=sum(sum(v['kind']=='create'for v in r['calls'])for r in rows),instructions=len(executed),bytes=len(raw))))


if __name__=='__main__':main()
