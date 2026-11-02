#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include "halo2/mcc_sound_vanilla_reference.hpp"
#include "halo2/mcc_sound_class_reference.hpp"
#include "halo2/mcc_sound_timing_reference.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/audio.hpp"
#include "halo2/source_runtime_resources.hpp"
#include "halo2/mcc_sound_dat_resources_tu15.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "audio/mcc_sound_variation_provider_internal.hpp"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <thread>
#include <set>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void need(bool yes,const char* why){if(!yes)throw FormatError(why);}
std::array<std::byte,4> encoded(std::uint32_t n){
    return {std::byte(n),std::byte(n>>8),std::byte(n>>16),std::byte(n>>24)};
}
void same_definition(const SoundVariationDefinition &a,const SoundVariationDefinition &b) {
    need(a.datum==b.datum && a.sound==b.sound && a.playback==b.playback && a.sound_class==b.sound_class &&
        a.range==b.range && a.scale==b.scale && a.permutations==b.permutations && a.used==b.used &&
        a.sample_rate_index==b.sample_rate_index && a.pitch_ranges.size()==b.pitch_ranges.size(),
        "Immutable canonical sound publication differs from the actual converted provider");
    for(unsigned n=0;n<a.pitch_ranges.size();++n)
        need(a.pitch_ranges[n].interval==b.pitch_ranges[n].interval && a.pitch_ranges[n].permutations==b.pitch_ranges[n].permutations,
            "Immutable canonical sound pitch interval differs from the actual converted provider");
}
struct CountedMemory final:SourcePhysicsContactMemory {
    SourceRuntimeMemory bank;
    mutable std::atomic<unsigned> reads{};
    std::atomic<unsigned> writes{};
    mutable std::function<void()> on_read;
    mutable std::optional<std::uint32_t> denied;
    void read(std::uint32_t at,std::span<std::byte> bytes)const override{
        ++reads;
        if(on_read){auto callback=std::exchange(on_read,{});callback();}
        if(denied&&at<=*denied&&std::uint64_t(*denied)<std::uint64_t(at)+bytes.size())
            throw std::out_of_range("actual native metadata owner unavailable");
        bank.read(at,bytes);
    }
    void write(std::uint32_t at,std::span<const std::byte> bytes)override{++writes;bank.write(at,bytes);}
};
struct Fixture {
    const Cache& cache;
    std::optional<MccSoundRuntimePlan> sound;
    MccRuntimeCacheDelivery delivery;
    std::shared_ptr<CountedMemory> memory=std::make_shared<CountedMemory>();
    std::shared_ptr<MccSoundTimingMemoryOwner> owner;
    std::unique_ptr<SourceRuntimeResources> resources;
    std::uint32_t globals{},primary{},legacy{};
    Fixture(const Cache& c):cache(c),delivery(MccRuntimeCacheDelivery::plan(c,0x90000000u,0x60000000u,
        [this](MccSoundRuntimePlan&& p){sound.emplace(std::move(p));})){
        need(sound.has_value(),"Actual canonical delivery lost its sound plan");
        const auto tags=delivery.tag_image();
        std::vector<std::byte> bytes(tags.reservation_bytes);
        const auto put=[&](const auto& region){
            need(region.address>=tags.index&&std::uint64_t(region.address-tags.index)+region.bytes.size()<=bytes.size(),
                 "Actual delivery image escapes its native allocation");
            std::ranges::copy(region.bytes,bytes.begin()+region.address-tags.index);
        };
        for(const auto& region:tags.regions)put(region);
        for(std::size_t n=0;n<c.bsps().size();++n)put(delivery.bsp_image(n).region);
        memory->bank.mount(tags.index,bytes,SourceRuntimeMemory::Access::ReadWrite,
            "actual canonical MCC delivery metadata test allocation");
        const auto address=[&](TagHandle h){return Bytes(memory->bank.view(tags.table+h.index()*16u,16)).u32(8);};
        globals=address({tags.globals});const Bytes g(memory->bank.view(globals+0xc0u,8));
        const Bytes sg(memory->bank.view(g.u32(4),36));primary=address({sg.u32(32)});legacy=address({sg.u32(4)});
        for(const auto [at,value]:std::array{std::pair{0x556d90u,tags.index},std::pair{0x4f0414u,tags.table},
                std::pair{0x4ecc1cu,globals},std::pair{0x52d664u,primary}})
            memory->bank.mount(at,encoded(value),SourceRuntimeMemory::Access::ReadWrite,
                "literal canonical metadata publication binding test");
        owner=std::make_shared<MccSoundTimingMemoryOwner>(memory);
        resources=std::make_unique<SourceRuntimeResources>(cache);
    }
    MccSoundVariationProvider provider()const{
        return detail::make_mcc_sound_variation_provider(cache,cache,delivery,*sound,*resources,owner);
    }
    std::uint32_t native(TagHandle h)const{return Bytes(memory->bank.view(delivery.index().table+h.index()*16u,16)).u32(8);}
};
// The caller supplies a distinct-inode clone below the active scratch owner.
// Renaming or editing installation/reference DAT owners is never permitted.
void retained_resources(Fixture& f,const MccSoundVariationProvider& provider,
    const std::filesystem::path& clone,nlohmann::json& report) {
    namespace fs=std::filesystem;const auto& cache=f.cache;const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
    need(scratch&&fs::is_directory(clone),"Retained-resource test needs its active owned clone");
    const auto root=fs::canonical(clone),owner=fs::canonical(scratch);
    need(root!=owner&&!root.lexically_relative(owner).empty()&&*root.lexically_relative(owner).begin()!="..",
        "Retained-resource clone is outside its scratch owner");
    need(!cache.original_sound_catalog(),"Retained prefix control requires an ordinary pre-catalog Cache");
    std::optional<TagHandle> selected;std::vector<EncodedSound> expected;std::optional<MccSoundResourceRead> first;
    for(const auto& sound:f.sound->sounds) {
        if(sound.source_flags&0x800u)continue;
        const Bytes header(f.memory->bank.view(sound.native_root,20));if(header.slice(10,1)[0]!=std::byte{1})continue;
        const Bytes range(f.memory->bank.view(f.sound->blocks[4].address+header.u16(8)*12u,12));
        if(!range.u16(10)||range.u16(10)>32)continue;
        const auto definition=provider.load(cache,sound.sound);
        if(std::to_integer<unsigned>(definition.sound[2])>=sound_class_count)continue;
        SoundLocalRandom probe(0x5821b36du);
        if(source_sound_gain(definition,probe)+source_sound_control_gain(definition,1)<=-64.f)continue;
        const auto pitch=source_sound_control_pitch(definition,float(source_sound_pitch(definition,probe)),1);
        if(source_sound_pitch_range(definition.pitch_ranges,-1,pitch)!=0)continue;
        std::vector<EncodedSound> candidates;bool decodable=true;
        for(unsigned p=0;p<definition.permutations.size();++p){auto encoded=cache.load_sound(sound.sound.value,0,p);
            try{if(!decode_sound(encoded).frames())decodable=false;}catch(const FormatError&){decodable=false;}
            candidates.push_back(std::move(encoded));}
        if(!decodable)continue;selected=sound.sound;expected=std::move(candidates);break;
    }
    need(selected&&!expected.empty(),"Retained-resource test has no independently decodable sound");
    const auto dat=f.resources->sound_dat_owner();
    (void)cache.load_sound(selected->value,0,0,[&](const auto& request){if(!first)first=request;return dat->read_encoded(cache,request);});
    std::set<fs::path> paths;for(const auto& sound:f.sound->sounds){const auto bank=sound.file==MccSoundDataFile::English?MccResourceBank::EnglishSounds:
        sound.file==MccSoundDataFile::Remastered?MccResourceBank::RemasteredSounds:MccResourceBank::NeutralSounds;paths.insert(cache.mcc_resource_bank_path(sound.sound,bank));}
    struct Hidden {
        std::vector<std::pair<fs::path,fs::path>> files;
        void restore(){while(!files.empty()){fs::rename(files.back().second,files.back().first);files.pop_back();}}
        ~Hidden(){for(auto i=files.rbegin();i!=files.rend();++i){std::error_code ignored;fs::rename(i->second,i->first,ignored);}}
    } hidden;
    for(const auto& path:paths){
        const auto relative=fs::canonical(path).lexically_relative(root);
        need(!relative.empty()&&*relative.begin()!=".."&&fs::hard_link_count(path)==1,"DAT mutation escaped its private inode owner");
        const fs::path target=path.string()+".retained-test-hidden";need(!fs::exists(target),"Owned DAT hiding path already exists");
        fs::rename(path,target);hidden.files.emplace_back(path,target);}
    unsigned controls{};const auto reject=[&](auto&& call){bool refused=false;try{call();}catch(const std::exception&){refused=true;}need(refused,"Retained-resource control was admitted");++controls;};
    reject([&]{(void)cache.load_sound(selected->value,0,0);});
    const auto unavailable_provider=f.provider();
    for(unsigned p=0;p<expected.size();++p){const auto actual=unavailable_provider.encoded(cache,*selected,0,p);const auto& before=expected[p];
        need(actual.data==before.data&&actual.codec==before.codec&&actual.sample_rate==before.sample_rate&&actual.channels==before.channels&&
            actual.samples==before.samples&&actual.packet_chunks.size()==before.packet_chunks.size(),"Retained provider reopened or changed unavailable DAT data");
        const auto a=decode_sound(actual),b=decode_sound(before);need(a.rate==b.rate&&a.stereo==b.stereo,"Unavailable DAT changed decoded PCM");}
    const Cache foreign=cache;reject([&]{(void)dat->read_encoded(foreign,*first);});
    std::optional<MultiplayerAudioSubmission> packet;const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
    MultiplayerAudio audio(cache,0x5821b36du,manual,[&](const auto& value){packet=value;});audio.bind_native_sound_metadata(unavailable_provider);audio.listeners(std::array{Vec3{}});
    need(bool(audio.play_impulse(selected->value))&&packet&&packet->pitch_range==0&&packet->permutation<expected.size(),"Uncached Audio failed with unavailable DAT paths");
    const auto pcm=decode_sound(expected[packet->permutation]);need(packet->frames==pcm.frames(),"Unavailable DAT Audio changed its PCM extent");
    const auto mixed=audio.render(256);need(mixed.size()==512&&std::ranges::all_of(mixed,[](auto v){return std::isfinite(v);}),"Unavailable DAT Audio returned invalid PCM");audio.stop();
    hidden.restore();
    {
        const auto chunk=std::ranges::find_if(f.sound->chunks,[&](const auto& c){return c.gestalt==first->gestalt&&c.source_address==first->descriptor;});
        need(chunk!=f.sound->chunks.end(),"Encoded mutation control lost its actual native descriptor");
        const Bytes row(f.memory->bank.view(chunk->native_address,12));const auto size=row.u32(4)&0x3fffffffu;
        const SourceSoundCacheReadTu15 request{chunk->native_address,row.u32(0),row.u32(4),(size+511u)&~511u};
        const auto sector=f.resources->sound_cache(request);const auto path=cache.mcc_resource_bank_path(first->sound,first->bank);
        std::fstream file(path,std::ios::in|std::ios::out|std::ios::binary);need(bool(file),"Cannot open private encoded-byte control");
        char old{};file.seekg(first->file_offset);file.read(&old,1);need(bool(file),"Cannot read private encoded-byte control");
        struct ByteRestore {std::fstream& file;std::uint32_t at;char byte;~ByteRestore(){file.clear();file.seekp(at);file.put(byte);file.flush();}} restore_byte{file,first->file_offset,old};
        file.seekp(first->file_offset);file.put(old^1);file.flush();need(bool(file),"Cannot write private encoded-byte control");
        need(cache.load_sound(selected->value,0,0).data!=expected.front().data,"Independent Cache reader did not observe the actual DAT mutation");
        need(provider.encoded(cache,*selected,0,0).data==expected.front().data&&f.resources->sound_cache(request)==sector,
            "An in-place DAT mutation changed retained PCM or native sector bytes");
    }
    {
        Fixture matching_owner(cache);const auto matching_provider=matching_owner.provider();
        need(matching_owner.resources->identity()==f.resources->identity()&&matching_owner.resources->sound_dat_owner()!=dat,
            "Independent equal-byte resource owners did not retain equal content identity");
        detail::require_mcc_sound_variation_resources(matching_provider,&provider,cache);
        reject([&]{detail::require_mcc_sound_variation_world_resources(provider,cache,*matching_owner.resources);});
    }
    // Append outside every previously admitted sector, preserving all encoded
    // payloads. Capture a real changed file owner without inventing a digest,
    // resource identity, native readiness or lease, then restore its extent.
    const auto path=cache.mcc_resource_bank_path(first->sound,first->bank);const auto extent=fs::file_size(path);
    struct ExtentRestore {fs::path path;std::uintmax_t size;~ExtentRestore(){std::error_code ignored;fs::resize_file(path,size,ignored);}} restore{path,extent};
    std::ofstream file(path,std::ios::binary|std::ios::app);file.put(char(0xa5));file.close();
    need(bool(file)&&fs::file_size(path)==extent+1,"Cannot append the owned DAT mutation control");
    Fixture changed_owner(cache);auto changed_provider=changed_owner.provider();
    need(changed_owner.resources->identity()!=f.resources->identity(),"Actual changed DAT did not change the complete resource identity");
    const auto reads=changed_owner.memory->reads.load();
    reject([&]{detail::require_mcc_sound_variation_resources(changed_provider,&provider,cache);});
    need(changed_owner.memory->reads==reads,"Resource preflight read native bytes");
    reject([&]{detail::require_mcc_sound_variation_world_resources(provider,cache,*changed_owner.resources);});
    detail::require_mcc_sound_variation_world_resources(provider,cache,*f.resources);
    f.owner->rebind(f.memory);reject([&]{(void)provider.encoded(cache,*selected,0,0);});
    const auto fresh=f.provider();detail::require_mcc_sound_variation_resources(fresh,&provider,cache);
    reject([&]{detail::require_mcc_sound_variation_resources(changed_provider,&provider,cache);});
    report.update(nlohmann::json{{"passed",true},{"selected_sound",selected->value},{"permutations",expected.size()},{"hidden_dat_files",paths.size()},
        {"unavailable_paths_uncached_audio_verified",true},{"unavailable_paths_new_provider_verified",true},{"retained_pcm_byte_equal",true},{"actual_changed_dat_preflight_refused",true},
        {"same_resources_stale_saved_epoch_admitted",true},{"foreign_world_resources_refused",true},{"negative_controls",controls},
        {"equal_bytes_distinct_dat_preflight_admitted",true},
        {"encoded_byte_mutation_isolated",true},{"typed_sector_mutation_isolated",true},
        {"native_world_or_source_voice_verified",false},{"physical_device_or_waveform_verified",false}});
}
}
int main(int argc,char**argv)try{
    if(argc==1 || (argc>1 && (!*argv[1] || !std::filesystem::is_regular_file(argv[1]))))return 77;
    if(argc!=2&&argc!=3&&argc!=5)return 2;
    const auto cache_owner=std::make_shared<const Cache>(argv[1]);const auto &cache=*cache_owner;
    Fixture f(cache);auto provider=f.provider();
    if(argc==5){need(std::string_view(argv[2])=="--retained-resources","Unknown retained resources mode");nlohmann::json report;
        retained_resources(f,provider,argv[3],report);std::ofstream output(argv[4]);output<<report.dump(2)<<'\n';need(bool(output),"Cannot write retained-resource report");std::cout<<report.dump()<<'\n';return 0;}
    const auto publication=MccSoundMetadataPublication::create(cache_owner);
    nlohmann::json report{{"map",cache.name()},{"scope","Actual MCC canonical delivery/native metadata parser and epoch controls; explicit publication fixture, not original gameplay or Audio admission."}};
    unsigned assertions{},loaded{},unsupported{},empty{},corrected{},class_matches{},pcm_order{},pcm_pairs{},immutable_matches{},lift_pitches{},permutation_time_witnesses{};
    std::set<unsigned> decoded_codecs;
    report["pcm_cases"]=nlohmann::json::array();
    report["count32_cases"]=nlohmann::json::array();
    const auto flags=plan_mcc_sound_vanilla_flags(cache);
    if(argc==3){
        need(std::string_view(argv[2])=="--flags","Unknown provider test mode");
        report["corrections"]=nlohmann::json::array();
        for(const auto& correction:flags.corrections){
            const auto* t=cache.find_tag(correction.sound,TagGroup{"snd!"});
            const auto native=f.native(correction.sound);const Bytes h(f.memory->bank.view(native,20));
            need(t&&h.u16(0)==correction.native_after,"Finite authored native flag correction was lost");
            const auto definition=provider.load(cache,correction.sound);
            need(Bytes(definition.sound).u16(0)==correction.native_after,"Provider returned raw donor flags");
            const auto* owner=cache.find_tag(correction.gestalt,TagGroup{"ugh!"});need(owner,"Missing corrected full-salt gestalt");
            report["corrections"].push_back({{"sound",correction.sound.value},{"gestalt",correction.gestalt.value},
                {"sound_name",t->name},{"gestalt_name",owner->name},{"source_flags",correction.source_flags},
                {"native_before",correction.native_before},{"native_after",correction.native_after},
                {"provider_native_flags_exact",true}});
        }
        report["native_sound_roots"]=f.sound->sounds.size();report["correction_count"]=flags.corrections.size();
        report["source_writes"]=f.memory->writes.load();std::cout<<report.dump(2)<<'\n';return 0;
    }
    report["unsupported_count_cases"]=nlohmann::json::array();
    const auto* globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);need(globals,"No actual raw globals");
    const auto rawg=cache.read_tag_bytes(globals->address,0xc8);const Bytes rawsg(cache.read_tag_bytes(Bytes(rawg).u32(0xc4),0x2c));
    const auto* legacy=cache.find_tag({rawsg.u32(12)},TagGroup{"sncl"});need(legacy,"No actual Legacy sncl owner");
    const Bytes rawclass(cache.read_tag_bytes(legacy->address,8));
    const auto raw_classes=cache.read_tag_bytes(rawclass.u32(4),std::size_t(rawclass.u32(0))*0x78u);
    auto expected_classes=project_mcc_sound_class_records(raw_classes);
    MccSoundRuntimeSource class_source;class_source.tags=cache.tags();
    class_source.read=[&](auto at,auto size){return cache.read_tag_bytes(at,size);};
    const auto class_reference=project_mcc_sound_class_reference(class_source,legacy->handle(),expected_classes);
    need(class_reference.status!=MccSoundClassReferenceStatus::CandidateMismatch,
         "Actual Legacy class bank differs from its complete authenticated projection");
    if(class_reference.status!=MccSoundClassReferenceStatus::UnrecognizedSource) {
        need(class_reference.owner==legacy->handle() && class_reference.native_records.size()==expected_classes.size(),
             "Authenticated class projection changed its complete Legacy owner");
        expected_classes=class_reference.native_records;
    }
    report["legacy_class_owner"]=legacy->datum;
    report["legacy_class_source_sha256"]=class_reference.source_bank_sha256;
    report["legacy_class_restored_words"]=class_reference.changed_words;
    const auto timing=build_mcc_sound_timing_reference(cache,*f.sound,
        [&](auto at,auto size){return f.delivery.runtime_address(at,size);});
    std::optional<TagHandle> selected,persistent;
    for(const auto& s:f.sound->sounds){
        const Bytes header(f.memory->bank.view(s.native_root,20));need(header.u16(0)==s.native_flags,"Canonical native flags differ");
        if(std::ranges::any_of(flags.corrections,[&](const auto& x){return x.sound==s.sound;}))++corrected;
        if(header.slice(10,1).front()==std::byte{}){++empty;continue;}
        const auto d= [&]()->std::optional<SoundVariationDefinition>{
            try{return provider.load(cache,s.sound);}catch(const FormatError&e){
                if(std::string(e.what())=="Unrecovered native sound permutation count"){
                    const auto first=header.u16(8);const auto count=Bytes(f.memory->bank.view(f.sound->blocks[4].address+first*12u,12)).u16(10);
                    need(count<1||count>32,"Provider rejected a supported permutation count");
                    report["unsupported_count_cases"].push_back({{"sound",s.sound.value},{"pitch",0},{"actual_count",count},{"rejected",true}});
                    ++unsupported;return {};
                }
                throw;
            }
        }();
        if(!d)continue;
        same_definition(*d,publication.load(cache,s.sound));++immutable_matches;
        const auto *identity=cache.find_tag(s.sound,TagGroup{"snd!"});need(identity,"Immutable comparison lost its sound identity");
        if(identity->name.find("midship_grav_lift")!=std::string::npos || identity->name.find("midship_activate_lift")!=std::string::npos)
            for(unsigned pitch=0;pitch<d->pitch_ranges.size();++pitch) {
                same_definition(provider.load(cache,s.sound,pitch),publication.load(cache,s.sound,pitch));++lift_pitches;
            }
        ++loaded;if(!selected)selected=s.sound;
        need(d->datum==s.sound.value&&Bytes(d->sound).u16(0)==s.native_flags,"Provider lost native full-salt header");
        const auto class_index=unsigned(std::to_integer<std::uint8_t>(d->sound[2]));
        need(class_index<rawclass.u32(0) && std::ranges::equal(d->sound_class,
             std::span(expected_classes).subspan(std::size_t(class_index)*92,92)),
             "Provider differs from the exact authenticated projected Legacy class");++class_matches;
        const auto& gestalt=cache.mcc_sound_gestalt_owner(s.sound);const Bytes rawroot(cache.read_tag_bytes(gestalt.address,0x78));
        const auto* rawtag=cache.find_tag(s.sound,TagGroup{"snd!"});need(rawtag,"Missing raw sound owner");
        const Bytes raws(cache.read_tag_bytes(rawtag->address,20));
        const auto source_range=unsigned(raws.u16(8));
        const Bytes range(cache.read_tag_bytes(rawroot.u32(0x2c)+source_range*12u,12));
        need(range.u16(10)==d->permutations.size(),"Native/Cache relative permutation counts differ");
        for(unsigned n=0;n<d->permutations.size();++n){
            const auto raw=cache.read_tag_bytes(rawroot.u32(0x34)+(unsigned(range.u16(8))+n)*16u,16);
            need(std::ranges::equal(std::span(d->permutations[n]).subspan(2,4),raw.subspan(2,4)),
                 "Native/Cache relative permutation chance/gain/dialogue order differs");
            const auto references=timing.permutation_references();
            const auto reference=std::ranges::find_if(references,[&](const auto& value){return
                value.sound==s.sound && value.relative_pitch==0 && value.relative_permutation==n;});
            if(reference!=references.end()) {
                need(reference->expected_native_permutation==d->permutations[n] &&
                     Bytes(d->permutations[n]).u16(6)==reference->original_ms,
                     "Provider permutation differs from its complete authenticated TU1.5 timing row");
                ++permutation_time_witnesses;
            } else need(Bytes(d->permutations[n]).u16(6)==Bytes(raw).u16(6),
                "Provider changed permutation milliseconds without an authenticated original witness");
            ++pcm_order;
        }
        if(d->permutations.size()>1&&!d->used.empty()&&!persistent)persistent=s.sound;
        if(d->permutations.size()==32){
            need(d->used.size()==4,"Actual32-permutation Legacy bank lost its four used bytes");
            need(cache.load_sound_variation(s.sound.value).permutations.size()==32,
                 "Ordinary Cache metadata still rejects the proven32 domain");
            auto state=imported_sound_range_state(*d);SoundLocalRandom random(1);
            need(state.last==-1&&state.forced==-1,"Fresh actual count32 range is not cold");
            need(source_sound_permutation(*d,state,random)==7&&state.last==7&&state.forced==-1&&
                 state.used==0x80u&&random.seed==0x5e8885dbu&&random.stores.size()==2,
                 "Actual32 metadata differs from original TU1.5 ordinary selector witness");
            report["count32_cases"].push_back({{"sound",s.sound.value},{"pitch",0},{"actual_count",32},
                {"selected",7},{"used_word",state.used},{"seed",random.seed},{"draws",random.stores.size()}});
        }
        const bool first_codec=decoded_codecs.insert(s.codec.compression).second;
        if(first_codec||d->permutations.size()==32){
            for(const unsigned p:std::set<unsigned>{0u,unsigned(d->permutations.size()-1)}){
                const auto before=cache.load_sound(s.sound.value,0,p);const auto pcm_before=decode_sound(before);
                const auto metadata=provider.load(cache,s.sound);
                const auto after=cache.load_sound(s.sound.value,0,p);const auto pcm_after=decode_sound(after);
                need(before.data==after.data&&before.sample_rate==after.sample_rate&&before.channels==after.channels&&
                     before.codec==after.codec&&before.samples==after.samples&&before.packet_chunks.size()==after.packet_chunks.size(),
                     "Native metadata provider changed actual Cache DAT/resource identity");
                for(unsigned n=0;n<before.packet_chunks.size();++n){
                    const auto& a=before.packet_chunks[n];const auto& b=after.packet_chunks[n];
                    need(a.bytes==b.bytes&&a.pcm_bytes==b.pcm_bytes&&a.reset_after==b.reset_after,
                         "Native metadata provider changed actual MCC packet ownership");
                }
                need(metadata.permutations[p]==d->permutations[p]&&pcm_before.rate==pcm_after.rate&&
                     pcm_before.stereo.size()==pcm_after.stereo.size()&&pcm_before.frames()&&
                     std::memcmp(pcm_before.stereo.data(),pcm_after.stereo.data(),pcm_before.stereo.size()*sizeof(float))==0,
                     "Native metadata provider changed the selected actual PCM permutation");
                ++pcm_pairs;report["pcm_cases"].push_back({{"sound",s.sound.value},{"pitch",0},{"permutation",p},
                    {"codec",before.codec},{"encoded_bytes",before.data.size()},{"pcm_frames",pcm_before.frames()},
                    {"sample_rate",before.sample_rate},{"actual_dat_and_pcm_byte_equal",true}});
            }
        }
        ++assertions;
    }
    need(selected.has_value()&&loaded&&class_matches==loaded,"No actual native metadata cases");
    const auto reject=[&](auto callback,const char* why,bool no_reads=false){
        const auto before=f.memory->reads.load();bool rejected{};try{callback();}catch(const std::exception&){rejected=true;}
        need(rejected,why);if(no_reads)need(f.memory->reads==before,"Rejected provider read before owner/identity admission");++assertions;
    };
    const auto sound=*selected;
    f.memory->reads=0;provider.require_current(cache);need(f.memory->reads==0,"Current provider check read metadata");++assertions;
    reject([&]{(void)provider.load(cache,{});},"NONE sound admitted",true);
    reject([&]{(void)provider.load(cache,{sound.value^0x10000u});},"Low16 alias admitted",true);
    reject([&]{(void)provider.load(cache,cache.globals_tag());},"Wrong tag group admitted",true);
    reject([&]{(void)provider.load(cache,sound,256);},"Invalid relative pitch admitted");
    const Cache foreign=cache;
    reject([&]{provider.require_current(foreign);},"Foreign Cache current check admitted",true);
    reject([&]{(void)provider.load(foreign,sound);},"Foreign Cache parsed metadata",true);
    const auto before_thread=f.memory->reads.load();std::atomic<unsigned> thread_rejects{};
    std::thread other([&]{try{provider.require_current(cache);}catch(const std::exception&){++thread_rejects;}
        try{(void)provider.load(cache,sound);}catch(const std::exception&){++thread_rejects;}});other.join();
    need(thread_rejects==2&&f.memory->reads==before_thread,"Foreign thread read native metadata");assertions+=2;
    const auto lease=capture_mcc_sound_timing_read_lease(f.owner);
    need(with_mcc_sound_timing_owned_read(lease,[&](const auto&){
        reject([&]{provider.require_current(cache);},"Recursive current check admitted",true);
        reject([&]{(void)provider.load(cache,sound);},"Recursive metadata parse admitted",true);
    })==MccSoundTimingReadStatus::Executed,"Outer actual read lease failed");
    // Strong local provider state survives destruction of its last public
    // wrapper from a real memory read callback, without a borrowed World.
    std::optional<MccSoundVariationProvider> last=f.provider();f.memory->on_read=[&]{last.reset();};
    const auto kept=last->load(cache,sound);need(!last&&kept.datum==sound.value,"Last-wrapper reset invalidated active parse");++assertions;
    const auto mutate=[&](std::uint32_t at,std::span<const std::byte> replacement,auto check){
        const auto old=f.memory->bank.view(at,replacement.size());const std::vector<std::byte> before(old.begin(),old.end());
        f.memory->bank.write(at,replacement);try{check();}catch(...){f.memory->bank.write(at,before);throw;}f.memory->bank.write(at,before);
    };
    const auto root=f.native(sound),index=f.delivery.index().table+sound.index()*16u;
    mutate(index+4,encoded(sound.value^0x10000u),[&]{reject([&]{(void)provider.load(cache,sound);},"Changed published salt admitted");});
    mutate(0x4f0414u,encoded(f.delivery.index().table+16u),[&]{reject([&]{(void)provider.load(cache,sound);},"Changed namespace pointer admitted");});
    mutate(root,encoded(Bytes(f.memory->bank.view(root,4)).u32(0)^1u),[&]{reject([&]{(void)provider.load(cache,sound);},"Changed native header admitted");});
    const Bytes sh(f.memory->bank.view(root,20));const auto range_at=f.sound->blocks[4].address+unsigned(sh.u16(8))*12u;
    const Bytes range(f.memory->bank.view(range_at,12));
    need(unsigned(range.u16(8))+unsigned(range.u16(10))+1u<=unsigned(f.sound->blocks[5].count),
         "In-bounds permutation remapping control is not actually in bounds");
    for(const unsigned offset:{2u,6u,8u,10u}){
        auto change=std::array{std::byte(range.u16(offset)+1u),std::byte((range.u16(offset)+1u)>>8)};
        mutate(range_at+offset,change,[&]{reject([&]{(void)provider.load(cache,sound);},"Changed immutable range/PCM mapping admitted");});
    }
    const auto permutation=f.sound->blocks[5].address+unsigned(range.u16(8))*16u;
    mutate(permutation+4,std::array{std::byte{0x7f}},[&]{reject([&]{(void)provider.load(cache,sound);},"Changed permutation/PCM association admitted");});
    f.memory->denied=f.legacy;reject([&]{(void)provider.load(cache,sound);},"Unavailable class owner admitted");f.memory->denied.reset();
    if(persistent){
        const auto p=*persistent;const auto native=f.native(p);const Bytes h(f.memory->bank.view(native,20));
        const auto at=f.sound->blocks[4].address+unsigned(h.u16(8))*12u;
        const auto prior=provider.load(cache,p);mutate(at+4,std::array{std::byte{0},std::byte{1}},[&]{
            const auto current=provider.load(cache,p);need(current.range[4]==std::byte{}&&current.range[5]==std::byte{1},"Provider froze live last/forced state");
            need(prior.range!=current.range,"Owned range snapshot changed without a reload");++assertions;
        });
        const auto flags_at=f.sound->blocks[7].address+(Bytes(prior.range).u16(6)>>3);
        mutate(flags_at,std::array{std::byte(std::to_integer<unsigned>(prior.used.front())^0x80u)},[&]{
            const auto current=provider.load(cache,p);need(current.used.front()!=prior.used.front(),"Provider froze actual live used bits");++assertions;
        });
    }
    f.owner->invalidate();reject([&]{provider.require_current(cache);},"Invalidated cached metadata remained current",true);
    reject([&]{(void)provider.load(cache,sound);},"Invalidated provider parsed",true);
    f.owner->rebind(f.memory);reject([&]{(void)provider.load(cache,sound);},"Identical-byte rebind revived old epoch",true);
    auto fresh=f.provider();need(fresh.load(cache,sound).sound==kept.sound,"Fresh identical-byte provider changed metadata");++assertions;
    f.owner->invalidate();f.owner.reset();reject([&]{fresh.require_current(cache);},"Outlived producer provider remained current",true);
    need(f.memory->writes==0,"Read-only provider wrote source metadata");
    report["native_sound_roots"]=f.sound->sounds.size();report["loaded_pitch0_definitions"]=loaded;
    report["immutable_publication_exact_definitions"]=immutable_matches;report["immutable_lift_pitch_matches"]=lift_pitches;
    report["native_flag_corrections_verified"]=corrected;report["legacy_class_rows_exact"]=class_matches;
    report["relative_permutation_metadata_order_exact"]=pcm_order;report["unsupported_count_definitions"]=unsupported;
    report["authenticated_permutation_time_rows_exact"]=permutation_time_witnesses;
    report["actual_dat_pcm_permutation_pairs_exact"]=pcm_pairs;
    report["empty_pitch_definitions"]=empty;report["assertions"]=assertions;report["source_writes"]=f.memory->writes.load();
    report["state_scope"]="Range last/forced/used copied at load, not a continuing alias or World selection/RNG writeback.";
    std::cout<<report.dump(2)<<'\n';
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
