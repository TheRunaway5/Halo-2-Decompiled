#include "halo2/native_tu15_sound_status_context.hpp"
#include <algorithm>
#include <stdexcept>
namespace halo2::tu15_status_context {
namespace {
constexpr std::uint64_t mask56=(std::uint64_t{1}<<56)-1;
std::int64_t signed24(Word24 w){return w.bits&0x800000u?std::int64_t(w.bits)-0x1000000:std::int64_t(w.bits);}
std::int64_t floor24(std::int64_t x){constexpr auto unit=std::int64_t{1}<<24;return x>=0?x/unit:-((-x+unit-1)/unit);}
Origin source(Producer p,std::uint32_t pc,std::uint32_t n){return{p,0,pc,n};}
void preflight(const Status& s,const Trace& t){
    constexpr auto modes=record0_clear_modes&~(1u<<15);
    if((s.known_mask()&modes)!=modes||(s.value()&modes)||(s.value()&s.known_mask()&((1u<<12)|(1u<<18)))||t.count>=t.steps.size())throw std::invalid_argument("Unsupported primitive context or trace capacity");
}
void valid(Word24 word){if(word.bits>0xffffffu)throw std::invalid_argument("Primitive word exceeds24bits");}
}
Status::Status(std::uint32_t value,std::uint32_t known,std::uint64_t evidence):value_(value),known_(known){
    if(value>0xffffffu||known>0xffffffu||!evidence)throw std::invalid_argument("Status needs explicit 24-bit incoming provenance");
    for(unsigned bit=0;bit<24;bit++)origins_[bit]={known&(1u<<bit)?Producer::incoming_known:Producer::incoming_unresolved,evidence,0,0};
}
Status Status::incoming(std::uint32_t v,std::uint32_t k,std::uint64_t e){return Status(v,k,e);}
void Status::require_record0_profile()const{
    if((known_&record0_clear_modes)!=record0_clear_modes||(value_&record0_clear_modes))throw std::invalid_argument("Record0 mode-sensitive bits must be established clear");
    if(value_&known_&((1u<<12)|(1u<<18)))throw std::invalid_argument("Known reserved status bits cannot be set");
}
std::uint32_t Status::numeric_value(std::uint32_t required)const{
    if(required>0xffffffu||(known_&required)!=required)throw std::invalid_argument("Numeric status read contains unresolved bits");
    return value_&required;
}
void Status::write(std::uint32_t mask,std::uint32_t bits,const Origin& origin){
    value_=(value_&~mask)|(bits&mask);known_|=mask;
    for(unsigned bit=0;bit<24;bit++)if(mask&(1u<<bit))origins_[bit]=origin;
}
void Status::sticky(unsigned bit,bool witness,const Origin& origin){
    const auto mask=1u<<bit;
    if(witness&&(!((known_&mask)&&(value_&mask))))write(mask,mask,origin);
}
void Operations::record(Trace& t,const Status& s,std::uint32_t v,std::uint32_t k,Producer op,std::uint32_t pc,std::uint32_t n,std::uint32_t mask,std::int64_t a,std::uint32_t out,bool limited){
    // The record0 caller preflights fixed trace capacity and emits <=787 steps.
    t.steps[t.count++]={op,pc,n,mask,v,k,s.value_,s.known_,a,out,limited};
}
void Operations::cold_flag_bset(Status& s,Trace& t){
    preflight(s,t);
    const auto v=s.value_,k=s.known_;s.write(1,0,source(Producer::cold_flag_bset,0x176,0));record(t,s,v,k,Producer::cold_flag_bset,0x176,0,1);
}
void Operations::loop_flag(Status& s,Trace& t,bool value,bool restoring,std::uint32_t pc,std::uint32_t n){
    preflight(s,t);
    const auto v=s.value_,k=s.known_;const auto p=restoring?Producer::loop_restore:Producer::loop_setup;s.write(1u<<15,value?1u<<15:0,source(p,pc,n));record(t,s,v,k,p,pc,n,1u<<15);
}
void Operations::arithmetic_flags(Status& s,std::int64_t value,const Origin& origin,bool writes_carry,bool carry){
    const auto raw=static_cast<std::uint64_t>(value)&mask56;std::uint32_t flags=0;
    const auto top=raw>>47;if(top!=0&&top!=0x1ff)flags|=0x20;
    if(((raw>>47)&1)==((raw>>46)&1))flags|=0x10;
    if(value<0)flags|=8;
    if(value==0)flags|=4;
    // Every admitted record0 product+one signed24 dry word fits signed49;
    // hence no56-bit arithmetic overflow: V is known clear, L stays sticky.
    if(writes_carry&&carry)flags|=1;
    s.write(writes_carry?0x3fu:0x3eu,flags,origin);
}
std::int64_t Operations::multiply(Status& s,Trace& t,Word24 input,Word24 gain,std::uint32_t n){
    preflight(s,t);valid(input);valid(gain);
    const auto v=s.value_,k=s.known_;const auto result=2*signed24(input)*signed24(gain);
    arithmetic_flags(s,result,source(Producer::multiply,0x18a,n),false,false);record(t,s,v,k,Producer::multiply,0x18a,n,0x3e,result);return result;
}
std::int64_t Operations::add_dry(Status& s,Trace& t,std::int64_t a,Word24 dry,std::uint32_t n){
    preflight(s,t);valid(dry);
    if(a<-(std::int64_t{1}<<47)||a>(std::int64_t{1}<<47))throw std::invalid_argument("Dry ADD requires the admitted full product");
    const auto v=s.value_,k=s.known_;const auto b=signed24(dry)*(std::int64_t{1}<<24),result=a+b;
    const auto unsigned_sum=(static_cast<std::uint64_t>(a)&mask56)+(static_cast<std::uint64_t>(b)&mask56);
    arithmetic_flags(s,result,source(Producer::dry_add,0x18f,n),true,(unsigned_sum>>56)!=0);record(t,s,v,k,Producer::dry_add,0x18f,n,0x3f,result);return result;
}
Word24 Operations::transfer(Status& s,Trace& t,std::int64_t a,std::uint32_t n){
    preflight(s,t);
    if(a<-(std::int64_t{1}<<55)||a>((std::int64_t{1}<<55)-1))throw std::invalid_argument("Transfer accumulator exceeds signed56");
    const auto v=s.value_,k=s.known_;const auto raw=static_cast<std::uint64_t>(a)&mask56;const bool growth=((raw>>46)^(raw>>45))&1;
    const auto shifted=floor24(a);const bool limited=shifted>0x7fffff||shifted< -0x800000;
    const auto result=Word24(static_cast<std::uint32_t>(std::clamp(shifted,std::int64_t{-0x800000},std::int64_t{0x7fffff}))&0xffffffu);
    const auto origin=source(Producer::bus_transfer,0x190,n);s.sticky(7,growth,origin);s.sticky(6,limited,origin);record(t,s,v,k,Producer::bus_transfer,0x190,n,(growth?0x80u:0u)|(limited?0x40u:0u),a,result.bits,limited);return result;
}
}
