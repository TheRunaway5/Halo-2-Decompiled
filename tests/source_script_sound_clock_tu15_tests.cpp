#include "halo2/source_script_sound_clock_tu15.hpp"
#include <bit>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace {
using namespace halo2;
void require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
template<class F>void fails(F&&f,const char*message){bool threw=false;try{f();}catch(const std::runtime_error&){threw=true;}require(threw,message);}
const SoundDriverFloatingPointContext fp{0x027f,0x1f80};
struct QueryCase {
    std::uint32_t source_case,mode,matched,start,value,seconds,tick,rate,delta,cw,mxcsr;
    std::int32_t expected;
};
constexpr QueryCase cases[]={
#include "source_script_sound_clock_tu15_cases.inc"
};
struct SelectionCase {
    std::uint32_t source_case,sound,pitch_count,count,prefix,class_flags,game_kind,control,cw,mxcsr,seed;
    std::array<std::uint16_t,32> chances;
    std::array<std::uint32_t,4> flags;
    std::uint32_t pitch,permutation,result_seed;
    std::array<std::uint32_t,4> result_flags;
};
constexpr SelectionCase selection_cases[]={
#include "source_script_sound_clock_selection_tu15_cases.inc"
};
void original_queries(){
    for(const auto &c:cases){
        const SourceScriptSoundClockTimeTu15 time{c.tick,std::bit_cast<std::int16_t>(std::uint16_t(c.rate)),c.delta};
        const SoundDriverFloatingPointContext controls{std::uint16_t(c.cw),c.mxcsr};
        std::int32_t result;
        if(c.mode==0)result=source_script_sound_explicit_remaining_tu15(c.matched,c.start,std::bit_cast<std::int32_t>(c.value),time,controls);
        else if(c.mode==1)result=source_script_sound_stored_remaining_tu15(c.matched,c.start,std::bit_cast<std::int32_t>(c.value),time,time,controls);
        else result=source_script_sound_language_remaining_tu15(c.matched,c.start,c.seconds,time,controls);
        if(result!=c.expected)throw std::runtime_error("Original query case "+std::to_string(c.source_case));
    }
}
void admission_policy(){
    SourceScriptSoundClockRowsTu15 rows;
    const auto initial=rows;
    SourceScriptSoundClockTimeTu15 now{100,30,0x3d088889};
    std::vector<unsigned> durations,stops;unsigned selections=0,reads=0;
    SourceScriptSoundClockPolicyTu15 policy;
    policy.time=[&]{++reads;return now;};
    policy.selected_seconds=[&](unsigned i,std::uint32_t sound,const auto&selector){
        require(rows[i].sound==sound && rows[i].selection==selector,"Unowned duration row");durations.push_back(i);return 0x3f800000u;
    };
    policy.select=[&](std::uint32_t sound,auto&selector){require(sound==42,"Wrong selection sound");++selections;selector={2,3};};
    policy.request_stop=[&](unsigned i){stops.push_back(i);};
    auto selected=source_script_sound_clock_admit_tu15(rows,42,policy,fp);
    require(selected==0 && durations.empty() && stops.empty() && selections==1 && reads==1,"First free admission");
    require(rows[0].sound==42 && rows[0].start_tick_bits==100 && rows[0].selection==SourceScriptSoundClockSelectionTu15{2,3} && rows[0].predicate==0 && rows[0].trailing==0xff,"Admission stores");
    for(unsigned i=1;i<16;++i)require(rows[i]==initial[i],"Unselected row modified");

    rows=initial;rows[4].sound=42;rows[4].predicate=0;rows[12].sound=42;rows[12].predicate=1;
    durations.clear();stops.clear();reads=0;
    selected=source_script_sound_clock_admit_tu15(rows,42,policy,fp);
    require(selected==12 && stops==std::vector<unsigned>{4} && durations.empty() && reads==1,"Last match overrides first candidate");
    require(source_script_sound_clock_find_first_tu15(rows,42)==4,"Queries require first match");
    rows[12].predicate=0;stops.clear();
    source_script_sound_clock_admit_tu15(rows,42,policy,fp);
    require(stops==std::vector<unsigned>({4,12}),"Every exact match requests its original ordered stop");

    for(unsigned i=0;i<16;++i){rows[i].sound=100+i;rows[i].start_tick_bits=100;}
    durations.clear();stops.clear();selections=reads=0;
    selected=source_script_sound_clock_admit_tu15(rows,42,policy,fp);
    require(!selected && durations.size()==16 && selections==0 && reads==16,"Full active clock refuses admission");
    rows[3].start_tick_bits=70;rows[6].sound=0xffffffff;
    durations.clear();reads=0;
    selected=source_script_sound_clock_admit_tu15(rows,42,policy,fp);
    require(selected==3 && durations==std::vector<unsigned>({0,1,2,3}) && reads==5,"First expired retains candidate and stops eligibility reads");

    // The clock is read after the selected-duration provider and again after
    // selection; providers may advance actual time without replacing rows.
    rows=initial;rows[0].sound=99;rows[0].start_tick_bits=100;now.tick_bits=100;
    policy.selected_seconds=[&](unsigned,std::uint32_t,const auto&){now.tick_bits=130;return 0x3f800000u;};
    policy.select=[&](std::uint32_t,auto&s){s={0xff,0xff};now.tick_bits=137;};
    selected=source_script_sound_clock_admit_tu15(rows,42,policy,fp);
    require(selected==0 && rows[0].start_tick_bits==137,"Fresh clock read phases");

    rows=initial;policy.select=[](std::uint32_t,auto&s){s.pitch_range=7;throw std::runtime_error("Real selection failure");};
    fails([&]{source_script_sound_clock_admit_tu15(rows,42,policy,fp);},"Selection failure was swallowed");
    require(rows[0].selection.pitch_range==7 && rows[0].selection.permutation==0xff && rows[0].sound==0xffffffff && rows[0].predicate==0xff,"Selection failure rolled back prior selector store");
    rows[0].sound=42;rows[0].predicate=0;stops.clear();policy.request_stop={};
    fails([&]{source_script_sound_clock_admit_tu15(rows,42,policy,fp);},"Missing real stop owner accepted");
}
void original_selection(){
    for(const auto&c:selection_cases){
        SourceScriptSoundClockSelectionInputTu15 input;
        input.sound=c.sound;input.pitch_range_count=std::uint8_t(c.pitch_count);
        input.permutation_count=std::bit_cast<std::int16_t>(std::uint16_t(c.count));
        if((c.class_flags&0x20) && std::uint16_t(c.prefix)!=0xffff)input.prefix_bit=std::uint16_t(c.prefix);
        if(input.permutation_count>0 && input.permutation_count<=32)input.skip_fractions=std::span(c.chances).first(unsigned(input.permutation_count));
        input.game_kind=std::bit_cast<std::int32_t>(c.game_kind);input.skip_control=std::uint8_t(c.control);
        auto flags=c.flags;auto seed=c.seed;SourceScriptSoundClockSelectionTu15 pair{0xaa,0xbb};
        source_script_sound_clock_select_tu15(input,flags,seed,pair,{std::uint16_t(c.cw),c.mxcsr});
        if(pair!=SourceScriptSoundClockSelectionTu15{std::uint8_t(c.pitch),std::uint8_t(c.permutation)} || seed!=c.result_seed || flags!=c.result_flags)
            throw std::runtime_error("Original selection case "+std::to_string(c.source_case));
    }
    SourceScriptSoundClockSelectionInputTu15 input;
    input.sound=42;input.pitch_range_count=1;input.permutation_count=3;input.prefix_bit=126;
    std::array<std::uint32_t,4> flags{};const auto before=flags;std::uint32_t seed=17;SourceScriptSoundClockSelectionTu15 pair{8,9};
    std::array<std::uint16_t,3> chances{};input.skip_fractions=chances;
    fails([&]{source_script_sound_clock_select_tu15(input,flags,seed,pair,fp);},"Unowned prefix bits accepted");
    require(flags==before && seed==17 && pair==SourceScriptSoundClockSelectionTu15{8,9},"Prefix admission failure mutated selection");
    input.prefix_bit=4;input.skip_fractions=std::span(chances).first(2);
    fails([&]{source_script_sound_clock_select_tu15(input,flags,seed,pair,fp);},"Truncated chance owner accepted");
    input.permutation_count=32;
    fails([&]{source_script_sound_clock_select_tu15(input,flags,seed,pair,fp);},"Unowned widened permutation domain accepted");
    input.permutation_count=0;
    fails([&]{source_script_sound_clock_select_tu15(input,flags,seed,pair,fp);},"Empty permutation domain accepted");
    input.permutation_count=3;input.prefix_bit={};input.skip_fractions={};
    bool stored=false;
    fails([&]{source_script_sound_clock_select_tu15(input,flags,seed,pair,fp,[&](auto pc,auto a,auto b){require(pc==0x2133ea && a==17 && b==17u*0x19660du+0x3c6ef35fu,"Unowned RNG store");stored=true;throw std::runtime_error("Observer failed");});},"RNG observer failure swallowed");
    require(stored && seed==17u*0x19660du+0x3c6ef35fu && pair==SourceScriptSoundClockSelectionTu15{8,9},"Failed selection rolled back prior actual RNG store");
}
void stopping_and_wrapping(){
    SourceScriptSoundClockRowsTu15 rows;rows[2]={42,17,{3,4},0x51,0x91};rows[10]={42,99,{7,8},0x52,0x92};
    const auto prior=rows;std::vector<unsigned> stops;
    auto selected=source_script_sound_clock_stop_first_tu15(rows,42,[&](unsigned i){require(rows==prior,"Stop stores preceded request");stops.push_back(i);});
    require(selected==2 && stops==std::vector<unsigned>{2},"Explicit stop first match");
    auto expected=prior;expected[2].sound=expected[2].start_tick_bits=0xffffffff;
    require(rows==expected,"Stop changed selector/predicate/unrelated rows");
    require(!source_script_sound_clock_stop_first_tu15(rows,0xffffffff,{}),"NONE stop matched empty row");
    require(!source_script_sound_clock_find_first_tu15(rows,0xffffffff),"NONE query matched empty row");
    const auto before=rows;
    fails([&]{source_script_sound_clock_stop_first_tu15(rows,42,[](unsigned){throw std::runtime_error("Stop failure");});},"Stop failure swallowed");
    require(rows==before,"Failed stop cleared clock metadata");
    const SourceScriptSoundClockTimeTu15 at{0x7fffffffu,30,0x3d088889u};
    require(source_script_sound_clock_expired_tu15(0x7ffffff0u,0x3f800000u,at,fp),"Expiry used widened arithmetic instead of signed wrap32");
}
void floating_point_owner(){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t cw_before,cw_after,sw_before,sw_after;asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw_before),"=m"(sw_before));
    const auto mxcsr_before=_mm_getcsr();
    const SourceScriptSoundClockTimeTu15 time{100,60,0x3c888889u};
    source_script_sound_explicit_remaining_tu15(true,100,31,time,{0x0e7f,0x7f80});
    asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw_after),"=m"(sw_after));
    require(cw_before==cw_after && sw_before==sw_after && mxcsr_before==_mm_getcsr(),"Query leaked floating point environment");
    fails([&]{source_script_sound_explicit_remaining_tu15(true,100,31,time,{0x027e,0x1f80});},"Unmasked x87 exceptions accepted");
    fails([&]{source_script_sound_explicit_remaining_tu15(true,100,31,time,{0x027f,0x1f00});},"Unmasked SSE exceptions accepted");
    fails([&]{source_script_sound_explicit_remaining_tu15(true,100,31,time,{0x027f,0x11f80});},"Invalid MXCSR accepted");
    require(source_script_sound_explicit_remaining_tu15(false,100,31,time,{0,0xffffffffu})==0,"Missing explicit query touched unavailable FP owner");
#endif
}
}
int main(){try{original_queries();admission_policy();original_selection();stopping_and_wrapping();floating_point_owner();std::cout<<"TU1.5 script sound query fixtures="<<std::size(cases)<<" selection fixtures="<<std::size(selection_cases)<<" and admission controls PASS\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
