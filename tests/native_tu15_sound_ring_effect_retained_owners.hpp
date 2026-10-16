#include <memory>
#include <utility>
#include <vector>
struct RetainedOwner { State state; bool initialized{};std::array<std::uintptr_t,14> addresses{}; };
static std::array<RetainedOwner,2> retained;
static Workspace shared_workspace;
static std::vector<Word> state_values(const State& state){
    std::vector<Word> words;
    auto one=[&](auto value){words.push_back(Word(value));};
    auto range=[&](const auto& values){for(auto value:values)one(value);};
    one(state.flags);for(const auto& ring:state.rings){one(ring.words.size());one(ring.write_byte_cursor);range(ring.words);}for(const auto& tap:state.taps)one(tap.read_byte_cursor);
    range(state.delays);range(state.reflection_gains);one(state.cross_gain);one(state.mono_gain);range(state.target_gains);range(state.current_gains);one(state.output_mode);
    for(const auto& filter:state.filters){one(filter.previous);one(filter.feedback);one(filter.feedforward);}range(state.diffusion_gains);range(state.wet_gains);range(state.cluster_taps);range(state.dormant_biquad_coefficients);
    const auto& fade=state.fade;one(fade.pending_mask);one(fade.sample_count);one(fade.step);one(fade.amplitude);one(fade.selected_read);one(fade.selected_write);one(fade.write_stride);one(fade.tap_cursor);range(fade.adjusted_taps);one(fade.bindings_initialized);one(fade.tap_owner);one(state.smoothing_count);
    return words;
}
static void patch_state(State& to,const State& from,unsigned offset){
    if(offset==4)to.flags=from.flags;
    else if(offset>=70&&offset<89)to.delays[offset-70]=from.delays[offset-70];
    else if(offset>=89&&offset<97)to.reflection_gains[offset-89]=from.reflection_gains[offset-89];
    else if(offset==97)to.cross_gain=from.cross_gain;
    else if(offset==98)to.mono_gain=from.mono_gain;
    else if(offset>=99&&offset<105)to.target_gains[offset-99]=from.target_gains[offset-99];
    else if(offset==105)to.output_mode=from.output_mode;
    else if(offset>=106&&offset<136){const unsigned j=(offset-106)/3,k=(offset-106)%3;if(k==0)to.filters[j].previous=from.filters[j].previous;else if(k==1)to.filters[j].feedback=from.filters[j].feedback;else to.filters[j].feedforward=from.filters[j].feedforward;}
    else if(offset>=136&&offset<143)to.diffusion_gains[offset-136]=from.diffusion_gains[offset-136];
    else if(offset>=143&&offset<147)to.wet_gains[offset-143]=from.wet_gains[offset-143];
    else if(offset>=147&&offset<159)to.cluster_taps[offset-147]=from.cluster_taps[offset-147];
    else if(offset>=179&&offset<189)to.dormant_biquad_coefficients[offset-179]=from.dormant_biquad_coefficients[offset-179];
    else if(offset>=189&&offset<287&&(offset-189)%7==4)to.rings[(offset-189)/7].write_byte_cursor=from.rings[(offset-189)/7].write_byte_cursor;
    else if(offset>=287&&offset<504&&(offset-287)%7==4)to.taps[(offset-287)/7]=from.taps[(offset-287)/7];
    else if(offset>=504&&offset<510)to.current_gains[offset-504]=from.current_gains[offset-504];
    else if(offset==526)to.smoothing_count=from.smoothing_count;
    else throw std::runtime_error("unimplemented explicit coupled fixture state patch");
}
static State& retain(State& fresh,unsigned index,const std::vector<unsigned>& state_patches,const std::vector<std::array<unsigned,2>>& history_patches){
    auto& owner=retained[index];
    if(!owner.initialized){owner.state=std::move(fresh);owner.initialized=true;for(unsigned j=0;j<14;j++)owner.addresses[j]=reinterpret_cast<std::uintptr_t>(owner.state.rings[j].words.data());}
    else {
        State expected=owner.state;
        for(auto offset:state_patches)patch_state(expected,fresh,offset);
        for(auto patch:history_patches){require(patch[0]<14&&patch[1]<fresh.rings[patch[0]].words.size(),"coupled history patch outside owner");expected.rings[patch[0]].words[patch[1]]=fresh.rings[patch[0]].words[patch[1]];}
        require(state_values(expected)==state_values(fresh),"retained owner differs outside declared original fixture patches");
        for(auto offset:state_patches)patch_state(owner.state,fresh,offset);
        for(auto patch:history_patches)owner.state.rings[patch[0]].words[patch[1]]=fresh.rings[patch[0]].words[patch[1]];
    }
    require(reinterpret_cast<std::uintptr_t>(&owner.state)>0xffffffffu&&reinterpret_cast<std::uintptr_t>(&shared_workspace)>0xffffffffu,"coupled native owners are not above4GiB");
    return owner.state;
}
static void stable_addresses(unsigned index){for(unsigned j=0;j<14;j++)require(retained[index].addresses[j]==reinterpret_cast<std::uintptr_t>(retained[index].state.rings[j].words.data()),"native history allocation identity changed");}
