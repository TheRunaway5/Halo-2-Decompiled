#include "audio/native_tu15_sound_campaign_audio.hpp"
#include "audio/native_campaign_audio_ducking_adapter.hpp"
#include "audio/native_campaign_audio_fades_adapter.hpp"
#include "audio/native_campaign_audio_retirement_adapter.hpp"
#include "audio/native_campaign_audio_stop_callback_adapter.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
using namespace halo2::native_tu15_sound;
void check(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
// Never executed. These records exercise only registry eligibility, supplying
// no game behavior or dependency result to a native kernel.
void candidate(CampaignContext&,Abi&){}
void run() {
    unsigned qualified{};
    for(const auto group:{campaign_channel_routines(),campaign_ducking_routines(),campaign_fade_routines(),campaign_retirement_routines(),campaign_stop_callback_routines()})
        for(const auto& entry:group) {
            check(entry.run&&entry.semantic_original_proven,"A proved typed candidate lost its eligibility metadata");++qualified;
        }
    check(qualified==11,"Regression requires the eleven actual proved typed candidates");
    const auto required=std::span(campaign_audio_required_entries);
    std::vector<CampaignRoutineEntry> table;
    for(const auto address:required)table.push_back({address,candidate});
    check(table.size()==40,"Regression requires all forty actual owner addresses");
    check(!campaign_audio_semantic_complete(required,table),"Presence alone falsely qualified the lifted candidates");
    for(auto& entry:table)entry.semantic_original_proven=true;
    check(campaign_audio_semantic_complete(required,table),"Complete unique semantic/proven owner set was refused");
    for(unsigned index=0;index<table.size();++index) {
        auto copy=table;copy[index].semantic_original_proven=false;
        check(!campaign_audio_semantic_complete(required,copy),"One unqualified owner was accepted despite forty addresses");
        copy=table;copy[index].run=nullptr;
        check(!campaign_audio_semantic_complete(required,copy),"A proof bit qualified a missing implementation");
        copy=table;copy.erase(copy.begin()+index);
        check(!campaign_audio_semantic_complete(required,copy),"An absent required owner was accepted");
        copy=table;copy[index].address=0xffffffffu;
        check(!campaign_audio_semantic_complete(required,copy),"An unrelated owner replaced a required address");
    }
    auto duplicate=table;duplicate.back()=duplicate.front();
    check(!campaign_audio_semantic_complete(required,duplicate),"Forty records concealed a duplicate and missing owner");
    duplicate=table;duplicate.push_back(table.front());
    check(!campaign_audio_semantic_complete(required,duplicate),"An ambiguous duplicate implementation was accepted");
    auto duplicate_required=campaign_audio_required_entries;duplicate_required.back()=duplicate_required.front();
    check(!campaign_audio_semantic_complete(duplicate_required,table),"Duplicated required inventory was accepted");
    check(!campaign_audio_semantic_complete({},table),"Empty inventory opened the semantic gate");
    std::cout<<"Campaign audio semantic gate passed: eleven real typed candidates qualify (four required, seven closed dependencies), all forty addresses alone fail, and each exact unique callable owner requires explicit semantic original proof\n";
}
}
int main()try{run();return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
