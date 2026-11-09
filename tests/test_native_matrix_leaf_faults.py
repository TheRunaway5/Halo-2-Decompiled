#!/usr/bin/env python3
"""Exercise every memory-fault boundary in the native matrix leaf bridges."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
HARNESS = r'''
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include <iostream>
#include <map>
#include <random>
#include <tuple>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
struct Bank : SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::array<std::byte,4096>> pages;
    mutable unsigned operations{};unsigned fail_at{};
    mutable std::vector<std::tuple<char,std::uint32_t,std::size_t>> accesses;
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        accesses.emplace_back('r',address,output.size());
        if(++operations==fail_at)throw std::runtime_error("injected read fault");
        for(unsigned i=0;i<output.size();++i)output[i]=pages.at((address+i)&~4095u).at((address+i)&4095u);
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        accesses.emplace_back('w',address,input.size());
        if(++operations==fail_at)throw std::runtime_error("injected write fault");
        for(unsigned i=0;i<input.size();++i)pages.at((address+i)&~4095u).at((address+i)&4095u)=input[i];
    }
    void put(std::uint32_t address,std::uint32_t value,unsigned bytes=4) {
        for(unsigned i=0;i<bytes;++i)pages[(address+i)&~4095u][(address+i)&4095u]=std::byte(value>>(i*8));
    }
};
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
    const unsigned shift=32u-n*8u;return std::uint32_t(std::int32_t(x<<shift)>>shift);
}
// BODIES
int main() {
    constexpr std::uint32_t data=0x1000000,sp=data+0xe000,a=data+0x1000,b=data+0x2000;
    std::mt19937 random(0x145560u);
    unsigned normal_cases=0,alignment_cases=0,fault_cases=0,native_instruction_callbacks=0;
    const auto one=[](unsigned entry,WorldState &s,bool original) {
        if(original) {
            switch(entry) {
            case 0:reference::run_145560(s);return;
            case 1:reference::run_144cf0(s);return;
            case 2:reference::run_2e44d0(s);return;
            default:reference::run_2e46e0(s);return;
            }
        }
        switch(entry) {
        case 0:production::run_145560(s);return;
        case 1:production::run_144cf0(s);return;
        case 2:production::run_2e44d0(s);return;
        default:production::run_2e46e0(s);return;
        }
    };
    for(unsigned entry=0;entry<4;++entry)for(unsigned variant=0;variant<(entry<2?12u:57u);++variant) {
        Bank input;
        for(unsigned offset=0;offset<65536;offset+=4)input.put(data+offset,std::uint32_t(random()));
        input.put(0x46a0d0u,0x3f800000u);input.put(0x46a0ecu,0x3089705fu);
        input.put(0x56e684u,variant&1u);input.put(0x56e680u,0x3089705fu);
        for(unsigned i=0;i<4;++i)input.put(0x45f490u+i*4u,i<2?0u:0x80000000u);
        for(unsigned i=0;i<16;++i) {
            input.put(a+i*4u,std::bit_cast<std::uint32_t>(float(i+1u)/16.f));
            input.put(b+i*4u,std::bit_cast<std::uint32_t>(float(i+3u)/8.f));
        }
        if(entry==1&&variant%3u==0)for(unsigned i=0;i<4;++i)input.put(a+i*4u,0);
        const auto out=std::array<std::uint32_t,4>{data+0x3000,a,a+16,b}[variant%4u];
        SourcePhysicsContactRuntimeAbi initial{};
        for(auto &value:initial.gp)value=std::uint32_t(random());
        initial.gp[4]=sp+(variant%4u)*4u;initial.gp[1]=entry<2?a:out;initial.gp[2]=b;
        initial.eflags=random()&0x8d5u;
        initial.x87_depth=2;initial.x87_status=3u<<11u;initial.x87[0]=1.25L;initial.x87[1]=-2.5L;
        initial.floating_point=SourcePhysicsFactoryFloatingPoint{0x27fu,0x1f80u|((variant%4u)<<13u)};
        for(unsigned i=0;i<8;++i) {
            const auto x=_mm_set_ps(float(i+4u),float(i+3u),float(i+2u),float(i+1u));
            std::memcpy(initial.xmm[i].data(),&x,16);
        }
        input.put(initial.gp[4],0xabcdef01u);
        input.put(initial.gp[4]+4u,entry<2?out:a);
        input.put(initial.gp[4]+8u,entry==1?a:b);
        if(variant>=12) {
            const auto offset=1u+(variant-12u)%15u;
            switch((variant-12u)/15u) {
            case 0:initial.gp[1]+=offset;break;
            case 1:input.put(initial.gp[4]+4u,a+offset);break;
            default:input.put(initial.gp[4]+8u,b+offset);break;
            }
        }
        unsigned fault_limit=0;
        for(unsigned fault=0;;++fault) {
            Bank expected=input,actual=input;expected.fail_at=actual.fail_at=fault;
            std::vector<SourcePhysicsContactRecordStore> es,ns;
            SourcePhysicsContactRecordObserver eo=[&](const auto &store){es.push_back(store);};
            SourcePhysicsContactRecordObserver no=[&](const auto &store){ns.push_back(store);};
            std::uint32_t epc=0,npc=0;
            SourcePhysicsWorldRuntimeServices er{},nr{};
            er.instruction=[&](std::uint32_t pc){epc=pc;};
            nr.instruction=[&](std::uint32_t pc){npc=pc;++native_instruction_callbacks;};
            const std::array<std::uint32_t,3> sites{0x38846eu,0x21f590u,0x33b595u};
            nr.instruction_cursor.position=&npc;nr.instruction_cursor.executor=std::this_thread::get_id();
            nr.instruction_cursor.callback_type=&nr.instruction.target_type();
            nr.instruction_cursor.literal_service_sites=true;nr.instruction_cursor.observation_only_stores=true;
            nr.instruction_cursor.service_sites=sites;
            WorldState x{expected,eo,er,SourcePhysicsContactImage::TitleUpdate15};
            WorldState y{actual,no,nr,SourcePhysicsContactImage::TitleUpdate15};
            std::string ee,ne;
            x.load(initial);try{one(entry,x,true);}catch(const std::exception &e){ee=e.what();}
            const auto ex=x.save();
            y.load(initial);try{one(entry,y,false);}catch(const std::exception &e){ne=e.what();}
            const auto na=y.save();
            _mm_setcsr(0x1f80u);
            bool same=ee==ne&&expected.pages==actual.pages&&expected.accesses==actual.accesses&&expected.operations==actual.operations&&es==ns&&
                ex.gp==na.gp&&ex.eflags==na.eflags&&ex.xmm==na.xmm&&epc==npc&&
                ex.floating_point->mxcsr==na.floating_point->mxcsr&&ex.x87_depth==na.x87_depth&&ex.x87_status==na.x87_status;
            for(unsigned i=0;i<ex.x87_depth;++i)same=same&&!std::memcmp(&ex.x87[i],&na.x87[i],10);
            if(!same) {
                std::cerr<<"entry "<<entry<<" variant "<<variant<<" fault "<<fault<<" gp "<<(ex.gp==na.gp)
                    <<" flags "<<ex.eflags<<'/'<<na.eflags<<" xmm "<<(ex.xmm==na.xmm)<<" mxcsr "
                    <<ex.floating_point->mxcsr<<'/'<<na.floating_point->mxcsr<<" pc "<<std::hex<<epc<<'/'<<npc<<'\n';
                for(unsigned i=0;i<8;++i)if(ex.gp[i]!=na.gp[i])std::cerr<<"gp"<<i<<' '<<ex.gp[i]<<'/'<<na.gp[i]<<'\n';
                throw std::runtime_error("Native matrix leaf memory-fault boundary mismatch");
            }
            if(fault)++fault_cases;else{++normal_cases;if(ee=="Original aligned-vector operand is unaligned")++alignment_cases;fault_limit=expected.operations;}
            if(fault>=fault_limit)break;
        }
    }
    if(native_instruction_callbacks)throw std::runtime_error("fault validation fell back to translated execution");
    std::cout<<normal_cases-alignment_cases<<" normal + "<<alignment_cases<<" alignment errors + "<<fault_cases
        <<" matrix fault prefixes passed; native callbacks=0\n";
}
'''


class MatrixLeafFaultTests(unittest.TestCase):
    def test_every_memory_fault_publishes_the_completed_native_abi(self):
        spec = importlib.util.spec_from_file_location('specialize_native_operands', ROOT / 'tools/specialize_native_operands.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        bodies = {'reference': [], 'production': []}
        helpers = []
        for path, names in (
            ('src/physics/physics_live_support_tu15_generated.cpp', ('145560', '144cf0')),
            ('src/physics/collision/source_collision_query_tu15_generated.cpp', ('2e44d0', '2e46e0')),
        ):
            canonical = (ROOT / path).read_text()
            if not helpers:
                for signature in ('__m128 native_load_half', 'void native_store_half'):
                    helpers.append(re.search(re.escape(signature) + r'\(.*?\n\}', canonical, re.S)[0])
            specialized, receipt = module.specialize(canonical)
            for name in names:
                self.assertIn(name, receipt['native_leaves'])
                # Keep explicit hardware alignment and SSE destination-order
                # corrections, while removing the admitted native entry hook.
                reference = module.leaf_body(specialized, name)[0]
                reference = re.sub(r'    // BEGIN native leaf' + name + r'\n.*?    // END native leaf' + name + r'\n', '', reference, flags=re.S)
                bodies['reference'].append(reference)
                bodies['production'].append(module.leaf_body(specialized, name)[0])
        rendered = '\n'.join(helpers) + '\n' + '\n'.join('namespace ' + namespace + ' {\n' + '\n'.join(rows) + '\n}'
                             for namespace, rows in bodies.items())
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            cpp = Path(folder) / 'faults.cpp';cpp.write_text(HARNESS.replace('// BODIES', rendered))
            binary = Path(folder) / 'faults'
            subprocess.run(['c++', '-std=c++20', '-O2', '-ffp-contract=off', '-fno-fast-math', '-frounding-math',
                            '-pthread', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), str(cpp), '-o', str(binary)], check=True)
            env = {k: v for k, v in os.environ.items() if k not in ('HALO2_REFERENCE_MATRIX_LEAVES', 'HALO2_VERIFY_MATRIX_LEAVES')}
            subprocess.run([str(binary)], env=env, check=True)


if __name__ == '__main__':
    unittest.main()
