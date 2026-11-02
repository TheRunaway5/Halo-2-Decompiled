#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/native_campaign_screen_flash_sample_tu15.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
// Sample the complete native156E40 over real source inputs. Only its output
// goes to private test storage; no source state or admission result is supplied.
class FlashSample final:public SourcePhysicsContactMemory {
    const SourceRuntimeMemory &source_;
    std::array<std::byte,24> output_{};
    static constexpr std::uint32_t destination=0xffffffe0u;
public:
    explicit FlashSample(const SourceRuntimeMemory& source):source_(source){}
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        if(at>=destination && std::uint64_t(at)+out.size()<=std::uint64_t(destination)+output_.size())
            std::copy_n(output_.begin()+at-destination,out.size(),out.begin());
        else source_.read(at,out);
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        require(at>=destination && std::uint64_t(at)+in.size()<=std::uint64_t(destination)+output_.size(),
            "Flash reference attempted to modify its source input");
        std::copy(in.begin(),in.end(),output_.begin()+at-destination);
    }
    PlayerScreenFlash sample(unsigned user,NativeTu15PhysicsFrameFp fp) {
        output_={};native_campaign_screen_flash_sample_tu15(*this,user,destination,fp);
        const Bytes b(output_);PlayerScreenFlash result;
        result.type=std::bit_cast<std::int32_t>(b.u32(0));result.intensity=b.f32(4);
        for(unsigned c=0;c<4;++c)result.color[c]=b.f32(8+c*4);
        return result;
    }
};
}
int overshield_live_feedback_test(int argc,char** argv) {
    require(argc==5,"Supply prepared Headlong, owned captures, report and --overshield-feedback");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::getenv("DISPLAY"))return 77;
    require(std::getenv("HALO2_TEST_SCRATCH"),"Overshield feedback needs owned scratch");
    const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
    nlohmann::json report={{"passed",false},{"cases",nlohmann::json::array()}};
    const auto save=[&]{std::ofstream f(argv[3]);f<<report.dump(2)<<'\n';require(bool(f),"Cannot save overshield feedback evidence");};
    save();bool passed=true;
    for(unsigned species=0;species<2;++species) {
        MultiplayerOptions o;o.map=argv[1];o.shared_maps=o.map.parent_path();o.local_players=2;
        o.width=640;o.height=480;o.frames=360;o.hidden=true;o.validation=true;o.mute=false;
        o.source_object_world=true;o.source_object_savegame_directory=output/("source-"+std::to_string(species));
        const std::array<std::array<std::uint8_t,9>,2> appearance{{{8,12,9,1,0,26,9,0,0},{3,4,3,4,1,5,31,0,0}}};
        o.selected_profiles.push_back(native_selected_player_configuration(species?"Elite":"Spartan",appearance[species],-1));
        o.selected_profiles.push_back(native_selected_player_configuration("Observer",appearance[1-species],-1));
        o.prepared_world=std::make_shared<MultiplayerPreparedWorld>(o);auto &prepared=*o.prepared_world;
        prepared.bind_source_local_views(prepared.local_slots);
        auto &match=prepared.match;auto &objects=*prepared.source_objects;auto &native=objects.tu15_world();
        const auto slot=prepared.local_slots.at(0);
        for(unsigned n=0;n<15;++n)match.step({});
        const auto unit=match.player(slot).source_current_object;
        unsigned row=~0u;std::uint32_t pickup_sound=null_datum;
        for(unsigned i=0;i<prepared.content.netgame_equipment.size();++i) {
            const auto &p=prepared.content.netgame_equipment[i];if(p.items.size()!=1 || p.items[0].group!=0x65716970u)continue;
            const auto tag=prepared.cache->load_source_tag_metadata(p.items[0].definition);
            const Bytes b(tag.bytes_at(tag.source_address()+0x12c,0x10));
            if(b.u16(0)==2){row=i;pickup_sound=b.u32(0xc);break;}
        }
        require(row!=~0u && pickup_sound!=null_datum,"Overshield has no authored pickup/sound");
        auto item=objects.netgame_object(row);if(item==null_datum)item=objects.create_netgame_object(row);
        require(item!=null_datum,"Original owner did not spawn the overshield");
        const auto position=objects.object_transform(item).position;
        const auto abi=native.prepare_source_call();require(abi.floating_point.has_value(),"Flash reference has no source FP policy");
        const NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
        unsigned inputs{},requests{},native_flash_frames{},rendered_flash_frames{},other_flash_frames{},mismatched_flash_frames{};
        unsigned world_vertices{},fp_vertices{};std::set<std::uint64_t> voices;std::size_t pcm_frames{};
        std::optional<std::uint32_t> contact_tick;bool moved=false,finished=false,expired=false;float maximum_shield{};
        std::vector<nlohmann::json> sound_requests;
        native.observe_sound_submissions([&](bool before,std::uint32_t sound,std::uint32_t request,
            const SourcePhysicsWorldRuntimeAbi&,const SourceRuntimeMemory& memory,auto,auto) {
            if(!before || sound!=pickup_sound)return;
            const Bytes b(memory.view(request,160));++requests;
            sound_requests.push_back({{"flags",b.u32(0)},{"kind",std::to_integer<unsigned>(memory.view(request+0xf,1)[0])},{"owner",b.u32(0x50)}});
        });
        o.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState& state) {
            ++inputs;
            if(!moved && inputs==5){native.set_object_position(unit,position);moved=true;}
            if(moved && !contact_tick) {
                const auto live=objects.live_objects();
                if(std::none_of(live.begin(),live.end(),[&](const auto& v){return v.object==item;}))contact_tick=state.tick;
            }
            maximum_shield=std::max(maximum_shield,state.players.at(slot).shield);
            if(contact_tick && state.tick>*contact_tick+75) {
                finished=true;SDL_Event quit{};quit.type=SDL_QUIT;require(SDL_PushEvent(&quit)==1,"Cannot finish overshield feedback capture");
            }
        };
        o.player_screen_flash_observer=[&](unsigned view,std::uint32_t tick,const PlayerScreenFlash& actual) {
            if(!contact_tick || tick<*contact_tick)return;
            FlashSample reference(native.memory());const auto expected=reference.sample(view,fp);
            if(view==0) {
                native_flash_frames+=expected.intensity>0;rendered_flash_frames+=actual.intensity>0;
                mismatched_flash_frames+=player_screen_flash_argb(expected)!=player_screen_flash_argb(actual) ||
                    (expected.intensity>0 && expected.type!=actual.type);
                if(tick>*contact_tick+65)expired=expected.intensity==0 && actual.intensity==0;
            } else other_flash_frames+=actual.intensity>0 || expected.intensity>0;
        };
        o.audio_observer=[&](const MultiplayerAudioSubmission& s) {
            if(s.sound!=pickup_sound)return;
            require(s.voice && s.frames,"Overshield sound has no real queued PCM");
            if(voices.insert(s.voice).second)pcm_frames+=s.frames;
        };
        o.shield_layer_observer=[&](unsigned view,unsigned owner,bool first_person,std::uint32_t shader,std::uint32_t,std::span<const Vertex> vertices) {
            if(owner!=slot)return;const auto& v=match.unit(slot).definition.vitality;
            if(view==0 && first_person && shader==v.overshield_first_person_shader)fp_vertices+=vertices.size();
            if(view==1 && !first_person && shader==v.overshield_shader)world_vertices+=vertices.size();
        };
        o.screenshot=output/(std::to_string(species)+"-pickup.ppm");
        const auto status=run_multiplayer(o);native.observe_sound_submissions({});
        const bool ok=status==0 && contact_tick && finished && maximum_shield>2.9f && requests==1 && voices.size()==1 &&
            pcm_frames && native_flash_frames && rendered_flash_frames && !mismatched_flash_frames && !other_flash_frames && expired && world_vertices && fp_vertices;
        report["cases"].push_back({{"species",species?"Elite":"Spartan"},{"passed",ok},{"consumed",contact_tick.has_value()},
            {"source_unit",unit},{"pickup_sound",pickup_sound},{"native_requests",requests},{"requests",sound_requests},
            {"queued_voices",voices.size()},{"pcm_frames",pcm_frames},{"native_flash_frames",native_flash_frames},
            {"rendered_flash_frames",rendered_flash_frames},{"mismatched_flash_frames",mismatched_flash_frames},
            {"other_view_flash_frames",other_flash_frames},{"expired",expired},{"maximum_shield",maximum_shield},
            {"world_vertices",world_vertices},{"fp_vertices",fp_vertices},{"finished",finished}});
        passed&=ok;save();std::cout<<report["cases"].back().dump()<<std::endl;
    }
    report["passed"]=passed;save();require(passed,"Native overshield pickup did not deliver its exact local flash, one queued sound and world/FP layers");
    return 0;
}
