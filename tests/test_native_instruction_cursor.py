#!/usr/bin/env python3
"""Compile/run the exact state cursor against its full callback contract."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from source_layout import source_file

SOURCE = r'''
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <iostream>
#include <thread>
#include <chrono>
using namespace halo2;
struct Memory final : SourcePhysicsContactMemory {
    void read(std::uint32_t,std::span<std::byte>)const override {throw std::runtime_error("unexpected read");}
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("unexpected write");}
};
struct Result {
    std::uint32_t position{};unsigned ticks{},callbacks{};
    std::vector<std::uint32_t> first,second;
    std::string error;
    bool same(const Result &other)const {
        return position==other.position && ticks==other.ticks && first==other.first &&
            second==other.second && error==other.error;
    }
};
Result run(bool fast,std::thread::id executor) {
    Result result;Memory memory;SourcePhysicsContactRecordObserver observe;
    bool first=false,second=false;
    constexpr std::array<std::uint32_t,6> sites{10,20,30,40,50,99};
    SourcePhysicsContactRuntimeServices services;
    services.instruction=[&](std::uint32_t pc) {
        ++result.callbacks;
        if(pc==99)throw std::runtime_error("service rejected instruction");
        if(std::this_thread::get_id()==executor) {
            result.position=pc;
            if(first)result.first.push_back(pc);
            if(second)result.second.push_back(pc);
        }
        switch(pc) {
            case 10:first=true;break;case 20:first=false;break;
            case 30:second=true;break;case 40:second=false;break;
            case 50:++result.ticks;break;
        }
    };
    if(fast)services.instruction_cursor={&result.position,&first,&second,sites,executor,&services.instruction.target_type()};
    physics_contact_runtime_detail::State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    try {
        for(const auto pc:{1u,2u,10u,11u,12u,20u,21u,30u,31u,40u,41u,50u,51u,99u,100u}) {
            if(fast)state.instruction(pc);else services.instruction(pc);
        }
    }catch(const std::exception &error){result.error=error.what();}
    return result;
}
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
Result literal(bool optimized,bool canonical=true,std::thread::id executor=std::this_thread::get_id()) {
    Result result;Memory memory;SourcePhysicsContactRecordObserver observe;bool first{},second{};
    const std::array<std::uint32_t,3> sites{0x38846eu,0x21f590u,canonical?0x33b595u:7u};
    SourcePhysicsContactRuntimeServices services;
    services.instruction=[&](std::uint32_t pc) {
        ++result.callbacks;
        if(pc==0x21f590u)throw std::runtime_error("literal service rejected before publication");
        if(std::this_thread::get_id()==executor) {
            result.position=pc;if(first)result.first.push_back(pc);if(second)result.second.push_back(pc);
        }
        if(pc==(canonical?0x33b595u:7u)) {
            ++result.ticks;first=result.ticks==1;second=result.ticks==2;
        }
    };
    services.instruction_cursor={&result.position,&first,&second,sites,executor,&services.instruction.target_type(),optimized};
    physics_contact_runtime_detail::State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    const auto emit=[&]<std::uint32_t pc>(){if(optimized)state.instruction_literal(pc);else state.instruction(pc);};
    try {
        emit.template operator()<1>();
        if(canonical)emit.template operator()<0x33b595u>();else emit.template operator()<7u>();
        emit.template operator()<2>();emit.template operator()<3>();
        if(canonical)emit.template operator()<0x33b595u>();else emit.template operator()<7u>();
        emit.template operator()<4>();
        if(canonical)emit.template operator()<0x33b595u>();else emit.template operator()<7u>();
        emit.template operator()<5>();emit.template operator()<0x38846eu>();
        emit.template operator()<6>();emit.template operator()<0x21f590u>();
    }catch(const std::exception &e){result.error=e.what();}
    return result;
}
void benchmark(bool optimized) {
    Memory memory;SourcePhysicsContactRecordObserver observe;SourcePhysicsContactRuntimeServices services;
    std::uint32_t position{};bool first{},second{};unsigned callbacks{};
    constexpr std::array<std::uint32_t,3> sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction=[&](auto pc){++callbacks;position=pc;};
    services.instruction_cursor={&position,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),optimized};
    physics_contact_runtime_detail::State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    const auto start=std::chrono::steady_clock::now();
    constexpr unsigned iterations=5000000;
    for(unsigned i=0;i<iterations;++i) {
        state.instruction_literal(0x44710);state.instruction_literal(0x44716);state.instruction_literal(0x44719);state.instruction_literal(0x4471a);
#if defined(__GNUC__) || defined(__clang__)
        asm volatile("" : "+m"(position) :: "memory");
#endif
    }
    require(position==0x4471a && callbacks==0,"literal benchmark lost cursor state or bypassed unexpected service");
    std::cout<<"instruction_ns="<<std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/(4*iterations)<<'\n';
}
int main(int argc,char **argv)try {
    const auto executor=std::this_thread::get_id();
    const auto reference=run(false,executor),fast=run(true,executor);
    require(reference.same(fast),"cursor changed trace, service, state or exception");
    require(fast.position==51 && fast.ticks==1 && !fast.error.empty(),"exception ordering was not exercised");
    require(fast.first==std::vector<std::uint32_t>{11,12,20} && fast.second==std::vector<std::uint32_t>{31,40},
        "active traces were not exercised");
    require(fast.callbacks<reference.callbacks,"ordinary instructions did not bypass callback");
    Result worker_reference,worker_fast;
    std::thread worker([&]{worker_reference=run(false,executor);worker_fast=run(true,executor);});worker.join();
    require(worker_reference.same(worker_fast) && worker_fast.position==0 &&
        worker_reference.callbacks==worker_fast.callbacks,"other executor bypassed full callback");
    for(bool canonical:{false,true}) {
        const auto generic=literal(false,canonical),specialized=literal(true,canonical);
        require(generic.same(specialized),"literal dispatch changed service/trace/exception state");
        require(specialized.position==6 && specialized.ticks==3 && !specialized.error.empty(),"literal service rejection not exercised");
        require(generic.callbacks==specialized.callbacks,"literal dispatch changed callback delivery");
    }
    std::thread literal_worker([&]{require(literal(false,true,executor).same(literal(true,true,executor)),"literal dispatch bypassed foreign executor");});literal_worker.join();
    Memory memory;SourcePhysicsContactRecordObserver observe;SourcePhysicsContactRuntimeServices services;
    unsigned calls{};services.instruction=[&](auto){++calls;};
    physics_contact_runtime_detail::State default_state{memory,observe,services,SourcePhysicsContactImage::MapPack11};
    default_state.instruction(1);default_state.instruction(2);
    require(calls==2,"default observer lost instructions");
    std::uint32_t position=7;
    services.instruction_cursor={&position,nullptr,nullptr,{},executor,&services.instruction.target_type()};
    const auto parent=services.instruction;
    unsigned wrapped_calls{};
    services.instruction=[&](auto pc){++wrapped_calls;parent(pc);};
    physics_contact_runtime_detail::State wrapped_state{memory,observe,services,SourcePhysicsContactImage::MapPack11};
    wrapped_state.instruction(3);
    require(wrapped_calls==1 && calls==3 && position==7,"replaced callback lost its adapter side effects");
    services.instruction={};position=7;
    services.instruction_cursor={&position,nullptr,nullptr,{},executor};
    physics_contact_runtime_detail::State empty_state{memory,observe,services,SourcePhysicsContactImage::MapPack11};
    empty_state.instruction(3);require(position==7,"absent observer wrote cursor");
    if(argc==2)benchmark(std::string_view(argv[1])=="literal");
    std::cout<<"PASS traces, service ordering, pre-publication exception, other executor, replaced/default/absent observers; "
        <<reference.callbacks<<" -> "<<fast.callbacks<<" callbacks\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
'''


class NativeInstructionCursor(unittest.TestCase):
    def test_exact_callback_contract(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            directory = Path(folder)
            source = directory / 'cursor.cpp'; source.write_text(SOURCE)
            binary = directory / 'cursor'
            directories = sorted({source_file(ROOT, name).parent for name in
                ('physics_contact_runtime_internal.hpp', 'physics_solver_internal.hpp', 'physics_step_internal.hpp')})
            subprocess.run(['c++', '-std=c++20', '-O2', '-pthread', '-I' + str(ROOT / 'include'),
                '-I' + str(ROOT / 'src'), *('-I' + str(path) for path in directories),
                str(source), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    unittest.main()
