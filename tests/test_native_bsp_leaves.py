#!/usr/bin/env python3
"""Compare the native BSP leaves with their authenticated generated bodies."""
import hashlib
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
PINS = {
    '14cdb0': 'f747aad6a36fcee70a3af5ab504d7fa8d9027e392c2175bee747f0bbb2752e30',
    '14ce30': 'e88ee3cb90b6c51bf61c5eeed47db65740c8eef039f31ae55adb65a05f55d069',
}

HARNESS = r'''
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_bsp_leaves.hpp"
#include <iostream>
#include <random>
#include <tuple>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
struct Bank : SourcePhysicsContactMemory {
    std::vector<std::byte> bytes=std::vector<std::byte>(65536);
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
        for(unsigned i=0;i<n;++i)bytes.at(a+i)=std::byte(x>>(8*i));
    }
};
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
    const unsigned shift=32u-n*8u;return std::uint32_t(std::int32_t(x<<shift)>>shift);
}
unsigned fallbacks=0;
// BODIES
int main(int argc,char**) {
    std::mt19937 random(0x14cdb0u);
    std::size_t cases=0,fault_cases=0,expected_fallbacks=0;
    const bool reference=std::getenv("HALO2_REFERENCE_MATRIX_LEAVES")!=nullptr;
    const bool verify=std::getenv("HALO2_VERIFY_MATRIX_LEAVES")!=nullptr;
    const auto check=[&](Bank input, std::array<std::uint32_t,8> gp, unsigned leaf,unsigned control,unsigned policy=0,unsigned fail_at=0) {
        Bank a=input,b=input;a.fail_at=b.fail_at=fail_at;
        std::vector<SourcePhysicsContactRecordStore> sa,sb;
        SourcePhysicsContactRecordObserver oa=[&](const auto &s){sa.push_back(s);};
        SourcePhysicsContactRecordObserver ob=[&](const auto &s){sb.push_back(s);};
        std::uint32_t pca=0,pcb=0;
        SourcePhysicsWorldRuntimeServices ra{},rb{};
        ra.instruction=[&](std::uint32_t pc){pca=pc;};
        rb.instruction=[&](std::uint32_t pc){pcb=pc;};
        const std::array<std::uint32_t,3> sites{0x38846eu,0x21f590u,0x33b595u};
        rb.instruction_cursor.position=&pcb;
        rb.instruction_cursor.executor=std::this_thread::get_id();
        rb.instruction_cursor.callback_type=&rb.instruction.target_type();
        rb.instruction_cursor.literal_service_sites=true;
        rb.instruction_cursor.observation_only_stores=true;
        rb.instruction_cursor.service_sites=sites;
        const bool trace=policy==2;
        rb.instruction_cursor.first_trace=&trace;
        if(policy==1)rb.instruction_cursor.position=nullptr;
        WorldState x{a,oa,ra,SourcePhysicsContactImage::TitleUpdate15};
        WorldState y{b,ob,rb,policy==3?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        x.gp=y.gp=gp;
        const auto initial_flags=random();x.flags.load(initial_flags);y.flags.load(initial_flags);
        for(unsigned i=0;i<8;++i) {
            const std::array<std::uint32_t,4> lanes{std::uint32_t(random()),std::uint32_t(random()),
                std::uint32_t(random()),std::uint32_t(random())};
            x.xmm[i]=y.xmm[i]=_mm_castsi128_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(lanes.data())));
        }
        _mm_setcsr(control);
        std::string expected_error,actual_error;
        try{if(leaf==0)reference_14cdb0(x);else reference_14ce30(x);}
        catch(const std::exception &e){expected_error=e.what();}
        const auto mx=_mm_getcsr();
        _mm_setcsr(control);
        try{if(leaf==0)run_14cdb0(y);else run_14ce30(y);}
        catch(const std::exception &e){actual_error=e.what();}
        const auto my=_mm_getcsr();
        _mm_setcsr(0x1f80u);
        bool same=expected_error==actual_error && a.bytes==b.bytes && sa==sb && x.gp==y.gp && x.flags.bits()==y.flags.bits() && mx==my && pca==pcb;
        if(!verify)same=same && a.operations==b.operations;
        for(unsigned i=0;i<8;++i)same=same&&!std::memcmp(&x.xmm[i],&y.xmm[i],16);
        if(!same) {
            std::cerr<<"case "<<cases<<" leaf "<<leaf<<" fault "<<fail_at<<" mem "<<(a.bytes==b.bytes)<<" stores "<<(sa==sb)
                <<" gp "<<(x.gp==y.gp)<<" flags "<<x.flags.bits()<<'/'<<y.flags.bits()
                <<" mxcsr "<<mx<<'/'<<my<<" pc "<<std::hex<<pca<<'/'<<pcb<<'\n';
            for(unsigned i=0;i<8;++i)if(x.gp[i]!=y.gp[i])std::cerr<<"gp"<<i<<' '<<x.gp[i]<<'/'<<y.gp[i]<<'\n';
            for(unsigned i=0;i<8;++i)if(std::memcmp(&x.xmm[i],&y.xmm[i],16))std::cerr<<"xmm"<<i<<" differs\n";
            throw std::runtime_error("BSP native/reference mismatch");
        }
        if(fail_at)++fault_cases;else ++cases;
        expected_fallbacks+=reference||verify||policy!=0;
        return a.operations;
    };
    const std::array<std::uint32_t,12> edges{0u,0x80000000u,0x3f800000u,0xbf800000u,
        1u,0x80000001u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u,0x7f7fffffu,0x00800000u};
    for(unsigned iteration=0;iteration<4000;++iteration) {
        Bank bank;
        std::array<std::uint32_t,8> gp{};for(auto &r:gp)r=random();
        gp[4]=0xf000u+(iteration%4u);gp[0]=0;gp[1]=0x1000u;gp[2]=0x4000u;
        bank.put(0x1004u,0x2000u);bank.put(0x100cu,0x3000u);
        // Alternating signed plane indices and up to 32 packed BSP nodes.
        // Both branches reach the next node; terminal branches distinguish
        // outside from leaf references while the plane selects the side.
        const unsigned depth=1u+iteration%32u;
        for(unsigned node=0;node<depth;++node) {
            bank.put(0x2000u+node*8u,(node&1u)?0xffffu:1u,2);
            for(unsigned side=0;side<2;++side) {
                const auto child=node+1u<depth?node+1u:(side?0x80002au:0xffffffu);
                bank.put(0x2000u+node*8u+2u+side*3u,child,3);
            }
        }
        for(unsigned address:{0x2ff0u,0x3000u,0x3010u,0x4000u})for(unsigned k=0;k<4;++k)
            bank.put(address+k*4u,iteration<edges.size()?edges[(iteration+k)%edges.size()]:random());
        const auto control=0x1f80u|((iteration%4u)<<13u)|((iteration&4u)?0x8040u:0u);
        const auto point_accesses=check(bank,gp,0,control);
        if(!verify&&iteration<12)for(unsigned fault=1;fault<=point_accesses;++fault)check(bank,gp,0,control,0,fault);
        if(iteration<3)check(bank,gp,0,control,iteration+1);
        // Plane extraction: positive/negative index and exact/partial alias.
        gp[0]=std::array<std::uint32_t,6>{0x5000u,0x3000u,0x3004u,0x2ffcu,gp[4]-4u,gp[4]-8u}[iteration%6u];
        gp[2]=(iteration&1u)?0x8000u:0u;
        bank.put(gp[4]+4u,0x1000u);
        const auto plane_accesses=check(bank,gp,1,control);
        if(!verify&&iteration<12)for(unsigned fault=1;fault<=plane_accesses;++fault)check(bank,gp,1,control,0,fault);
        if(iteration<3)check(bank,gp,1,control,iteration+1);
    }
    if(fallbacks!=expected_fallbacks)throw std::runtime_error("unexpected native admission count");
    std::cout<<cases<<" BSP cases + "<<fault_cases<<" fault prefixes passed; translated calls="<<fallbacks<<'\n';
}
'''


