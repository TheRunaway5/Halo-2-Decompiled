#include "physics/runtime/native_tu15_mopp_ray_dispatch_tables.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage=native_mopp_ray_dispatch_detail;
constexpr std::uint32_t page=0x30b000u, index_table=0x30b738u,target_table=0x30b6b8u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
constexpr std::array targets{
    0x30aaaeu,0x30aae3u,0x30ab22u,0x30ab56u,
    0x30ab94u,0x30abc8u,0x30ac06u,0x30ac42u,
    0x30ac84u,0x30acc6u,0x30ad0cu,0x30ad4au,
    0x30ad9bu,0x30ae16u,0x30b17du,0x30b1dbu,
    0x30b328u,0x30b335u,0x30b34bu,0x30b36au,
    0x30b437u,0x30b477u,0x30b4c0u,0x30b51bu,
    0x30b52bu,0x30b544u,0x30b5a3u,0x30b5a9u,
    0x30b5b8u,0x30b5d0u,0x30b5f2u,0x30b6afu
};
constexpr std::uint32_t elsewhere=0xcc271u;

struct Fixture {
    std::uint32_t eax=7u, target=0x30b6afu, fault_pc{};
    std::uint8_t index=4u;
    bool ordinary=true, sparse_index{}, sparse_target{}, trace{}, authentic=true, observer{}, changes_target{};
};
struct Access {
    std::uint32_t pc,address,ecx;
    std::size_t width;
    bool operator==(const Access &)const=default;
};
struct Result {
    std::array<std::uint32_t,8> registers{};
    std::vector<Access> reads;
    std::vector<std::uint32_t> instructions;
    std::uint32_t pc{},target{},eflags{};
    unsigned observed{};
    bool native{};
    std::string fault;
};
struct Bank final : SourcePhysicsContactMemory {
    const Fixture &f;
    Result &r;
    SourceDirectPageIndex index;
    mutable std::array<std::byte,4096> data{};
    WorldState *live=nullptr;

    Bank(const Fixture &f,Result &r):f(f),r(r) {
        if(f.ordinary) {
            direct_pages_=&index;
            auto &block=index.blocks[page>>22u];
            block=std::make_unique<SourceDirectPageIndex::Block>();
            (*block)[(page>>12u)&1023u]={data.data(),false};
        }
        store_target(f.target);
        const auto address=index_table+f.eax;
        if(address>=page&&address-page<4096u)data[address-page]=std::byte(f.index);
    }
    void store_target(std::uint32_t target) const {
        std::memcpy(data.data()+target_table-page+4u*f.index,&target,4u);
    }
    std::optional<std::span<const std::byte>> try_read_view(
        std::uint32_t address,std::size_t size) const noexcept override {
        if(f.sparse_index && size==1u && address>=index_table && address-index_table<108u)
            return std::span<const std::byte>{data}.subspan(address-page,size);
        if(f.sparse_target && size==4u && address>=target_table && address-target_table<=124u)
            return std::span<const std::byte>{data}.subspan(address-page,size);
        return std::nullopt;
    }
    void read(std::uint32_t address,std::span<std::byte> data_out)const override {
        if(const auto view=try_read_view(address,data_out.size())) {
            std::memcpy(data_out.data(),view->data(),data_out.size());
            return;
        }
        r.reads.push_back({r.pc,address,live->gp[1],data_out.size()});
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected dispatcher read fault");
        if(address>=page&&address-page<=4096u-data_out.size())
            std::memcpy(data_out.data(),data.data()+address-page,data_out.size());
        else if(data_out.size()==1u)data_out[0]=std::byte(f.index);
        else throw std::runtime_error("Unexpected dispatcher read address");
        if(r.pc==0x30aaa0u&&f.changes_target)store_target(0x30b328u);
    }
    void write(std::uint32_t,std::span<const std::byte>)override {
        throw std::runtime_error("Unexpected dispatcher write");
    }
};

