#!/usr/bin/env python3
"""Lower whole unchanged TU1.5 deferred notification and base PCM voice bodies.

The atomic aggregate, native event synchronization/fence, virtual calls and
optional timer/PIO/resource leaves require actual native owners.
"""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate

ENTRIES=[0x38918c,0x3890f0,0x388978,0x388915,0x38cad7,0x3894e7,
         0x38bfd8,0x38b2da,0x38cbe8,0x389148,0x38cbb9,0x389325,
         0x38bf73,0x38cc02,0x386fd1,0x386eaa,0x386905,0x387b67,
         0x387ace,0x38405a,0x383da9,0x383dce,0x385d69,0x38a99e,
         0x383d40,0x383d70,0x383d92,0x38e574,0x38524f,0x383dec]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);p.add_argument('--data-output',type=Path,default=Path('src/native_tu15_sound_frontend_dpc_data.cpp'));a=p.parse_args()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'DSOUND')
 bodies={e:_trace(code,e,set(ENTRIES)) for e in ENTRIES};assert not any(b.tables for b in bodies.values())
 instructions={};addresses={}
 for pc in sorted(set().union(*(b.code for b in bodies.values()))):
  op=code.at(pc);assert bytes(op.bytes)==image.read(pc,op.size);instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+op.size]=pc+op.size
  for operand in op.operands:
   if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
   if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
 paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{e:08x}':e for e in ENTRIES},body_sites={f'{e:08x}':sorted(b.code) for e,b in bodies.items()},instruction_map=instructions,address_map={f'{pc:08x}':v for pc,v in addresses.items()},routines=[dict(entry=e,tables=[],byte_tables=[]) for e in ENTRIES])
 a.correspondence=a.manifest.with_name('sound-frontend-dpc-bodies.json');a.correspondence.parent.mkdir(parents=True,exist_ok=True);a.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n');a.additional_entries=Path(__file__)
 generate(a,paired,complete_listings=True,code_override=code)
 text=a.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_sound_frontend_dpc_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_sound_frontend_dpc_tu15').replace('source_physics_live_support_tu15_owns_entry','source_sound_frontend_dpc_tu15_owns_entry');a.output.write_text(text)
 ranges=[(0x42122c,8),(0x4212b0,8),(0x421304,0x60),(0x38f84c,16),(0x38f99c,8),(0x38fa3c,16),(0x41b7f4,4),(0x41b7b8,4)]
 lines=['#include "halo2/native_tu15_sound_frontend_dpc_data.hpp"','#include "halo2/source_runtime_memory.hpp"','#include <algorithm>','#include <array>','#include <stdexcept>','namespace halo2 {','namespace {',
 'struct Literal {std::uint32_t address;std::span<const std::byte> bytes;};',
 'bool present(const SourceRuntimeMemory& m,const Literal& row) {',
 '    std::size_t covered=0;',
 '    m.visit_committed([&](auto first,auto bytes){',
 '        const auto begin=std::max<std::uint64_t>(first,row.address);',
 '        const auto end=std::min<std::uint64_t>(std::uint64_t(first)+bytes.size(),std::uint64_t(row.address)+row.bytes.size());',
 '        if(begin<end)covered+=std::size_t(end-begin);',
 '    });',
 '    if(!covered)return false;',
 '    if(covered!=row.bytes.size())throw std::invalid_argument("Native DPC immutable operand is only partially owned");',
 '    for(std::size_t i=0;i<row.bytes.size();++i) {',
 '        const auto p=row.address+std::uint32_t(i);',
 '        if(!m.is_read_only(p,1)||m.view(p,1)[0]!=row.bytes[i])',
 '            throw std::invalid_argument("Native DPC existing immutable operand differs in access or bytes");',
 '    }',
 '    return true;',
 '}',
 '}',
 'void mount_native_tu15_sound_frontend_dpc_data(SourceRuntimeMemory& m) {']
 for index,(pc,size) in enumerate(ranges):
  raw=image.read(pc,size);lines.append(f'    constexpr std::array raw{index}{{'+','.join(f'std::byte{{0x{v:02x}}}' for v in raw)+'};')
 lines.append('    const std::array rows{'+','.join(f'Literal{{0x{pc:x},raw{index}}}' for index,(pc,size) in enumerate(ranges))+'};')
 lines+=['    std::array<bool,rows.size()> owned{};',
 '    for(std::size_t i=0;i<rows.size();++i)owned[i]=present(m,rows[i]);',
 '    std::array<std::uint32_t,rows.size()> added{};std::size_t count=0;',
 '    try {',
 '        for(std::size_t i=0;i<rows.size();++i)if(!owned[i]) {',
 '            m.mount(rows[i].address,rows[i].bytes,SourceRuntimeMemory::Access::ReadOnly,"Authenticated TU1.5 immutable PCM voice/DPC operand");',
 '            added[count++]=rows[i].address;',
 '        }',
 '    }catch(...) {',
 '        // This quiescent cold assembly owns only these newly added rows.',
 '        // Existing immutable backing is never released or replaced.',
 '        while(count)m.release(added[--count]);',
 '        throw;',
 '    }',
 '}','}'];a.data_output.write_text('\n'.join(lines)+'\n')
 manual_sites=sorted(_trace(code,0x3889f1,{0x3889f1}).code)+[0x387073,0x38707a,0x387080]
 manual={f'{pc:08x}':image.read(pc,code.at(pc).size).hex() for pc in manual_sites}
 data=dict(original_xbe_sha256=NEW_PIN,ranges=[dict(source=pc,size=size,hex=image.read(pc,size).hex()) for pc,size in ranges],data_cpp_sha256=hashlib.sha256(a.data_output.read_bytes()).hexdigest())
 a.manifest.with_name('data-manifest.json').write_text(json.dumps(data,indent=2)+'\n')
 report=json.loads(a.manifest.read_text());report.update(manual_instruction_bytes=manual,manual_cpp_sha256=hashlib.sha256(Path('src/source_sound_pending_aggregate_tu15.cpp').read_bytes()).hexdigest(),sound_frontend_dpc_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),generated_cpp_sha256=hashlib.sha256(a.output.read_bytes()).hexdigest());a.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(bodies=len(ENTRIES),instructions=len(instructions))))
if __name__=='__main__':main()
