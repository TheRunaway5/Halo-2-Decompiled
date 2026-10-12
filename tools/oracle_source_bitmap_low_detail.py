#!/usr/bin/env python3
"""Literal shared low-detail blob, source initialization prefix and headers.

Copy the actual2048-byte map header through unchanged122911/REP MOVSD,
walk every actual bitmap with122C70/122D00, and run12C640 to its original
213760 I/O boundary. Only the XPhysicalProtect storage-permission service
is supplied; its unused return cannot affect descriptor or header values.
Physical stage bounds are declared input, not recovered whole-title budgets.
Complete12C820/359E0 run on the five actual unmodified low-detail descriptions.
No capture, GPU, guest, resource readiness, I/O completion or mip choice is used.
"""
from pathlib import Path
import collections,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail,PIN,STOP
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
def sha(raw):return hashlib.sha256(raw).hexdigest()
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def main():
 c=Cache(ROOT/'assets/disc/maps/zanzibar.map');r=Retail();x=Xbe(ROOT/'assets/disc/default.xbe')
 executed={};entries=collections.Counter();phase=[''];protections=[];io=[]
 watched=[0x122911,0x122c70,0x122d00,0x13ddd0,0x12c640,0x12c820,0x359e0,0x12c960,0x2d15da,0x213760]
 def observe(g,pc,size,_):
  assert bytes(g.mem_read(pc,size))==x.read(pc,size),(hex(pc),'changed source instruction')
  executed[pc]=size
  if pc in watched:entries[f'{pc:08X}']+=1
  if phase[0]=='initialize' and pc==0x2d15da:
   sp=g.reg_read(UC_X86_REG_ESP);protections.append([r.r(sp+i*4)for i in range(1,4)])
   #External storage permission only.12C640 never examines the return value.
   g.reg_write(UC_X86_REG_EAX,0);g.reg_write(UC_X86_REG_ESP,sp+16);g.reg_write(UC_X86_REG_EIP,r.r(sp))
  if phase[0]=='initialize' and pc==0x213760:
   sp=g.reg_read(UC_X86_REG_ESP);io.append(dict(location=g.reg_read(UC_X86_REG_ECX),argv=[r.r(sp+i*4)for i in range(1,7)]));g.emu_stop()
 r.g.hook_add(UC_HOOK_CODE,observe)
 def prepare(argv=(),registers=()):
  for reg in[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:r.g.reg_write(reg,0)
  r.g.reg_write(UC_X86_REG_EFLAGS,2);r.prepare(argv,registers)
 def run(entry,argv=(),registers=(),stop=STOP):
  prepare(argv,registers);r.g.emu_start(entry,stop,count=2000000);assert r.g.reg_read(UC_X86_REG_EIP)==stop,hex(r.g.reg_read(UC_X86_REG_EIP))
 #1228F0's literal cache-header copy destination is547700. It makes
 #547858/54785C exactly file-header158/15C (source pointer and byte count).
 header=r.alloc(bytes(c.data[:2048]));phase[0]='header copy'
 run(0x122911,registers=[(UC_X86_REG_ESI,header)],stop=0x12291d)
 assert bytes(r.g.mem_read(0x547700,2048))==bytes(c.data[:2048])
 location,length=r.r(0x547858),r.r(0x54785c);assert location==c.u32(0x158) and length==c.u32(0x15c)
 assert location==0x01c3b000 and length==0x2a600;c.require(location,length);blob=bytes(c.data[location:location+length])
 #Relocate only pointers to actual immutable tag/group metadata. The
 #original iterator sees every actual map tag and all806 bitmap counts.
 index=c.u32(16);header_magic=c.u32(index)-index-32;tag_header=bytearray(c.data[index:index+32])
 groups,count=struct.unpack_from('<II',tag_header);group_bytes=bytes(c.data[groups-header_magic:groups-header_magic+count*12])
 groups_ptr=r.alloc(group_bytes);struct.pack_into('<I',tag_header,0,groups_ptr);struct.pack_into('<I',tag_header,8,r.table)
 tags_ptr=r.alloc(tag_header);r.w(0x547f00,tags_ptr);bitmap_count=0;bitmap_records=0
 for datum,t in c.tags.items():
  address=t['address']
  if t['group']=='bitm' and address:
   _,at=c.tag(datum,'bitm');address=r.alloc(bytes(c.data[at:at+0x4c]));bitmap_count+=1;bitmap_records+=c.u32(at+0x44)
  r.g.mem_write(r.table+(datum&65535)*16,struct.pack('<4I',int.from_bytes(t['group'].encode(),'big'),datum,address,t['size']))
 assert bitmap_count==806
 stage=2;allocation_bytes=(length+4095)&~4095;initializations=[]
 #Two physical addresses prove relocation is an input, never an inferred
 #root pointer. The second can be compared with source retained headers,
 #but no retained output is read or used to construct these source words.
 for base in[0x08100000,0x018ed000]:
  phase[0]='initialize';protections.clear();io.clear();r.w(0x4e6420,stage)
  lower=base-0x100000;upper=base+allocation_bytes;r.w(0x4e642c+stage*4,lower);r.w(0x4e6440+stage*4,upper)
  r.w(0x4e646c,0xdeadbeef);r.g.mem_write(0x4e6468,b'\x00')
  prepare();r.g.emu_start(0x12c640,STOP,count=2000000)
  assert r.g.reg_read(UC_X86_REG_EIP)==0x213760 and len(io)==1 and protections==[[base|0x80000000,allocation_bytes,0x404]]
  assert r.r(0x4e6440+stage*4)==base and r.r(0x4e646c)==(base|0x80000000) and r.g.mem_read(0x4e6468,1)[0]==1
  assert io[0]['location']==location and io[0]['argv'][:3]==[(length+511)&~511,base|0x80000000,0] and io[0]['argv'][4:]==[3,7]
  initializations.append(dict(declared_stage=stage,declared_lower=lower,declared_upper=upper,source_top=base,
    source_shared_pointer=r.r(0x4e646c),source_shared_available=1,allocation_bytes=allocation_bytes,protection_argv=protections[0],io_boundary=io[0]))
 absent=[]
 for offset,size in[(0,length),(location,0),(0,0)]:
  phase[0]='absent';r.w(0x547858,offset);r.w(0x54785c,size);r.g.mem_write(0x4e6468,b'\x01');r.w(0x4e646c,0x12345678)
  run(0x12c640);assert r.g.mem_read(0x4e6468,1)[0]==0 and r.r(0x4e646c)==0x12345678
  absent.append(dict(location=offset,bytes=size,source_available=0,source_shared_pointer=r.r(0x4e646c)))
 r.w(0x547858,location);r.w(0x54785c,length)
 shader=next(t for t in c.tags.values()if t['group']=='shad'and t['name'].endswith('\\energy_blade_bottom'))
 _,at=c.tag(shader['datum'],'shad');definition=c.block(at+32,124)[0]
 sword=[c.u32(p)for p in c.block(definition+4,12)];assert sword==[0xe2e1016c,0xe2e2016d,0xe2e3016e,0xe2e4016f]
 captures=[];fixtures=[]
 for datum in[0xf4f91373,*sword]:
  tag,at=c.tag(datum,'bitm');records=c.block(at+0x44,116);assert len(records)==1
  raw=bytes(c.data[records[0]:records[0]+116]);record=r.alloc(raw)
  low_offset,stored=struct.unpack_from('<II',raw,0x60);fmt,width,height,depth=struct.unpack_from('<4H',raw,0x68)
  assert raw[0x5c:0x60]==bytes(4) and fmt in[14,16] and stored==128 and depth==1
  admitted=((width+3)//4)*((height+3)//4)*(8 if fmt==14 else 16)
  assert low_offset+stored<=len(blob);payload=blob[low_offset:low_offset+stored]
  #Compare immutable disk payloads to reject an accidental substitution of
  #a same-dimension full-resource mip. They are independently authored.
  pointer,size=struct.unpack_from('<II',raw,0x1c)[0],struct.unpack_from('<I',raw,0x34)[0]
  path=c.path.parent/[c.path.name,'mainmenu.map','shared.map','single_player_shared.map'][pointer>>30]
  with path.open('rb')as stream:stream.seek(pointer&0x3fffffff);full=stream.read(size)
  assert len(full)==size;w,h=struct.unpack_from('<HH',raw,4);offset=0;mip=0
  while(w,h)!=(width,height):
   assert w>=width and h>=height;offset+=((w+3)//4)*((h+3)//4)*(8 if fmt==14 else 16);w=max(1,w//2);h=max(1,h//2);mip+=1
  tail=full[offset:offset+admitted];assert len(tail)==admitted and tail!=payload[:admitted]
  headers=[]
  for initialization in initializations:
   phase[0]='header';r.g.mem_write(record,raw);r.w(0x4e646c,initialization['source_shared_pointer'])
   run(0x12c820,registers=[(UC_X86_REG_ESI,record)])
   mutated=bytes(r.g.mem_read(record,116));header_words=words(mutated[0x5c:0x70])
   assert mutated[:0x5c]==raw[:0x5c] and mutated[0x70:]==raw[0x70:]
   assert header_words[:3]==[0x40001,(initialization['source_shared_pointer']+low_offset)&0xfffffff,0] and header_words[4]==0
   decoded=dict(width=1<<((header_words[3]>>20)&15),height=1<<((header_words[3]>>24)&15),depth=1<<((header_words[3]>>28)&15),
     dimensions=(header_words[3]>>4)&15,mips=(header_words[3]>>16)&15,format=(header_words[3]>>8)&255)
   assert decoded==dict(width=width,height=height,depth=depth,dimensions=2,mips=1,format=12 if fmt==14 else 15)
   headers.append(dict(base=initialization['source_shared_pointer'],words=header_words,decoded=decoded))
   inputs=[datum,initialization['source_shared_pointer'],*words(raw[0x5c:0x70]),struct.unpack_from('<H',raw,0xa)[0],struct.unpack_from('<H',raw,0xe)[0]]
   fixtures.append((inputs,header_words))
  captures.append(dict(bitmap=f'{datum:08X}',name=tag['name'],raw_record_hex=raw.hex(),raw_low_detail_hex=raw[0x5c:0x70].hex(),
    blob_offset=low_offset,file_offset=location+low_offset,stored_bytes=stored,admitted_bytes=admitted,payload_hex=payload.hex(),
    admitted_payload_sha256=sha(payload[:admitted]),stored_payload_sha256=sha(payload),headers=headers,
    same_dimension_hardware_mip=dict(partition=0,mip=mip,byte_offset=offset,bytes=admitted,sha256=sha(tail),matches_low_detail=False)))
  print('original low detail',f'{datum:08X}',width,height,f'{headers[-1]["words"][3]:08X}',admitted,'authored bytes',flush=True)
 fixture=ROOT/'tests/fixtures/retail_source_bitmap_low_detail.tsv'
 fixture.write_text('# bitmap,declaredsharedbase,authoreddescription5C..6C,type,flags | original12C820 five-word header\n'+''.join(' '.join(f'{v:08X}'for v in inputs)+' | '+' '.join(f'{v:08X}'for v in expected)+'\n'for inputs,expected in fixtures))
 report=dict(xbe_sha256=PIN,cache_sha256=sha(c.data),tool_sha256=sha(Path(__file__).read_bytes()),fixture_sha256=sha(fixture.read_bytes()),
   source_entries=dict(entries),unique_instructions=len(executed),instruction_sha256=sha(b''.join(x.read(pc,size)for pc,size in sorted(executed.items()))),
   original_header_words=len(fixtures)*5,header_copy=dict(source_entry='122911',stop='12291D',destination='547700',bytes=2048,file_fields=['158','15C'],source_globals=['547858','54785C']),
   shared_blob=dict(offset=location,bytes=length,sha256=sha(blob)),bitmap_tags=bitmap_count,bitmap_records=bitmap_records,
   initializations=initializations,absent_blob_controls=absent,bitmaps=captures,
   services_replaced=['2D15DA XPhysicalProtect: storage permission only; unused return0, source allocation arithmetic and all stores unchanged'],
   source_instruction_patches=[],pointer_relocations=['Actual tag/group metadata copied to explicit hoststorage; group/header/table pointers alone relocated'],
   scope=__doc__+' Caller physical bounds are explicit. Original213760 is stopped before I/O; no diskread result or resident hardware state is supplied. Authoredblob is read independently fromdisc for payload provenance. ActualD15E0 fallback/request/readiness remains the separate sampler proof.')
 output=ROOT/'analysis/multiplayer-shading/source-bitmap-low-detail-original.json';output.write_text(json.dumps(report,indent=2)+'\n')
 fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k!='bitmaps'},indent=2)+'\n')
 print(f'{len(fixtures)} original low-detail headers/{len(fixtures)*5} words; {sum(v["admitted_bytes"]for v in captures)} independently authored bytes; source12C640 stops beforeI/O')
if __name__=='__main__':main()
