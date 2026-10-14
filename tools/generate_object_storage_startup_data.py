#!/usr/bin/env python3
"""Declare authenticated cold data reached by complete TU1.5 B87F0 startup."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--manifest',type=Path,required=True);a=ap.parse_args()
 assert sha(NEW_DEFAULT)==NEW_PIN;x=Xbe(NEW_DEFAULT);c=_Code(x,'.text');u=lambda p:struct.unpack('<I',x.read(p,4))[0]
 table=[u(p)for p in range(0x474b90,0x474bc4,4)];records=sorted(set(table).union(*(set(u(p+0x84+4*i)for i in range(16))for p in table))-{0});assert len(records)==17
 callbacks={0x473a00:0x1183f0,0x473a38:0x1190f0,0x473a70:0xc5a70};assert all(u(p)==v for p,v in callbacks.items())
 banks=[(0x4eaba0,32),(0x4eabc0,1),(0x4eabc4,4),(0x4eabc8,0x2008),(0x4ecbd0,4),(0x4ecbdc,4),(0x4ecbe0,12),(0x4ecbec,4),(0x4ecbf0,4),(0x4ecbf8,4),(0x4ecbfc,1),(0x4ecc00,12),(0x51dae8,4)]
 assert all(not any(x.read(p,n))for p,n in banks)
 names=[0x45efb8,0x45ef8c,0x45f030,0x45f038,0x45ef84,0x45ef7c,0x45ef74,0x45ef6c,0x45ef64,0x45ef50,0x45ef38,0x461944,0x461938,0x466e00]
 lines=['#include "halo2/native_tu15_object_storage.hpp"','#include <cstring>','namespace halo2 {','namespace {','template<std::size_t N> auto encode(const std::array<std::uint32_t,N>& words) {','    std::array<std::byte,N*4> bytes{};','    for(unsigned i=0;i<N;++i)for(unsigned j=0;j<4;++j)bytes[i*4+j]=std::byte(words[i]>>(8*j));','    return bytes;','}','}','void mount_native_tu15_object_storage_data(SourceRuntimeMemory& memory) {']
 rows=[]
 for p in records:
  words=struct.unpack('<49I',x.read(p,0xc4));assert u(p+0xc4)==0
  lines.append(f'    memory.mount(0x{p:x},encode(std::array<std::uint32_t,49>{{'+','.join(f'0x{v:x}u'for v in words)+'}),SourceRuntimeMemory::Access::ReadOnly,"source object-type program metadata");')
  lines.append(f'    memory.mount(0x{p+0xc4:x},std::array<std::byte,4>{{}},SourceRuntimeMemory::Access::ReadWrite,"source object-type cold chain link");')
  rows.append(dict(address=p,immutable_words=list(words),cold_chain_link=p+0xc4))
 lines.append('    memory.mount(0x474b90,encode(std::array<std::uint32_t,13>{'+','.join(f'0x{v:x}u'for v in table)+'}),SourceRuntimeMemory::Access::ReadOnly,"source thirteen object-type identities");')
 for p,v in callbacks.items():lines.append(f'    memory.mount(0x{p:x},encode(std::array<std::uint32_t,1>{{0x{v:x}u}}),SourceRuntimeMemory::Access::ReadOnly,"source widget child constructor identity");')
 for p,n in banks:lines.append(f'    memory.mount(0x{p:x},std::array<std::byte,{n}>{{}},SourceRuntimeMemory::Access::ReadWrite,"source B87F0 cold constructor destination");')
 strings=[]
 for p in names:
  value=x.read(p,256).split(b'\0',1)[0];n=(len(value)+4)&~3;raw=x.read(p,n);strings.append(dict(address=p,value=value.decode('ascii'),bytes=n,sha256=hashlib.sha256(raw).hexdigest()))
  lines.append(f'    memory.mount(0x{p:x},std::array<std::byte,{n}>{{'+','.join(f'std::byte({v})'for v in raw)+'},SourceRuntimeMemory::Access::ReadOnly,"source object-storage authored constructor string");')
 lines+=['}','}'];a.output.write_text('\n'.join(lines)+'\n')
 entries=[0xb87f0,0xd6d70,0x1183f0,0x1190f0,0xc5a70,0x10d990,0x10b510,0xc1fd0,0x16e3d0,0x1268e0,0x191040,0x1d2e70,0x29ff80,0x11f390];bodies={f'{e:08x}':sorted(_trace(c,e,set(entries)).code)for e in entries}
 d=dict(original_sha256=NEW_PIN,generator_sha256=sha(__file__),generated_cpp_sha256=sha(a.output),object_type_records=rows,object_type_table=table,child_constructor_identities=callbacks,cold_banks=[dict(address=p,bytes=n)for p,n in banks],strings=strings,body_sites=bodies,complete_reused_source_bodies=len(entries),static_instruction_sites=sum(map(len,bodies.values())),cold_data_bytes=sum(n for p,n in banks)+len(records)*0xc8+52+12+sum(r['bytes']for r in strings));a.manifest.write_text(json.dumps(d,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(cold_data_bytes=d['cold_data_bytes'],bodies=len(entries),static_sites=d['static_instruction_sites'])))
if __name__=='__main__':main()
