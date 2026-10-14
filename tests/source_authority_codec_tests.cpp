#include "halo2/source_authority_codec.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <zlib.h>
#ifndef _WIN32
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception &e){failed=true;
    std::cout<<"Rejected: "<<e.what()<<'\n';}check(failed,"Malformed authority archive accepted");}
struct Directory {
    std::filesystem::path path;
    Directory(){auto p=(std::filesystem::temp_directory_path()/"halo2-authority-codec-XXXXXX").string();
        check(::mkdtemp(p.data()),"Cannot create private codec files");path=p;}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t time{};mutable std::atomic<std::uint64_t> counter{};
    std::uint32_t milliseconds()const noexcept override{return time;}
    std::uint64_t cycles()const noexcept override{return counter.fetch_add(1000);}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
constexpr SourceMatchAuthority::AuthorityDigest binding{std::byte{0x15},std::byte{0x43}};
struct Owner {
    Cache cache;LevelContent content;Directory files;std::shared_ptr<Clock> clock=std::make_shared<Clock>();
    std::unique_ptr<MultiplayerObjectRuntime> world;std::unique_ptr<MultiplayerMatch> match;
    std::unique_ptr<SourceMatchAuthority> authority;
    Owner(const char *xbe,const char *map,std::uint32_t time):cache(map,{},OriginalData::load_xbe(xbe)),content(cache.load_content(0,true)){
        clock->time=time;world=std::make_unique<MultiplayerObjectRuntime>(xbe,cache,0,files.path,clock);
        UiVariant variant{"Slayer",0,{{0x59,5}}};auto rules=slayer_rules(variant);
        rules.weapon=cache.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
        world->configure_session(source_game_variant(variant),1,30,rules.random_seed);
        match=std::make_unique<MultiplayerMatch>(content,0,cache.spawns(),rules,MultiplayerKingServices{},
            [this]{return 100000u+(match?unsigned(match->simulation().ticks()/30):0);},world.get());
        match->configure_weapon_pickups();const auto borrowed=std::shared_ptr<const Cache>(&cache,[](const Cache*){});
        match->configure_grenades(borrowed,*cache.original_data()->grenade_constants);
        match->configure_melee_animation(borrowed,*cache.original_data()->melee_constants);
        for(unsigned id=0;id<4;++id){const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> appearance;
            std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());const auto name="Codec seat "+std::to_string(id);
            check(match->join(name,-1,native_selected_player_configuration(name,appearance,-1))==id,"Source codec roster admission failed");}
        const auto fp=cache.load_first_person_binding(rules.weapon,2);const std::array<unsigned,2> locals{0,1};
        match->configure_replicated_first_person(fp);match->configure_local_views(locals,fp);
        authority=std::make_unique<SourceMatchAuthority>(*match,*world);
    }
    void step(const std::array<Controls,16> &input){clock->time+=33;match->step(input);}
};
std::vector<std::byte> invalid_state_enum(const std::vector<std::byte> &frame){
    const auto get=[&](std::size_t at){std::uint64_t n{};for(unsigned i=0;i<8;++i)n|=std::to_integer<std::uint64_t>(frame[at+i])<<(8*i);return n;};
    std::vector<std::byte> raw(static_cast<std::size_t>(get(16)));uLongf n=raw.size();
    check(uncompress(reinterpret_cast<Bytef *>(raw.data()),&n,reinterpret_cast<const Bytef *>(frame.data()+64),frame.size()-64)==Z_OK,
        "Cannot inspect private enum fixture");
    // A named state field with enumeration tag14, before the source-memory
    // blobs. Change its encoded underlying value without constructing an
    // invalid C++ enum in this test, then regenerate the valid envelope hash.
    const std::array<std::byte,16> needle{std::byte{5},std::byte{},std::byte{'s'},std::byte{'t'},std::byte{'a'},std::byte{'t'},std::byte{'e'},std::byte{14}};
    const auto at=std::search(raw.begin(),raw.end(),needle.begin(),needle.end());
    check(at!=raw.end(),"Private fixture has no idle state enum");at[8]=std::byte{255};
    std::vector<std::byte> result(frame.begin(),frame.begin()+64),compressed(compressBound(raw.size()));uLongf size=compressed.size();
    check(compress2(reinterpret_cast<Bytef *>(compressed.data()),&size,reinterpret_cast<const Bytef *>(raw.data()),raw.size(),Z_BEST_SPEED)==Z_OK,
        "Cannot pack private malformed enum fixture");
    for(unsigned i=0;i<8;++i)result[24+i]=std::byte(std::uint64_t(size)>>(8*i));
    SHA256(reinterpret_cast<const unsigned char *>(raw.data()),raw.size(),reinterpret_cast<unsigned char *>(result.data()+32));
    result.insert(result.end(),compressed.begin(),compressed.begin()+size);return result;
}
void write(const std::filesystem::path &path,std::span<const std::byte> data){
    std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char *>(data.data()),std::streamsize(data.size()));
    check(bool(file),"Cannot write private authority fixture");
}
std::vector<std::byte> read(const std::filesystem::path &path){
    std::ifstream file(path,std::ios::binary|std::ios::ate);check(bool(file),"Cannot read private authority fixture");
    const auto size=file.tellg();check(size>=0&&std::uint64_t(size)<=SourceAuthorityCodecLimits{}.encoded_bytes,"Private archive extent is invalid");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));file.seekg(0);
    file.read(reinterpret_cast<char *>(bytes.data()),std::streamsize(bytes.size()));check(bool(file),"Private authority fixture is truncated");return bytes;
}
void receive(const char *xbe,const char *map,const char *before,const char *after){
    const auto decoded=decode_source_authority(read(before)),expected=decode_source_authority(read(after));
    Owner target(xbe,map,900000);auto prepared=target.authority->prepare(decoded,binding);
    check(target.authority->commit(std::move(prepared)),"Child process could not restore portable authority");
    for(unsigned tick=0;tick<40;++tick){std::array<Controls,16> input{};
        for(unsigned id=0;id<4;++id){input[id].yaw=target.match->unit(id).yaw;input[id].forward=id&1?0:.1f;}target.step(input);}
    const auto actual=target.authority->checkpoint(binding);
    check(actual.world.world.bank==expected.world.world.bank,"Cross-process source world diverged after forty ticks");
    check(encode_source_authority(actual)==encode_source_authority(expected),"Cross-process full owning continuation values differ");
    std::cout<<"PASS separate exec process: complete named owning checkpoint matches producer after forty future source ticks\n";
}
void run(const char *exe,const char *xbe,const char *map,const char *receiver=nullptr){
    Owner a(xbe,map,1000),b(xbe,map,900000);
    for(unsigned tick=0;tick<20;++tick){std::array<Controls,16> input{};for(unsigned id=0;id<4;++id)input[id].yaw=a.match->unit(id).yaw;
        input[0].fire=tick<12;input[0].reload=tick==12;input[0].grenade=tick>=17;a.step(input);}
    const auto saved=a.authority->checkpoint(binding);
    check(a.match->unit(0).weapon.loaded==54&&a.match->unit(0).grenade_action.held()!=null_datum,"Codec cut omitted active weapon/grenade state");
    const auto encoded=encode_source_authority(saved);std::cout<<"Encoded complete source cut: "<<encoded.size()<<" bytes\n";
    const auto decoded=decode_source_authority(encoded);
    check(encoded==encode_source_authority(decoded),"Authority archive round trip changed owning values");
    auto prepared=b.authority->prepare(decoded,binding);check(b.authority->commit(std::move(prepared)),"Decoded complete source cut did not restore");
    check(a.world->memory().checkpoint()==b.world->memory().checkpoint(),"Decoded source bank differs after publication");
    auto bad=encoded;bad.pop_back();rejects([&]{decode_source_authority(bad);});
    bad=encoded;bad.push_back(std::byte{});rejects([&]{decode_source_authority(bad);});
    bad=encoded;bad[8]=std::byte{2};rejects([&]{decode_source_authority(bad);});
    bad=encoded;bad[64+bad.size()/3]^=std::byte{1};rejects([&]{decode_source_authority(bad);});
    auto limits=SourceAuthorityCodecLimits{};limits.decoded_bytes=128;rejects([&]{decode_source_authority(encoded,limits);});
    rejects([&]{encode_source_authority(saved,limits);});
    limits={};limits.allocated_bytes=1;rejects([&]{decode_source_authority(encoded,limits);});
    limits={};limits.container_elements=1;rejects([&]{decode_source_authority(encoded,limits);});
    const auto invalid_enum=invalid_state_enum(encoded);rejects([&]{decode_source_authority(invalid_enum);});
    auto pointer=saved;pointer.match.simulation.player.value.custom_animation=std::make_shared<AnimationClip>();
    rejects([&]{encode_source_authority(pointer);});
    auto incompatible=decoded;incompatible.match.simulation.player.value.definition.radius+=.25f;
    const auto rejected=decode_source_authority(encode_source_authority(incompatible));
    const auto before_rejection=b.world->memory().checkpoint();
    rejects([&]{b.authority->prepare(rejected,binding);});
    check(b.world->memory().checkpoint()==before_rejection,"Rejected decoded native definition published source backing");
    for(unsigned tick=0;tick<40;++tick){std::array<Controls,16> input{};
        for(unsigned id=0;id<4;++id){input[id].yaw=a.match->unit(id).yaw;input[id].forward=id&1?0:.1f;}
        a.step(input);b.step(input);
        check(a.world->memory().checkpoint()==b.world->memory().checkpoint(),"Decoded source world diverged on future input");}
    std::cout<<"PASS complete actual TU1.5 owning archive round trip, active four-player restoration, corrupt/trailing/version/budget/pointer rejection and forty future complete source bank ticks\n";
#ifndef _WIN32
    Directory transfer;const auto before=transfer.path/"before.h2authority",after=transfer.path/"after.h2authority";
    write(before,encoded);write(after,encode_source_authority(a.authority->checkpoint(binding)));
    const std::vector<std::string> arguments{std::filesystem::canonical(receiver?receiver:exe).string(),"--receive",std::filesystem::canonical(xbe).string(),
        std::filesystem::canonical(map).string(),before.string(),after.string()};
    std::vector<char *> argv;for(const auto &argument:arguments)argv.push_back(const_cast<char *>(argument.c_str()));argv.push_back(nullptr);
    pid_t child{};check(posix_spawn(&child,arguments.front().c_str(),nullptr,nullptr,argv.data(),environ)==0,"Cannot execute independent source codec receiver");
    int status{};check(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"Independent source codec receiver failed");
#endif
}
}
int main(int argc,char **argv)try{
    if(argc==6&&std::string_view(argv[1])=="--receive"){
        std::cout<<std::unitbuf;receive(argv[2],argv[3],argv[4],argv[5]);return 0;
    }
    std::cout<<std::unitbuf;if((argc!=3&&argc!=4)||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    run(argv[0],argv[1],argv[2],argc==4?argv[3]:nullptr);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
