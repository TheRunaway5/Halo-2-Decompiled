#include "halo2/native_tu15_sound_graph_storage.hpp"
#include "native_tu15_sound_graph_storage_literals.hpp"
#include <algorithm>
#include <stdexcept>
namespace halo2::tu15_sound_graph_storage {
namespace {
constexpr std::array<unsigned,14> ring_words{6000,6000,6000,6000,680,612,640,720,1392,1684,2044,2476,24000,24000};
void require_index(unsigned value,unsigned count){if(value>=count)throw std::out_of_range("Native GP graph view index");}
}
GraphStorage::GraphStorage():chorus_bindings_{{
    {mixbin(6),mixbin(14),fanout_destination(0)},
    {mixbin(7),mixbin(14),fanout_destination(1)},
    {mixbin(8),mixbin(14),fanout_destination(2)},
    {mixbin(9),mixbin(14),fanout_destination(3)}}} {
    modules_.fanout_parameters=tu15_sound_gain_fanout::authored_parameters();
    modules_.distortion_parameters=tu15_sound_mono_distortion::authored_initial_parameters();
    for(unsigned j=0;j<2;j++){
        modules_.accumulator_parameters[j]=tu15_sound_two_product_accumulator::authored_parameters();
        modules_.accumulator_states[j].flags=Word24(2);
        literal_reverb_parameters(reverbs_[j].state,j);
        for(unsigned k=0;k<14;k++)reverbs_[j].state.rings[k].words.resize(ring_words[k]);
    }
    for(unsigned j=0;j<4;j++){
        modules_.chorus_parameters[j]=tu15_sound_chorus::authored_parameters();
        modules_.controller_parameters[j]=tu15_sound_controller_arithmetic::authored_parameters();
    }
    modules_.crosstalk_state.flags=Word24(2);
}
std::unique_ptr<GraphStorage> GraphStorage::fresh_literal(){return std::unique_ptr<GraphStorage>(new GraphStorage);}
void GraphStorage::validate_seed(const RetainedScratchSeed& seed){
    for(unsigned j=0;j<2;j++){
        if(seed.reverb_unused_tails[j].size()!=51008)throw std::invalid_argument("Retained reverb padding extent");
        for(unsigned k=0;k<14;k++){
            if(seed.reverb_histories[j][k].size()!=ring_words[k])throw std::invalid_argument("Retained reverb ring extent");
            for(auto word:seed.reverb_histories[j][k])if(word>0xffffffu)throw std::invalid_argument("Retained history word exceeds24bits");
        }
    }
    for(const auto sink:seed.controller_sinks){if(sink.size()!=640)throw std::invalid_argument("Retained controller sink extent");for(auto word:sink)if(word.bits>0xffffffu)throw std::invalid_argument("Retained sink word exceeds24bits");}
    if(seed.final_unused_tail.size()!=3904)throw std::invalid_argument("Retained final padding extent");
}
std::unique_ptr<GraphStorage> GraphStorage::retained_scratch_literal(const RetainedScratchSeed& seed){
    validate_seed(seed);auto result=fresh_literal();
    for(unsigned j=0;j<2;j++){
        for(unsigned k=0;k<14;k++)std::copy(seed.reverb_histories[j][k].begin(),seed.reverb_histories[j][k].end(),result->reverbs_[j].state.rings[k].words.begin());
        std::copy(seed.reverb_unused_tails[j].begin(),seed.reverb_unused_tails[j].end(),result->reverb_tails_[j].begin());
    }
    for(unsigned j=0;j<4;j++)std::copy(seed.controller_sinks[j].begin(),seed.controller_sinks[j].end(),result->controller_sinks_[j].words().begin());
    std::copy(seed.final_unused_tail.begin(),seed.final_unused_tail.end(),result->final_tail_.begin());return result;
}
std::span<Word24,6> GraphStorage::unresolved_prefix()noexcept{return std::span<Word24,6>(mix_with_prefix_.data(),6);}
std::span<const Word24,32> GraphStorage::fanout_input()const noexcept{return std::span<const Word24,32>(mix_with_prefix_.data(),32);}
Block GraphStorage::mixbin(unsigned j){require_index(j,32);return Block(mix_with_prefix_.data()+6+j*32,32);}
Block GraphStorage::fanout_destination(unsigned j){require_index(j,5);return Block(j<4?fanout_four_.data()+j*32:workspace_.data(),32);}
std::array<Block,8> GraphStorage::fanout_routes(){return{fanout_destination(0),fanout_destination(1),fanout_destination(2),fanout_destination(3),fanout_destination(4),fanout_destination(4),fanout_destination(4),fanout_destination(4)};}
std::span<Word24,401> GraphStorage::chorus_history(unsigned j){require_index(j,4);return std::span<Word24,401>(chorus_history_.data()+j*400,401);}
const tu15_sound_chorus::Bindings& GraphStorage::chorus_bindings(unsigned j)const{require_index(j,4);return chorus_bindings_[j];}
Block GraphStorage::reverb_common_input(unsigned j){require_index(j,2);return mixbin(j==0?12:11);}
Block GraphStorage::reverb_common_output(unsigned j,unsigned channel){require_index(j,2);require_index(channel,4);return mixbin((j==0?19:15)+channel);}
ReverbAdapter& GraphStorage::reverb(unsigned j){require_index(j,2);return reverbs_[j];}
const ReverbAdapter& GraphStorage::reverb(unsigned j)const{require_index(j,2);return reverbs_[j];}
std::span<const std::byte,51008> GraphStorage::reverb_unused_tail(unsigned j)const{require_index(j,2);return reverb_tails_[j];}
std::span<Word24,640> GraphStorage::controller_sink(unsigned j){require_index(j,4);return controller_sinks_[j].words();}
std::uint32_t GraphStorage::controller_sink_byte_cursor(unsigned j)const{require_index(j,4);return controller_sinks_[j].byte_cursor();}
tu15_sound_controller::Sink& GraphStorage::controller_sink_owner(unsigned j){require_index(j,4);return controller_sinks_[j];}
const tu15_sound_controller::Sink& GraphStorage::controller_sink_owner(unsigned j)const{require_index(j,4);return controller_sinks_[j];}
std::size_t GraphStorage::actual_title_scratch_bytes()const noexcept{
    std::size_t bytes=final_tail_.size();for(const auto& tail:reverb_tails_)bytes+=tail.size();for(const auto& sink:controller_sinks_)bytes+=sink.words().size()*sizeof(Word24);for(const auto& reverb:reverbs_)for(const auto& ring:reverb.state.rings)bytes+=ring.words.size()*sizeof(std::uint32_t);return bytes;
}
void GraphStorage::copy_common_to_reverb_buffers(){
    for(auto word:workspace_)if(word.bits>0xffffffu)throw std::invalid_argument("Common workspace copy exceeds24bits");
    for(unsigned j=0;j<2;j++){
        for(auto word:reverb_common_input(j))if(word.bits>0xffffffu)throw std::invalid_argument("Common input copy exceeds24bits");
        for(unsigned channel=0;channel<4;channel++)for(auto word:reverb_common_output(j,channel))if(word.bits>0xffffffu)throw std::invalid_argument("Common output copy exceeds24bits");
    }
    for(unsigned j=0;j<544;j++)raw_workspace_[j/16][j%16]=workspace_[j].bits;
    for(unsigned j=0;j<2;j++){
        const auto input=reverb_common_input(j);for(unsigned k=0;k<32;k++)reverbs_[j].input[k]=input[k].bits;
        for(unsigned channel=0;channel<4;channel++){const auto output=reverb_common_output(j,channel);for(unsigned k=0;k<32;k++)reverbs_[j].outputs[channel][k]=output[k].bits;}
    }
}
void GraphStorage::copy_reverb_buffers_to_common(){
    // Check the complete prospective copy before changing common owners.
    for(const auto& block:raw_workspace_)for(auto word:block)if(word>0xffffffu)throw std::invalid_argument("Reverb workspace copy exceeds24bits");
    for(const auto& reverb:reverbs_){for(auto word:reverb.input)if(word>0xffffffu)throw std::invalid_argument("Reverb input copy exceeds24bits");for(const auto& output:reverb.outputs)for(auto word:output)if(word>0xffffffu)throw std::invalid_argument("Reverb output copy exceeds24bits");}
    for(unsigned j=0;j<544;j++)workspace_[j]=Word24(raw_workspace_[j/16][j%16]);
    for(unsigned j=0;j<2;j++){
        auto input=reverb_common_input(j);for(unsigned k=0;k<32;k++)input[k]=Word24(reverbs_[j].input[k]);
        for(unsigned channel=0;channel<4;channel++){auto output=reverb_common_output(j,channel);for(unsigned k=0;k<32;k++)output[k]=Word24(reverbs_[j].outputs[channel][k]);}
    }
}
}
