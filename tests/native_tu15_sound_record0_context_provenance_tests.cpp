#include "halo2/native_tu15_sound_record0_context.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>
namespace sc=halo2::tu15_status_context;
namespace g=halo2::tu15_sound_gain_fanout;
using W=g::Word24;
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F action){try{action();}catch(const std::invalid_argument&){return;}throw std::runtime_error("Missing rejection");}
constexpr std::uint64_t mask56=(std::uint64_t{1}<<56)-1;
std::int64_t signed24(std::uint32_t value){return value<0x800000?value:std::int64_t(value)-0x1000000;}
std::uint32_t conditions(std::int64_t value){
    const auto raw=std::uint64_t(value)&mask56;
    bool equal=true;const bool integer_sign=(raw&(std::uint64_t{1}<<47))!=0;
    for(unsigned bit=48;bit<56;bit++)equal&=((raw&(std::uint64_t{1}<<bit))!=0)==integer_sign;
    std::uint32_t result=equal?0:0x20;
    if(bool(raw&(std::uint64_t{1}<<47))==bool(raw&(std::uint64_t{1}<<46)))result|=0x10;
    if(value<0)result|=8;
    if(value==0)result|=4;
    return result;
}
void origin_write(sc::Status const& old,sc::Status const& now,unsigned bit,bool witness,sc::Producer producer,std::uint32_t pc){
    const auto mask=1u<<bit;
    if(witness&&(!(old.known_mask()&mask)||!(old.value()&mask))){
        check(now.origins()[bit].producer==producer&&now.origins()[bit].source_pc==pc&&now.origins()[bit].evidence==0,"Missing actual witness origin");
    }else check(now.origins()[bit]==old.origins()[bit],"Inherited sticky origin overwritten");
}
int main(){try{
    std::uint64_t transfer_cases=0,arithmetic_cases=0,full_calls=0,guard_cases=0;
    std::set<std::int64_t> values{0,1,-1,(std::int64_t{1}<<55)-1,-(std::int64_t{1}<<55)};
    for(unsigned bit=0;bit<55;bit++)for(int sign:{-1,1}){
        const auto value=sign*(std::int64_t{1}<<bit);
        for(int delta:{-1,0,1})if(value+delta>=-(std::int64_t{1}<<55)&&value+delta<(std::int64_t{1}<<55))values.insert(value+delta);
    }
    for(unsigned bits=0;bits<16;bits++)for(std::int64_t low:{std::int64_t{0},std::int64_t{1},(std::int64_t{1}<<23)-1,std::int64_t{1}<<23}){
        const auto value=(std::int64_t(bits)<<44)|low;values.insert(value);values.insert(-value);
    }
    std::uint64_t seed=0x1a2b3c4d5e6f7;
    for(unsigned i=0;i<1024;i++){
        seed=seed*6364136223846793005ULL+1442695040888963407ULL;
        const auto raw=seed&mask56;
        values.insert(raw&(std::uint64_t{1}<<55)?std::int64_t(raw)-std::int64_t(std::uint64_t{1}<<56):std::int64_t(raw));
    }
    sc::Trace trace;
    for(const auto value:values)for(unsigned ccr=0;ccr<256;ccr++)for(unsigned knowledge:{0u,1u,0x3fu,0xffu}){
        const auto incoming=0xe00300u|ccr;
        auto status=sc::Status::incoming(incoming,sc::record0_clear_modes|0xffff00u|knowledge,0x927);
        const auto old=status;trace.count=0;
        const auto result=sc::Operations::transfer(status,trace,value,41);
        const auto raw=std::uint64_t(value)&mask56;
        const bool growth=bool(raw&(std::uint64_t{1}<<46))!=bool(raw&(std::uint64_t{1}<<45));
        const bool limited=value<-(std::int64_t{0x800000}<<24)||value>=(std::int64_t{0x800000}<<24);
        const auto expected_word=value<-(std::int64_t{0x800000}<<24)?0x800000u:value>=(std::int64_t{0x800000}<<24)?0x7fffffu:std::uint32_t(raw>>24)&0xffffffu;
        const auto writes=(growth?0x80u:0u)|(limited?0x40u:0u);
        check(result.bits==expected_word,"Independent literal transfer mismatch");
        check(status.value()==(old.value()|writes)&&status.known_mask()==(old.known_mask()|writes),"Transfer sticky knowledge mismatch");
        check(trace.count==1&&trace.steps[0].written_mask==writes&&trace.steps[0].limited==limited,"Transfer trace mismatch");
        origin_write(old,status,7,growth,sc::Producer::bus_transfer,0x190);
        origin_write(old,status,6,limited,sc::Producer::bus_transfer,0x190);
        for(unsigned bit=0;bit<24;bit++)if(bit!=6&&bit!=7)check(status.origins()[bit]==old.origins()[bit],"Transfer touched unrelated provenance");
        if((status.known_mask()&0xffu)!=0xffu){rejects([&]{status.numeric_value(0xff);});guard_cases++;}
        transfer_cases++;
    }
    const std::array<std::uint32_t,9> edge{0,1,0xffffff,0x7fffff,0x800000,0x400000,0xc00000,0x800001,0x7ffffe};
    for(auto input:edge)for(auto gain:edge)for(auto dry:edge)for(unsigned ccr=0;ccr<256;ccr++){
        auto status=sc::Status::incoming(0xc00300|ccr,sc::record0_clear_modes,0x921);const auto before=status;trace.count=0;
        const auto product=sc::Operations::multiply(status,trace,W(input),W(gain),13);
        const auto expected=2*signed24(input)*signed24(gain);
        check(product==expected,"Full product mismatch");
        check((status.value()&0x3e)==conditions(expected)&&(status.value()&1)==(before.value()&1),"MPY condition/carry mismatch");
        check(!(status.known_mask()&1)&&status.origins()[0]==before.origins()[0],"MPY invented carry provenance");
        const auto dry56=signed24(dry)*(std::int64_t{1}<<24);
        const auto result=sc::Operations::add_dry(status,trace,product,W(dry),13);
        const bool carry=((std::uint64_t(product)&mask56)+(std::uint64_t(dry56)&mask56))>mask56;
        check(result==expected+dry56&&(status.value()&0x3f)==(conditions(result)|(carry?1u:0u)),"ADD conditions mismatch");
        check((status.known_mask()&0x3f)==0x3f&&status.origins()[0].producer==sc::Producer::dry_add,"ADD carry not established");
        check((status.value()&0xffffc0)==(before.value()&0xffffc0),"Arithmetic altered inherited bits");
        for(unsigned bit=6;bit<24;bit++)check(status.origins()[bit]==before.origins()[bit],"Arithmetic altered inherited origins");
        arithmetic_cases++;
    }
    g::Controls controls{g::ScalingMode::none,false,false,false,g::AddressingMode::linear,g::AddressingMode::linear,g::AddressingMode::linear,g::AddressingMode::linear,g::AddressingMode::linear};
    g::Parameters parameters{};std::array<W,32> input{};
    std::array<std::array<W,32>,5> outputs{};
    std::array<g::Output,8> routes{outputs[0],outputs[1],outputs[2],outputs[3],outputs[4],outputs[4],outputs[4],outputs[4]};
    constexpr std::array<unsigned,8> inherited{8,9,12,18,19,21,22,23};
    for(unsigned high=0;high<256;high++)for(unsigned knowledge=0;knowledge<3;knowledge++)for(unsigned flags=0;flags<4;flags++){
        unsigned upper=0;for(unsigned i=0;i<8;i++)if(high&(1u<<i))upper|=1u<<inherited[i];
        const unsigned ccr=(high*13)&0xffu;
        unsigned known=sc::record0_clear_modes;
        if(knowledge==1)known|=0xff;
        if(knowledge==2)known|=(0xffff00u&~upper);
        auto status=sc::Status::incoming(upper|ccr,known,0x1a0+high);
        const auto entry=status;g::State state{W(flags),{}};
        for(unsigned frame=0;frame<2;frame++){
            const auto before=status;trace.count=800;
            halo2::tu15_record0_context::process(controls,parameters,state,input,routes,status,trace);
            const bool prefix=frame==0&&!(flags&1),dry=(flags&2)!=0;
            check(trace.count==(dry?786u:530u)+(prefix?1u:0u),"Full loop/trace count mismatch");
            const auto carry=(prefix||dry)?0u:(before.value()&1u);
            check(status.value()==((entry.value()&0xffffc0u)|0x14|carry),"Full zero path status mismatch");
            const auto carry_known=(prefix||dry)?1u:(before.known_mask()&1u);
            check(status.known_mask()==(entry.known_mask()|0x3e|carry_known),"Zero path invented sticky/carry knowledge");
            for(unsigned bit=8;bit<24;bit++)if(bit!=15)check(status.origins()[bit]==entry.origins()[bit],"Full path altered inherited upper provenance");
            check(status.origins()[15].producer==sc::Producer::loop_restore&&status.origins()[15].source_pc==0x191,"Outer LF restoration origin missing");
            check(!(status.value()&(1u<<15))&&!(status.value()&(1u<<16)),"Loop context leaked");
            origin_write(entry,status,7,false,sc::Producer::bus_transfer,0x190);origin_write(entry,status,6,false,sc::Producer::bus_transfer,0x190);
            check(state.flags.bits==(flags|1u)&&state.published_gain_source==g::GainSource::sequence_begin,"Typed state projection mismatch");
            full_calls++;
        }
    }
    std::cout<<"{\"passed\":true,\"literal_transfer_cases\":"<<transfer_cases<<",\"full_width_arithmetic_cases\":"<<arithmetic_cases<<",\"retained_full_calls\":"<<full_calls<<",\"numeric_unknown_rejections\":"<<guard_cases<<",\"hardware_context_proven\":false}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
