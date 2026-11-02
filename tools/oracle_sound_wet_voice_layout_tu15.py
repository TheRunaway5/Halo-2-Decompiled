#!/usr/bin/env python3
"""Execute unchanged TU1.5 wet voice data/format/attachment CPU owners.

Voice objects, descriptors and GP source-base identities are declared component
inputs. State/device guard methods are explicit ABI boundaries, not original
kernel execution. No allocator, device success, DSP data or timing is supplied.
The complete SDK CPU owners and their ordinary callees execute unpatched.
"""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn import x86_const as R


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 if a.output.exists():p.error('Preserve existing reference results')
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'DSOUND')
 pairing=Path('analysis/audio-spatial-20261011/sdk-image-pairing-v26');pair=json.loads((pairing/'report.json').read_text())
 canonical={};entries={}
 for owner in pair['owners']:
  entry=int(owner['base_entry'],16);variant=owner['variants'][-1];assert variant['label']=='title_update_15';entries[entry]=int(variant['entry'],16)
  paths=[pairing/'instructions'/f'{entry:08x}-{label}.asm'for label in('base','title_update_15')]
  texts=[v.read_text().splitlines()for v in paths]
  assert len(texts[0])==len(texts[1])
  for old,new in zip(*texts):
   before=int(old.split()[0],16);after=int(new.split()[0],16);raw=bytes.fromhex(new.split()[1]);assert image.read(after,len(raw))==raw
   assert after not in canonical or canonical[after]==before;canonical[after]=before
 def address(base):
  row=next(v for v in pair['immutable_ranges']if int(v['base_address'],16)==base)
  selected=row['variants'][-1];assert selected['label']=='title_update_15'
  target=int(selected['address'],16);raw=image.read(target,row['bytes']);assert hashlib.sha256(raw).hexdigest()==row['sha256'];return target,raw
 def words(base,n):return list(struct.unpack('<'+'I'*n,address(base)[1]))
 bins=pair['voice_initialization_bins'][-1];assert bins['label']=='title_update_15'
 immutable=[dict(identity=int(r['address'],16),hex=r['hex'])for r in bins['regions']]
 for r in immutable:assert image.read(r['identity'],len(bytes.fromhex(r['hex']))).hex()==r['hex']
 default,raw=address(0x385748);immutable.append(dict(identity=default,hex=raw.hex()))
 assert struct.unpack('<HHIIHHH',raw)==(1,1,48000,192000,4,24,0)
 providers=dict(source_defaults=words(0x385854,18),environment_defaults=words(0x385828,9),pcm_guid=words(0x385798,4),adpcm_guid=words(0x3857a8,4),
  default_format_identity=default,spatial_bins_identity=int(bins['regions'][1]['address'],16),parent_bins_identity=int(bins['regions'][3]['address'],16),
  channel_bins_table_identity=int(bins['table_address'],16),unsigned_correction_bits=words(0x45dbb8,1)[0],frequency_reciprocal_bits=words(0x45dde0,1)[0],immutable=immutable)
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
 for s in image.sections:cpu.mem_write(s.va,image.section_bytes(s))
 bank=0x1000000;cpu.mem_map(bank,0x10000);voice,data,descriptor,alternate=bank+0x1000,bank+0x2000,bank+0x3000,bank+0x4000
 stack,stop=bank+0xe000,bank+0xff00;source_pointer=0x391410
 # An external test return sentinel terminates decoder lookahead. It is not
 # part of the XBE and the code hook stops before it executes.
 cpu.mem_write(stop,b'\xf4')
 extents={voice:0x180,data:0xe0,descriptor:24,alternate:0xe0,source_pointer:4}
 read=lambda at,n=4:int.from_bytes(cpu.mem_read(at,n),'little')
 write=lambda at,v,n=4:cpu.mem_write(at,int(v&((1<<(n*8))-1)).to_bytes(n,'little'))
 guards={entries[0x379c90]:('device','enter'),entries[0x379cb2]:('device','leave'),entries[0x379cc9]:('state','enter'),entries[0x379cee]:('state','leave')}
 runtime={};executed={};rows=[]
 def observe(_,pc,size,__):
  if pc==stop:runtime['complete']=True;cpu.emu_stop();return
  if pc in guards:
   runtime['guards'].append(list(guards[pc]));sp=cpu.reg_read(R.UC_X86_REG_ESP)
   # These guard methods' CPU callers use no return value. No kernel body or
   # local guard-state result is manufactured; their paired calls are external.
   cpu.reg_write(R.UC_X86_REG_ESP,sp+4);cpu.reg_write(R.UC_X86_REG_EIP,read(sp));return
  assert pc in canonical,(hex(pc),'Unpaired CPU dependency');raw=image.read(pc,size);assert bytes(cpu.mem_read(pc,size))==raw
  executed[f'{pc:08x}']=raw.hex();runtime['pc']=canonical[pc]
 def store(_,access,at,size,value,__):
  if any(start<=at and at+size<=start+length for start,length in extents.items()):
   runtime['stores'].append([runtime['pc'],at,size,read(at,size),value&((1<<(size*8))-1)])
 cpu.hook_add(UC_HOOK_CODE,observe);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
 def invalid(_,access,at,size,value,__):
  print('Original wet voice unresolved access',dict(pc=hex(cpu.reg_read(R.UC_X86_REG_EIP)),address=hex(at),access=access,phase=runtime.get('phase'),last_canonical_pc=hex(runtime.get('pc',0))),flush=True);return False
 cpu.hook_add(UC_HOOK_MEM_INVALID,invalid)
 for bin in range(15,23):
  for variant,base in enumerate((0x1200000,0x1200040,0x1200f80,0xfffff400)):
   for separate in(0,1):
    rounding=(bin+variant+separate)%4;control=0x27f|(rounding<<10);mxcsr=0x1f80|(rounding<<13)
    cpu.mem_write(voice,bytes([0x5b])*0x180);cpu.mem_write(data,bytes(0xe0));cpu.mem_write(alternate,bytes(0xe0));
    cpu.mem_write(descriptor,struct.pack('<6I',24,0x100000,0,0,0,bin));write(source_pointer,base)
    write(voice+0x70,data);write(voice+0x80,alternate if separate else data);write(voice+0x12,0x40,2);write(voice+0x14,0x12345678);write(voice+0x64,0xa5,1)
    regions=[dict(address=at,hex=bytes(cpu.mem_read(at,size)).hex())for at,size in extents.items()]
    phases=[('initialize',data,[descriptor])]+([('initialize',alternate,[descriptor])]if separate else[])+[('format',voice,[]),('attach',voice,[]),('attach',voice,[])]
    calls=[];actions=[]
    for phase,(kind,self,args)in enumerate(phases):
     runtime.clear();runtime.update(stores=[],guards=[],phase=kind);cpu.mem_write(stack-0x800,bytes([0xa5])*0x900);cpu.mem_write(stack,struct.pack('<'+'I'*(1+len(args)),stop,*args))
     for reg in('EAX','EDX','EBX','EBP','ESI','EDI'):cpu.reg_write(getattr(R,'UC_X86_REG_'+reg),0x56781234)
     cpu.reg_write(R.UC_X86_REG_ECX,self);cpu.reg_write(R.UC_X86_REG_ESP,stack);cpu.reg_write(R.UC_X86_REG_EFLAGS,2)
     for reg,v in((R.UC_X86_REG_FPCW,control),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,mxcsr)):cpu.reg_write(reg,v)
     cpu.emu_start(entries[{'initialize':0x37cef1,'format':0x38094b,'attach':0x382be3}[kind]],0,count=10000)
     assert runtime.get('complete')and cpu.reg_read(R.UC_X86_REG_ESP)==stack+4+len(args)*4
     calls.append(dict(kind=kind,self=self,arguments=args,result=cpu.reg_read(R.UC_X86_REG_EAX)&0xffffffff,stores=list(runtime['stores']),guards=list(runtime['guards'])))
     if separate and phase==1:
      # Distinct format/source owners are real independently initialized CPU
      # objects. Poison only the source owner's unused format header as an
      # explicit adversarial input boundary, never as a returned SDK result.
      raw=struct.pack('<HBBII',0x69,6,32,44100,36);cpu.mem_write(alternate+0xc,raw);actions.append(dict(after_phase=phase,address=alternate+0xc,hex=raw.hex()))
    source=alternate if separate else data
    layout=dict(input_bin=read(source+0xd4),byte_offset=(read(source+0xbc)-base)&0xffffffff,byte_extent=read(source+0xc0),cursor_word=read(voice+0x154),hardware_format=read(voice+0x14))
    assert layout['byte_extent']==128 and read(data+0xf,1)==24 and layout['input_bin']==bin
    rows.append(dict(name=f'bin{bin}_base{base:08x}_separate{separate}',control=control,mxcsr=mxcsr,regions=regions,calls=calls,actions=actions,layout=layout,
     final_regions=[dict(address=at,hex=bytes(cpu.mem_read(at,size)).hex())for at,size in extents.items()]))
 result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,original_pairing_sha256=hashlib.sha256((pairing/'report.json').read_bytes()).hexdigest(),
  original_default_format_hex=address(0x385748)[1].hex(),default_format_address=default,providers=providers,voice=voice,data=data,alternate=alternate,descriptor=descriptor,source_pointer=source_pointer,
  executed_original_instructions=executed,canonical_store_PC_map={f'{pc:08x}':canonical[pc]for pc in map(lambda v:int(v,16),executed)},
  declared_guard_ABI_boundaries={str(k):v for k,v in guards.items()},original_instruction_patches=[],supplied_allocator_device_or_playback_results=[],
  cases=rows,physical_DMA_alignment_or_feedback_clock_verified=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),retained_large_artifacts=[])
 raw=json.dumps(result,separators=(',',':'))+'\n';assert len(raw)<1024*1024,len(raw)
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(raw);print(json.dumps(dict(cases=len(rows),whole_original_calls=sum(len(v['calls'])for v in rows),instructions=len(executed),bytes=len(raw))))


if __name__=='__main__':main()
