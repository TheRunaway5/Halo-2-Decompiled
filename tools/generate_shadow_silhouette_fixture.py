#!/usr/bin/env python3
"""Freeze original20's full seven-draw color-shadow generation, no selection guess."""
import collections,hashlib,json,re,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'analysis/multiplayer-shading/original-xemu/shadow-gpu-audit'
OUT=ROOT/'tests/fixtures'
def fbits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def digest(b):return hashlib.sha256(b).hexdigest()
def main():
 records=json.loads((SRC/'original20-executed-shadow-draws.json').read_text())['records'][:7]
 texture_records=json.loads((SRC/'original20-shadow-generation-textures.json').read_text())['records']
 textures={r['texture']['resource']:r['texture']for r in texture_records};texture_ids=list(textures)
 texture_by_event={r['event']:r for r in texture_records}
 proof=json.loads((SRC/'original20-shadow-vertex-fixture-proof.json').read_text())['records']
 selection={r['event']:r for r in proof}
 states={r['event']:r['vk']for r in json.loads((SRC/'original20-inventory.json').read_text())['candidates']}
 data=bytearray(struct.pack('<4I',0x47533248,1,len(records),len(textures)))
 texture_proof=[]
 for resource in texture_ids:
  t=textures[resource];assert t['format']=='B8G8R8A8_UNORM'
  data.extend(struct.pack('<3I',t['width'],t['height'],t['mip_count']))
  alpha=collections.Counter()
  for i,m in enumerate(t['mips']):
   raw=(SRC/m['file']).read_bytes();assert digest(raw)==m['sha256']
   assert len(raw)==max(1,t['width']>>i)*max(1,t['height']>>i)*4
   data.extend(struct.pack('<I',len(raw)));data.extend(raw);alpha.update(raw[3::4])
  texture_proof.append(dict(resource=resource,width=t['width'],height=t['height'],mips=t['mip_count'],alpha_histogram=dict(alpha)))
 proofs=[];vertices=indices_count=checked=0
 for r in records:
  event=r['event'];selected=selection[event];s=states[event]
  assert s['inputAssembly']['topology']==6 and not s['inputAssembly']['primitiveRestartEnable']
  assert s['rasterizer']['cullMode']==1 and not s['rasterizer']['frontCCW'] and s['rasterizer']['depthClampEnable']
  assert not s['depthStencil']['depthTestEnable']and not s['depthStencil']['depthWriteEnable']
  blend=s['colorBlend']['blends'][0];assert not blend['enabled']and blend['writeMask']==15
  tr=texture_by_event[event];assert tr['sampler']==dict(address_u=0,address_v=0,address_w=0,magnify=2,minify=2,mip=2,max_lod=float(tr['texture']['mip_count']-1),min_lod=0.,bias=0.)
  indexraw=(SRC/r['indices']['file']).read_bytes();assert digest(indexraw)==r['indices']['sha256']and r['index_buffer']['byteStride']==4
  indices=struct.unpack('<'+'I'*r['count'],indexraw);count=max(indices)+r['vertex_offset']+1
  asm=(SRC/f'original20-{event}-0.spvasm').read_text()
  inline=(SRC/f'original20-{event}-0-uniform1.bin').read_bytes()
  attributes=[[0.,0.,0.,1.]for _ in range(16)]
  for offset,index in re.findall(r'PushConstants_var.inlineValue\[(\d+)\];[^;]+;\s+\*v(\d+) =',asm):
   attributes[int(index)]=list(struct.unpack_from('<4f',inline,int(offset)*16))
  uniform=(SRC/f'original20-{event}-0-uniform0.bin').read_bytes()
  conversion=(*struct.unpack_from('<2f',uniform,5336),struct.unpack_from('<f',uniform,3076)[0],0.)
  data.extend(struct.pack('<5I',event,r['count'],count,texture_ids.index(tr['texture']['resource']),selected['authored_class']))
  data.extend(struct.pack('<64f',*sum(attributes,[])));data.extend(uniform[:3072]);data.extend(struct.pack('<4f',*conversion))
  buffers={v['slot']:v for v in r['vertex_buffers']};decoded=[]
  for vertex in range(count):
   attrs=[[0.,0.,0.,1.]for _ in range(4)]
   for a in r['attributes']:
    index=int(a['name'][1:]);assert index<4 and not a['genericEnabled']
    fmt=a['format'];assert fmt['type']==0
    vb=buffers[a['vertexBuffer']];raw=(SRC/vb['file']).read_bytes();assert digest(raw)==vb['sha256']
    at=vertex*vb['binding']['byteStride']+a['byteOffset']
    if fmt['compType']==3 and fmt['compByteWidth']==2:
     values=[max(-1.,v/32767.)for v in struct.unpack_from('<'+'h'*fmt['compCount'],raw,at)]
    elif fmt['compType']==2 and fmt['compByteWidth']==1:
     values=[v/255. for v in raw[at:at+fmt['compCount']]]
    else:raise ValueError(fmt)
    attrs[index][:len(values)]=values
   decoded.append([list(struct.unpack('<4f',struct.pack('<4f',*v)))for v in attrs])
   data.extend(struct.pack('<16f',*sum(decoded[-1],[])))
  inputs=re.findall(r'Input float4\* v(\d+) :',asm)
  for d in r['debug']:
   assert d['valid']and len(inputs)==len(d['inputs'])
   for index,source in zip(inputs,d['inputs']):
    actual=[fbits(v)for v in decoded[d['index']+r['vertex_offset']][int(index)]]
    assert actual==[int(v,16)for v in source['words']],(event,d['index'],index)
    checked+=4
  data.extend(indexraw);vertices+=count;indices_count+=r['count']
  proofs.append(dict(event=event,vertices=count,indices=r['count'],class_index=selected['authored_class'],program_sha256=selected['sha256'],vertex_buffers=r['vertex_buffers'],index_buffer=r['indices']))
 filename='retail_shadow_silhouette.bin';(OUT/filename).write_bytes(data)
 target=(SRC/'original20-shadow-target-7758.bgra8').read_bytes();assert len(target)==128*128*4
 (OUT/'retail_shadow_silhouette_expected.bgra8').write_bytes(target)
 result=dict(fixture=filename,bytes=len(data),sha256=digest(data),target_sha256=digest(target),draws=proofs,textures=texture_proof,vertices=vertices,indices=indices_count,debug_input_words_checked=checked,scope=__doc__)
 (SRC/'original20-shadow-silhouette-fixture-proof.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({k:result[k]for k in('vertices','indices','debug_input_words_checked','bytes','sha256')}))
if __name__=='__main__':main()
