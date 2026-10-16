#include "halo2/native_tu15_sound_controller.hpp"
#include "halo2/native_tu15_sound_graph_storage.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
namespace ctl=halo2::tu15_sound_controller;
namespace ar=halo2::tu15_sound_controller_arithmetic;
using Word=ctl::Word24;using J=nlohmann::json;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class Span>void check_words(Span words,const J& expected,const char* label){check(words.size()==expected.size(),label);for(unsigned n=0;n<words.size();n++)check(words[n].bits==expected[n].template get<std::uint32_t>(),label);}
struct FixtureGate:ctl::Gate{bool value=true;unsigned calls{};bool can_copy()noexcept override{calls++;return value;}};
struct Submission {unsigned index;const std::uint16_t* owner;std::array<std::uint16_t,320> words;};
struct FixtureEndpoint:ctl::PacketEndpoint {
    std::vector<std::uint32_t> returns;std::vector<Submission> calls;
    std::uint32_t submit(unsigned index,std::span<const std::uint16_t,320> words)noexcept override{
        Submission call{};call.index=index;call.owner=words.data();std::copy(words.begin(),words.end(),call.words.begin());calls.push_back(call);
        return returns.at(calls.size()-1);
    }
};
ctl::ConsumerSeed seed(std::uint32_t reader){ctl::ConsumerSeed s{};s.reader_byte_cursor=reader;for(auto& packet:s.packets)packet.fill(0xcdcd);return s;}
void check_packets(const ctl::Consumer& consumer,const J& expected){for(unsigned p=0;p<2;p++){auto words=consumer.packet(p);for(unsigned n=0;n<320;n++)check(words[n]==expected[p][n].get<std::uint16_t>(),"retained CPU packet");}}
std::array<Word,640> word_array(const J& values){std::array<Word,640> result{};check(values.size()==640,"ring fixture extent");for(unsigned n=0;n<640;n++)result[n]=Word(values[n]);return result;}
int main(){try{
    J vectors;std::cin>>vectors;unsigned calls=0,emissions=0,cpu_calls=0,copy_cases=0,extension_cases=0;bool owners_above4G=true;
    for(const auto& scenario:vectors["producer_cases"]){
        auto graph=halo2::tu15_sound_graph_storage::GraphStorage::fresh_literal();
        FixtureGate gate;FixtureEndpoint endpoint;endpoint.returns.assign(100,0);endpoint.calls.reserve(100);
        std::array<std::unique_ptr<ctl::Consumer>,4> consumers;
        std::array<const void*,4> sink_addresses{},input_addresses{},state_addresses{};
        for(unsigned port=0;port<4;port++){
            consumers[port]=std::make_unique<ctl::Consumer>(port,gate,&endpoint,seed(0));
            sink_addresses[port]=&graph->controller_sink_owner(port);input_addresses[port]=graph->mixbin(27+port).data();state_addresses[port]=&graph->effects().controller_states[port];
            owners_above4G=owners_above4G&&reinterpret_cast<std::uintptr_t>(sink_addresses[port])>0xffffffffu&&reinterpret_cast<std::uintptr_t>(input_addresses[port])>0xffffffffu&&reinterpret_cast<std::uintptr_t>(state_addresses[port])>0xffffffffu;
        }
        for(const auto& row:scenario["calls"]){
            const unsigned port=row["port"];const auto& f=row["fixture"];const unsigned mask=f[0];auto& sink=graph->controller_sink_owner(port);
            auto& parameters=graph->effects().controller_parameters[port];auto& state=graph->effects().controller_states[port];auto storage=graph->mixbin(27+port);
            if(mask&1u)state.flags=Word(f[1]);if(mask&2u)parameters={Word(f[2]),Word(f[3])};if(mask&4u)state.counter=Word(f[4]);
            if(mask&8u)for(unsigned n=0;n<32;n++)storage[n]=Word(f[6+n]);
            if(mask&(64u|128u)){
                std::array<Word,640> retained{};std::copy(sink.words().begin(),sink.words().end(),retained.begin());
                if(mask&128u)retained.fill(Word(f[39].get<std::uint32_t>()*0x010101u));
                sink.restore({retained,mask&64u?f[38].get<std::uint32_t>():sink.byte_cursor(),sink.last_copied_count(),sink.descriptor_constructed()});
            }
            const auto& before=row["before"];const auto& after=row["after"];auto raw=before["state_words"].get<std::vector<std::uint32_t>>();
            check(state.flags.bits==raw[4]&&state.counter.bits==raw[16]&&parameters.threshold.bits==raw[7]&&parameters.gain.bits==raw[8],"retained producer arithmetic entry");
            check(sink.byte_cursor()==raw[13],"retained producer cursor entry");check_words(sink.words(),row["before_ring_words"],"retained producer ring entry");check_words(storage,before["input_words"],"producer in-place alias entry");
            for(auto modifier:before["modifiers"])check(modifier==65535,"source linear modifiers");
            const bool cold=!(state.flags.bits&1u);auto result=ctl::process({Word(before["sr"]),ar::AddressingMode::linear,ar::AddressingMode::linear,ar::AddressingMode::linear},*graph,port);
            if(cold){raw[9]=0x4000;raw[10]=0x4bd2;raw[12]=raw[6];raw[14]=raw[0];raw[15]=raw[1]-1;}
            raw[4]=state.flags.bits;raw[16]=state.counter.bits;raw[11]=sink.last_copied_count();raw[13]=sink.byte_cursor();
            check(raw==after["state_words"].get<std::vector<std::uint32_t>>(),"all39 source state fields");
            check_words(storage,after["output_words"],"all32 packed/output alias words");check_words(sink.words(),row["after_ring_words"],"all640 retained ring words");
            check(sink.descriptor_constructed()&&result.copy.copied_words==raw[11]&&result.copy.cursor_before==before["state_words"][13]&&result.copy.cursor_after==raw[13]&&result.copy.descriptor_constructed_this_call==cold,"actual copy result");
            check(result.copy.first_chunk_words+result.copy.second_chunk_words==raw[11],"actual split chunks");
            check(result.arithmetic.produced_count==row["emissions"].size(),"source emission count");
            for(unsigned n=0;n<result.arithmetic.produced_count;n++){
                const auto& actual=result.arithmetic.emissions[n];const auto& expected=row["emissions"][n];
                check(actual.input_index==expected["input_index"]&&actual.fractional_product==expected["fractional_product"]&&actual.limited_product.bits==expected["limited_product"]&&actual.output.bits==expected["output"],"source product/limit trace");
                for(unsigned add=0;add<8;add++)check(actual.addition_results[add]==expected["addition_results"][add],"source eight additions");
            }
            check(sink_addresses[port]==&sink&&input_addresses[port]==storage.data()&&state_addresses[port]==&state,"persistent native producer allocation identity");
            auto& consumer=*consumers[port];const auto& cpu=row["CPU"];check(consumer.reader_byte_cursor()==cpu["reader_before"],"retained original CPU reader entry");
            const unsigned index=cpu["packet_index"];const auto* packet_address=consumer.packet(index).data();const auto called=endpoint.calls.size();auto packet=consumer.callback(*graph,index);
            check(packet.copied_samples==320&&packet.reader_after==cpu["reader_after"]&&packet.actual_submission_result==0&&endpoint.calls.size()==called+1,"retained original CPU callback result");
            check_packets(consumer,cpu["packets"]);check(endpoint.calls.back().owner==packet_address&&consumer.packet(index).data()==packet_address,"actual stable packet owner submitted");
            calls++;cpu_calls++;emissions+=result.arithmetic.produced_count;
        }
    }
    for(const auto& row:vectors["copy_controls"]){
        auto words=word_array(row["ring_words"]);ctl::Sink sink({words,row["producer"],0,false});FixtureGate gate;FixtureEndpoint endpoint;endpoint.returns={0};endpoint.calls.reserve(2);
        ctl::Consumer consumer(row["port"],gate,&endpoint,seed(row["reader"]));const unsigned index=row["packet_index"];auto result=consumer.callback(sink,index);
        check(result.copied_samples==320&&result.reader_after==row["reader_after"]&&gate.calls==1&&endpoint.calls.size()==1,"original72 CPU copy control");
        auto packet=consumer.packet(index);for(unsigned n=0;n<320;n++)check(packet[n]==row["expected_packet"][n].get<std::uint16_t>(),"original72 copied halfword");copy_cases++;
    }
    for(const auto& row:vectors["CPU_extension_controls"]){
        std::array<Word,640> words{};words.fill(Word(0x7d7d7d));ctl::Sink sink({words,row["producer_cursor"],0,false});FixtureGate gate;FixtureEndpoint endpoint;endpoint.calls.reserve(2);
        const std::string kind=row["kind"];gate.value=kind=="reset_with_explicit_services"?row["gate_fixture"].get<unsigned>()!=0:kind!="callback_gate_false";
        if(kind=="reset_with_explicit_services")for(const auto& event:row["external_services"])if(event["service"]=="external_device_submission")endpoint.returns.push_back(event["supplied_return"]);
        ctl::Consumer consumer(row["port"],gate,kind=="reset_null_device"?nullptr:&endpoint,seed(row["reader_before"]));
        if(kind=="callback_gate_false"){
            auto result=consumer.callback(sink,row["packet_index"]);check(!result.copied_samples&&!result.actual_submission_result&&gate.calls==1&&endpoint.calls.empty(),"callback gate-false source boundary");
        }else{
            const auto result=consumer.reset(sink);check(result.returned_bits==row["returned_bits"],"original reset returned bits");
            if(kind=="reset_null_device")check(!gate.calls&&endpoint.calls.empty()&&!result.cleared_packets&&!result.submitted_packets,"reset null source boundary");
            else check(result.cleared_packets==row["zeroed_packet_count"]&&result.submitted_packets==endpoint.calls.size()&&gate.calls==1,"reset actual clears/submits");
        }
        check(consumer.reader_byte_cursor()==row["reader_after"],"reset/gate source reader");
        for(unsigned p=0;p<2;p++)for(auto sample:consumer.packet(p))check(sample==(p<endpoint.calls.size()?0:0xcdcd),"reset/gate packet retained contents");
        if(endpoint.calls.empty())check(!consumer.last_actual_submission_result(),"no phantom submission result");
        else check(consumer.last_actual_submission_result()==endpoint.returns.back(),"actual endpoint error retained");
        extension_cases++;
    }
    check(calls==208&&cpu_calls==208&&copy_cases==72&&extension_cases==324&&owners_above4G,"complete retained matrix");
    std::cout<<J{{"passed",true},{"retained_producer_calls",calls},{"source_emissions",emissions},{"individual_additions",emissions*8},{"coupled_original_CPU_callbacks",cpu_calls},{"original_copy_controls",copy_cases},{"original_reset_gate_error_controls",extension_cases},{"all39_source_fields",true},{"all640_ring_words_per_call",true},{"stable_owners_above4GiB",owners_above4G},{"physical_hardware_or_endpoint_receipt_proven",false}}.dump()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
