#!/usr/bin/env python3
"""Exercise checked counters through the real contact EventMemory lifetime guard."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import validate_native_phantom_body_velocity_port_tu15 as core

ROOT = core.ROOT
DRIVER = r'''
#include "halo2/native_tu15_contact_point_create.hpp"
#include "halo2/native_tu15_island_activation.hpp"
#include "halo2/native_tu15_retained_counter_update.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <cstring>
#include <iostream>
using namespace halo2;
void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
constexpr unsigned base=0x100000,manager=base,world=base+0x200,island=base+0x400;
constexpr unsigned first=base+0x700,second=base+0x740,body=base+0x800,scratch=base+0x1000,event=scratch+0x30;
void put(SourcePhysicsContactMemory&m,unsigned p,unsigned value,unsigned n=4){std::array<std::byte,4>b{};std::memcpy(b.data(),&value,4);m.write(p,std::span(b).first(n));}
unsigned word(SourcePhysicsContactMemory&m,unsigned p,unsigned n=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));unsigned v{};std::memcpy(&v,b.data(),4);return v;}
void initial(SourceRuntimeMemory&m){
 std::array<std::byte,0x2000>b{};m.mount(base,b,SourceRuntimeMemory::Access::ReadWrite,"declared native contact activation before-inputs");
 put(m,manager+8,world);put(m,first+0x20,body);put(m,second+0x20,base+0x900);
 put(m,world+8,base+0x600);put(m,world+0x10,2);put(m,world+0x14,base+0x640);put(m,world+0x18,1);put(m,base+0x640,island);
 put(m,body+8,world);put(m,body+0x44,island);put(m,body+6,2,2);put(m,world+0x108,base+0x680);put(m,world+0x10c,1);put(m,base+0x680,body);
}
struct Completed{};
template<class F>void contact(SourcePhysicsContactMemory&m,F operation){
 NativeTu15PhysicsFrameFp fp{0x27f,0,0x1f80};NativeTu15ContactPointCreateServices services;
 NativeTu15ContactPointCreateStats stats;
 services.register_constraint=[&](auto&view,auto w,auto,auto,auto&numeric){check(w==world,"Actual point registration world differs");operation(view,numeric);throw Completed{};};
 bool stopped{};try{(void)native_tu15_create_contact_point(m,manager,{first},{second},{},base+0x1200,{scratch},fp,0,services,{},&stats);}catch(const Completed&){stopped=true;}
 check(stopped&&stats.registrations==1&&stats.allocations==0&&stats.completed==0,"Contact guard model fabricated point completion");
}
struct Opaque final:SourcePhysicsContactMemory{
 SourceRuntimeMemory&bank;mutable unsigned reads{};unsigned writes{};explicit Opaque(SourceRuntimeMemory&m):bank(m){}
 void read(unsigned p,std::span<std::byte>b)const override{++reads;bank.read(p,b);}
 void write(unsigned p,std::span<const std::byte>b)override{++writes;bank.write(p,b);}
};
int main()try{
 SourceRuntimeMemory memory;initial(memory);std::map<std::pair<unsigned,unsigned>,unsigned> guards;unsigned audit{};
 source_runtime_register_write_guard(memory,&guards,[&](auto p,auto n){++guards[{p,unsigned(n)}];});
 source_runtime_register_write_observer(memory,&audit,[&](auto,auto){++audit;});
 try{contact(memory,[&](auto&view,auto&fp){native_tu15_activate_physics_body(view,{body},fp,{});});}
 catch(const std::runtime_error&e){if(std::string(e.what())=="Native counter update requires checked read-modify-write storage"){std::cerr<<"CURRENT_EVENT_VIEW_RED: "<<e.what()<<'\n';return 31;}throw;}
 check(word(memory,body+6,2)==1&&word(memory,world+0xc)==1&&word(memory,world+0x18)==0,"Actual native activation counters differ");
 check(guards[{body+6,2}]==1&&guards[{world+0xc,4}]==1&&guards[{world+0x18,4}]==1,"Event forwarding repeated counter write admission");
 source_runtime_unregister_write_observer(memory,&audit);source_runtime_unregister_write_guard(memory,&guards);
 for(auto [address,width]:{std::pair{event+2,2u},std::pair{event+0x10,4u},std::pair{event+0xf,2u}}){
  SourceRuntimeMemory fresh;initial(fresh);unsigned callbacks{};
  contact(fresh,[&](auto&view,auto&){bool refused{};try{native_tu15_adjust_retained_counter(view,0x316d6eu,address,width,1,{});}catch(const std::runtime_error&e){refused=std::string(e.what()).find("unprovided DATA lifetime owner")!=std::string::npos;}check(refused,"Event lifetime/padding counter did not refuse before DATA");++callbacks;});
  check(callbacks==1,"Missing actual EventMemory callback");
 }
 SourceRuntimeMemory fresh;initial(fresh);Opaque opaque(fresh);
 contact(opaque,[&](auto&view,auto&){const auto reads=opaque.reads,writes=opaque.writes;bool refused{};try{native_tu15_adjust_retained_counter(view,0x316d6eu,world+0xc,4,1,{});}catch(const std::runtime_error&e){refused=std::string(e.what())=="Native contact event counter requires checked outer storage";}check(refused&&reads==opaque.reads&&writes==opaque.writes,"Unknown event backing read/wrote or was admitted");});
 check(word(fresh,world+0xc)==0,"Unknown event backing committed counter");
 std::cout<<"GREEN: real contact EventMemory forwards native body activation; three counter sites admitted once; three lifetime refusals; unknown backing refuses before DATA\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.source, args.output = args.source.resolve(), args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    report = dict(passed=False, scope=__doc__, original_point_algorithm_comparison=False,
                  point_completion_claimed=False, live_admission=False, owned_scratch=str(scratch), retained_large_artifacts=[])
    pins = {}
    try:
        cpp = scratch / 'event-view.cpp'
        cpp.write_text(DRIVER)
        units = [args.source / 'src/runtime/source_runtime_memory.cpp', args.source / 'src/physics/native_tu15_island_activation.cpp']
        event = Path('src/physics/native_tu15_contact_point_create.cpp')
        pins = {str(p): core.sha(p) for p in (*units, ROOT / event, args.source / event, Path(__file__))}
        report['source_sha256'] = pins
        report['results'] = []
        for compiler in ('g++', 'clang++'):
            for variant in ('current', 'candidate'):
                binary = scratch / (compiler + '-' + variant)
                command = [compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
                           '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections', '-pthread',
                           '-I' + str(args.source / 'include'), '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                           '-I' + str(ROOT / 'src/physics'), str(cpp), *map(str, units),
                           str((ROOT if variant == 'current' else args.source) / event), '-o', str(binary)]
                p = subprocess.run(command, capture_output=True, text=True)
                (args.output / (binary.name + '-build.log')).write_text(p.stdout + p.stderr)
                assert p.returncode == 0, p.stderr[-5000:]
                p = subprocess.run([str(binary)], capture_output=True, text=True)
                (args.output / (binary.name + '-run.log')).write_text(p.stdout + p.stderr)
                expected = (31, 'CURRENT_EVENT_VIEW_RED') if variant == 'current' else (0, 'GREEN')
                assert p.returncode == expected[0] and expected[1] in p.stdout + p.stderr, p.stderr[-5000:]
                report['results'].append(dict(compiler=compiler, variant=variant, returncode=p.returncode, command=command))
        assert all(core.sha(Path(name)) == value for name, value in pins.items())
        report['passed'] = True
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(path.stat().st_blocks * 512 for path in scratch.rglob('*') if path.is_file())
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
