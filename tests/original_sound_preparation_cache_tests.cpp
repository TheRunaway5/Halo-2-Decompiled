#include "halo2/cache.hpp"
#include "halo2/mcc_original_sound_runtime_delivery_tu15.hpp"
#include "halo2/original_data.hpp"
#include "halo2/physics_contact_record.hpp"
#include "halo2/source_runtime_mcc_preparation.hpp"
#include "halo2/source_runtime_resource_preparation.hpp"
#include "halo2/source_runtime_resources.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "audio/mcc_sound_variation_provider_internal.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace {
using namespace halo2;using J=nlohmann::json;
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
std::string hash(std::span<const std::byte> data){
    std::array<unsigned char,32> result;unsigned size{};
    require(EVP_Digest(data.data(),data.size(),result.data(),&size,EVP_sha256(),nullptr)==1&&size==32,"Cache qualification digest failed");
    constexpr char hex[]="0123456789abcdef";std::string out;for(auto b:result){out+=hex[b>>4];out+=hex[b&15];}return out;
}
template<class F>void rejects(F&& call,std::uint64_t& controls){bool refused=false;try{call();}catch(const FormatError&){refused=true;}
    require(refused,"Invalid original preparation/resource owner was admitted");++controls;}
struct Names {
    std::map<std::uint32_t,std::pair<std::uint32_t,std::string>> original;
    std::map<std::uint32_t,std::string> native;
    void bind(std::uint32_t old,std::uint32_t id,const std::string& name){
        const auto [i,added]=original.emplace(old,std::pair{id,name});require(added||i->second==std::pair{id,name},"Actual Cache import aliases original identities");
        const auto [n,unique]=native.emplace(id,name);require(unique||n->second==name,"Actual Cache import aliases native StringIDs");
    }
};
struct Inputs {
    std::vector<OriginalSoundRuntimeBindingTu15> bindings;
    std::shared_ptr<Names> names=std::make_shared<Names>();
    MccOriginalSoundRuntimeInputTu15 view()const{
        const auto retained=names;
        return {bindings,{retained,[retained](auto old,auto label){const auto& n=retained->original.at(old);require(n.second==label,"Original import label changed");return n.first;},
            [retained](auto id){return retained->native.at(id);}}};
    }
    Inputs(const Cache& cache,const J& catalog,const std::string& map){
        for(const auto& entry:catalog.at("cases"))if(entry.at("map")==map){
            const auto bank=PreparedOriginalSoundBank::load(entry.at("directory").get<std::string>(),entry.at("sha256").get<std::string>());
            const auto& m=bank->metadata();const Tag* tag{};
            for(const auto& t:cache.tags())if(t.address&&t.group=="snd!"&&t.name==m.name){require(!tag,"Actual Cache original target path is ambiguous");tag=&t;}
            require(tag,"Actual Cache original target is missing");
            const Bytes snd(cache.read_tag_bytes(tag->address,28));require(!(snd.u16(0)&0x800u),"Actual Cache original target is not Classic");
            const auto& owner=cache.mcc_sound_gestalt_owner(tag->handle());const Bytes g(cache.read_tag_bytes(owner.address,120));
            const auto name=[&](unsigned index){require(index<g.u32(24),"Actual Cache name ordinal escapes its gestalt");
                const auto id=Bytes(cache.read_tag_bytes(g.u32(28)+index*4u,4)).u32(0);return std::pair{id,cache.string_id(id)};};
            std::map<std::string,std::uint32_t> ranges;
            const auto count=std::to_integer<unsigned>(snd.slice(10,1)[0]);
            require(std::uint64_t(snd.u16(8))+count<=g.u32(40),"Actual Cache pitch ranges escape their owner");
            for(unsigned n=0;n<count;++n){const auto at=g.u32(44)+(snd.u16(8)+n)*12u;const auto row=cache.read_tag_bytes(at,12);const auto named=name(Bytes(row).u16(0));
                require(ranges.emplace(named.second,at).second,"Actual Cache original pitch name is ambiguous");}
            for(const auto& pitch:m.pitch_ranges){require(pitch.import_name_id&&ranges.contains(pitch.name),"Actual Cache lacks the exact original pitch name");
                const Bytes range(cache.read_tag_bytes(ranges.at(pitch.name),12));const auto rn=name(range.u16(0));names->bind(*pitch.import_name_id,rn.first,rn.second);
                require(std::uint64_t(range.u16(8))+range.u16(10)<=g.u32(48),"Actual Cache permutations escape their owner");
                std::map<std::string,std::uint32_t> permutations;
                for(unsigned n=0;n<range.u16(10);++n){const Bytes row(cache.read_tag_bytes(g.u32(52)+(range.u16(8)+n)*16u,16));const auto named=name(row.u16(0));
                    require(permutations.emplace(named.second,named.first).second,"Actual Cache original permutation name is ambiguous");}
                for(const auto& p:pitch.permutations){require(p.import_name_id&&permutations.contains(p.name),"Actual Cache lacks the exact original permutation name");names->bind(*p.import_name_id,permutations.at(p.name),p.name);}
            }
            bindings.push_back({tag->handle(),bank});
        }
        require(!bindings.empty(),"Admitted original catalog has no selected-map bindings");
    }
};
// Declared read-only component memory: actual canonical image bytes and the
// documented index/gestalt selector words. No native World, allocator, source
// clock, original caller execution, voice or device is represented here.
struct TimingMemory final:SourcePhysicsContactMemory {
    SourceRuntimeCacheTagImage image;
    std::map<unsigned,std::vector<std::byte>> scalars;
    mutable std::uint64_t reads{},writes{};
    explicit TimingMemory(SourceRuntimeCacheTagImage value):image(std::move(value)){}
    void read(unsigned at,std::span<std::byte> out)const override{
        ++reads;
        for(const auto& [base,bytes]:scalars)if(at>=base&&std::uint64_t(at)-base+out.size()<=bytes.size()){
            std::ranges::copy(std::span(bytes).subspan(at-base,out.size()),out.begin());return;}
        for(const auto& region:image.regions)if(at>=region.address&&std::uint64_t(at)-region.address+out.size()<=region.bytes.size()){
            std::ranges::copy(std::span(region.bytes).subspan(at-region.address,out.size()),out.begin());return;}
        throw FormatError("Canonical timing fixture read has no complete owner");
    }
    void write(unsigned,std::span<const std::byte>)override{++writes;throw FormatError("Timing verification attempted a source write");}
    static void word(std::vector<std::byte>& bytes,unsigned at,unsigned value){for(unsigned n=0;n<4;++n)bytes.at(at+n)=std::byte(value>>(8*n));}
    void scalar(unsigned at,unsigned value){auto& b=scalars[at];b.resize(4);word(b,0,value);}
    std::byte& byte(unsigned at){for(auto& r:image.regions)if(at>=r.address&&std::uint64_t(at)-r.address<r.bytes.size())return r.bytes[at-r.address];
        throw FormatError("Canonical timing mutation has no actual region");}
};
J timing_memory(const Cache& cache,const SourceRuntimeMccPreparation& prepared,const MccSoundRuntimePlan& baseline,std::uint64_t& controls){
    const auto& plan=prepared.sound;const auto& timing=prepared.timing;auto memory=std::make_shared<TimingMemory>(prepared.delivery.tag_image());
    const auto* globals=cache.find_tag({prepared.delivery.index().globals},tag_groups::globals);require(globals,"Actual timing fixture lost its globals");
    const auto child=Bytes(cache.read_tag_bytes(globals->address+0xc0u,8)).u32(4);
    const auto projected=project_mcc_sound_globals(cache.read_tag_bytes(child,44));const auto* primary=cache.find_tag({Bytes(projected).u32(32)},TagGroup{"ugh!"});
    require(primary,"Actual timing fixture lost its primary gestalt");const auto primary_root=prepared.delivery.runtime_address(primary->address,88);
    memory->scalar(0x556d90u,memory->image.index);memory->scalar(0x4f0414u,memory->image.table);memory->scalar(0x52d664u,primary_root);
    memory->scalar(0x4ecc1cu,prepared.delivery.runtime_address(globals->address,0xc8));
    constexpr unsigned stack=0x71000000u;memory->scalars[stack].resize(12);
    const auto owner=std::make_shared<MccSoundTimingMemoryOwner>(memory);const auto bound=bind_mcc_sound_timing_memory(timing,owner);
    std::uint64_t prefix{},original{};
    for(bool replaced:{false,true}){
        const auto selected=std::ranges::find_if(timing.bindings(),[&](const auto& b){return plan.original->contains(b.sound)==replaced;});
        require(selected!=timing.bindings().end(),"Actual timing fixture lacks both preserved and original bindings");const auto& b=*selected;
        auto resource=std::make_shared<const EncodedSound>(replaced?plan.original->bank()->encoded(b.sound,b.relative_pitch,b.relative_permutation):
            cache.load_sound(b.sound.value,b.relative_pitch,b.relative_permutation));
        const auto proof=authenticate_mcc_sound_timing_resource(bound,owner,b.sound,b.relative_pitch,b.relative_permutation,resource);
        MccSoundTimingEntry entry{0x21f590u,0x21f6f8u};entry.eax=std::uint32_t(resource->data.size());entry.ebx=primary_root;
        entry.ecx=b.native_permutation-plan.blocks[5].address;entry.edx=plan.blocks[5].address;entry.esi=b.native_root;entry.edi=b.native_range;entry.esp=stack;
        entry.stack={entry.return_address,entry.edi,entry.eax};for(unsigned n=0;n<3;++n)TimingMemory::word(memory->scalars[stack],n*4,entry.stack[n]);
        const auto result=lookup_mcc_sound_original_timing(bound,owner,entry,proof);
        require(result.status==MccSoundTimingLookupStatus::Reference&&result.original==b.original,"Actual Cache timing inputs failed current canonical memory/resource validation");
        if(replaced)++original;else{
            ++prefix;auto old=entry;old.edi=baseline.blocks[4].address+(entry.edi-plan.blocks[4].address);old.edx=baseline.blocks[5].address;
            old.stack[1]=old.edi;for(unsigned n=0;n<3;++n)TimingMemory::word(memory->scalars[stack],n*4,old.stack[n]);
            require(lookup_mcc_sound_original_timing(bound,owner,old,proof).status!=MccSoundTimingLookupStatus::Reference,"Old retained prefix addresses borrowed joined timing authority");++controls;
            for(unsigned n=0;n<3;++n)TimingMemory::word(memory->scalars[stack],n*4,entry.stack[n]);
            auto& byte=memory->byte(b.native_range);byte^=std::byte{1};
            require(lookup_mcc_sound_original_timing(bound,owner,entry,proof).status!=MccSoundTimingLookupStatus::Reference,"Changed preserved native input retained joined timing authority");byte^=std::byte{1};++controls;
        }
        const auto reads=memory->reads;
        if(replaced){owner->rebind(memory);require(lookup_mcc_sound_original_timing(bound,owner,entry,proof).status==MccSoundTimingLookupStatus::StaleOrForeignOwner&&memory->reads==reads,
            "Identical canonical rebind retained old timing proof or read stale bytes");++controls;}
    }
    require(prefix&&original&&!memory->writes,"Actual Cache timing fixture missed a source or performed a native write");
    return J{{"bound_canonical_bindings",timing.bindings().size()},{"preserved_resource_lookups",prefix},{"original_resource_lookups",original},
        {"read_only_fixture",true},{"declared_lookup_entry",true},{"source_writes",memory->writes},{"live_world_verified",false}};
}
std::shared_ptr<TimingMemory> provider_memory(const Cache& cache,const MccRuntimeCacheDelivery& delivery){
    auto memory=std::make_shared<TimingMemory>(delivery.tag_image());
    const Bytes index(delivery.index().region.bytes);const auto table=delivery.index().table-delivery.index().region.address;
    const auto address=[&](TagHandle h){return index.u32(table+h.index()*16u+8);};
    const auto* globals=cache.find_tag({delivery.index().globals},tag_groups::globals);require(globals,"Provider fixture lost its actual globals");
    const auto child=Bytes(cache.read_tag_bytes(globals->address+0xc0,8)).u32(4);
    const auto projected=project_mcc_sound_globals(cache.read_tag_bytes(child,44));
    memory->scalar(0x556d90u,memory->image.index);memory->scalar(0x4f0414u,memory->image.table);
    memory->scalar(0x4ecc1cu,address(globals->handle()));memory->scalar(0x52d664u,address({Bytes(projected).u32(32)}));
    return memory;
}
void same_definition(const SoundVariationDefinition& a,const SoundVariationDefinition& b){
    require(a.datum==b.datum&&a.sound==b.sound&&a.playback==b.playback&&a.sound_class==b.sound_class&&a.scale==b.scale&&
        a.range==b.range&&a.permutations==b.permutations&&a.used==b.used&&a.sample_rate_index==b.sample_rate_index&&a.pitch_ranges.size()==b.pitch_ranges.size(),
        "Original provider metadata differs from its exact retained native graph");
    for(unsigned n=0;n<a.pitch_ranges.size();++n)require(a.pitch_ranges[n].interval==b.pitch_ranges[n].interval&&a.pitch_ranges[n].permutations==b.pitch_ranges[n].permutations,
        "Original provider changed a native pitch interval or permutation count");
}
void same_encoded(const EncodedSound& a,const EncodedSound& b){
    require(a.name==b.name&&a.sample_rate==b.sample_rate&&a.channels==b.channels&&a.codec==b.codec&&a.samples==b.samples&&
        a.sound_class==b.sound_class&&a.data==b.data&&a.packet_chunks.size()==b.packet_chunks.size(),"Provider selected a different encoded resource");
    for(unsigned n=0;n<a.packet_chunks.size();++n){const auto& x=a.packet_chunks[n];const auto& y=b.packet_chunks[n];
        require(x.bytes==y.bytes&&x.pcm_bytes==y.pcm_bytes&&x.reset_after==y.reset_after,"Provider changed Cache packet boundaries");}
}
J variation_audio(const Cache& cache,const SourceRuntimeMccPreparation& prepared,const SourceRuntimeResources& resources,const MccOriginalSoundRuntimeInputTu15& original,std::uint64_t& controls){
    auto memory=provider_memory(cache,prepared.delivery);auto owner=std::make_shared<MccSoundTimingMemoryOwner>(memory);
    const auto provider=detail::make_mcc_sound_variation_provider(cache,cache,prepared.delivery,prepared.sound,resources,owner);
    const auto bank=prepared.sound.original->bank();std::uint64_t definitions{},encoded_routes{},voices{},decoded_frames{},counterfactual_frames{};
    const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
    std::optional<MultiplayerAudioSubmission> submitted;std::uint64_t packets{};
    MultiplayerAudio audio(cache,0x5821b36du,manual,[&](const auto& packet){submitted=packet;++packets;});
    audio.bind_native_sound_metadata(provider);audio.listeners(std::array{Vec3{}});
    TagHandle selected{};
    for(const auto& binding:prepared.sound.original->bindings()){
        selected=binding.sound;
        for(unsigned pitch=0;pitch<binding.bank->metadata().pitch_ranges.size();++pitch){
            const auto definition=provider.load(cache,binding.sound,pitch);same_definition(definition,bank->load(binding.sound,pitch));++definitions;
            for(unsigned p=0;p<definition.permutations.size();++p){same_encoded(provider.encoded(cache,binding.sound,pitch,p),bank->encoded(binding.sound,pitch,p));++encoded_routes;}
        }
        submitted.reset();const auto before=packets;const auto voice=audio.play_impulse(binding.sound.value);
        require(voice&&submitted&&packets==before+1&&submitted->sound==binding.sound.value,"Original native metadata did not reach the real Audio packet owner");
        const auto encoded=provider.encoded(cache,binding.sound,submitted->pitch_range,submitted->permutation);const auto pcm=decode_sound(encoded);
        require(pcm.frames()&&pcm.rate==48000&&submitted->frames==pcm.frames(),"Audio packet PCM extent differs from its selected original resource");
        const auto mcc=decode_sound(cache.load_sound(binding.sound.value,submitted->pitch_range,submitted->permutation));
        if(mcc.frames()!=pcm.frames())++counterfactual_frames;
        auto mixed=audio.render(256);require(mixed.size()==512&&std::ranges::all_of(mixed,[](float value){return std::isfinite(value);}),"Original Audio manual mixer returned invalid PCM");
        decoded_frames+=pcm.frames();++voices;audio.stop();
    }
    const auto preserved=std::ranges::find_if(prepared.timing.bindings(),[&](const auto& s){return !prepared.sound.original->contains(s.sound);});
    require(preserved!=prepared.timing.bindings().end(),"Original provider fixture lacks preserved Cache resources");
    same_encoded(provider.encoded(cache,preserved->sound,preserved->relative_pitch,preserved->relative_permutation),
        cache.load_sound(preserved->sound.value,preserved->relative_pitch,preserved->relative_permutation));
    rejects([&]{(void)provider.encoded(cache,{selected.value^0x10000u},0,0);},controls);
    rejects([&]{(void)provider.encoded(cache,selected,0,0xffffffffu);},controls);
    const auto d=provider.load(cache,selected);const auto first=Bytes(d.permutations.front()).u16(12);
    auto& locator=memory->byte(prepared.sound.blocks[8].address+first*12u);locator^=std::byte{1};
    rejects([&]{(void)provider.encoded(cache,selected,0,0);},controls);locator^=std::byte{1};
    auto& handle=memory->byte(prepared.sound.blocks[8].address+first*12u+8);handle^=std::byte{1};
    same_encoded(provider.encoded(cache,selected,0,0),bank->encoded(selected,0,0));handle^=std::byte{1};
    auto& range=memory->byte(prepared.sound.blocks[4].address+Bytes(d.sound).u16(8)*12u+8);range^=std::byte{1};
    rejects([&]{(void)provider.encoded(cache,selected,0,0);},controls);range^=std::byte{1};
    detail::require_mcc_sound_variation_resources(provider,&provider,cache);
    rejects([&]{detail::require_mcc_sound_variation_resources(provider,nullptr,cache);},controls);
    // Actual independent canonical allocation, rather than an invented digest.
    std::optional<MccSoundRuntimePlan> foreign_plan;
    const auto foreign_delivery=MccRuntimeCacheDelivery::plan(cache,0x88000000u,0x60000000u,[&](MccSoundRuntimePlan&& p){foreign_plan.emplace(std::move(p));},original);
    auto foreign_memory=provider_memory(cache,foreign_delivery);auto foreign_owner=std::make_shared<MccSoundTimingMemoryOwner>(foreign_memory);
    rejects([&]{(void)detail::make_mcc_sound_variation_provider(cache,cache,foreign_delivery,*foreign_plan,resources,foreign_owner);},controls);
    audio.prepare(selected.value);const auto packet_count=packets;const auto frame=audio.presentation_frames();
    owner->rebind(memory);
    rejects([&]{audio.prepare(selected.value);},controls);
    rejects([&]{(void)audio.play_impulse(selected.value);},controls);
    rejects([&]{(void)audio.render(256);},controls);
    require(packets==packet_count&&audio.presentation_frames()==frame&&!audio.voices(),"Stale original Audio advanced the backend");
    const auto reads=memory->reads;rejects([&]{(void)provider.encoded(cache,selected,0,0);},controls);require(reads==memory->reads,"Stale encoded provider read source bytes");
    const auto fresh=detail::make_mcc_sound_variation_provider(cache,cache,prepared.delivery,prepared.sound,resources,owner);
    detail::require_mcc_sound_variation_resources(fresh,&provider,cache); // Saved epoch may be stale.
    audio.bind_native_sound_metadata(fresh);require(bool(audio.play_impulse(selected.value)),"Fresh original binding failed actual Audio playback");audio.stop();
    require(!memory->writes&&!foreign_memory->writes,"Variation/resource verification wrote native bytes");
    J empty_tables=J::array();for(unsigned n=0;n<prepared.sound.blocks.size();++n)if(!prepared.sound.blocks[n].count)
        empty_tables.push_back({{"table",n},{"count",0},{"address",prepared.sound.blocks[n].address}});
    return J{{"provider_definitions",definitions},{"provider_encoded_routes",encoded_routes},{"audio_original_voices",voices},{"literal_empty_tables",empty_tables},
        {"decoded_original_frames",decoded_frames},{"audio_packets_with_different_mcc_frame_count",counterfactual_frames},
        {"preserved_cache_encoded_pair",true},{"mutable_chunk_handle_ignored",true},{"source_writes",0},
        {"same_resource_stale_saved_epoch_admitted",true},{"incompatible_plan_provider_creation_refused",true},
        {"manual_pcm_fixture",true},{"live_world_or_checkpoint_restore_executed",false},{"device_or_original_waveform_verified",false}};
}
}
int main(int argc,char** argv){J report{{"passed",false},{"goal_complete",false},{"live_world_execution_verified",false},
    {"native_mcc_prefix_dat_backing_verified",false},{"audio_device_or_waveform_verified",false},{"large_artifacts_retained",J::array()}};
    if(argc!=6||!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[3]))return 77;
    try{
        report["phase"]="catalog";const std::string context=argv[2];
        require(!context.empty()&&std::filesystem::path(context).filename()==context,"Original catalog context must be explicitly declared");
        std::ifstream input(argv[3],std::ios::binary);const std::string raw((std::istreambuf_iterator<char>(input)),{});
        require(hash(std::as_bytes(std::span(raw.data(),raw.size())))==argv[4],"Original preparation catalog differs from its external admission pin");
        const auto catalog=J::parse(raw);const std::filesystem::path map=argv[1];
        report["phase"]="cache";auto cache=std::make_shared<Cache>(map,map.parent_path(),OriginalData::native_tu15());require(cache->is_mcc(),"Original preparation qualification requires actual MCC content");
        const auto cache_hash=hash(cache->source_runtime_cache_bytes());
        const Inputs original(*cache,catalog,context);const auto in=original.view();
        report["phase"]="baseline";std::optional<MccSoundRuntimePlan> baseline;
        const auto canonical=MccRuntimeCacheDelivery::plan(*cache,0x90000000u,0x60000000u,[&](MccSoundRuntimePlan&& p){require(!baseline,"Duplicate baseline sound plan");baseline.emplace(std::move(p));});
        require(bool(baseline),"Actual Cache baseline lost its native sound plan");
        const auto timing=build_mcc_sound_timing_reference(*cache,*baseline,[&](auto a,auto n){return canonical.runtime_address(a,n);});
        require(!timing.bindings().empty()&&!timing.stored_references().empty()&&!timing.permutation_references().empty(),"Actual Cache baseline lacks positive original timing references");
        report["baseline_bindings"]=timing.bindings().size();report["baseline_stored_headers"]=timing.stored_references().size();report["baseline_permutation_times"]=timing.permutation_references().size();
        SourceRuntimeResources::Identity ordinary_identity;
        {SourceRuntimeResources ordinary(*cache);ordinary.require_sound_plan(*baseline);ordinary_identity=ordinary.identity();}
        std::unique_ptr<SourceRuntimeMccPreparation> metadata;
        SourceRuntimeResources::Identity original_identity;std::uint64_t controls{},original_routes{},sectors{},preserved{},stored{},permutation_times{};
        report["phase"]="prepare_original";
        {
            SourceRuntimeResourcePreparation preparation(cache,in);
            rejects([&]{SourceRuntimeResourcePreparation duplicate(cache,in);},controls);
            require(!take_prepared_mcc_world_metadata(*cache,0x80061000u,0x10000000u),"Foreign reservation consumed original metadata");
            metadata=take_prepared_mcc_world_metadata(*cache,0x90000000u,0x60000000u);require(bool(metadata),"Actual original preparation has no complete metadata");
            const auto& plan=metadata->sound;const auto& joined=metadata->timing;
            require(plan.original&&metadata->delivery.original_sound_owner()==plan.original,"Actual original preparation split canonical ownership");
            plan.original->require_plan(plan);
            require(plan.original->parent().bytes==baseline->bytes,"Actual original preparation changed the retained parent bank");
            report["phase"]="preserved_timing";
            for(const auto& b:timing.bindings())if(!plan.original->contains(b.sound)){
                const auto found=std::ranges::find_if(joined.bindings(),[&](const auto& n){return n.sound==b.sound&&n.relative_pitch==b.relative_pitch&&n.relative_permutation==b.relative_permutation;});
                require(found!=joined.bindings().end()&&found->gestalt==b.gestalt&&found->source_root==b.source_root&&found->native_root==b.native_root&&
                    found->source_range==b.source_range&&found->source_permutation==b.source_permutation&&found->path==b.path&&found->range_name==b.range_name&&found->permutation_name==b.permutation_name&&
                    found->named_key==b.named_key&&found->encoded_payload==b.encoded_payload&&found->original==b.original&&
                    found->native_range==plan.blocks[4].address+(b.native_range-baseline->blocks[4].address)&&
                    found->native_permutation==plan.blocks[5].address+(b.native_permutation-baseline->blocks[5].address),"Actual original join lost or changed a preserved MCC timing binding");++preserved;
            }
            require(preserved>0,"Actual original join has no positive preserved MCC timing bindings");
            for(const auto& b:timing.stored_references())if(!plan.original->contains(b.sound)){
                const auto found=std::ranges::find(joined.stored_references(),b.sound,&MccSoundTimingStoredReference::sound);
                require(found!=joined.stored_references().end()&&found->original_ms==b.original_ms&&found->source_root==b.source_root&&found->native_root==b.native_root&&found->expected_native_header==b.expected_native_header,
                    "Actual original join lost a preserved stored-time reference");++stored;
            }
            for(const auto& b:timing.permutation_references())if(!plan.original->contains(b.sound)){
                const auto found=std::ranges::find_if(joined.permutation_references(),[&](const auto& n){return n.sound==b.sound&&n.relative_pitch==b.relative_pitch&&n.relative_permutation==b.relative_permutation;});
                require(found!=joined.permutation_references().end()&&found->original_ms==b.original_ms&&found->named_key==b.named_key&&found->encoded_payload==b.encoded_payload&&
                    found->expected_native_header==b.expected_native_header&&found->expected_native_range==b.expected_native_range&&found->expected_native_permutation==b.expected_native_permutation&&
                    found->native_range==plan.blocks[4].address+(b.native_range-baseline->blocks[4].address)&&found->native_permutation==plan.blocks[5].address+(b.native_permutation-baseline->blocks[5].address),
                    "Actual original join lost or misrebased a preserved U16 timing reference");++permutation_times;
            }
            report["phase"]="original_resources";
            {
                SourceRuntimeResources resources(*cache);resources.require_sound_plan(plan);original_identity=resources.identity();
                require(resources.original_sound_owner()==plan.original&&original_identity.resident==plan.original->bind_resource_identity(ordinary_identity.resident)&&original_identity.shared==ordinary_identity.shared&&original_identity!=ordinary_identity,
                    "Actual resource preparation omitted or changed the immutable original binding");
                rejects([&]{resources.require_sound_plan(*baseline);},controls);
                auto bad=plan;bad.bytes.back()^=std::byte{1};rejects([&]{resources.require_sound_plan(bad);},controls);
                const auto bank=plan.original->bank();
                for(const auto& b:plan.original->bindings())for(unsigned pitch=0;pitch<b.bank->metadata().pitch_ranges.size();++pitch){
                    const auto native=bank->load(b.sound,pitch);const auto& r=b.bank->metadata().pitch_ranges[pitch];
                    for(unsigned p=0;p<r.permutations.size();++p){++original_routes;const auto first=Bytes(native.permutations[p]).u16(12);const auto& permutation=r.permutations[p];
                        for(unsigned c=0;c<permutation.chunks.size();++c){const auto& chunk=permutation.chunks[c];const auto at=bank->block(8).address+(first+c)*12u;const Bytes row(bank->bytes().subspan(at-bank->address(),12));
                            const SourceSoundCacheReadTu15 request{at,row.u32(0),row.u32(4),chunk.sector_bytes};
                            require(std::ranges::equal(resources.sound_cache(request),b.bank->sectors().subspan(chunk.offset,chunk.sector_bytes)),"Actual prepared resource boundary changed original sectors");++sectors;
                        }
                    }
                }
                require(resources.identity()==original_identity,"Prepared original reads changed immutable resource identity");
                report["phase"]="original_variation_audio";report["variation_audio"]=variation_audio(*cache,*metadata,resources,in,controls);
            }
            report["phase"]="canonical_timing_memory";report["timing_memory"]=timing_memory(*cache,*metadata,*baseline,controls);
            report["phase"]="consumed_preparation";
            rejects([&]{SourceRuntimeResources consumed(*cache);},controls);
            rejects([&]{(void)take_prepared_mcc_world_metadata(*cache,0x90000000u,0x60000000u);},controls);
        }
        report["phase"]="ordinary_owner";
        {SourceRuntimeResources ordinary(*cache);require(ordinary.identity()==ordinary_identity,"Original preparation altered ordinary resource identity after its scope");
            rejects([&]{ordinary.require_sound_plan(metadata->sound);},controls);}
        require(hash(cache->source_runtime_cache_bytes())==cache_hash,"Original preparation changed raw Cache bytes");
        require(original_routes>0&&sectors>0,"Actual original resource preparation missed selected-map coverage");
        report.update(J{{"passed",true},{"map",context},{"mcc_map",map.filename().string()},{"original_bindings",original.bindings.size()},{"original_routes",original_routes},{"original_sector_reads",sectors},
            {"preserved_mcc_timing_bindings",preserved},{"preserved_stored_headers",stored},{"preserved_permutation_times",permutation_times},{"negative_controls",controls},
            {"raw_cache_unchanged",true},{"resource_identity_binds_original",true},{"retry_falls_back_to_ordinary",false}});report.erase("phase");
    }catch(const std::exception& e){report["error"]=e.what();}
    std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';if(!out)return 2;std::cout<<report.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
