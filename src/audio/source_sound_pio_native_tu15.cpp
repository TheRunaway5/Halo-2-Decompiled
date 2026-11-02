#include "halo2/source_sound_pio_native_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
namespace halo2 {namespace {
class Owner {
    SourcePhysicsContactMemory& memory;
    const SourcePhysicsWorldRuntimeServices& services;
    const SourcePhysicsContactRecordObserver& observer;
    std::array<std::uint32_t,8> g;
    std::uint32_t flags;
    void at(std::uint32_t pc)const{if(services.instruction)services.instruction(pc);}
    std::uint32_t read(std::uint32_t address)const{
        std::array<std::byte,4> bytes;memory.read(address,bytes);std::uint32_t value{};
        for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
    }
    static std::vector<std::byte> bytes(std::uint32_t value){std::vector<std::byte> out(4);for(unsigned i=0;i<4;++i)out[i]=std::byte(value>>(8*i));return out;}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        auto data=bytes(value);SourcePhysicsContactRecordStore event{pc,address,std::vector<std::byte>(4),data};
        memory.read(address,event.before);memory.write(address,data);if(observer)observer(event);
    }
    void action(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        auto* owner=dynamic_cast<SourceSoundPioActionMemory*>(&memory);
        if(!owner)throw std::runtime_error("Native sound PIO requires an actual action owner");
        auto data=bytes(value);owner->write_pio_action(pc,address,data);
        if(observer)observer({pc,address,{},data});
    }
    // Defined arithmetic flags follow the integer operation. The recovered
    // reference profile clears undefined logic AF; unrelated flags, including
    // DF, remain the caller's. No host x87/SIMD environment is loaded/changed.
    void arithmetic(std::uint32_t value,bool carry,bool overflow,bool auxiliary=false){
        constexpr std::uint32_t mask=0x8d5u;
        const auto parity=std::popcount(value&255u)%2u==0;
        flags=(flags&~mask)|(carry?1u:0u)|(parity?4u:0u)|(auxiliary?16u:0u)|(value==0?0x40u:0u)|(value&0x80000000u?0x80u:0u)|(overflow?0x800u:0u);
    }
    void logic(std::uint32_t value){arithmetic(value,false,false);}
    void compare(std::uint32_t left,std::uint32_t right){const auto value=left-right;arithmetic(value,left<right,((left^right)&(left^value)&0x80000000u)!=0,((left^right^value)&16u)!=0);}
    void advance(std::uint32_t& value,std::uint32_t step,bool preserve_carry=false){const auto old=value;value+=step;const auto carry=(flags&1u)!=0;
        arithmetic(value,preserve_carry?carry:value<old,((~(old^step))&(old^value)&0x80000000u)!=0,((old^step^value)&16u)!=0);}
    void shift_byte(std::uint32_t& value){const auto old=value;value<<=8u;arithmetic(value,(old&(1u<<24u))!=0,false);}
    void push(std::uint32_t pc,std::uint32_t value){g[4]-=4u;store(pc,g[4],value);}
    std::uint32_t pop(){const auto value=read(g[4]);g[4]+=4u;return value;}
public:
    Owner(SourcePhysicsContactMemory& m,const SourcePhysicsWorldRuntimeAbi& abi,
        const SourcePhysicsWorldRuntimeServices& s,const SourcePhysicsContactRecordObserver& o)
        :memory(m),services(s),observer(o),g(abi.gp),flags(abi.eflags){}
    void save(SourcePhysicsWorldRuntimeAbi& abi)const{abi.gp=g;abi.eflags=flags;}
    void prefix(){
        // Poll the actual source queue, then initialize all256 voice rows.
        for(;;){at(0x38e0ce);g[0]=read(0xfe820010);at(0x38e0d3);g[0]&=0xfffffffcu;logic(g[0]);
            at(0x38e0d6);compare(g[0],32);at(0x38e0d9);if(g[0]>=32)break;}
        at(0x38e0db);g[1]=0;logic(0);at(0x38e0dd);g[0]=0;logic(0);
        at(0x38e0df);push(0x38e0df,g[6]);
        for(;;){
            // The original reloads this mutable table for every row.
            at(0x38e0e0);g[6]=read(0x391390);at(0x38e0e6);g[2]=g[1];
            at(0x38e0e8);g[2]&=0xffffu;logic(g[2]);
            at(0x38e0ee);store(0x38e0ee,g[6]+g[0]+0x7cu,g[2]);
            at(0x38e0f2);advance(g[0],128);at(0x38e0f7);advance(g[1],1,true);
            at(0x38e0f8);compare(g[0],0x8000);at(0x38e0fd);if(g[0]>=0x8000)break;
        }
        // Five table resources and the notifier resource are published in
        // their authored order. Each pointer is read at its own source site.
        constexpr std::array<std::array<std::uint32_t,4>,6> tables{{
            {0x38e0ff,0x391394,0x38e104,0xfe80202c},{0x38e109,0x3913d4,0x38e10e,0xfe802038},
            {0x38e113,0x3913e4,0x38e118,0xfe80203c},{0x38e11d,0x3913b4,0x38e122,0xfe802030},
            {0x38e127,0x3913c4,0x38e12c,0xfe802034},{0x38e131,0x3913a4,0x38e136,0xfe80115c}}};
        for(const auto& row:tables){at(row[0]);g[0]=read(row[1]);at(row[2]);action(row[2],row[3],g[0]);}
        at(0x38e13b);g[0]=0xfff;
        constexpr std::array<std::array<std::uint32_t,2>,5> tracking{{
            {0x38e140,0xfe8202a4},{0x38e145,0xfe8202b0},{0x38e14a,0xfe8202a0},{0x38e14f,0xfe8202a8},{0x38e154,0xfe8202ac}}};
        for(const auto& row:tracking){at(row[0]);action(row[0],row[1],g[0]);}
    }
    void suffix(){
        at(0x38e159);action(0x38e159,0xfe820804,0x7ff);
        at(0x38e163);g[0]=read(0x391414);at(0x38e168);action(0x38e168,0xfe820808,g[0]);
        // Pack the four currently selected HRTF routing bytes. Loads and
        // cached operands preserve the source's mutable-read ordering.
        at(0x38e16d);g[0]=read(0x38f988);at(0x38e172);g[2]=read(g[0]+24u);
        at(0x38e175);g[6]=read(g[0]+16u);at(0x38e178);push(0x38e178,31);
        at(0x38e17a);g[1]=pop();at(0x38e17b);g[2]&=g[1];logic(g[2]);
        at(0x38e17d);shift_byte(g[2]);at(0x38e180);g[6]&=g[1];logic(g[6]);
        at(0x38e182);g[2]|=g[6];logic(g[2]);at(0x38e184);g[6]=read(g[0]+8u);
        at(0x38e187);g[0]=read(g[0]);at(0x38e189);shift_byte(g[2]);
        at(0x38e18c);g[6]&=g[1];logic(g[6]);at(0x38e18e);g[2]|=g[6];logic(g[2]);
        at(0x38e190);g[0]&=g[1];logic(g[0]);at(0x38e192);shift_byte(g[2]);
        at(0x38e195);g[2]|=g[0];logic(g[2]);at(0x38e197);action(0x38e197,0xfe8202c0,g[2]);
        at(0x38e19d);g[0]=0;logic(0);at(0x38e19f);g[6]=pop();at(0x38e1a0);(void)pop();
    }
};
}
bool source_sound_pio_native_tu15_owns_entry(std::uint32_t entry){return entry==0x38e0ce||entry==0x38e159;}
void source_run_sound_pio_native_tu15(SourcePhysicsContactMemory& memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observer){
    if(!source_sound_pio_native_tu15_owns_entry(entry))throw std::invalid_argument("Unsupported native sound PIO entry");
    Owner owner(memory,abi,services,observer);
    try{if(entry==0x38e0ce)owner.prefix();owner.suffix();}catch(...){owner.save(abi);throw;}
    owner.save(abi);
}
}
