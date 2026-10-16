#include "halo2/native_tu15_sound_record0_context.hpp"
#include <iostream>
#include <stdexcept>
using namespace halo2;namespace g=tu15_sound_gain_fanout;namespace sc=tu15_status_context;using Word=g::Word24;
void check(bool x){if(!x)throw std::runtime_error("Record0 context guard failed");}
unsigned rejected=0,nominal=0;
template<class F>void reject(F action){try{action();}catch(const std::invalid_argument&){rejected++;return;}throw std::runtime_error("Missing context rejection");}
int main(){try{
 for(auto v:{0x1000000u,0xffffffffu}){reject([&]{sc::Status::incoming(v,0xffffff,1);});reject([&]{sc::Status::incoming(0,v,1);});}reject([]{sc::Status::incoming(0,0xffffff,0);});
 g::Controls c{g::ScalingMode::none,false,false,false,g::AddressingMode::linear,g::AddressingMode::linear,g::AddressingMode::linear,g::AddressingMode::linear,g::AddressingMode::linear};
 g::Parameters p{};g::State s{Word(),{}};std::array<Word,32> input{};std::array<std::array<Word,32>,5> outputs{};std::array<g::Output,8> routes{outputs[0],outputs[1],outputs[2],outputs[3],outputs[4],outputs[4],outputs[4],outputs[4]};sc::Trace trace;trace.count=7;
 auto guard=[&](sc::Status& context){const auto old=context;const auto before_s=s;const auto before_p=p;const auto before_i=input;const auto before_o=outputs;const auto before_t=trace;reject([&]{tu15_record0_context::process(c,p,s,input,routes,context,trace);});check(context==old&&s==before_s&&p==before_p&&input==before_i&&outputs==before_o&&trace==before_t);};
 for(unsigned bit=0;bit<24;bit++)if(sc::record0_clear_modes&(1u<<bit)){
  auto missing=sc::Status::incoming(0,0xffffffu&~(1u<<bit),9);guard(missing);auto set=sc::Status::incoming(1u<<bit,0xffffff,9);guard(set);
 }
 for(auto bit:{12u,18u}){auto x=sc::Status::incoming(1u<<bit,0xffffff,9);guard(x);}
 auto context=sc::Status::incoming(0,sc::record0_clear_modes,9);
 for(unsigned i=0;i<32;i++){input[i].bits=0x1000000;guard(context);input[i]=Word();}
 for(unsigned i=0;i<8;i++){p.gains[i].bits=0x1000000;guard(context);p.gains[i]=Word();}
 for(unsigned j=0;j<5;j++)for(unsigned i=0;i<32;i++){outputs[j][i].bits=0x1000000;guard(context);outputs[j][i]=Word();}
 s.flags.bits=0x1000000;guard(context);s.flags=Word();s.published_gain_source=static_cast<g::GainSource>(42);guard(context);s.published_gain_source.reset();
 for(auto value:{g::ScalingMode::down,g::ScalingMode::up,g::ScalingMode::reserved}){c.entry_scaling=value;guard(context);}c.entry_scaling=g::ScalingMode::none;
 for(auto member:{&g::Controls::arithmetic_saturation,&g::Controls::sixteen_bit_arithmetic,&g::Controls::double_precision_multiply}){c.*member=true;guard(context);c.*member=false;}
 for(auto member:{&g::Controls::input,&g::Controls::destination_table,&g::Controls::gain_sequence,&g::Controls::output,&g::Controls::state_relative}){c.*member=g::AddressingMode::modulo;guard(context);c.*member=g::AddressingMode::linear;}
 auto old_routes=routes;routes[0]=input;guard(context);routes=old_routes;
 std::array<Word,64> shared{};routes[0]=std::span<Word,32>(shared.data(),32);routes[1]=std::span<Word,32>(shared.data()+1,32);guard(context);routes=old_routes;
 tu15_record0_context::process(c,p,s,input,routes,context,trace);check(trace.count==531&&s.flags.bits==1&&(context.known_mask()&0x3f)==0x3f&&context.numeric_value(0x3f)==0x14);nominal++;
 const auto unresolved=context;reject([&]{context.numeric_value(0xffffff);});reject([&]{context.numeric_value(0xc0);});reject([&]{context.numeric_value(0x1000000);});check(context==unresolved);
 auto warm=sc::Status::incoming(1,sc::record0_clear_modes,99);tu15_record0_context::process(c,p,s,input,routes,warm,trace);check(trace.count==530&&!(warm.known_mask()&1)&&warm.value()&1&&warm.origins()[0].producer==sc::Producer::incoming_unresolved);reject([&]{warm.numeric_value(1);});nominal++;
 auto sticky=sc::Status::incoming(0xc0,sc::record0_clear_modes|0xc0,77);const auto S=sticky.origins()[7],L=sticky.origins()[6];tu15_record0_context::process(c,p,s,input,routes,sticky,trace);check(sticky.numeric_value(0xc0)==0xc0&&sticky.origins()[7]==S&&sticky.origins()[6]==L);nominal++;
 input.fill(Word(0x400000));p.gains.fill(Word(0x400000));auto growth=sc::Status::incoming(0,sc::record0_clear_modes,12);tu15_record0_context::process(c,p,s,input,routes,growth,trace);check(growth.numeric_value(0x80)==0x80&&!(growth.known_mask()&0x40)&&growth.origins()[7].producer==sc::Producer::bus_transfer);nominal++;
 input.fill(Word(0x800000));p.gains.fill(Word(0x800000));auto limit=sc::Status::incoming(0,sc::record0_clear_modes,13);tu15_record0_context::process(c,p,s,input,routes,limit,trace);check(limit.numeric_value(0x40)==0x40&&!(limit.known_mask()&0x80)&&limit.origins()[6].producer==sc::Producer::bus_transfer);nominal++;
 s.flags=Word(2);auto mixed=sc::Status::incoming(0,sc::record0_clear_modes,14);tu15_record0_context::process(c,p,s,input,routes,mixed,trace);check(trace.count==787&&(mixed.known_mask()&1));nominal++;
 sc::Trace primitive;auto q=sc::Status::incoming(0,sc::record0_clear_modes,16);const auto saved=q;
 Word bad;bad.bits=0x1000000;reject([&]{sc::Operations::multiply(q,primitive,bad,Word(),0);});reject([&]{sc::Operations::add_dry(q,primitive,std::int64_t{1}<<48,Word(),0);});reject([&]{sc::Operations::transfer(q,primitive,std::int64_t{1}<<55,0);});check(q==saved&&primitive.count==0);
 primitive.count=800;reject([&]{sc::Operations::multiply(q,primitive,Word(),Word(),0);});check(q==saved&&primitive.count==800);
 std::cout<<"{\"passed\":true,\"no_mutation_rejections\":"<<rejected<<",\"provenance_nominal_controls\":"<<nominal<<",\"full_status_or_reset_proven\":false}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
