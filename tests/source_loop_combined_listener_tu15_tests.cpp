#include "halo2/source_loop_combined_listener_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool value,const char* why){if(!value)throw FormatError(why);}
Vec3 vector(const Json& value){return {std::bit_cast<float>(value.at(0).get<std::uint32_t>()),
    std::bit_cast<float>(value.at(1).get<std::uint32_t>()),std::bit_cast<float>(value.at(2).get<std::uint32_t>())};}
std::array<std::byte,68> decode(const std::string& text){
    need(text.size()==136,"Original source location extent differs");std::array<std::byte,68> value{};
    for(unsigned i=0;i<68;++i)value[i]=std::byte(std::stoul(text.substr(i*2,2),nullptr,16));return value;
}
void word(std::array<std::byte,68>& bytes,unsigned at,std::uint32_t value){
    for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(8*n));
}
template<class F>void refuses(F&& operation,const char* why){bool rejected=false;
    try{operation();}catch(const FormatError&){rejected=true;}need(rejected,why);}
}
int main(int argc,char** argv)try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    need(argc==2 || argc==3,"Supply original listener fixture and optional prepared MCC level");
    if(argc==3 && !*argv[2])return 77;
    const auto fixture=Json::parse(std::ifstream(argv[1]));
    need(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture.at("cases").size()==512 && fixture.at("inverse_points").size()==128 &&
        fixture.at("entry_boundaries_verified_against_complete_functions").at("0012a4b2")=="00129f10" &&
        fixture.at("every_executed_site_has_rooted_function_provenance").get<bool>() &&
        fixture.at("original_instruction_patches").empty() && fixture.at("substituted_calls").empty(),
        "Listener reference authority/scope differs");
    unsigned queries{},matrices{},missing{},columns{},nonzero_gains{};
    for(const auto& row:fixture.at("cases")){
        const auto rounding=row.at("rounding").get<unsigned>();
        const SoundDriverFloatingPointContext fp{std::uint16_t(0x027f|(rounding<<10)),0x1f80u|(rounding<<13)};
        SourceObjectAudioViewsTu15 views;
        for(unsigned i=0;i<4;++i)if(!row.at("views")[i].is_null()){
            const auto& input=row.at("views")[i];
            views[i]=SourceObjectAudioViewTu15{vector(input.at("position")),input.at("cluster"),
                SourceObjectAudioViewTu15::Orientation{vector(input.at("forward")),vector(input.at("up"))},input.at("leaf")};
            const auto result=source_loop_listener_transform_tu15(*views[i],fp);
            if(result!=row.at("matrices")[i].get<SourceLoopListenerTransformTu15>())
                throw FormatError("Original listener matrix differs at query "+std::to_string(queries)+" view "+std::to_string(i)+
                    " expected="+row.at("matrices")[i].dump()+" actual="+Json(result).dump());++matrices;
        }
        MultiplayerAudioBsp bsp;bsp.clusters.resize(4);bsp.encoded_distances=row.at("encoded");
        bsp.distance_lower=std::bit_cast<float>(row.at("lower").get<std::uint32_t>());
        bsp.distance_upper=std::bit_cast<float>(row.at("upper").get<std::uint32_t>());
        const auto cells=row.at("cells").get<std::string>();
        for(unsigned n=0;n<cells.size();n+=2)bsp.pair_distances.push_back(std::stoul(cells.substr(n,2),nullptr,16));
        const auto location=decode(row.at("source"));
        std::uint16_t before_control,after_control;asm volatile("fnstcw %0":"=m"(before_control));const auto before_csr=_mm_getcsr();
        const auto actual=source_loop_combined_listener_tu15(location,row.at("maximum"),bsp,views,fp);
        asm volatile("fnstcw %0":"=m"(after_control));
        need(before_control==after_control && before_csr==_mm_getcsr(),"Combined listener leaked its caller FP controls/status");
        if(actual.listener!=row.at("listener") || actual.distance!=row.at("distance") ||
            (actual.listener!=0xffffffffu && actual.local_point!=row.at("local_point").get<std::array<std::uint32_t,3>>()))
            throw FormatError("Native listener query differs from original case "+std::to_string(queries));
        std::array<std::uint32_t,4> amplitudes{};
        if(actual.listener!=0xffffffffu){SoundDriverParameterConstants parameters{};
            parameters.normalized_parameter_range_epsilon_bits=source_loop_combined_constants_tu15.normalization_epsilon;
            parameters.mono_channel_count_bits=source_loop_combined_constants_tu15.one;
            amplitudes=source_loop_combined_mix_tu15({},actual.local_point,row.at("minimum"),row.at("maximum"),actual.distance,
                source_loop_combined_constants_tu15,parameters,fp).accumulated;
        }else ++missing;
        need(amplitudes==row.at("amplitudes").get<std::array<std::uint32_t,4>>(),"Original listener-to-four-gain chain differs");
        nonzero_gains+=std::ranges::any_of(amplitudes,[](auto value){return value!=0;});
        columns+=location[2]!=std::byte{};++queries;
    }
    unsigned inverse{};
    for(const auto& row:fixture.at("inverse_points")){
        const auto rounding=row.at("rounding").get<unsigned>();
        const auto result=source_loop_listener_local_point_tu15(row.at("matrix").get<SourceLoopListenerTransformTu15>(),
            row.at("point").get<std::array<std::uint32_t,3>>(),
            {std::uint16_t(0x027f|(rounding<<10)),0x1f80u|(rounding<<13)});
        if(result!=row.at("result").get<std::array<std::uint32_t,3>>())
            throw FormatError("Original inverse listener point differs at "+std::to_string(inverse));++inverse;
    }
    need(missing && missing<queries && columns && nonzero_gains,"Listener reference coverage is incomplete");
    std::cout<<"PASS "<<queries<<" original listener queries and four-gain chains, "<<matrices<<" original camera matrices, "
        <<inverse<<" inverse points; "<<missing<<" missing listeners and "<<nonzero_gains<<" nonzero contributions; no source voice/device supplied\n";
    if(argc==3){
        auto cache=std::make_shared<const Cache>(argv[2]);need(cache->is_mcc(),"Listener binding requires prepared MCC content");
        const auto geometry=cache->load_bsp_atmosphere(0);SourceObjectAudioViewsTu15 views;
        for(const auto& spawn:cache->spawns())if(spawn.bsp==0){
            const auto point=spawn.position+Vec3{0,0,.625f};const auto found=geometry.location_at(point);
            if(found.cluster>=0){views[1]=SourceObjectAudioViewTu15{point,std::int16_t(found.cluster),
                SourceObjectAudioViewTu15::Orientation{{0,1,0},{0,0,1}},found.leaf};break;}
        }
        need(views[1].has_value(),"Actual scenario has no declared camera point in its BSP");
        MultiplayerAudio audio(cache,17,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});audio.enroll_continuation();
        std::array<std::byte,68> location{};word(location,0,0x01000001u);
        word(location,12,std::bit_cast<std::uint32_t>(views[1]->position.x+3));
        word(location,16,std::bit_cast<std::uint32_t>(views[1]->position.y+4));word(location,20,std::bit_cast<std::uint32_t>(views[1]->position.z));
        word(location,0x28,std::bit_cast<std::uint32_t>(views[1]->leaf));word(location,0x2c,std::uint16_t(views[1]->cluster));
        const auto maximum=std::bit_cast<std::uint32_t>(64.f);
        refuses([&]{(void)audio.combined_loop_listener(location,maximum);},"Unbound camera frame was silently invented");
        audio.object_loop_listeners(0,views);const auto first=audio.combined_loop_listener(location,maximum);
        need(first.has_value(),"Actual submitted listener produced no query result");
        const auto same=[&](const auto& value){return value && value->local_point==first->local_point && value->distance==first->distance;};
        const auto checkpoint=audio.checkpoint();
        auto invalid=views;invalid[1]->leaf=std::int32_t(geometry.leaves.size());
        refuses([&]{audio.object_loop_listeners(0,invalid);},"Foreign leaf was accepted as an actual camera location");
        need(same(audio.combined_loop_listener(location,maximum)),"Rejected camera changed the bound listener");
        views[1]->orientation=SourceObjectAudioViewTu15::Orientation{{1,0,0},{0,0,1}};
        audio.object_loop_listeners(0,views);need(!same(audio.combined_loop_listener(location,maximum)),"Camera rotation did not change the local source point");
        const std::array positions{views[1]->position};audio.listeners(positions);
        refuses([&]{(void)audio.combined_loop_listener(location,maximum);},"Position-only update retained stale orientation");
        auto replacement=audio.prepare_restore(checkpoint);
        {auto held=audio.hold_publication(replacement);need(audio.begin_commit(replacement,held)&&audio.commit(replacement,held),"Listener checkpoint failed to publish");}
        need(same(audio.combined_loop_listener(location,maximum)),"Actual camera/BSP state did not restore exactly");
        views[1]->orientation.reset();audio.object_loop_listeners(0,views);
        refuses([&]{(void)audio.combined_loop_listener(location,maximum);},"Missing orientation was silently guessed");
        need(audio.voices()==0,"Listener queries created a PCM/admission owner");
        std::cout<<"PASS actual MCC camera/BSP binding, invalid leaf rejection, stale-orientation retirement and exact checkpoint restoration; zero voices\n";
    }
    return 0;
#else
    (void)argc;(void)argv;return 77;
#endif
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
