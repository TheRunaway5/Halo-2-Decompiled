#include "physics/runtime/physics_step_internal.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <new>

namespace {
bool count_allocations{};
std::size_t allocations{};
void require(bool value,const char *message) {
    if(!value)throw std::runtime_error(message);
}
struct Memory final:halo2::SourcePhysicsContactMemory {
    std::array<std::byte,16> bytes;
    mutable std::array<char,3> order{};
    mutable unsigned calls{};
    bool reject_read{},reject_write{};
    Memory() {for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte(0xa0u+i);}
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        order.at(calls++)='r';require(address==0x100 && output.size()<=bytes.size(),"Unexpected store read");
        if(reject_read)throw std::runtime_error("read rejected");
        std::copy_n(bytes.begin(),output.size(),output.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        order.at(calls++)='w';require(address==0x100 && input.size()<=bytes.size(),"Unexpected store write");
        if(reject_write)throw std::runtime_error("write rejected");
        std::copy(input.begin(),input.end(),bytes.begin());
    }
};
}
void *operator new(std::size_t size) {
    if(count_allocations)++allocations;
    if(auto *p=std::malloc(size?size:1))return p;
    throw std::bad_alloc();
}
void operator delete(void *p)noexcept {std::free(p);}
void operator delete(void *p,std::size_t)noexcept {std::free(p);}

int main()try {
    using namespace halo2;
    constexpr std::uint64_t value=0xfedcba9876543210ull;
    for(const unsigned width:{0u,1u,2u,4u,8u})for(const bool observed:{false,true}) {
        Memory memory;const auto before=memory.bytes;auto expected=before;
        for(unsigned i=0;i<width;++i)expected[i]=std::byte(value>>(8*i));
        SourcePhysicsContactRecordObserver observer;
        if(observed)observer=[&](const auto &store) {
            memory.order.at(memory.calls++)='o';
            require(store.instruction==0x1234 && store.address==0x100,"Store identity changed");
            require(store.before.size()==width && store.written.size()==width,"Observed store width changed");
            require(std::equal(store.before.begin(),store.before.end(),before.begin()),"Observed before bytes changed");
            require(std::equal(store.written.begin(),store.written.end(),expected.begin()),"Observed written bytes changed");
            require(memory.bytes==expected,"Observer ran before store publication");
        };
        const auto allocated=allocations;count_allocations=true;
        try {physics_step_detail::w(memory,observer,0x1234,0x100,width,value);}
        catch(...) {count_allocations=false;throw;}
        count_allocations=false;
        require(observed || allocations==allocated,"Unobserved scalar store allocated byte buffers");
        require(memory.bytes==expected,"Scalar store changed bytes outside its width");
        if(observed)require(memory.calls==3 && memory.order==std::array<char,3>{'r','w','o'},
            "Observed store read/write/observer order changed");
        else require(memory.calls==1 && memory.order[0]=='w',
            "Unobserved original MOV performed a synthetic destination read");
    }
    // The original store never reads its destination. A memory owner may
    // reject reads while accepting an authenticated write-only operation.
    for(const unsigned width:{1u,2u,4u,8u}) {
        Memory memory;memory.reject_read=true;
        physics_step_detail::w(memory,{},0x1234,0x100,width,value);
        require(memory.calls==1 && memory.order[0]=='w',"Write-only original store consumed a destination preimage");
        for(unsigned i=0;i<width;++i)require(memory.bytes[i]==std::byte(value>>(8*i)),"Write-only store changed source bytes");
    }
    {
        Memory memory;memory.reject_write=true;const auto before=memory.bytes;bool threw=false;
        try {physics_step_detail::w(memory,{},0x1234,0x100,4,value);}catch(const std::runtime_error&){threw=true;}
        require(threw && memory.calls==1 && memory.order[0]=='w' && memory.bytes==before,
            "Unobserved store bypassed write validation or changed rejected bytes");
    }
    for(unsigned failure=0;failure<3;++failure) {
        Memory memory;const auto before=memory.bytes;
        memory.reject_read=failure==0;memory.reject_write=failure==1;
        bool observed=false,threw=false;
        SourcePhysicsContactRecordObserver observer=[&](const auto &) {
            observed=true;throw std::runtime_error("observer rejected");
        };
        try {physics_step_detail::w(memory,observer,0x1234,0x100,8,value);}
        catch(const std::runtime_error &){threw=true;}
        require(threw && observed==(failure==2),"Store continued after a rejected boundary");
        require(failure==2 || memory.bytes==before,"Rejected memory store changed its backing");
        require(memory.calls==(failure==0?1u:2u),"Rejected store changed its memory calls");
    }
    std::cout<<"PASS scalar store widths, zero allocations without observer, exact bytes and ordered failure/observer boundaries\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
