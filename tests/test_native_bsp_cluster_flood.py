#!/usr/bin/env python3
"""Whole-driver portal flood parity with genuine generated predicate callees."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
HARNESS = r'''
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_bsp_cluster_flood.hpp"
#include <iostream>
#include <random>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
struct Bank : SourcePhysicsContactMemory {
    std::vector<std::byte> bytes=std::vector<std::byte>(0x800000u);
    mutable unsigned operations{};unsigned fail_at{};
    void read(std::uint32_t a,std::span<std::byte> b)const override {
        if(++operations==fail_at)throw std::runtime_error("injected read fault");
        if(a>bytes.size()||b.size()>bytes.size()-a)throw std::runtime_error("read bounds");
        std::copy_n(bytes.begin()+a,b.size(),b.begin());
    }
    void write(std::uint32_t a,std::span<const std::byte> b)override {
        if(++operations==fail_at)throw std::runtime_error("injected write fault");
        if(a>bytes.size()||b.size()>bytes.size()-a)throw std::runtime_error("write bounds");
        std::copy(b.begin(),b.end(),bytes.begin()+a);
    }
    void put(std::uint32_t a,std::uint32_t x,unsigned n=4) {
        for(unsigned i=0;i<n;++i)bytes.at(a+i)=std::byte(x>>(i*8));
    }
    void real(std::uint32_t a,float x){put(a,std::bit_cast<std::uint32_t>(x));}
};
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
    const auto shift=32u-n*8u;return std::uint32_t(std::int32_t(x<<shift)>>shift);
}
static void run_123210(WorldState &);
static void run_243450(WorldState &);
static void run_14cea0(WorldState &);
static void run_14d0e0(WorldState &);
static void reference_14d0e0(WorldState &);
// BODIES
int main() {
    std::mt19937 random(0x14d0e0u);
    std::uint64_t cases=0,fault_cases=0,predicate_calls=0,reference_driver_instructions=0,native_driver_instructions=0;
    for(unsigned iteration=0;iteration<120;++iteration) {
        Bank input;
        constexpr std::uint32_t bsp=0x10000,clusters=0x20000,portals=0x30000,
            collision=0x40000,planes=0x50000,vertices=0x60000,point=0x70000,output=0x80000,stack=0x780000;
        const unsigned count=2u+iteration%8u;
        input.put(0x4ecc18u,bsp);input.put(0x4ecc0cu,collision);input.put(0x4f3ce4u,0x100u+iteration);
        input.put(bsp+0xa0u,clusters);input.put(bsp+0x60u,portals);input.put(bsp+0x18u,collision);
        input.put(collision+12u,planes);
        for(unsigned axis=0;axis<3;++axis)for(unsigned side=0;side<2;++side) {
            input.put(0x44ac8cu+(axis*2u+side)*6u,(axis+1u)%3u,2);
            input.put(0x44ac8eu+(axis*2u+side)*6u,(axis+2u)%3u,2);
        }
        // Bidirectional ring plus duplicate and self edges exercise DFS visit
        // marks, reused portal tests, capacity exhaustion, and portal rejection.
        for(unsigned cluster=0;cluster<count;++cluster) {
            const auto node=clusters+cluster*0xb0u;
            input.put(node+0x8cu,iteration%13u==0?0u:3u);
            input.put(node+0x90u,0x90000u+cluster*16u);
            for(unsigned j=0;j<3;++j)input.put(0x90000u+cluster*16u+j*2u,cluster*3u+j,2);
            if(iteration%7u==0 && cluster==count-1)
                input.put(0x4f3ce8u+cluster*4u,0x100u+iteration);
            for(unsigned j=0;j<3;++j) {
                const auto edge=cluster*3u+j;
                const auto portal=portals+edge*36u;
                const auto next=j==2?cluster:(cluster+(j==0?1:count-1))%count;
                input.put(portal,(j&1u)?next:cluster,2);input.put(portal+2u,(j&1u)?cluster:next,2);input.put(portal+4u,edge);
                input.real(portal+20u,2.f);
                const unsigned nv=iteration%11u==0?0u:4u;
                input.put(portal+28u,nv);input.put(portal+32u,vertices+edge*128u);
                input.real(planes+edge*16u,1.f);
                input.real(planes+edge*16u+12u,(iteration%5u==0 && j==0)?3.f:0.f);
                for(unsigned vertex=0;vertex<4;++vertex) {
                    input.real(vertices+edge*128u+vertex*12u+4u,vertex<2?-1.f:1.f);
                    input.real(vertices+edge*128u+vertex*12u+8u,(vertex==0||vertex==3)?-1.f:1.f);
                }
            }
        }
        const std::array<std::int32_t,8> limits{-1,0,1,2,3,8,32767,-32768};
        SourcePhysicsContactRuntimeAbi initial{};
        for(auto &value:initial.gp)value=std::uint32_t(random());
        initial.gp[4]=stack+(iteration%4u);initial.eflags=random()&0x8d5u;
        for(auto &lanes:initial.xmm)for(auto &value:lanes)value=std::uint32_t(random());
        initial.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t((iteration&1u)?0x27fu:0x37fu),
            0x1f80u|((iteration%4u)<<13u)|((iteration&4u)?0x8040u:0u)};
        input.put(initial.gp[4],0xabcdef01u);input.put(initial.gp[4]+4u,iteration%count);
        input.put(initial.gp[4]+8u,point);input.real(initial.gp[4]+12u,1.f);
        input.put(initial.gp[4]+16u,std::uint32_t(limits[iteration%limits.size()]));input.put(initial.gp[4]+20u,output);
        if(iteration%17u==0) {
            // A single output aliases a saved nonvolatile stack word. Later
            // recursive nodes still count but must respect exhausted capacity.
            input.put(initial.gp[4]+16u,1);input.put(initial.gp[4]+20u,initial.gp[4]-8u);
        }
        unsigned fault_limit=0;
        for(unsigned fault=0;;++fault) {
        Bank a=input,b=input;a.fail_at=b.fail_at=fault;
        std::vector<SourcePhysicsContactRecordStore> sa,sb;
        SourcePhysicsContactRecordObserver oa=[&](const auto &store){sa.push_back(store);};
        SourcePhysicsContactRecordObserver ob=[&](const auto &store){sb.push_back(store);};
        std::uint32_t pca=0,pcb=0;
        SourcePhysicsWorldRuntimeServices ra{},rb{};
        ra.instruction=[&](std::uint32_t pc){pca=pc;if(pc>=0x14d0e0u&&pc<=0x14d1f6u)++reference_driver_instructions;};
        rb.instruction=[&](std::uint32_t pc){pcb=pc;if(pc==0x14cea0u)++predicate_calls;
            if(pc>=0x14d0e0u&&pc<=0x14d1f6u)++native_driver_instructions;};
        const std::array<std::uint32_t,3> sites{0x38846eu,0x21f590u,0x33b595u};
        rb.instruction_cursor.position=&pcb;rb.instruction_cursor.executor=std::this_thread::get_id();
        rb.instruction_cursor.callback_type=&rb.instruction.target_type();
        rb.instruction_cursor.literal_service_sites=true;rb.instruction_cursor.observation_only_stores=true;
        rb.instruction_cursor.service_sites=sites;
        WorldState x{a,oa,ra,SourcePhysicsContactImage::TitleUpdate15};
        WorldState y{b,ob,rb,SourcePhysicsContactImage::TitleUpdate15};
        std::string expected_error,actual_error;
        x.load(initial);try{reference_14d0e0(x);}catch(const std::exception &e){expected_error=e.what();}
        const auto expected=x.save();
        y.load(initial);try{run_14d0e0(y);}catch(const std::exception &e){actual_error=e.what();}
        const auto actual=y.save();
        _mm_setcsr(0x1f80u);
        bool same=expected_error==actual_error&&a.operations==b.operations&&a.bytes==b.bytes&&sa==sb&&expected.gp==actual.gp&&expected.xmm==actual.xmm&&
            expected.eflags==actual.eflags&&expected.x87_depth==actual.x87_depth&&
            expected.x87_status==actual.x87_status&&expected.floating_point->mxcsr==actual.floating_point->mxcsr&&pca==pcb;
        if(!same) {
            std::cerr<<"case "<<iteration<<" fault "<<fault<<" mem "<<(a.bytes==b.bytes)<<" stores "<<(sa==sb)
                <<" flags "<<expected.eflags<<'/'<<actual.eflags<<" mxcsr "<<expected.floating_point->mxcsr
                <<'/'<<actual.floating_point->mxcsr<<" pc "<<std::hex<<pca<<'/'<<pcb<<'\n';
            for(unsigned i=0;i<8;++i)if(expected.gp[i]!=actual.gp[i])std::cerr<<"gp"<<i<<' '<<expected.gp[i]<<'/'<<actual.gp[i]<<'\n';
            throw std::runtime_error("Native cluster flood differs from generated original");
        }
        if(fault)++fault_cases;else {++cases;if(iteration==1)fault_limit=a.operations;}
        if(fault>=fault_limit)break;
        }
    }
    if(!reference_driver_instructions||!predicate_calls)throw std::runtime_error("missing positive driver/predicate coverage");
    if(!std::getenv("HALO2_REFERENCE_BSP_CLUSTER_FLOOD")&&native_driver_instructions)
        throw std::runtime_error("native driver used translated instructions");
    if(std::getenv("HALO2_REFERENCE_BSP_CLUSTER_FLOOD")&&!native_driver_instructions)
        throw std::runtime_error("reference control was not exercised");
    std::cout<<cases<<" cluster floods + "<<fault_cases<<" injected fault prefixes passed; native route driver instructions="<<native_driver_instructions
        <<"; real predicate calls="<<predicate_calls<<'\n';
}
'''


class ClusterFloodTests(unittest.TestCase):
    def setUp(self):
        spec = importlib.util.spec_from_file_location('native_bsp_cluster_replacements', ROOT / 'tools/native_bsp_cluster_replacements.py')
        self.helper = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.helper)

    def test_generator_authenticates_every_copy_and_rejects_semantic_drift(self):
        for path in (SOURCE, ROOT / 'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp',
                     ROOT / 'src/campaign/ai/source_ai_frame_tu15_generated.cpp'):
            source = path.read_text()
            self.assertEqual(self.helper.install_native_bsp_cluster_flood(source), source)
            with self.assertRaisesRegex(ValueError, 'body changed'):
                self.helper.install_native_bsp_cluster_flood(source.replace('0x14d145u,(0x24u', '0x14d145u,(0x26u')
                    if path == SOURCE else source.replace('0x14d145u,(selected(0x24u', '0x14d145u,(selected(0x26u'))

    def test_full_driver_with_real_predicate_callees(self):
        source = SOURCE.read_text()
        bodies = []
        for name in ('123210', '243450', '14cea0', '14d0e0'):
            body = re.search(r'static void run_' + name + r'\(WorldState &s\) \{.*?\n\}', source, re.S)[0]
            if name == '14d0e0':
                bodies.append(body.replace(self.helper.PREFIX, '').replace('run_14d0e0', 'reference_14d0e0'))
            bodies.append(body)
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            cpp = Path(folder) / 'flood.cpp';cpp.write_text(HARNESS.replace('// BODIES', '\n'.join(bodies)))
            binary = Path(folder) / 'flood'
            subprocess.run(['c++', '-std=c++20', '-O2', '-ffp-contract=off', '-fno-fast-math', '-frounding-math',
                            '-pthread', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), str(cpp), '-o', str(binary)], check=True)
            env = {k: v for k, v in os.environ.items() if k != 'HALO2_REFERENCE_BSP_CLUSTER_FLOOD'}
            subprocess.run([str(binary)], env=env, check=True)
            env['HALO2_REFERENCE_BSP_CLUSTER_FLOOD'] = '1'
            subprocess.run([str(binary)], env=env, check=True)

    def test_specialized_driver_and_predicate_closure_has_no_instruction_callbacks(self):
        spec = importlib.util.spec_from_file_location('specialize_native_operands', ROOT / 'tools/specialize_native_operands.py')
        specializer = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(specializer)
        canonical = SOURCE.read_text()
        specialized, receipt = specializer.specialize(canonical)
        names = ('123210', '243450', '14cea0', '14d0e0')
        for name in names[:-1]:
            self.assertIn(name, receipt['native_leaves'])
        bodies = [f'static void reference_run_{name}(WorldState &);' for name in names]
        for name in names:
            reference = specializer.leaf_body(canonical, name)[0].replace(self.helper.PREFIX, '')
            bodies.append(re.sub(r'\brun_([0-9a-f]+)', r'reference_run_\1', reference))
            bodies.append(specializer.leaf_body(specialized, name)[0])
        harness = '#include "physics/runtime/native_tu15_portal_predicate_leaves.hpp"\n' + HARNESS
        harness = harness.replace('reference_14d0e0(x)', 'reference_run_14d0e0(x)')
        harness = harness.replace('if(iteration==1)fault_limit=a.operations;', '')
        harness = harness.replace('std::uint64_t cases=0,', 'std::uint64_t all_native_callbacks=0,reference_predicates=0,cases=0,')
        harness = harness.replace('pca=pc;if(pc>=', 'pca=pc;if(pc==0x14cea0u)++reference_predicates;if(pc>=')
        harness = harness.replace('pcb=pc;if(pc==', 'pcb=pc;++all_native_callbacks;if(pc==')
        harness = harness.replace('||!predicate_calls', '||!reference_predicates')
        harness = harness.replace('std::cout<<cases',
            'if(all_native_callbacks)throw std::runtime_error("native BSP dependency closure emitted translated callbacks");\n    std::cout<<cases')
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            cpp = Path(folder) / 'closure.cpp';cpp.write_text(harness.replace('// BODIES', '\n'.join(bodies)))
            binary = Path(folder) / 'closure'
            subprocess.run(['c++', '-std=c++20', '-O2', '-ffp-contract=off', '-fno-fast-math', '-frounding-math',
                            '-pthread', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), str(cpp), '-o', str(binary)], check=True)
            env = {k: v for k, v in os.environ.items() if k not in (
                'HALO2_REFERENCE_BSP_CLUSTER_FLOOD', 'HALO2_REFERENCE_MATRIX_LEAVES', 'HALO2_VERIFY_MATRIX_LEAVES')}
            subprocess.run([str(binary)], env=env, check=True)


if __name__ == '__main__':
    unittest.main()
