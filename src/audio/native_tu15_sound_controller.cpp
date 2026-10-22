#include "halo2/native_tu15_sound_controller.hpp"
#include "halo2/native_tu15_sound_graph_storage.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace halo2::tu15_sound_controller {
namespace {
void valid_word(Word24 word){if(word.bits>0xffffffu)throw std::invalid_argument("Controller native word exceeds 24 bits");}
void valid_cursor(std::uint32_t cursor){if(cursor>=ring_bytes||(cursor&3u))throw std::invalid_argument("Controller byte cursor is outside aligned owned ring");}
void valid_index(unsigned index,unsigned count){if(index>=count)throw std::out_of_range("Controller native owner index");}
bool overlap(const void* a,std::size_t an,const void* b,std::size_t bn){
    if(!an||!bn)return false;
    const auto x=reinterpret_cast<std::uintptr_t>(a),y=reinterpret_cast<std::uintptr_t>(b);
    if(x>std::numeric_limits<std::uintptr_t>::max()-an||y>std::numeric_limits<std::uintptr_t>::max()-bn)throw std::invalid_argument("Controller native object extent wraps");
    return x<y+bn&&y<x+an;
}
}
void Sink::restore(const SinkSeed& seed){
    valid_cursor(seed.byte_cursor);
    if(seed.last_copied_count>32||(!seed.descriptor_constructed&&seed.last_copied_count))throw std::invalid_argument("Controller retained descriptor phase");
    for(auto word:seed.words)valid_word(word);
    if(seed.words.data()!=words_.data())std::copy(seed.words.begin(),seed.words.end(),words_.begin());
    byte_cursor_=seed.byte_cursor;last_copied_count_=seed.last_copied_count;descriptor_constructed_=seed.descriptor_constructed;
}
void Sink::validate_contents()const{
    valid_cursor(byte_cursor_);
    if(last_copied_count_>32)throw std::invalid_argument("Controller retained count exceeds block");
    for(auto word:words_)valid_word(word);
}
void Sink::validate(bool cold)const{
    validate_contents();
    if(!descriptor_constructed_&&!cold)throw std::invalid_argument("Controller source descriptor has not been constructed");
}
CopyResult Sink::append(std::span<const Word24> prefix,bool cold){
    validate(cold);
    if(prefix.size()>32)throw std::invalid_argument("Controller copied prefix exceeds block");
    if(overlap(prefix.data(),prefix.size_bytes(),this,sizeof(*this)))throw std::invalid_argument("Controller copied prefix overlaps its sink owner");
    for(auto word:prefix)valid_word(word);
    CopyResult result{};result.cursor_before=byte_cursor_;result.copied_words=static_cast<std::uint32_t>(prefix.size());
    const auto start=byte_cursor_/4;result.first_chunk_words=std::min(result.copied_words,ring_words-start);result.second_chunk_words=result.copied_words-result.first_chunk_words;
    for(unsigned n=0;n<result.copied_words;n++)words_[(start+n)%ring_words]=prefix[n];
    byte_cursor_=(byte_cursor_+4*result.copied_words)%ring_bytes;
    last_copied_count_=result.copied_words;descriptor_constructed_=true;
    result.cursor_after=byte_cursor_;result.descriptor_constructed_this_call=cold;return result;
}
ProducerResult process(const tu15_sound_controller_arithmetic::Controls& controls,
                       const tu15_sound_controller_arithmetic::Parameters& parameters,
                       tu15_sound_controller_arithmetic::State& state,std::span<Word24,32> storage,Sink& sink){
    const bool cold=!(state.flags.bits&1u);sink.validate(cold);
    // Reject every cross-owner alias before approved arithmetic mutates anything.
    if(overlap(storage.data(),storage.size_bytes(),&sink,sizeof(sink))||overlap(&state,sizeof(state),&sink,sizeof(sink))||overlap(&parameters,sizeof(parameters),&sink,sizeof(sink)))throw std::invalid_argument("Controller arithmetic aliases the retained sink owner");
    ProducerResult result{};result.arithmetic=tu15_sound_controller_arithmetic::process(controls,parameters,state,storage);
    result.copy=sink.append(storage.first(result.arithmetic.produced_count),cold);return result;
}
ProducerResult process(const tu15_sound_controller_arithmetic::Controls& controls,tu15_sound_graph_storage::GraphStorage& graph,unsigned port){
    valid_index(port,4);auto& effects=graph.effects();
    return process(controls,effects.controller_parameters[port],effects.controller_states[port],graph.mixbin(27+port),graph.controller_sink_owner(port));
}
Consumer::Consumer(unsigned port,Gate& gate,PacketEndpoint* endpoint,const ConsumerSeed& seed)
    :port_(port),gate_(gate),endpoint_(endpoint),reader_byte_cursor_(seed.reader_byte_cursor),packets_(seed.packets){valid_index(port,4);valid_cursor(reader_byte_cursor_);}
std::span<const std::uint16_t,packet_samples> Consumer::packet(unsigned index)const{valid_index(index,2);return packets_[index];}
CallbackResult Consumer::callback(const Sink& sink,unsigned index){
    valid_index(index,2);sink.validate_contents();valid_cursor(reader_byte_cursor_);
    if(!endpoint_)throw std::invalid_argument("Controller callback needs a real packet endpoint");
    CallbackResult result{};result.reader_before=result.reader_after=reader_byte_cursor_;
    if(!gate_.can_copy())return result;
    const auto producer=sink.byte_cursor();const auto words=sink.words();const auto start=reader_byte_cursor_/4;
    for(unsigned n=0;n<packet_samples;n++)packets_[index][n]=static_cast<std::uint16_t>((words[(start+n)%ring_words].bits>>8)&0xffffu);
    const auto distance=(producer+ring_bytes-reader_byte_cursor_)%ring_bytes;
    reader_byte_cursor_=distance>=0x500u?(reader_byte_cursor_+0x500u)%ring_bytes:(producer+ring_bytes-0x280u)%ring_bytes;
    result.copied_samples=packet_samples;result.reader_after=reader_byte_cursor_;
    last_submission_=endpoint_->submit(index,packets_[index]);result.actual_submission_result=last_submission_;
    return result;
}
CallbackResult Consumer::callback(const tu15_sound_graph_storage::GraphStorage& graph,unsigned index){return callback(graph.controller_sink_owner(port_),index);}
ResetResult Consumer::reset(const Sink& sink){
    ResetResult result{};
    if(!endpoint_)return result;
    sink.validate_contents();valid_cursor(reader_byte_cursor_);
    if(gate_.can_copy())reader_byte_cursor_=sink.byte_cursor();
    for(unsigned index=0;index<2;index++){
        packets_[index].fill(0);result.cleared_packets++;
        last_submission_=endpoint_->submit(index,packets_[index]);result.submitted_packets++;
        if(*last_submission_&0x80000000u){result.returned_bits=0x80004005u;return result;}
    }
    return result;
}
ResetResult Consumer::reset(const tu15_sound_graph_storage::GraphStorage& graph){return reset(graph.controller_sink_owner(port_));}
}