bool supported(std::uint32_t selected) {
    return std::find(targets.begin(),targets.end(),selected)!=targets.end();
}
std::uint32_t canonical(std::uint32_t selected) {
    if(supported(selected)||selected==elsewhere)return selected;
    throw std::runtime_error("Unbound TU1.5 contact dispatch target");
}
std::uint32_t select(WorldState &s,std::uint32_t raw,bool native) {
    if(!supported(raw)) {
        if(native)(void)canonical(raw);
        throw std::runtime_error("Required original solver dispatch target unavailable");
    }
    return raw;
}
Result run(const Fixture &f,bool use_native) {
    Result result;Bank bank{f,result};
    result.pc=0x30aa9du;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){result.pc=pc;result.instructions.push_back(pc);};
    services.instruction_cursor={&result.pc,&f.trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    if(!f.authentic)services.instruction_cursor={};
    SourcePhysicsContactRecordObserver observer;
    if(f.observer)observer=[&](const auto &){++result.observed;};
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={f.eax,0xffff8444u,0x1337u,0x1200u,0x62000000u,0x62001000u,0x77u,0x88u};
    input.eflags=0x256u;
    WorldState state{bank,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.live=&state;
    std::uint32_t raw;
    try {
        if(use_native)result.native=stage::read_dispatch_tables(state,raw);
        if(!result.native) {
            state.instruction_literal(0x30aaa0u);
            state.set(1,r(state.memory,index_table+state.gp[0],1u),4,0);
            state.instruction_literal(0x30aaa7u);
            raw=canonical(r(state.memory,target_table+state.gp[1]*4u,4u));
        }
        result.target=select(state,raw,result.native);
    } catch(const std::runtime_error &e) {result.fault=e.what();}
    result.registers=state.gp;result.eflags=state.save().eflags;
    return result;
}
void compare(const Fixture &f,bool admitted) {
    if(stage::reference_dispatch_tables())admitted=false;
    const auto a=run(f,false),b=run(f,true);
    if(a.pc!=b.pc||a.target!=b.target||a.fault!=b.fault||a.registers!=b.registers||
       a.eflags!=b.eflags||a.reads!=b.reads||a.instructions!=b.instructions||
       a.observed!=0u||b.observed!=0u||b.native!=admitted) {
        std::cerr<<"Dispatcher EAX, table value "<<std::hex<<f.eax<<'/'<<unsigned(f.index)
                 <<" target "<<a.target<<'/'<<b.target<<" PC "<<a.pc<<'/'<<b.pc
                 <<" fault "<<a.fault<<'/'<<b.fault<<'\n';
        throw std::runtime_error("MOPP dispatcher tables differ");
    }
}
unsigned prove() {
    unsigned compared{};
    auto check=[&](const Fixture &f,bool admitted){compare(f,admitted);++compared;};
    for(unsigned index=0;index<256u;++index) {
        Fixture f;f.index=std::uint8_t(index);f.target=targets[index%targets.size()];
        check(f,true);
        f.target=elsewhere;check(f,true);
        f.target=0x12345678u;check(f,true);
    }
    for(const auto index:{0u,999u,0x8c7u,0xfffff8c8u}) {
        Fixture f;f.eax=index;f.index=0u;f.observer=true;
        check(f,true);
    }
    for(const auto index:{0x8c8u,0xfffff8c7u,0xabc00000u}) {
        Fixture f;f.eax=index;
        check(f,false);
        f.changes_target=true;check(f,false);
        f.fault_pc=0x30aaa0u;check(f,false);
    }
    for(const auto pc:{0u,0x30aaa0u,0x30aaa7u}) {
        Fixture f;f.ordinary=false;f.fault_pc=pc;f.changes_target=true;
        check(f,false);
    }
    for(const auto eax:{0u,107u})for(const auto index:{0u,31u}) {
        Fixture sparse;sparse.ordinary=false;sparse.sparse_index=sparse.sparse_target=true;
        sparse.eax=eax;sparse.index=std::uint8_t(index);
        check(sparse,true);
        sparse.observer=true;check(sparse,true);
        sparse.target=elsewhere;check(sparse,true);
        sparse.target=0x12345678u;check(sparse,true);
        sparse.sparse_target=false;check(sparse,false);
        sparse.fault_pc=0x30aaa7u;check(sparse,false);
        sparse.sparse_index=false;sparse.sparse_target=true;sparse.fault_pc=0x30aaa0u;check(sparse,false);
        sparse.fault_pc=0;sparse.changes_target=true;check(sparse,false);
    }
    Fixture sparse;sparse.ordinary=false;sparse.sparse_index=sparse.sparse_target=true;
    sparse.eax=108u;check(sparse,false);
    sparse.eax=0u;sparse.index=32u;check(sparse,false);
    sparse.index=0u;sparse.trace=true;check(sparse,false);
    sparse.trace=false;sparse.authentic=false;check(sparse,false);
    Fixture f;f.trace=true;check(f,false);
    f.trace=false;f.authentic=false;check(f,false);

    // The byte and dword tables overlap for large byte values. The raw
    // dword must contain the authored byte even when it aliases the input.
    Fixture alias;alias.index=40u;alias.eax=target_table+40u*4u-index_table;
    check(alias,true);
    return compared;
}
}

int main() try {
    std::cout<<"30AAA0 dispatcher tables: "<<prove()<<" exact continuation and provider comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}
