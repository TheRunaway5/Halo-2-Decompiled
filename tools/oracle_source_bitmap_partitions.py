#!/usr/bin/env python3
"""Original energy-blade bitmap partition headers and allocation prefix.

Actual Zanzibar bitmap/resource bytes are the authority. Complete12D6D0 and
359E0 run unchanged.12D5A0 runs through unchanged13D370/16B910 and stops at
213760 before resource I/O; no readiness, I/O result, GPU or guest is supplied.
Preallocated storage and the declared cache address/capacity are explicit
caller inputs. Original12C0D0 pool counts/strides and original pool/cache
constructors initialize their metadata. Captured mip offsets are never read.
"""
from pathlib import Path
import argparse,collections,hashlib,json,struct,sys,subprocess
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail,PIN,STOP
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
def sha(raw):return hashlib.sha256(raw).hexdigest()
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);a=ap.parse_args()
 c=Cache(ROOT/'assets/disc/maps/zanzibar.map');r=Retail()
 sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
 x=Xbe(ROOT/'assets/disc/default.xbe');executed={};entries=collections.Counter();phase=[''];stops=[]
 watched=[0x16b5f0,0x16b650,0x16b7a0,0x13d170,0x13d8b0,0x13d370,0x16b840,0x16b910,0x12d5a0,0x12d6d0,0x359e0,0x213760]
 def observe(g,pc,size,_):
  assert bytes(g.mem_read(pc,size))==x.read(pc,size),(hex(pc),'changed source instruction')
  executed[pc]=size
  if pc in watched:entries[f'{pc:08X}']+=1
  if phase[0]=='allocation'and pc==0x213760:
   sp=g.reg_read(UC_X86_REG_ESP)
   stops.append(dict(entry=f'{pc:08X}',resource_pointer=f'{g.reg_read(UC_X86_REG_ECX):08X}',
     argv=[r.r(sp+i*4)for i in range(1,7)]));g.emu_stop()
 r.g.hook_add(UC_HOOK_CODE,observe)
 def run(entry,argv=(),registers=()):
  for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:r.g.reg_write(reg,0)
  r.g.reg_write(UC_X86_REG_EFLAGS,2)
  return r.run(entry,argv=argv,registers=registers)
 # Literal producer12C0D0 uses512 image entries,40-byte image records,
 #512 cache entries,24-byte cache records, and4096-byte allocation blocks.
 image_pool=r.alloc(bytes(0x508c));cache_bank=r.alloc(bytes(0x30fc))
 run(0x16b5f0,[image_pool,0x453504,0],[(UC_X86_REG_ECX,0),(UC_X86_REG_EDX,40),(UC_X86_REG_EBX,512)])
 run(0x16b7a0,registers=[(UC_X86_REG_ESI,image_pool)])
 run(0x13d170,[cache_bank,0x4534d8,0,12,512,0x12d200,0x12d1a0,0x12d160,0])
 # This bounded caller declares an empty4MiB resource allocation bank. It
 #does not infer whole-title memory-budget initialization or pressure state.
 run(0x13d8b0,[1024],[(UC_X86_REG_EAX,cache_bank)])
 r.w(0x4e6454,image_pool);r.w(0x4e6464,cache_bank);r.w(0x4e6460,0x08100000)
 r.w(cache_bank+0x2c,2)
 cases=[];fixtures=[]
 shader=next(t for t in c.tags.values()if t['group']=='shad'and t['name'].endswith('\\energy_blade_bottom'))
 _,shader_at=c.tag(shader['datum'],'shad');definitions=c.block(shader_at+32,124);assert len(definitions)==1
 bindings=c.block(definitions[0]+4,12);assert len(bindings)==4 and all(c.u32(p+4)==0 for p in bindings)
 tags=[c.tags[c.u32(p)]for p in bindings]
 assert len(tags)==4
 for tag in tags:
  _,base=c.tag(tag['datum'],'bitm');records=c.block(base+0x44,116);assert len(records)==1
  raw=bytes(c.data[records[0]:records[0]+116]);record=r.alloc(raw)
  width,height,depth,kind,format,flags=struct.unpack_from('<6H',raw,4);maximum=struct.unpack_from('<H',raw,0x14)[0]
  assert kind==0 and depth==1 and flags==0x0203 and format in [14,16]
  pointers=words(raw[0x1c:0x28]);sizes=words(raw[0x34:0x40]);payloads=[]
  for pointer,size in zip(pointers,sizes):
   path=c.path.parent/[c.path.name,'mainmenu.map','shared.map','single_player_shared.map'][pointer>>30]
   with path.open('rb')as stream:stream.seek(pointer&0x3fffffff);payload=stream.read(size)
   assert len(payload)==size;payloads.append(payload)
  full=payloads[0];full_offsets=[];offset=0;w,h=width,height
  while w>=4 and h>=4:
   full_offsets.append(offset);offset+=((w+3)//4)*((h+3)//4)*(8 if format==14 else 16);w>>=1;h>>=1
  for partition in range(3):
   phase[0]='allocation';stops.clear();r.prepare([partition,0,0],[(UC_X86_REG_EAX,0),(UC_X86_REG_ECX,record)])
   r.g.reg_write(UC_X86_REG_EFLAGS,2);r.g.emu_start(0x12d5a0,STOP,count=200000)
   assert r.g.reg_read(UC_X86_REG_EIP)==0x213760 and len(stops)==1
   handle=r.r(record+0x28+partition*4);address=r.r(record+0x40+partition*4)
   slot=(handle&65535);image_table=r.r(image_pool+0x44);image=image_table+slot*40
   header=words(bytes(r.g.mem_read(image+0x14,20)));fmt=header[3]
   decoded=dict(width=1<<((fmt>>20)&15),height=1<<((fmt>>24)&15),depth=1<<((fmt>>28)&15),
     dimensions=(fmt>>4)&15,mips=(fmt>>16)&15,format=(fmt>>8)&255)
   assert decoded['width']==width>>partition and decoded['height']==height>>partition and decoded['dimensions']==2
   length=0;w,h=decoded['width'],decoded['height']
   levels=[]
   for mip in range(decoded['mips']):
    count=((w+3)//4)*((h+3)//4)*(8 if format==14 else 16)
    at=full_offsets[partition]+length;piece=payloads[partition][length:length+count]
    assert len(piece)==count and piece==full[at:at+count]
    levels.append(dict(authored_mip=partition+mip,width=w,height=h,bytes=count,sha256=sha(piece)))
    length+=count;w=max(1,w>>1);h=max(1,h>>1)
   assert length<=sizes[partition]and decoded['width']>> (decoded['mips']-1)>=4
   assert stops[0]['resource_pointer']==f'{pointers[partition]:08X}'and stops[0]['argv'][:2]==[(sizes[partition]+511)&~511,address]
   # Compare the completed pure header producer independently of the
   #allocation prefix; its five words must be identical at another output.
   out=r.alloc(bytes(20));phase[0]='header';run(0x12d6d0,[partition,out],[(UC_X86_REG_EAX,record)])
   assert words(bytes(r.g.mem_read(out,20)))==header
   inputs=[tag['datum'],partition,width,height,depth,kind,format,flags,maximum,pointers[partition],sizes[partition],address]
   fixtures.append((inputs,header));cases.append(dict(bitmap=f'{tag["datum"]:08X}',name=tag['name'],record_hex=raw.hex(),
     partition=partition,header=[f'{word:08X}'for word in header],decoded=decoded,source_handle=f'{handle:08X}',
     runtime_arrays={f'{offset:02X}':[f'{r.r(record+offset+i*4):08X}'for i in range(3)]for offset in [0x28,0x34,0x40]},
     cached_partition0_address=f'{r.r(record+0x54):08X}',
     source_io_boundary=stops[0],payload=dict(pointer=f'{pointers[partition]:08X}',bytes=sizes[partition],sha256=sha(payloads[partition]),
       admitted_bytes=length,padding_bytes=sizes[partition]-length,levels=levels)))
 fixture=ROOT/'tests/fixtures/retail_source_bitmap_partitions.tsv'
 fixture.write_text('# bitmap partition dimensions/type/format/flags/maximum/resourcepointer/size/address | original20-byte header\n'+
   ''.join(' '.join(f'{v:08X}'for v in inputs)+' | '+' '.join(f'{v:08X}'for v in expected)+'\n'for inputs,expected in fixtures))
 report=dict(xbe_sha256=PIN,cache_sha256=sha(c.data),shader=f'{shader["datum"]:08X}',shader_bitmap_bank_hex=bytes(c.data[bindings[0]:bindings[-1]+12]).hex(),
   tool_sha256=sha(Path(__file__).read_bytes()),fixture_sha256=sha(fixture.read_bytes()),
   cases=cases,header_words=len(fixtures)*5,unique_instructions=len(executed),
   instruction_sha256=sha(b''.join(x.read(pc,size)for pc,size in sorted(executed.items()))),source_entries=dict(entries),
   services_replaced=[],source_instruction_patches=[],declared_storage=dict(image_pool_bytes=0x508c,cache_bank_bytes=0x30fc,
      images=512,image_stride=40,cache_entries=512,cache_stride=24,allocation_shift=12,capacity_blocks=1024,address=0x08100000),
   boundary=__doc__)
 if a.native:
  native=subprocess.run([str(a.native.resolve())],cwd=ROOT,capture_output=True,text=True)
  print(native.stdout,end='');print(native.stderr,end='',file=sys.stderr);assert native.returncode==0
  report['native_validation']=dict(binary_sha256=sha(a.native.read_bytes()),return_code=0,stdout=native.stdout.strip(),metadata_words=48,
    levels=sum(len(case['payload']['levels'])for case in cases),payload_bytes=sum(case['payload']['admitted_bytes']for case in cases))
 output=ROOT/'analysis/multiplayer-shading/source-bitmap-partitions-original.json';output.write_text(json.dumps(report,indent=2)+'\n')
 fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2)+'\n')
 print(f'{len(fixtures)} full original partition headers/{report["header_words"]} words; allocation prefix stops at213760; every admitted BC mip-tail byte exact')
if __name__=='__main__':main()
