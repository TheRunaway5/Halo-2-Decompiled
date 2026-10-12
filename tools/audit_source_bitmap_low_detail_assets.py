#!/usr/bin/env python3
"""Freeze authored low-detail descriptions/blobs and original headers for all 21 maps.

No native result supplies the fixtures. Original 12C820/359E0 build every
present header; original XDK 3FDBC0 proves all six cube-face offsets. Source
metadata/blobs are immutable disc/map-pack bytes. Declared shared base is
08100000. The native planar/cube pixel expectation is direct BC bytes or
NV6 Morton-addressed BGRA expansion; volume format-11 decoding stays unsupported.
"""
from pathlib import Path
import argparse,collections,hashlib,json,struct,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_dynamic_model_shader_assets import MAPS
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail,PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
def sha(raw):return hashlib.sha256(raw).hexdigest()
def fnv(raw):
 value=14695981039346656037
 for byte in raw:value=((value^byte)*1099511628211)&0xffffffffffffffff
 return value
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def cube_rgba(raw,width,height,stride):
 out=bytearray()
 for face in range(6):
  for y in range(height):
   for x in range(width):
    at=0;destination=1;bit=1
    while bit<width or bit<height:
     for coordinate,dimension in[(x,width),(y,height)]:
      if bit<dimension:
       if coordinate&bit:at|=destination
       destination<<=1
     bit<<=1
    b,g,r,a=raw[face*stride+at*4:face*stride+at*4+4];out.extend([r,g,b,a])
 return bytes(out)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);args=ap.parse_args()
 r=Retail();x=Xbe(ROOT/'assets/disc/default.xbe');executed={};entries=collections.Counter()
 def observe(g,pc,size,_):
  assert bytes(g.mem_read(pc,size))==x.read(pc,size),(hex(pc),'changed source instruction')
  executed[pc]=size
  if pc in[0x12c820,0x359e0,0x3fdbc0]:entries[f'{pc:X}']+=1
 r.g.hook_add(UC_HOOK_CODE,observe);record=r.alloc(bytes(116));outputs=r.alloc(bytes(20));shared=0x08100000;r.w(0x4e646c,shared)
 def run(entry,argv=(),registers=()):
  for reg in[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:r.g.reg_write(reg,0)
  r.g.reg_write(UC_X86_REG_EFLAGS,2);return r.run(entry,argv=argv,registers=registers)
 fixture=ROOT/'tests/fixtures/retail_source_bitmap_low_detail_assets.tsv';lines=['# P/N map datum index recordoffset bloboffset blobsize type flags raw5C..6C(5words) | originalheader5words storedbytes admittedbytes outputbytes expectedpixelFNV64 | exactpaddedpayloadhex(-forabsent)']
 maps=[];totals=collections.Counter();metadata=collections.Counter();cube_controls=[];special_cube=[]
 cube_fixture=ROOT/'tests/fixtures/retail_source_bitmap_low_detail_cube_layouts.tsv'
 cube_lines=['# map datum index face sharedbase authoredoffset originalheader5words | originalData RowPitch SlicePitch Format Size']
 for name in MAPS:
  path=ROOT/'assets/disc/maps'/f'{name}.map'
  if not path.exists():path=ROOT/'assets/map-pack/maps'/f'{name}.map'
  c=Cache(path);blob_at,blob_size=c.u32(0x158),c.u32(0x15c);c.require(blob_at,blob_size)
  blob=bytes(c.data[blob_at:blob_at+blob_size]);counts=collections.Counter();descriptors=collections.Counter();stored_total=admitted_total=0
  for datum,tag in c.tags.items():
   if tag['group']!='bitm' or not tag['address']:continue
   _,at=c.tag(datum,'bitm')
   for index,where in enumerate(c.block(at+0x44,116)):
    raw=bytes(c.data[where:where+116]);common,offset,stored=struct.unpack_from('<3I',raw,0x5c)
    fmt,width,height,depth=struct.unpack_from('<4H',raw,0x68);kind,flags=struct.unpack_from('<H',raw,0xa)[0],struct.unpack_from('<H',raw,0xe)[0]
    present=bool(blob_at and blob_size and(common or stored));assert common==0
    header=[0]*5;payload=b'';admitted=output_bytes=pixel_hash=0
    if present:
     assert kind in[0,1,2]and fmt in[11,14,15,16]and offset+stored<=blob_size
     assert not(flags&0x20), 'special NV0x33 format outside authored admitted set'
     assert width>0 and height>0 and depth>0 and all(v&(v-1)==0 for v in[width,height,depth])and not(flags&0x10)
     payload=blob[offset:offset+stored];r.g.mem_write(record,raw);run(0x12c820,registers=[(UC_X86_REG_ESI,record)])
     mutated=bytes(r.g.mem_read(record,116));assert mutated[:0x5c]==raw[:0x5c]and mutated[0x70:]==raw[0x70:]
     header=words(mutated[0x5c:0x70]);assert header[:3]==[0x40001,(shared+offset)&0xfffffff,0]and header[4]==0 and(header[3]>>16)&15==1
     assert 1<<((header[3]>>20)&15)==width and 1<<((header[3]>>24)&15)==height and 1<<((header[3]>>28)&15)==depth
     if kind==0:
      assert fmt in[14,15,16]and depth==1;admitted=((width+3)//4)*((height+3)//4)*(8 if fmt==14 else 16);expected=payload[:admitted]
      assert len(expected)==admitted;output_bytes=len(expected);pixel_hash=fnv(expected)
     elif kind==2:
      assert fmt==11 and width==height==4 and depth==1 and stored==768;admitted=6*width*height*4
      layout=[]
      for face in range(6):
       run(0x3fdbc0,argv=[record+0x5c,face,0,*[outputs+i*4 for i in range(5)]])
       actual=words(bytes(r.g.mem_read(outputs,20)));assert actual==[((shared+offset)|0x80000000)+face*128,16,64,header[3],0]
       layout.append(actual)
       cube_lines.append(name+' '+' '.join(f'{v:X}'for v in[datum,index,face,shared,offset,*header])+' | '+' '.join(f'{v:X}'for v in actual))
      expected=cube_rgba(payload,width,height,128);output_bytes=len(expected);pixel_hash=fnv(expected)
      cube_controls.append(dict(map=name,bitmap=f'{datum:08X}',index=index,header=header,faces=layout))
      if name=='zanzibar' and datum==0xe17d0008 and index==0:
       for declared_format in[10,11]:
        for special in[False,True]:
         control=bytearray(raw);control_flags=(flags&~0x20)|(0x20 if special else 0)
         struct.pack_into('<H',control,0xe,control_flags);struct.pack_into('<H',control,0x68,declared_format)
         r.g.mem_write(record,bytes(control));run(0x12c820,registers=[(UC_X86_REG_ESI,record)])
         actual_header=words(bytes(r.g.mem_read(record+0x5c,20)))
         nv=0x33 if special else struct.unpack('<I',x.read(0x450768+declared_format*4,4))[0]
         assert ((actual_header[3]>>8)&255)==nv
         special_cube.append(dict(map=name,bitmap=f'{datum:08X}',index=index,record_offset=where,
           flags=control_flags,format=declared_format,header=actual_header))
     else:
      assert kind==1 and fmt==11 and width==height==depth==4 and stored==256;admitted=width*height*depth*4
     assert admitted<=stored;counts['present']+=1;counts[f'kind{kind}_format{fmt}']+=1;stored_total+=stored;admitted_total+=admitted
    else:counts['absent']+=1
    counts['records']+=1;descriptors[(int(present),kind,fmt,flags,width,height,depth,stored)]+=1
    inputs=[datum,index,where,blob_at,blob_size,kind,flags,*words(raw[0x5c:0x70])]
    out=[*header,stored,admitted,output_bytes,pixel_hash]
    lines.append(('P'if present else'N')+' '+name+' '+' '.join(f'{v:X}'for v in inputs)+' | '+' '.join(f'{v:X}'for v in out)+' | '+(payload.hex()if payload else'-'))
  totals.update(counts);metadata.update(descriptors)
  maps.append(dict(map=name,file=str(path.relative_to(ROOT)),cache_sha256=sha(c.data),shared_blob=dict(offset=blob_at,bytes=blob_size,sha256=sha(blob)),
    counts=dict(counts),stored_bytes=stored_total,admitted_bytes=admitted_total,descriptors=[dict(present=k[0],kind=k[1],format=k[2],flags=f'{k[3]:04X}',width=k[4],height=k[5],depth=k[6],stored_bytes=k[7],count=v)for k,v in sorted(descriptors.items())]))
  print('literal lowdetail assets',name,dict(counts),flush=True);c.close()
 assert totals['records']==35442 and totals['present']==23157 and totals['absent']==12285
 assert totals['kind0_format14']==10254 and totals['kind0_format15']==8634 and totals['kind0_format16']==3598 and totals['kind2_format11']==524 and totals['kind1_format11']==147
 fixture.write_text('\n'.join(lines)+'\n')
 cube_fixture.write_text('\n'.join(cube_lines)+'\n')
 assert len(special_cube)==4 and special_cube[-2]['header'][3]==0x0221062d and special_cube[-1]['header'][3]==0x0221332d
 special_fixture=ROOT/'tests/fixtures/retail_source_bitmap_low_detail_cube_formats.tsv'
 special_fixture.write_text('# map datum index recordoffset flags declaredformat | original complete12C820 header5words\n'+''.join(
  v['map']+' '+' '.join(f'{w:X}'for w in[int(v['bitmap'],16),v['index'],v['record_offset'],v['flags'],v['format']])+' | '+
  ' '.join(f'{w:X}'for w in v['header'])+'\n'for v in special_cube))
 report=dict(xbe_sha256=PIN,tool_sha256=sha(Path(__file__).read_bytes()),fixture_sha256=sha(fixture.read_bytes()),shared_base=shared,
  counts=dict(totals),maps=maps,cube_fixture_sha256=sha(cube_fixture.read_bytes()),original_header_words=totals['present']*5,original_cube_layout_words=totals['kind2_format11']*6*5,
  source_entries=dict(entries),unique_instructions=len(executed),instruction_sha256=sha(b''.join(x.read(pc,size)for pc,size in sorted(executed.items()))),
  services_replaced=[],source_instruction_patches=[],cube_layouts=cube_controls,special_cube_format_controls=special_cube,
  special_cube_fixture_sha256=sha(special_fixture.read_bytes()),original_special_cube_header_words=len(special_cube)*5,
  scope=__doc__+' Raw padded payloads are exact disc reads. Header and face-layout outputs are original CPU words. Native RGBA cube expected pixels use the independently specified NV6 Morton/BGRA decode; no GPU pixel comparison is claimed. Explicitly unsupported volume format-11 has no decoded-pixel expectation.')
 if args.native:
  result=subprocess.run([str(args.native.resolve())],cwd=ROOT,capture_output=True,text=True);print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr);assert result.returncode==0
  report['native_validation']=dict(binary_sha256=sha(args.native.read_bytes()),return_code=0,stdout=result.stdout.strip())
 output=ROOT/'analysis/multiplayer-shading/source-bitmap-low-detail-assets.json';output.write_text(json.dumps(report,indent=2)+'\n')
 fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k not in['cube_layouts','maps']},indent=2)+'\n')
 cube_fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k not in['cube_layouts','maps']},indent=2)+'\n')
 special_fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k not in['cube_layouts','maps']},indent=2)+'\n')
 print(json.dumps({k:report[k]for k in['counts','original_header_words','original_cube_layout_words','source_entries','unique_instructions']},indent=2))
if __name__=='__main__':main()
