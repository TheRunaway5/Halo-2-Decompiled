#include "halo2/source_authority_codec.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_authority_delta.hpp"
#include "halo2/authority_archive_protocol.hpp"
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
    std::uint32_t result_seconds{};unsigned budget_birth_ticks{};
    std::unique_ptr<MultiplayerObjectRuntime> world;std::unique_ptr<MultiplayerMatch> match;
    std::unique_ptr<SourceMatchAuthority> authority;
    Owner(const char *xbe,const char *map,std::uint32_t time):cache(map,{},OriginalData::load_xbe(xbe)),content(cache.load_content(0,true)){
        clock->time=time;result_seconds=100000u+time/1000u;world=std::make_unique<MultiplayerObjectRuntime>(xbe,cache,0,files.path,clock);
        UiVariant variant{"Slayer",0,{{0x59,5}}};auto rules=slayer_rules(variant);
        rules.weapon=cache.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
        // A finite complete default respawn/suicide/betrayal interval plus one
        // source second; actual lifecycle state is never supplied by the test.
        budget_birth_ticks=rules.respawn_ticks+rules.suicide_ticks+rules.betrayal_ticks+30;
        world->configure_session(source_game_variant(variant),1,30,rules.random_seed);
        match=std::make_unique<MultiplayerMatch>(content,0,cache.spawns(),rules,MultiplayerKingServices{},
            [this]{return result_seconds;},world.get());
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
    void step(const std::array<Controls,16> &input){clock->time+=33;++result_seconds;match->step(input);}
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
    const auto network_limits=source_authority_network_codec_limits();
    {
        const auto offline=encode_source_authority(saved);std::uint64_t declared{};
        for(unsigned i=0;i<8;++i)declared|=std::to_integer<std::uint64_t>(offline[16+i])<<(8*i);
        std::cerr<<"Measured source checkpoint: "<<offline.size()<<" encoded / "<<declared<<" declared decoded bytes\n";
        for(const auto &region:saved.world.world.bank.regions)if(region.bytes.size()>=1024*1024)
            std::cerr<<"Source region "<<std::hex<<region.address<<std::dec<<" bytes="<<region.bytes.size()
                <<" page_bytes="<<region.page_bytes<<" committed="<<std::count(region.committed.begin(),region.committed.end(),1)
                <<'/'<<region.committed.size()<<" owner="<<region.owner<<'\n';
        auto pilot_limits=AuthorityDeltaLimits{};pilot_limits.decoded_bytes=SourceAuthorityCodecLimits{}.decoded_bytes;
        const auto pilot_image=AuthorityArchiveImage::read(offline,pilot_limits);
        SourceAuthorityCodecStats pilot_stats;
        const auto pilot=decode_source_authority_raw(pilot_image.raw(),pilot_image.digest(),SourceAuthorityCodecLimits{},&pilot_stats);
        const auto &bank=pilot.world.world.bank;
        std::uint64_t bank_claim=bank.regions.size()*sizeof(SourceRuntimeMemory::CheckpointRegion)+
            SourceRuntimeMemory::restore_storage_overhead(bank);
        for(const auto &region:bank.regions)bank_claim+=region.bytes.size()+region.owner.size()+region.committed.size();
        std::cerr<<"Measured normalized native: "<<pilot_stats.native_claimed<<" bank="<<bank_claim
            <<" other="<<(pilot_stats.native_claimed-bank_claim)<<" metadata="<<pilot_stats.metadata_bytes
            <<" parserpeak="<<pilot_stats.working_peak<<"\n";
    }
    const auto encoded=encode_source_authority(saved,network_limits);
    const auto image=AuthorityArchiveImage::read(encoded,source_authority_network_delta_limits());
    const auto direct_image=encode_source_authority_image(saved,network_limits);
    check(direct_image.digest()==image.digest()&&direct_image.raw().size()==image.raw().size()&&
        std::ranges::equal(direct_image.raw(),image.raw())&&
        direct_image.encode(source_authority_network_delta_limits())==encoded,
        "Direct canonical serialization changed complete source ownership bytes");
    std::cout<<"Network source image input: "<<encoded.size()<<" encoded / "<<image.decoded_size()<<" canonical bytes\n";
    SourceAuthorityCodecStats measured;
    const auto canonical=decode_source_authority_raw(image.raw(),image.digest(),network_limits,&measured);
    auto staged_limit=network_limits;staged_limit.staged_bytes=measured.staged_claimed-1;
    rejects([&]{decode_source_authority_raw(image.raw(),image.digest(),staged_limit);});
    rejects([&]{decode_source_authority(encoded,staged_limit);});
    {
        SourceAuthorityCodecStats indexed_stats;
        const auto indexed=decode_source_authority_image(image,network_limits,&indexed_stats);
        check(encode_source_authority(indexed,network_limits)==encoded,
            "Immutable image decoder changed complete canonical owning state");
        check(indexed_stats.native_claimed==measured.native_claimed &&
            indexed_stats.staged_claimed==measured.staged_claimed &&
            indexed_stats.metadata_bytes==measured.metadata_bytes &&
            indexed_stats.working_peak+image.decoded_size()==measured.working_peak+128,
            "Immutable image decoder changed native guards or retained a redundant raw byte bank");
        auto staged=b.authority->prepare(indexed,binding);
        check(b.authority->commit(std::move(staged)) && a.world->memory().checkpoint()==b.world->memory().checkpoint(),
            "Immutable image decoder did not pass actual coupled native preparation/publication");
        rejects([&]{decode_source_authority_image(image,staged_limit);});
        auto bound=network_limits;bound.decoded_bytes=image.decoded_size()-1;
        rejects([&]{decode_source_authority_image(image,bound);});
        bound=network_limits;bound.allocated_bytes=1;
        rejects([&]{decode_source_authority_image(image,bound);});
        bound=network_limits;bound.container_elements=1;
        rejects([&]{decode_source_authority_image(image,bound);});
        bound=network_limits;bound.working_bytes=image.decoded_size();
        rejects([&]{decode_source_authority_image(image,bound);});
    }
    {
        const AuthorityArchiveDecodeLimits limits{network_limits.encoded_bytes,network_limits.decoded_bytes,
            network_limits.allocated_bytes,network_limits.container_elements,sizeof(SourceMatchAuthority::Checkpoint),network_limits.working_bytes};
        std::unique_ptr<ParsedAuthorityArchive> retained;const std::byte *retained_data{};
        {
            const auto caller=AuthorityArchiveImage::from_raw({image.raw().begin(),image.raw().end()},source_authority_network_delta_limits());
            retained_data=caller.raw().data();retained=parse_authority_archive_image(caller,limits);
            check(retained && retained->raw().data()==retained_data,
                "Immutable grammar index copied the actual canonical image");
        }
        check(retained->raw().data()==retained_data && std::ranges::equal(retained->raw(),image.raw()) &&
            retained->expected_hash()==image.digest() && retained->node(0).tag==13,
            "Immutable grammar index lost its exact byte owner after caller image destruction");
        const std::string_view name="version";
        check(std::ranges::equal(retained->name(1),std::as_bytes(std::span(name))) &&
            retained->name(retained->size()).empty() && retained->data(retained->size()).empty(),
            "Immutable grammar index changed checked name/data range semantics");
        auto caller=std::vector<std::byte>(image.raw().begin(),image.raw().end());
        const auto independent=parse_authority_archive_raw(caller,image.digest(),limits);
        check(independent && independent->raw().data()!=caller.data(),
            "Generic span recovery decoder lost its independent owning byte copy");
        std::fill(caller.begin(),caller.end(),std::byte{});
        check(std::ranges::equal(independent->raw(),image.raw()),
            "Generic span recovery decoder retained a mutable caller borrow");
        auto wrong_hash=image.digest();wrong_hash[0]^=std::byte{1};
        rejects([&]{decode_source_authority_raw(image.raw(),wrong_hash,network_limits);});
    }
    {
        auto malformed=std::vector<std::byte>(image.raw().begin(),image.raw().end());malformed[0]=std::byte{};
        const auto grammar=AuthorityArchiveImage::from_raw(std::move(malformed),source_authority_network_delta_limits());
        rejects([&]{decode_source_authority_image(grammar,network_limits);});
        malformed.assign(image.raw().begin(),image.raw().end());malformed[7]^=std::byte{1};
        const auto schema=AuthorityArchiveImage::from_raw(std::move(malformed),source_authority_network_delta_limits());
        rejects([&]{decode_source_authority_image(schema,network_limits);});
        auto corrupt=encoded;corrupt[corrupt.size()/2]^=std::byte{1};
        rejects([&]{AuthorityArchiveImage::read(corrupt,source_authority_network_delta_limits());});
    }
    check(encode_source_authority(canonical,network_limits)==encoded,"Raw network materialization changed owning state");
    std::cout<<"Encoded complete source cut: "<<encoded.size()<<" bytes; canonical: "<<image.decoded_size()
        <<" bytes; dynamic native: "<<measured.native_claimed<<" metadata: "<<measured.metadata_bytes
        <<" parser peak: "<<measured.working_peak<<" staged input: "<<measured.staged_claimed
        <<" network decode reservation: "<<network_limits.working_bytes<<" bytes\n";
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
    rejects([&]{encode_source_authority_image(saved,limits);});
    limits={};limits.allocated_bytes=1;rejects([&]{decode_source_authority(encoded,limits);});
    limits={};limits.container_elements=1;rejects([&]{decode_source_authority(encoded,limits);});
    const auto invalid_enum=invalid_state_enum(encoded);rejects([&]{decode_source_authority(invalid_enum);});
    const auto invalid_enum_image=AuthorityArchiveImage::read(invalid_enum,source_authority_network_delta_limits());
    rejects([&]{decode_source_authority_image(invalid_enum_image,network_limits);});
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
    // Measure an actual full16-seat source roster using the existing producer
    // after the deterministic four-seat/cross-process checks have completed.
    for(unsigned slot=4;slot<multiplayer_capacity;++slot){
        const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> appearance;
        std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
        const auto name="Budget seat "+std::to_string(slot);
        check(a.match->join(name,-1,native_selected_player_configuration(name,appearance,-1))==slot,
            "Full-roster network budget pilot could not admit an actual source player");
    }
    const auto all_bound=[&]{
        for(unsigned slot=0;slot<multiplayer_capacity;++slot){const auto &player=a.match->player(slot);const auto &unit=a.match->unit(slot);
            if(!player.connected||player.source_datum==null_datum||!player.source_current_unit||!unit.exists||
                unit.source_datum==null_datum||unit.source_player!=player.source_datum||player.source_current_object!=unit.source_datum)return false;
        }return true;
    };
    unsigned birth_ticks=0;
    for(;birth_ticks<a.budget_birth_ticks&&(birth_ticks<10||!all_bound());++birth_ticks){
        std::array<Controls,multiplayer_capacity> input{};
        for(unsigned slot=0;slot<multiplayer_capacity;++slot)input[slot].yaw=a.match->unit(slot).yaw;
        a.step(input);
    }
    std::cerr<<"16-seat body lifecycle: "<<birth_ticks<<'/'<<a.budget_birth_ticks<<" genuine source ticks\n";
    bool full_bodies=true;
    for(unsigned slot=0;slot<multiplayer_capacity;++slot){
        const auto &player=a.match->player(slot);const auto &unit=a.match->unit(slot);
        const bool bound=player.connected&&player.source_datum!=null_datum&&player.source_current_unit&&unit.exists&&
            unit.source_datum!=null_datum&&unit.source_player==player.source_datum&&player.source_current_object==unit.source_datum;
        if(!bound){
            std::cerr<<"16-seat body admission slot="<<slot<<" connected="<<player.connected<<" current="<<player.source_current_unit
                <<" exists="<<unit.exists<<" player="<<std::hex<<player.source_datum<<" object="<<player.source_current_object
                <<" unit="<<unit.source_datum<<" unit_player="<<unit.source_player<<std::dec
                <<" countdown="<<player.respawn.countdown<<" lives="<<player.source_lives_remaining<<'\n';
            full_bodies=false;continue;
        }
        const auto body=a.world->source_object_body(unit.source_datum);
        check(body&&a.world->memory().view(body,4).size()==4,"Full-roster budget pilot lacks actual source body backing");
    }
    check(full_bodies,"Full-roster budget pilot lacks an actual bound source unit");
    const auto maximum=encode_source_authority(a.authority->checkpoint(binding),network_limits);
    const auto maximum_image=AuthorityArchiveImage::read(maximum,source_authority_network_delta_limits());
    SourceAuthorityCodecStats maximum_stats;
    const auto maximum_cut=decode_source_authority_raw(maximum_image.raw(),maximum_image.digest(),network_limits,&maximum_stats);
    check(std::ranges::count_if(maximum_cut.match.players,[](const auto &player){return player.connected;})==multiplayer_capacity,
        "Full-roster budget pilot omitted source players");
    std::cout<<"PASS16-player network budget: "<<maximum.size()<<" encoded / "<<maximum_image.decoded_size()
        <<" canonical bytes; native="<<maximum_stats.native_claimed<<" staged="<<maximum_stats.staged_claimed<<'\n';
}
}
int main(int argc,char **argv)try{
    if(argc==6&&std::string_view(argv[1])=="--receive"){
        std::cout<<std::unitbuf;receive(argv[2],argv[3],argv[4],argv[5]);return 0;
    }
    std::cout<<std::unitbuf;if((argc!=3&&argc!=4)||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    run(argv[0],argv[1],argv[2],argc==4?argv[3]:nullptr);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