class NativeBspLeaves(unittest.TestCase):
    def test_authenticated_bodies_and_default_native_route(self):
        spec = importlib.util.spec_from_file_location('specialize_native_operands', ROOT / 'tools/specialize_native_operands.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        canonical = SOURCE.read_text()
        specialized, receipt = module.specialize(canonical)
        bodies = []
        for name, pin in PINS.items():
            original = module.leaf_body(canonical, name)
            self.assertEqual(hashlib.sha256(original[0].replace(original[1] + ' &s', 'STATE &s').encode()).hexdigest(), pin)
            self.assertIn(name, receipt['native_leaves'])
            bodies.append(original[0].replace('run_' + name, 'reference_' + name))
            native = module.leaf_body(specialized, name)[0]
            marker = '    // END native leaf' + name + '\n'
            self.assertIn(marker, native)
            bodies.append(native.replace(marker, marker + '    ++fallbacks;\n'))
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            source = Path(folder) / 'bsp.cpp'
            source.write_text(HARNESS.replace('// BODIES', '\n'.join(bodies)))
            binary = Path(folder) / 'bsp'
            subprocess.run(['c++', '-std=c++20', '-O2', '-ffp-contract=off', '-fno-fast-math', '-pthread',
                            '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'include'), str(source), '-o', str(binary)], check=True)
            clean = {k: v for k, v in os.environ.items() if k not in ('HALO2_REFERENCE_MATRIX_LEAVES', 'HALO2_VERIFY_MATRIX_LEAVES')}
            for setting in (None, 'HALO2_REFERENCE_MATRIX_LEAVES', 'HALO2_VERIFY_MATRIX_LEAVES'):
                env = clean.copy()
                if setting:
                    env[setting] = '1'
                subprocess.run([str(binary)], env=env, check=True)


if __name__ == '__main__':
    unittest.main()
