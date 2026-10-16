#include "halo2/native_tu15_sound_controller.hpp"
#include "halo2/native_tu15_sound_graph_storage.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <type_traits>
namespace ctl=halo2::tu15_sound_controller;namespace ar=halo2::tu15_sound_controller_arithmetic;
using Word=ctl::Word24;
unsigned rejects{},nominals{};
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
struct Snapshot {
    std::array<Word,640> words;std::uint32_t cursor,count;bool constructed;
    explicit Snapshot(const ctl::Sink& s):cursor(s.byte_cursor()),count(s.last_copied_count()),constructed(s.descriptor_constructed()){std::copy(s.words().begin(),s.words().end(),words.begin());}
    bool equal(const ctl::Sink& s)const{return cursor==s.byte_cursor()&&count==s.last_copied_count()&&constructed==s.descriptor_constructed()&&std::equal(words.begin(),words.end(),s.words().begin());}
};
template<class Function>void reject_sink(ctl::Sink& sink,Function action){const Snapshot before(sink);bool threw=false;try{action();}catch(const std::exception&){threw=true;}check(threw&&before.equal(sink),"sink rejection mutated owned values");rejects++;}
struct FixtureGate:ctl::Gate {unsigned calls{};bool value=true;bool can_copy()noexcept override{calls++;return value;}};
struct FixtureEndpoint:ctl::PacketEndpoint {unsigned calls{};std::uint32_t result=0x80070005u;std::uint32_t submit(unsigned,std::span<const std::uint16_t,320>)noexcept override{calls++;return result;}};
template<class Function>void reject_consumer(ctl::Consumer& consumer,FixtureGate& gate,FixtureEndpoint& endpoint,Function action){
    const auto reader=consumer.reader_byte_cursor();const auto last=consumer.last_actual_submission_result();const auto gate_calls=gate.calls,endpoint_calls=endpoint.calls;
    std::array<std::array<std::uint16_t,320>,2> words;for(unsigned p=0;p<2;p++)std::copy(consumer.packet(p).begin(),consumer.packet(p).end(),words[p].begin());
    bool threw=false;try{action();}catch(const std::exception&){threw=true;}check(threw&&reader==consumer.reader_byte_cursor()&&last==consumer.last_actual_submission_result()&&gate_calls==gate.calls&&endpoint_calls==endpoint.calls,"consumer rejection mutated state or called endpoint");
    for(unsigned p=0;p<2;p++)check(std::equal(words[p].begin(),words[p].end(),consumer.packet(p).begin()),"consumer rejection changed packet");rejects++;
}
int main(){try{
    static_assert(!std::is_copy_constructible_v<ctl::Sink>&&!std::is_move_constructible_v<ctl::Sink>);
    static_assert(!std::is_copy_constructible_v<ctl::Consumer>&&!std::is_move_constructible_v<ctl::Consumer>);
    std::array<Word,640> values{};for(unsigned n=0;n<640;n++)values[n]=Word((n*7951u)&0xffffffu);
    ctl::Sink sink({values,0x9fc,7,true});std::array<Word,33> prefix{};
    for(std::uint32_t cursor=0;cursor<4096;cursor++)if(cursor>=2560||(cursor&3u))reject_sink(sink,[&]{sink.restore({values,cursor,7,true});});
    for(auto invalid:{0x1000000u,0xffffffffu})for(unsigned index=0;index<640;index++){
        auto corrupt=values;corrupt[index].bits=invalid;reject_sink(sink,[&]{sink.restore({corrupt,0,0,false});});
    }
    for(unsigned count=1;count<=32;count++)reject_sink(sink,[&]{sink.restore({values,0,count,false});});
    for(auto count:{33u,0xffffffffu})reject_sink(sink,[&]{sink.restore({values,0,count,true});});
    reject_sink(sink,[&]{sink.append(prefix,false);});
    for(unsigned index=0;index<32;index++){
        prefix[index].bits=0x1000000;reject_sink(sink,[&]{sink.append(std::span<const Word>(prefix.data(),32),false);});prefix[index]=Word();
    }
    for(unsigned count=1;count<=32;count++)reject_sink(sink,[&]{sink.append(sink.words().first(count),false);});
    ctl::Sink unbound;reject_sink(unbound,[&]{unbound.append(std::span<const Word>(),false);});
    ar::Parameters parameters=ar::authored_parameters();ar::State state{};std::array<Word,32> storage{};
    const ar::Controls linear{Word(),ar::AddressingMode::linear,ar::AddressingMode::linear,ar::AddressingMode::linear};
    auto reject_producer=[&](const ar::Controls& controls,ctl::Sink& target){
        const Snapshot before(target);const auto before_state=state;const auto before_parameters=parameters;const auto before_storage=storage;
        bool threw=false;try{ctl::process(controls,parameters,state,storage,target);}catch(const std::exception&){threw=true;}
        check(threw&&before.equal(target)&&state==before_state&&parameters==before_parameters&&storage==before_storage,"producer rejection mutated owners");rejects++;
    };
    state.flags=Word(1);reject_producer(linear,unbound);state.flags=Word();
    for(unsigned index=0;index<32;index++){storage[index].bits=0x1000000;reject_producer(linear,sink);storage[index]=Word();}
    for(unsigned bit=10;bit<24;bit++){auto controls=linear;controls.entry_status.bits=1u<<bit;reject_producer(controls,sink);}
    for(unsigned mode=1;mode<=3;mode++)for(unsigned field=0;field<3;field++){
        auto controls=linear;auto value=static_cast<ar::AddressingMode>(mode);
        if(field==0)controls.storage_read=value;else if(field==1)controls.storage_write=value;else controls.state_relative=value;reject_producer(controls,sink);
    }
    for(auto invalid:{0x1000000u,0xffffffffu}){
        state.flags.bits=invalid;reject_producer(linear,sink);state.flags=Word();
        state.counter.bits=invalid;reject_producer(linear,sink);state.counter=Word();
        parameters.threshold.bits=invalid;reject_producer(linear,sink);parameters=ar::authored_parameters();
        parameters.gain.bits=invalid;reject_producer(linear,sink);parameters=ar::authored_parameters();
        auto controls=linear;controls.entry_status.bits=invalid;reject_producer(controls,sink);
    }
    const Snapshot alias_before(sink);const auto state_before=state;
    bool threw=false;try{ctl::process(linear,parameters,state,sink.words().first<32>(),sink);}catch(const std::exception&){threw=true;}
    check(threw&&alias_before.equal(sink)&&state==state_before,"producer sink alias rejection");rejects++;
    FixtureGate gate;FixtureEndpoint endpoint;ctl::ConsumerSeed consumer_seed{};consumer_seed.reader_byte_cursor=0;for(auto& packet:consumer_seed.packets)packet.fill(0xcafe);
    ctl::Consumer consumer(0,gate,&endpoint,consumer_seed);
    for(auto index:{2u,3u,640u,0xffffffffu})reject_consumer(consumer,gate,endpoint,[&]{consumer.callback(sink,index);});
    for(unsigned index=0;index<640;index++){
        const auto saved=sink.words()[index];sink.words()[index].bits=0x1000000;
        reject_consumer(consumer,gate,endpoint,[&]{consumer.callback(sink,0);});reject_consumer(consumer,gate,endpoint,[&]{consumer.reset(sink);});sink.words()[index]=saved;
    }
    for(auto port:{4u,5u,0xffffffffu}){
        bool rejected=false;try{ctl::Consumer other(port,gate,&endpoint,consumer_seed);}catch(const std::exception&){rejected=true;}check(rejected&&!gate.calls&&!endpoint.calls,"bad consumer port guard");rejects++;
    }
    for(auto cursor:{1u,2u,3u,2560u,2564u,0xffffffffu}){
        auto initial=consumer_seed;initial.reader_byte_cursor=cursor;bool rejected=false;try{ctl::Consumer other(0,gate,&endpoint,initial);}catch(const std::exception&){rejected=true;}check(rejected&&!gate.calls&&!endpoint.calls,"bad consumer initial cursor guard");rejects++;
    }
    ctl::Consumer null_consumer(0,gate,nullptr,consumer_seed);reject_consumer(null_consumer,gate,endpoint,[&]{null_consumer.callback(sink,0);});
    auto graph=halo2::tu15_sound_graph_storage::GraphStorage::fresh_literal();
    for(auto port:{4u,5u,0xffffffffu}){bool rejected=false;try{ctl::process(linear,*graph,port);}catch(const std::exception&){rejected=true;}check(rejected,"graph producer bad port");rejects++;}
    // Nominal methods perform actual owner writes, including zero-count setup.
    auto zero=unbound.append(std::span<const Word>(),true);check(!zero.copied_words&&zero.descriptor_constructed_this_call&&unbound.descriptor_constructed()&&!unbound.byte_cursor(),"zero-count constructed owner");nominals++;
    const Snapshot same_before(sink);sink.restore({sink.words(),sink.byte_cursor(),sink.last_copied_count(),sink.descriptor_constructed()});check(same_before.equal(sink),"same-owner seed restore");nominals++;
    prefix[0]=Word(0x7fffff);prefix[1]=Word(0x800000);auto copied=sink.append(std::span<const Word>(prefix.data(),2),false);
    check(copied.copied_words==2&&copied.first_chunk_words==1&&copied.second_chunk_words==1&&sink.byte_cursor()==4&&sink.words()[639]==prefix[0]&&sink.words()[0]==prefix[1],"real split copy");nominals++;
    // Original callback ignores a failing submit return for its copy/cursor;
    // the native observer retains the actual return without claiming success.
    const auto callback=consumer.callback(sink,1);check(callback.copied_samples==320&&callback.actual_submission_result==0x80070005u&&consumer.last_actual_submission_result()==0x80070005u&&endpoint.calls==1,"actual callback error retained");nominals++;
    endpoint.result=1;auto reset=consumer.reset(sink);check(!reset.returned_bits&&reset.submitted_packets==2&&consumer.last_actual_submission_result()==1,"nonnegative actual reset result");nominals++;
    const auto null_reset=null_consumer.reset(sink);check(!null_reset.returned_bits&&!null_reset.submitted_packets&&!null_consumer.last_actual_submission_result(),"null reset no phantom receipt");nominals++;
    gate.value=false;const auto previous_last=consumer.last_actual_submission_result();const auto previous_reader=consumer.reader_byte_cursor();const auto previous_endpoint=endpoint.calls;auto skipped=consumer.callback(sink,0);
    check(!skipped.copied_samples&&!skipped.actual_submission_result&&previous_last==consumer.last_actual_submission_result()&&previous_reader==consumer.reader_byte_cursor()&&previous_endpoint==endpoint.calls,"false gate retains prior real error state");nominals++;
    std::cout<<"{\"passed\":true,\"rejections_before_mutation\":"<<rejects<<",\"nominal_controls\":"<<nominals<<",\"physical_hardware_or_endpoint_receipt_proven\":false}\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
