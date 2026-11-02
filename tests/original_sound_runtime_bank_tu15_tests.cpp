#include "halo2/original_sound_runtime_bank_tu15.hpp"
#include "halo2/sound_promotion.hpp"
#include "halo2/audio.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#if HALO2_TEST_ORIGINAL_NATIVE_PREFIX
#include "audio/mcc_sound_class_qualified.hpp"
#include "halo2/mcc_original_sound_runtime_delivery_tu15.hpp"
#include "audio/mcc_sound_timing_reference_profile.hpp"
#include "halo2/physics_contact_record.hpp"
#endif

namespace {
using namespace halo2;using J=nlohmann::json;
constexpr unsigned promotion_burst_calls=256,promotion_tail_calls=64;
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::string hash(std::span<const std::byte> data){std::array<unsigned char,32> bytes{};unsigned count{};check(EVP_Digest(data.data(),data.size(),bytes.data(),&count,EVP_sha256(),nullptr)==1&&count==32,"Digest failed");constexpr char hex[]="0123456789abcdef";std::string out;for(auto n:bytes){out+=hex[n>>4];out+=hex[n&15];}return out;}
void put(std::span<std::byte> data,unsigned at,std::uint32_t value){Bytes(data).require(at,4);for(unsigned n=0;n<4;++n)data[at+n]=std::byte(value>>(8*n));}
std::vector<std::byte> original(const J& ref,std::uint64_t at,std::size_t n){std::ifstream file(ref.at("path").get<std::string>(),std::ios::binary);file.seekg(std::streamoff(at));std::vector<std::byte> bytes(n);file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(n));check(file.gcount()==std::streamsize(n),"Original metadata slice is truncated");return bytes;}
struct Symbols {std::map<std::uint32_t,std::pair<std::uint32_t,std::string>> forward;std::map<std::uint32_t,std::string> names;};
#if HALO2_TEST_ORIGINAL_NATIVE_PREFIX
struct PrefixFixture {
    struct Allocation {std::uint32_t at,size,offset;};
    std::vector<std::byte> data;
    std::map<std::uint32_t,Allocation> allocations;
    std::vector<Tag> tags;
    std::map<std::uint32_t,TagHandle> owners;
    std::map<std::uint32_t,std::string> names;
    MccSoundRuntimePlan native;
    MccSoundRuntimeSource view()const{return{tags,[this](auto at,auto n){return read(at,n);},
        [this](TagHandle h){return owners.at(h.value);},[this](auto at,auto n){if(n)(void)read(at,n);return at;}};}
    MccSoundVanillaSource timing_source()const{return {view(),[this](auto id){return names.at(id);},{}};}
    std::span<const std::byte> read(std::uint32_t at,std::size_t n)const{
        auto i=allocations.upper_bound(at);check(i!=allocations.begin(),"Missing actual MCC source allocation");--i;
        const auto offset=std::uint64_t(at)-i->second.at;check(offset<=i->second.size&&n<=i->second.size-offset,"Actual MCC source extent is unprovided");
        return Bytes(data).slice(i->second.offset+offset,n);
    }
    static std::pair<std::shared_ptr<PrefixFixture>,OriginalSoundRuntimePrefixTu15> load(const J& input){
        auto out=std::make_shared<PrefixFixture>();std::ifstream file(input.at("path").get<std::string>(),std::ios::binary|std::ios::ate);check(bool(file),"Actual MCC fixture is missing");
        const auto size=file.tellg();check(size>0&&size<=128*1024*1024,"Actual MCC fixture is excessive");out->data.resize(std::size_t(size));file.seekg(0);file.read(reinterpret_cast<char*>(out->data.data()),size);
        check(bool(file)&&hash(out->data)==input.at("sha256").get<std::string>(),"Actual MCC source fixture changed");
        for(const auto& a:input.at("allocations")){Allocation row{a.at("address"),a.at("bytes"),a.at("offset")};check(out->allocations.emplace(row.at,row).second,"Actual MCC fixture aliases allocation bases");}
        out->tags.resize(input.at("slot_count"));std::set<unsigned> slots;
        for(const auto& t:input.at("tags")){const auto datum=t.at("datum").get<std::uint32_t>();check(slots.insert(datum&65535).second,"Actual MCC directory aliases a slot");
            out->tags.at(datum&65535)={t.at("group"),t.at("name"),datum,t.at("address"),t.at("size")};}
        for(const auto& t:input.at("null_rows")){const auto hex=t.at("literal_hex").get<std::string>();check(hex.size()==32,"Actual null directory row is truncated");
            std::array<std::byte,16> raw;for(unsigned n=0;n<16;++n)raw[n]=std::byte(std::stoul(hex.substr(n*2,2),nullptr,16));const Bytes b(raw);
            const auto slot=t.at("slot").get<unsigned>();check(b.u32(4)==0xffffffffu&&slots.insert(slot).second,"Actual null directory row is unproven/aliased");
            out->tags.at(slot)={TagGroup{b.u32(0)}.string(),"",b.u32(4),b.u32(8),b.u32(12)};}
        check(slots.size()==out->tags.size(),"Actual directory fixture omitted serialized slots");
        for(auto i=input.at("string_names").begin();i!=input.at("string_names").end();++i)out->names.emplace(std::stoul(i.key()),i.value());
        for(auto i=input.at("owners").begin();i!=input.at("owners").end();++i)out->owners.emplace(std::stoul(i.key()),TagHandle{i.value()});
        const MccSoundRuntimeSource source{out->tags,[out](auto at,auto n){return out->read(at,n);},
            [out](TagHandle h){return out->owners.at(h.value);},[out](auto at,auto n){if(n)(void)out->read(at,n);return at;}};
        auto generic=plan_mcc_sound_runtime_delivery(source,0x59000000u);out->native=mcc_sound_class_detail::stage_qualified(source,generic);
        const auto globals=std::ranges::find_if(out->tags,[](const auto& t){return t.group=="matg"&&t.address;});check(globals!=out->tags.end(),"Actual MCC prefix has no globals");
        const auto at=Bytes(out->read(globals->address+0xc0u,8)).u32(4);const TagHandle legacy{Bytes(out->read(at,44)).u32(12)};
        const auto root=std::ranges::find(out->native.patches,legacy,&MccSoundRuntimePatch::owner);check(root!=out->native.patches.end(),"Actual MCC prefix has no Legacy class patch");
        const Bytes classes(root->after);check(classes.u32(0)==54,"Actual MCC prefix class count changed");
        constexpr std::array<unsigned,11> strides{56,20,4,10,12,16,52,1,12,28,44};
        OriginalSoundRuntimePrefixTu15 prefix;prefix.owner=out;
        for(unsigned n=0;n<11;++n){const auto b=out->native.blocks[n];prefix.tables[n]=Bytes(out->native.bytes).slice(b.address-out->native.address,std::size_t(b.count)*strides[n]);}
        prefix.sound_classes=Bytes(out->native.bytes).slice(classes.u32(4)-out->native.address,54*92u);
        return{out,prefix};
    }
};
struct TimingMemory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> regions;mutable std::uint64_t reads{},writes{};
    auto bytes(std::uint32_t at,std::size_t size){auto i=regions.upper_bound(at);check(i!=regions.begin(),"Timing fixture memory is unowned");--i;
        check(std::uint64_t(at)-i->first+size<=i->second.size(),"Timing fixture extent escapes its owner");return std::span(i->second).subspan(at-i->first,size);}
    void read(std::uint32_t at,std::span<std::byte> output)const override{++reads;auto i=regions.upper_bound(at);check(i!=regions.begin(),"Timing read has no fixture owner");--i;
        std::ranges::copy(Bytes(i->second).slice(at-i->first,output.size()),output.begin());}
    void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw FormatError("Pure original timing attempted a write");}
    void scalar(std::uint32_t at,std::uint32_t x){regions[at].resize(4);put(regions[at],0,x);}
};
J timing_join(const std::shared_ptr<PrefixFixture>& fixture,const MccSoundRuntimePlan& actual){
    const auto source=fixture->timing_source();const auto parent=mcc_sound_timing_detail::build_class_qualified(source,fixture->native);
    const auto joined=mcc_sound_timing_detail::build_class_qualified(source,actual);
    check(!joined.bindings().empty(),joined.gaps().empty()?"Original timing admitted no bindings":joined.gaps().front().reason.c_str());
    check(parent.owners().size()==joined.owners().size(),"Original timing changed MCC diagnostic owner coverage");
    for(unsigned n=0;n<parent.owners().size();++n)check(parent.owners()[n].gestalt==joined.owners()[n].gestalt&&parent.owners()[n].status==joined.owners()[n].status&&parent.owners()[n].reason==joined.owners()[n].reason,"Original timing promoted/changed a parent MCC owner status");
    auto memory=std::make_shared<TimingMemory>();
    for(const auto& [at,a]:fixture->allocations){const auto b=fixture->read(at,a.size);memory->regions[at]={b.begin(),b.end()};}
    memory->regions[actual.address]=actual.bytes;
    for(const auto& p:actual.patches)std::ranges::copy(p.after,memory->bytes(p.source_address,p.after.size()).begin());
    const auto globals=std::ranges::find_if(fixture->tags,[](const auto& t){return !t.handle().is_null()&&t.address&&t.group=="matg";});check(globals!=fixture->tags.end(),"Timing fixture has no real globals");
    const auto child=Bytes(fixture->read(globals->address+0xc0u,8)).u32(4);const auto row=project_mcc_sound_globals(fixture->read(child,44));
    std::ranges::copy(row,memory->bytes(child,36).begin());const TagHandle primary{Bytes(row).u32(32)};
    const auto primary_tag=std::ranges::find(fixture->tags,primary.value,&Tag::datum);check(primary_tag!=fixture->tags.end(),"Timing fixture lost its real primary owner");
    // Explicit component memory fixture: actual slot-complete identities and
    // projected metadata, with a declared index/stack. This is no live World.
    constexpr unsigned index=0x10000000u,table=index+4096u,stack=0x11000000u;
    memory->regions[index].resize(32);put(memory->regions[index],8,table);put(memory->regions[index],16,globals->datum);put(memory->regions[index],24,fixture->tags.size());put(memory->regions[index],28,0x74616773);
    memory->regions[table].resize(fixture->tags.size()*16u);
    for(unsigned n=0;n<fixture->tags.size();++n){const auto& t=fixture->tags[n];auto& b=memory->regions[table];put(b,n*16,t.type().value);put(b,n*16+4,t.datum);put(b,n*16+8,t.address);put(b,n*16+12,t.size);}
    memory->scalar(0x556d90u,index);memory->scalar(0x4f0414u,table);memory->scalar(0x52d664u,primary_tag->address);memory->regions[stack].resize(12);
    auto owner=std::make_shared<MccSoundTimingMemoryOwner>(memory);const auto bound=bind_mcc_sound_timing_memory(joined,owner);
    std::uint64_t routes{},controls{},stored{},permutation_times{},callbacks{};
    const auto read=[&](auto at,auto n){return Bytes(actual.bytes).slice(at-actual.address,n);};
    for(const auto& b:joined.bindings())if(actual.original->contains(b.sound)){
        const auto original=std::ranges::find(actual.original->bindings(),b.sound,&OriginalSoundRuntimeBindingTu15::sound);check(original!=actual.original->bindings().end(),"Original timing binding lost its bank");
        const auto& meta=original->bank->metadata();const auto& pitch=meta.pitch_ranges.at(b.relative_pitch);const auto& permutation=pitch.permutations.at(b.relative_permutation);
        auto encoded=std::make_shared<EncodedSound>(original->bank->encoded(b.relative_pitch,b.relative_permutation));
        const auto proof=authenticate_mcc_sound_timing_resource(bound,owner,b.sound,b.relative_pitch,b.relative_permutation,encoded);
        MccSoundTimingEntry entry{0x21f590u,0x21f6f8u};entry.eax=Bytes(read(b.native_permutation,16)).u32(8);entry.ebx=primary_tag->address;
        entry.ecx=b.native_permutation-actual.blocks[5].address;entry.edx=actual.blocks[5].address;entry.esi=b.native_root;entry.edi=b.native_range;entry.esp=stack;
        entry.stack={entry.return_address,entry.edi,entry.eax};for(unsigned n=0;n<3;++n)put(memory->regions[stack],n*4,entry.stack[n]);
        auto check_lookup=[&]{return lookup_mcc_sound_original_timing(bound,owner,entry,proof);};
        const auto result=check_lookup();check(result.status==MccSoundTimingLookupStatus::Reference&&result.original==b.original&&b.original.encoded_bytes==encoded->data.size()&&
            b.original.sample_rate==std::to_integer<unsigned>(meta.sound[3])&&b.original.encoding==std::to_integer<unsigned>(meta.sound[4])&&b.original.compression==std::to_integer<unsigned>(meta.sound[5]),"Joined timing does not use exact original duration operands");
        const auto reference=std::ranges::find_if(joined.permutation_references(),[&](const auto& p){return p.sound==b.sound&&p.relative_pitch==b.relative_pitch&&p.relative_permutation==b.relative_permutation;});
        check(reference!=joined.permutation_references().end()&&reference->original_ms==Bytes(permutation.literal).u16(6)&&std::ranges::equal(reference->expected_native_permutation,read(b.native_permutation,16)),"Original authored U16 time/catalog differs");++permutation_times;
        if(!b.relative_pitch&&!b.relative_permutation){const auto s=std::ranges::find(joined.stored_references(),b.sound,&MccSoundTimingStoredReference::sound);
            check(s!=joined.stored_references().end()&&s->original_ms==std::bit_cast<std::int32_t>(Bytes(meta.sound).u32(16))&&std::ranges::equal(s->expected_native_header,memory->bytes(b.native_root,20)),"Original stored duration/catalog differs");++stored;}
        if(!routes){
            check(with_mcc_sound_original_timing(bound,owner,entry,proof,[&](const auto& operands){
                check(operands==b.original,"Original timing execution transaction changed authored operands");++callbacks;
            })==MccSoundTimingLookupStatus::Reference&&callbacks==1,"Original timing transaction did not execute exactly once");
            encoded->data[0]^=std::byte{1};check(check_lookup().status==MccSoundTimingLookupStatus::StaleOrForeignResource,"Changed original encoded bytes retained timing admission");encoded->data[0]^=std::byte{1};++controls;
            const auto playback=actual.blocks[0].address+Bytes(memory->bytes(b.native_root,20)).u16(6)*56u;
            for(const auto at:{b.native_root,b.native_range,b.native_permutation,playback}){
                auto byte=memory->bytes(at,1);byte[0]^=std::byte{1};check(check_lookup().status!=MccSoundTimingLookupStatus::Reference,"Changed original native input retained timing admission");byte[0]^=std::byte{1};++controls;
            }
            auto selectors=memory->bytes(b.native_range+4u,2);const auto a=selectors[0],c=selectors[1];selectors[0]^=std::byte{0x7f};selectors[1]^=std::byte{0x7f};
            check(check_lookup().status==MccSoundTimingLookupStatus::Reference,"Mutable original selectors became duration authority");selectors[0]=a;selectors[1]=c;
            auto foreign=std::make_shared<MccSoundTimingMemoryOwner>(memory);check(lookup_mcc_sound_original_timing(bound,foreign,entry,proof).status==MccSoundTimingLookupStatus::StaleOrForeignOwner,"Foreign same-address original memory admitted");++controls;
        }
        ++routes;
    }
    check(routes>0&&memory->writes==0,"Original timing fixture wrote memory or missed original routes");
    const auto before=memory->reads;owner->rebind(memory);const auto& b=joined.bindings().back();
    bool stale=false;try{(void)authenticate_mcc_sound_timing_resource(bound,owner,b.sound,b.relative_pitch,b.relative_permutation,std::make_shared<const EncodedSound>(actual.original->bank()->encoded(b.sound,b.relative_pitch,b.relative_permutation)));}catch(const FormatError&){stale=true;}
    check(stale&&memory->reads==before,"Identical-byte rebind retained original timing epoch or read stale memory");++controls;
    auto broken=actual;broken.bytes.back()^=std::byte{1};const auto rejected=mcc_sound_timing_detail::build_class_qualified(source,broken);
    check(rejected.bindings().empty()&&!rejected.gaps().empty(),"Broken original canonical seal retained timing bindings");++controls;
    {auto changed=*fixture;const auto target=actual.original->bindings().front().sound;const auto& t=changed.tags.at(target.index());
        const auto a=changed.allocations.at(t.address);changed.data[a.offset]^=std::byte{1};
        const auto invalid=mcc_sound_timing_detail::build_class_qualified(changed.timing_source(),actual);
        check(invalid.bindings().empty()&&!invalid.gaps().empty(),"Original timing borrowed a changed raw parent projection");++controls;}
    {auto changed=source;changed.string_id=[](auto){return std::string("foreign-native-name");};
        const auto invalid=mcc_sound_timing_detail::build_class_qualified(changed,actual);
        check(invalid.bindings().empty()&&!invalid.gaps().empty(),"Original timing borrowed a foreign native StringID namespace");++controls;}
    return J{{"original_routes",routes},{"original_stored_headers",stored},{"original_permutation_times",permutation_times},{"parent_bindings",parent.bindings().size()},
        {"joined_bindings",joined.bindings().size()},{"original_callbacks",callbacks},{"negative_controls",controls},{"native_writes",memory->writes},{"live_world_verified",false}};
}
J resource_join(const std::shared_ptr<PrefixFixture>& fixture,const MccSoundRuntimePlan& actual,
    const MccOriginalSoundRuntimeInputTu15& input){
    const auto& owner=*actual.original;const auto bank=owner.bank();
    const auto replica=stage_mcc_original_sound_runtime_delivery_tu15(fixture->view(),fixture->native,input,8*1024*1024u);
    check(replica.original!=actual.original&&replica.original->resource_identity()==owner.resource_identity(),"Original resource digest depends on host owner identity");
    const auto parse_hash=[](const std::string& hex){std::array<std::byte,32> out;check(hex.size()==64,"Digest extent differs");for(unsigned n=0;n<32;++n)out[n]=std::byte(std::stoul(hex.substr(n*2,2),nullptr,16));return out;};
    const auto resident=parse_hash(hash(fixture->data));const auto bound=owner.bind_resource_identity(resident);
    check(bound!=resident&&bound==replica.original->bind_resource_identity(resident)&&bound==owner.bind_resource_identity(resident),"Original resource resident binding lost stable content identity");
    auto changed=resident;changed[0]^=std::byte{1};check(owner.bind_resource_identity(changed)!=bound,"Original sound identity omitted its resident owner");
    const auto moved_parent=mcc_sound_class_detail::stage_qualified(fixture->view(),plan_mcc_sound_runtime_delivery(fixture->view(),0x69000000u));
    const auto moved=stage_mcc_original_sound_runtime_delivery_tu15(fixture->view(),moved_parent,input,8*1024*1024u);
    check(moved.original->resource_identity()!=owner.resource_identity()&&moved.original->bind_resource_identity(resident)!=bound,
        "Original resource identity omitted its actual native publication layout");
    std::map<unsigned,std::vector<std::byte>> sectors;std::uint64_t routes{},prefix{},empty{},controls{};
    for(const auto& b:owner.bindings())for(unsigned pitch=0;pitch<b.bank->metadata().pitch_ranges.size();++pitch){const auto actual_sound=bank->load(b.sound,pitch);const auto& r=b.bank->metadata().pitch_ranges[pitch];
        for(unsigned permutation=0;permutation<r.permutations.size();++permutation){const auto& p=r.permutations[permutation];const auto first=Bytes(actual_sound.permutations[permutation]).u16(12);
            for(unsigned n=0;n<p.chunks.size();++n){const auto& c=p.chunks[n];const auto descriptor=bank->block(8).address+(first+n)*12u;
                const auto expected=b.bank->sectors().subspan(c.offset,c.sector_bytes);const auto raw=bank->bytes().subspan(descriptor-bank->address(),12);const Bytes row(raw);
                const SourceSoundCacheReadTu15 request{descriptor,row.u32(0),row.u32(4),c.sector_bytes};const auto bytes=owner.read_cache_resource(request);
                check(bytes&&std::ranges::equal(*bytes,expected),"Joined original descriptor did not read its actual full sectors");
                const auto [i,added]=sectors.emplace(descriptor,std::vector<std::byte>(expected.begin(),expected.end()));check(added||std::ranges::equal(i->second,expected),"Original joined descriptor aliases different sectors");++routes;
            }
        }
    }
    const auto reject=[&](const auto& request){bool refused=false;try{(void)owner.read_cache_resource(request);}catch(const FormatError&){refused=true;}check(refused,"Joined sound resource admitted a foreign/changed request");++controls;};
    for(const auto& c:actual.chunks){const Bytes raw(bank->bytes().subspan(c.native_address-bank->address(),12));const auto bytes=raw.u32(4)&0x3fffffffu;
        const SourceSoundCacheReadTu15 request{c.native_address,raw.u32(0),raw.u32(4),(bytes+511u)&~511u};
        if(!bytes){reject(request);++empty;continue;}
        check(!owner.read_cache_resource(request),"Joined original bank consumed a preserved external prefix resource");++prefix;
    }
    const auto descriptor=sectors.begin()->first;const Bytes row(bank->bytes().subspan(descriptor-bank->address(),12));
    const SourceSoundCacheReadTu15 good{descriptor,row.u32(0),row.u32(4),std::uint32_t(sectors.begin()->second.size())};
    {bool refused=false;try{(void)moved.original->read_cache_resource(good);}catch(const FormatError&){refused=true;}
        check(refused,"Different canonical allocation admitted an old original descriptor");++controls;}
    {auto q=good;++q.descriptor;reject(q);}
    {auto q=good;q.descriptor=bank->block(8).address+std::uint32_t(bank->block(8).count)*12u;reject(q);}
    {auto q=good;q.encoded_offset^=512u;reject(q);}
    {auto q=good;q.size_flags^=1u;reject(q);}
    {auto q=good;q.bytes+=512u;reject(q);}
    if(!actual.chunks.empty()){
        const auto at=actual.chunks.front().native_address;const Bytes p(bank->bytes().subspan(at-bank->address(),12));const auto raw=p.u32(4)&0x3fffffffu;
        const SourceSoundCacheReadTu15 q{at,p.u32(0),p.u32(4),(raw+511u)&~511u};
        auto bad=q;bad.encoded_offset^=512u;reject(bad);bad=q;bad.size_flags^=1u;reject(bad);bad=q;bad.bytes+=512u;reject(bad);
    }
    return J{{"original_sector_routes",routes},{"unique_original_descriptors",sectors.size()},{"validated_external_prefix_routes",prefix},
        {"empty_prefix_routes_refused",empty},{"negative_controls",controls},{"replica_resource_identity_equal",true},
        {"resident_binding_differs",true},{"moved_publication_identity_differs",true},{"legacy_prefix_bytes_read",0},{"live_world_verified",false}};
}
J canonical(const std::shared_ptr<PrefixFixture>& fixture,std::span<const OriginalSoundRuntimeBindingTu15> bindings,
    const OriginalSoundRuntimeStringsTu15& strings){
    const auto& base=fixture->native;const auto source_hash=hash(fixture->data);const auto before=base.bytes;
    const MccOriginalSoundRuntimeInputTu15 input{bindings,strings};
    const auto actual=stage_mcc_original_sound_runtime_delivery_tu15(fixture->view(),base,input,8*1024*1024u);
    check(actual.original&&actual.address==base.address&&actual.bytes.size()>base.bytes.size(),"Canonical original sound append lost its owner or allocation");
    actual.original->require_plan(actual);check(!actual.original->parent().original&&actual.original->parent().bytes==before,"Canonical original sound parent lineage changed");
    check(std::ranges::equal(std::span(actual.bytes).first(before.size()),before)&&base.bytes==before&&hash(fixture->data)==source_hash,"Canonical original sound append changed its caller/source prefix");
    auto graph=actual.original->bank();std::uint64_t roots{},untouched{},routes{},encoded{},refusals{};
    constexpr std::array<unsigned,11> strides{56,20,4,10,12,16,52,1,12,28,44};
    auto read=[](const MccSoundRuntimePlan& p,std::uint32_t at,std::size_t n){if(!n)return std::span<const std::byte>{};check(at>=p.address,"Canonical table precedes its owner");return Bytes(p.bytes).slice(at-p.address,n);};
    for(unsigned n=0;n<11;++n){check(actual.blocks[n].address==graph->block(n).address&&actual.blocks[n].count==graph->block(n).count,"Canonical root descriptor differs from its resource owner");
        check(std::ranges::equal(read(actual,actual.blocks[n].address,std::size_t(base.blocks[n].count)*strides[n]),read(base,base.blocks[n].address,std::size_t(base.blocks[n].count)*strides[n])),"Canonical append changed existing table rows/ordinals");}
    check(actual.sounds.size()==base.sounds.size()&&actual.patches.size()==base.patches.size()&&actual.gestalts.size()==base.gestalts.size(),"Canonical append discarded existing native owners");
    for(unsigned n=0;n<base.patches.size();++n){const auto& a=actual.patches[n];const auto& b=base.patches[n];
        check(a.owner==b.owner&&a.source_address==b.source_address&&a.before==b.before,"Canonical append changed a raw patch preimage or full identity");
        if(b.after.size()==88){check(std::ranges::equal(a.after,graph->bytes().subspan(graph->gestalt_root()-graph->address(),88)),"Canonical append omitted a shared gestalt root");++roots;}
        else if(actual.original->contains(a.owner)){check(std::ranges::equal(a.after,graph->bytes().subspan(graph->sound_root(a.owner)-graph->address(),20)),"Canonical original root differs from its exact graph binding");++roots;}
        else{check(a.after==b.after,"Canonical append changed an unrelated native definition/class root");++untouched;}
    }
    for(unsigned n=0;n<base.sounds.size();++n){const auto& a=actual.sounds[n];const auto& b=base.sounds[n];
        check(a.sound==b.sound&&a.gestalt==b.gestalt&&a.source_root==b.source_root&&a.native_root==b.native_root&&a.source_flags==b.source_flags&&a.remastered_index==b.remastered_index&&a.mcc_extension==b.mcc_extension,"Canonical append changed a raw/native definition identity");
        if(actual.original->contains(a.sound)){
            const auto d=graph->load(a.sound);check(a.file==MccSoundDataFile::Original&&a.native_flags==Bytes(d.sound).u16(0)&&a.codec.sample_rate==std::to_integer<unsigned>(d.sound[3])&&a.codec.encoding==std::to_integer<unsigned>(d.sound[4])&&a.codec.compression==std::to_integer<unsigned>(d.sound[5]),"Canonical original definition retained an MCC DAT/codec route");
            const auto binding=std::ranges::find(bindings,a.sound,&OriginalSoundRuntimeBindingTu15::sound);check(binding!=bindings.end(),"Canonical original route has no admitted bank");
            for(unsigned p=0;p<binding->bank->metadata().pitch_ranges.size();++p)for(unsigned v=0;v<binding->bank->metadata().pitch_ranges[p].permutations.size();++v){check(graph->encoded(a.sound,p,v).data==binding->bank->encoded(p,v).data,"Canonical native route changed original encoded data");++encoded;}
        }else check(a.file==b.file&&a.native_flags==b.native_flags&&a.codec.sample_rate==b.codec.sample_rate&&a.codec.encoding==b.codec.encoding&&a.codec.compression==b.codec.compression,"Canonical append changed an unrelated native codec/route");
    }
    check(actual.chunks.size()==base.chunks.size(),"Canonical append discarded existing MCC resource routes");
    for(unsigned n=0;n<base.chunks.size();++n){const auto& a=actual.chunks[n];const auto& b=base.chunks[n];check(a.gestalt==b.gestalt&&a.source_address==b.source_address&&a.source_index==b.source_index&&a.file_offset==b.file_offset&&a.encoded_bytes==b.encoded_bytes&&a.decoded_bytes==b.decoded_bytes&&a.reset_after==b.reset_after&&a.native_address==actual.blocks[8].address+(b.native_address-base.blocks[8].address),"Canonical append changed or misrebased an existing native chunk route");++routes;}
    auto reject=[&](auto&& call){bool bad=false;try{call();}catch(const std::exception&){bad=true;}check(bad,"Canonical original publication admitted a malformed/changed owner");++refusals;};
    {auto changed=actual;changed.bytes.back()^=std::byte{1};reject([&]{actual.original->require_plan(changed);});}
    {auto changed=actual;changed.patches.front().after.back()^=std::byte{1};reject([&]{actual.original->require_plan(changed);});}
    {auto changed=actual;++changed.chunks.front().file_offset;reject([&]{actual.original->require_plan(changed);});}
    {auto changed=actual;changed.sounds.front().file=MccSoundDataFile::Original;changed.sounds.front().native_flags^=1;reject([&]{actual.original->require_plan(changed);});}
    {auto changed=actual;changed.original.reset();reject([&]{actual.original->require_plan(changed);});}
    reject([&]{(void)stage_mcc_original_sound_runtime_delivery_tu15(fixture->view(),actual,input,8*1024*1024u);});
    reject([&]{(void)stage_mcc_original_sound_runtime_delivery_tu15(fixture->view(),base,input,std::uint32_t(base.bytes.size()));});
    {auto changed=base;changed.patches.front().before[0]^=std::byte{1};reject([&]{(void)stage_mcc_original_sound_runtime_delivery_tu15(fixture->view(),changed,input,8*1024*1024u);});}
    {auto changed=base;changed.patches.pop_back();reject([&]{(void)stage_mcc_original_sound_runtime_delivery_tu15(fixture->view(),changed,input,8*1024*1024u);});}
    {auto changed=base;changed.blocks[4].address=0xfffffff0u;reject([&]{(void)stage_mcc_original_sound_runtime_delivery_tu15(fixture->view(),changed,input,8*1024*1024u);});}
    {auto changed=base;changed.chunks.back().native_address=changed.chunks.front().native_address;reject([&]{(void)stage_mcc_original_sound_runtime_delivery_tu15(fixture->view(),changed,input,8*1024*1024u);});}
    {auto changed=base;changed.sounds.front().native_flags^=1;reject([&]{(void)stage_mcc_original_sound_runtime_delivery_tu15(fixture->view(),changed,input,8*1024*1024u);});}
    {auto changed=std::vector<OriginalSoundRuntimeBindingTu15>(bindings.begin(),bindings.end());changed.front().sound.value^=0x10000u;reject([&]{(void)stage_mcc_original_sound_runtime_delivery_tu15(fixture->view(),base,{changed,strings},8*1024*1024u);});}
    check(base.bytes==before&&hash(fixture->data)==source_hash,"A failed canonical publication modified caller/source memory");
    return J{{"changed_native_roots",roots},{"preserved_native_roots",untouched},{"rebased_mcc_chunk_routes",routes},{"original_encoded_routes",encoded},{"refusals",refusals},
        {"timing_join",timing_join(fixture,actual)},{"resource_join",resource_join(fixture,actual,input)}};
}
#endif
struct Sparse {
    std::map<std::uint32_t,std::vector<std::byte>> segments;
    void add(std::uint32_t at,std::vector<std::byte> bytes){check(segments.emplace(at,std::move(bytes)).second,"Duplicate declared reference slice");}
    std::span<std::byte> memory(std::uint32_t at,std::size_t n){auto i=segments.upper_bound(at);check(i!=segments.begin(),"Unprovided native reference field");--i;check(std::uint64_t(at-i->first)+n<=i->second.size(),"Unprovided native reference extent");return std::span(i->second).subspan(at-i->first,n);}
};
SoundVariationDefinition direct(const PreparedOriginalSoundBank& bank,unsigned pitch){
    const auto& m=bank.metadata();const auto& r=m.pitch_ranges.at(pitch);SoundVariationDefinition d;d.datum=m.origin->sound.value;d.sound=m.sound;d.playback=m.playback;d.scale=m.scale;d.sound_class=m.sound_class;d.range=r.literal;d.sample_rate_index=std::to_integer<unsigned>(m.sound[3]);
    for(const auto& p:m.pitch_ranges)d.pitch_ranges.push_back({p.interval,std::int16_t(p.permutations.size())});
    for(const auto& p:r.permutations)d.permutations.push_back(p.literal);
    if(r.permutations.size()>1&&!(Bytes(m.sound_class).u16(10)&0x20u)){check(r.used.has_value(),"Original required used owner is absent");d.used=*r.used;}return d;
}
void same_rng(const SoundLocalRandom& a,const SoundLocalRandom& b){check(a.seed==b.seed&&a.stores.size()==b.stores.size(),"Native table graph changed source RNG");for(unsigned n=0;n<a.stores.size();++n)check(a.stores[n].instruction==b.stores[n].instruction&&a.stores[n].before==b.stores[n].before&&a.stores[n].after==b.stores[n].after,"Native table graph changed ordered RNG stores");}
struct Aliases {
    std::map<std::pair<unsigned,unsigned>,unsigned> forward,reverse;
    void bind(unsigned table,unsigned original,unsigned native){
        auto [a,added]=forward.emplace(std::pair{table,original},native);
        auto [b,unique]=reverse.emplace(std::pair{table,native},original);
        check((added||a->second==native)&&(unique||b->second==original),"Native compaction changed original table aliasing");
    }
};
std::uint64_t literals(OriginalSoundRuntimeBankTu15& runtime,TagHandle sound,
    const PreparedOriginalSoundBank& bank,const Symbols& symbols,Aliases& aliases){
    const auto& m=bank.metadata();const auto d=runtime.load(sound);auto normalized=d.sound;
    for(unsigned at:{6u,7u,8u,9u,11u,12u})normalized[at]=m.sound[at];
    check(normalized==m.sound,"Native sound root changed non-rebased original fields");
    const Bytes source(m.sound),target(d.sound);
    aliases.bind(0,source.u16(6),target.u16(6));aliases.bind(1,std::to_integer<unsigned>(m.sound[11]),std::to_integer<unsigned>(d.sound[11]));
    std::uint64_t compared=1;
    auto imported=[&](unsigned old,unsigned index,unsigned original_id){
        aliases.bind(2,old,index);const auto block=runtime.block(2);check(index<unsigned(block.count),"Rebased import escapes its native table");
        const auto native=Bytes(runtime.memory(block.address+index*4u,4)).u32(0);
        check(native==symbols.forward.at(original_id).first,"Native table does not retain its actual imported StringID");++compared;
    };
    for(unsigned pitch=0;pitch<m.pitch_ranges.size();++pitch){const auto& r=m.pitch_ranges[pitch];const auto actual=runtime.load(sound,pitch);const Bytes a(actual.range),b(r.literal);
        aliases.bind(4,source.u16(8)+pitch,target.u16(8)+pitch);aliases.bind(3,b.u16(2),a.u16(2));imported(b.u16(0),a.u16(0),*r.import_name_id);
        auto range=actual.range;for(unsigned at:{0u,1u,2u,3u,8u,9u})range[at]=r.literal[at];
        if(r.used){range[6]=r.literal[6];range[7]=r.literal[7];for(unsigned n=0;n<r.used->size();++n){
            aliases.bind(7,(b.u16(6)>>3u)+n,(a.u16(6)>>3u)+n);const auto block=runtime.block(7);
            check(runtime.memory(block.address+(a.u16(6)>>3u)+n,1)[0]==(*r.used)[n],"Native used-byte owner changed literal state");++compared;}}
        check(range==r.literal,"Native pitch range changed non-rebased original fields");++compared;
        for(unsigned n=0;n<r.permutations.size();++n){const auto& p=r.permutations[n];const Bytes original(p.literal),native(actual.permutations[n]);
            aliases.bind(5,b.u16(8)+n,a.u16(8)+n);imported(original.u16(0),native.u16(0),*p.import_name_id);
            auto permutation=actual.permutations[n];for(unsigned at:{0u,1u,12u,13u})permutation[at]=p.literal[at];
            check(permutation==p.literal,"Native permutation changed non-rebased original fields");++compared;
            for(unsigned c=0;c<p.chunks.size();++c){aliases.bind(8,original.u16(12)+c,native.u16(12)+c);const auto block=runtime.block(8);
                const auto raw=runtime.memory(block.address+(native.u16(12)+c)*12u,12);auto chunk=std::vector<std::byte>(raw.begin(),raw.end());
                check(Bytes(raw).u32(0)%512u==0,"Native sector locator lost its real alignment");
                std::copy_n(p.chunks[c].literal->begin(),4,chunk.begin());check(std::ranges::equal(chunk,*p.chunks[c].literal),"Native chunk changed original flags or opaque word");++compared;
            }
        }
    }
    if(m.promotion){const auto old=std::to_integer<unsigned>(m.sound[12]),index=std::to_integer<unsigned>(d.sound[12]);aliases.bind(9,old,index);
        const auto block=runtime.block(9);const auto at=block.address+index*28u;const auto raw=runtime.memory(at,28);const Bytes actual(raw);auto row=std::vector<std::byte>(raw.begin(),raw.end());
        for(unsigned n:{4u,5u,6u,7u,12u,13u,14u,15u})row[n]=m.promotion->literal[n];
        check(std::ranges::equal(row,m.promotion->literal),"Native promotion changed non-rebased initial state");++compared;
        for(unsigned n=0;n<m.promotion->rules.size();++n){check(std::ranges::equal(runtime.memory(actual.u32(4)+n*16u,16),m.promotion->rules[n]),"Native promotion changed a literal rule");++compared;}
        for(unsigned n=0;n<m.promotion->counters.size();++n){check(std::ranges::equal(runtime.memory(actual.u32(12)+n*4u,4),m.promotion->counters[n]),"Native promotion changed an initial counter");++compared;}
    }
    return compared;
}
std::uint64_t variation(const SoundVariationDefinition& reference,const SoundVariationDefinition& native){
    check(reference.playback==native.playback&&reference.scale==native.scale&&reference.sound_class==native.sound_class&&reference.sample_rate_index==native.sample_rate_index&&reference.used==native.used&&reference.permutations.size()==native.permutations.size()&&reference.pitch_ranges.size()==native.pitch_ranges.size(),"Native variation inputs differ from original records");
    for(unsigned n=0;n<reference.pitch_ranges.size();++n)check(reference.pitch_ranges[n].interval==native.pitch_ranges[n].interval&&reference.pitch_ranges[n].permutations==native.pitch_ranges[n].permutations,"Native graph lost original pitch intervals");
    std::uint64_t compared{};
    for(auto seed:{0u,0x5821b36du,0xffffffffu,0x80000000u}){
        SoundLocalRandom a(seed),b(seed);auto ar=imported_sound_range_state(reference),br=imported_sound_range_state(native);
        check(ar.last==br.last&&ar.forced==br.forced&&ar.used==br.used,"Native graph changed initial range state");
        check(std::bit_cast<std::uint32_t>(source_sound_gain(reference,a))==std::bit_cast<std::uint32_t>(source_sound_gain(native,b)),"Native graph changed source gain");
        const auto pa=source_sound_pitch(reference,a),pb=source_sound_pitch(native,b);check(std::bit_cast<std::uint64_t>(pa)==std::bit_cast<std::uint64_t>(pb),"Native graph changed source pitch");
        for(float control:{0.f,.5f,1.f}){
            const auto ca=source_sound_control_pitch(reference,float(pa),control),cb=source_sound_control_pitch(native,float(pb),control);
            check(std::bit_cast<std::uint32_t>(ca)==std::bit_cast<std::uint32_t>(cb)&&source_sound_pitch_range(reference.pitch_ranges,-1,ca)==source_sound_pitch_range(native.pitch_ranges,-1,cb),"Native graph changed source range selection");
            check(std::bit_cast<std::uint32_t>(source_sound_control_gain(reference,control))==std::bit_cast<std::uint32_t>(source_sound_control_gain(native,control)),"Native graph changed source gain control");
        }
        for(unsigned n=0;n<40;++n){check(source_sound_permutation(reference,ar,a)==source_sound_permutation(native,br,b)&&ar.last==br.last&&ar.forced==br.forced&&ar.used==br.used,"Native graph changed source permutation/state");++compared;}
        same_rng(a,b);
    }return compared;
}
std::uint64_t promotion(OriginalSoundRuntimeBankTu15& runtime,TagHandle target,const PreparedOriginalSoundBank& bank,const J& ref,const SoundPromotionConstants& constants,J& coverage){
    const auto& m=bank.metadata();check(m.promotion.has_value(),"Promotion control requires its real owner");
    const auto magic=ref.at("magic").get<std::uint32_t>();const auto gh=ref.at("gestalt").get<std::uint32_t>();const auto sound=ref.at("sound").get<std::uint32_t>();
    const auto& pr=ref.at("promotion");const auto p=pr.at("literal").get<std::uint32_t>();const auto row=original(ref,p,28);const Bytes original_row(row);
    Sparse reference,extra;
    reference.add(gh+magic,original(ref,gh,88));reference.add(sound+magic,original(ref,sound,20));reference.add(p+magic,row);
    reference.add(original_row.u32(4),original(ref,std::uint32_t(original_row.u32(4)-magic),m.promotion->rules.size()*16));
    reference.add(original_row.u32(12),original(ref,std::uint32_t(original_row.u32(12)-magic),m.promotion->counters.size()*4));
    constexpr std::uint32_t tag_table=0x11000000u,system=0x12000000u,output=0x13000000u;
    auto reference_tag=original(ref,ref.at("sound_tag"),16);reference.add(tag_table+m.origin->sound.index()*16u,std::move(reference_tag));
    auto native_tag=original(ref,ref.at("sound_tag"),16);put(native_tag,4,target.value);put(native_tag,8,runtime.sound_root(target));extra.add(tag_table+target.index()*16u,std::move(native_tag));
    reference.add(system+0x84u,std::vector<std::byte>(4));reference.add(output,std::vector<std::byte>(4));extra.add(system+0x84u,std::vector<std::byte>(4));extra.add(output,std::vector<std::byte>(4));
    const auto native_sound=runtime.load(target);const auto native_row=runtime.block(9).address+std::to_integer<unsigned>(native_sound.sound[12])*28u;
    const auto native_counter=Bytes(runtime.memory(native_row,28)).u32(12);const auto native_rules=Bytes(runtime.memory(native_row,28)).u32(4);
    const auto saved_row=std::vector<std::byte>(runtime.memory(native_row,28).begin(),runtime.memory(native_row,28).end());
    const auto counter_bytes=m.promotion->counters.size()*4u;const auto saved_counters=std::vector<std::byte>(runtime.memory(native_counter,counter_bytes).begin(),runtime.memory(native_counter,counter_bytes).end());
    check(std::ranges::equal(runtime.memory(native_rules,m.promotion->rules.size()*16),reference.memory(original_row.u32(4),m.promotion->rules.size()*16)),"Native promotion reordered its sound-local pitch selectors");
    SoundPromotionServices a,b;a.memory=[&](auto at,auto n){return reference.memory(at,n);};
    b.memory=[&](auto at,auto n)->std::span<std::byte>{if(at>=runtime.address()&&std::uint64_t(at)+n<=std::uint64_t(runtime.address())+runtime.bytes().size())return runtime.memory(at,n);return extra.memory(at,n);};
    std::uint64_t compared{};std::set<unsigned> selectors;std::set<std::int32_t> returns;
    auto event=[&](const SoundStartupStore& s,bool native){const auto row_base=native?native_row:p+magic,counter=native?native_counter:original_row.u32(12);std::string owner;std::uint32_t offset{};
        if(s.address>=row_base&&s.address+s.bytes<=row_base+28u){owner="promotion";offset=s.address-row_base;}
        else if(s.address>=counter&&std::uint64_t(s.address)+s.bytes<=std::uint64_t(counter)+m.promotion->counters.size()*4){owner="counter";offset=s.address-counter;}
        else{check(s.address==output&&s.bytes==4,"Promotion wrote an unprovided owner");owner="selector";}
        return J::array({s.instruction,owner,offset,s.bytes,s.before,s.after});};
    for(unsigned call=0;call<promotion_burst_calls+promotion_tail_calls;++call){const auto tick=call<promotion_burst_calls?1000u:4000u+(call-promotion_burst_calls)*20u;put(reference.memory(system+0x84u,4),0,tick);put(extra.memory(system+0x84u,4),0,tick);
        J ae=J::array(),be=J::array();const auto ar=source_sound_promote({system,tag_table,gh+magic,0},m.origin->sound.value,output,constants,a,[&](const auto& s){ae.push_back(event(s,false));});
        const auto br=source_sound_promote({system,tag_table,runtime.gestalt_root(),0},target.value,output,constants,b,[&](const auto& s){be.push_back(event(s,true));});
        check(ar==br&&ae==be&&std::ranges::equal(reference.memory(output,4),extra.memory(output,4)),"Native promotion changed return, selectors or ordered stores");
        const auto selector=Bytes(reference.memory(output,4)).u32(0);check(selector<m.promotion->rules.size(),"Original promotion returned a selector outside its declared rules");
        selectors.insert(selector);returns.insert(ar);const auto pitch=Bytes(m.promotion->rules[selector]).u16(0);
        auto& counts=coverage[m.name][std::to_string(selector)+":"+std::to_string(pitch)];counts=counts.is_null()?1u:counts.get<unsigned>()+1u;
        compared+=ae.size();
    }
    // Each comparison starts from the authenticated initial state, including
    // when two original sounds genuinely share the same promotion row.
    std::ranges::copy(saved_row,runtime.memory(native_row,28).begin());std::ranges::copy(saved_counters,runtime.memory(native_counter,counter_bytes).begin());
    check(selectors.size()==m.promotion->rules.size()&&returns.contains(0)&&returns.contains(1),"Promotion controls missed an authored rule or suppression branch");
    return compared;
}
}
int main(int argc,char** argv){J report{{"passed",false},{"live_world_or_voice_binding_verified",false},{"declared_promotion_clock_inputs",true},{"xbox_waveform_or_device_verified",false},{"raw_audio_retained",false}};try{
    check(argc==3,"Supply owned native runtime plan and bounded output");std::ifstream input(argv[1]);const auto plan=J::parse(input);std::ifstream wave_file(plan.at("waveforms").get<std::string>());const auto waves=J::parse(wave_file);
    std::map<std::array<std::string,3>,J> expected;for(const auto& row:waves.at("cases"))expected.emplace(row.at("key").get<std::array<std::string,3>>(),row);
    const auto raw_constants=plan.at("promotion_constants").get<std::array<std::uint32_t,3>>();const SoundPromotionConstants constants{raw_constants[0],raw_constants[1],raw_constants[2]};
    std::set<std::array<std::string,3>> decoded;std::map<std::string,std::shared_ptr<const PreparedOriginalSoundBank>> previous;
    std::uint64_t bindings{},permutations{},sectors{},variations{},promotion_calls{},promotion_stores{},rejections{},string_changes{},literal_comparisons{},mixed_namespace_rejections{},prefix_rows{},prefix_bytes{},prefix_sounds{},cache_reads{},cache_refusals{},descriptor_collisions{};
    for(const auto& group:plan.at("groups")){
        report["active_map"]=group.at("map");auto tags=std::make_shared<std::vector<Tag>>();for(const auto& tag:group.at("tags"))tags->push_back({tag.at("group"),tag.at("name"),tag.at("datum"),tag.at("address"),tag.at("size")});
        auto symbols=std::make_shared<Symbols>();for(const auto& row:group.at("strings")){const auto old=row.at("original").get<std::uint32_t>(),native=row.at("native").get<std::uint32_t>();const auto name=row.at("name").get<std::string>();symbols->forward.emplace(old,std::pair{native,name});auto [i,added]=symbols->names.emplace(native,name);check(added||i->second==name,"Native StringID namespace is inconsistent");string_changes+=old!=native;}
        OriginalSoundRuntimeStringsTu15 strings{symbols,[symbols](auto old,auto label){const auto& row=symbols->forward.at(old);check(row.second==label,"Original StringID label differs");return row.first;},[symbols](auto id){return symbols->names.at(id);}};
        std::vector<OriginalSoundRuntimeBindingTu15> input_bindings;for(const auto& row:group.at("bindings")){const auto& c=row.at("case");input_bindings.push_back({{row.at("sound")},PreparedOriginalSoundBank::load(c.at("directory").get<std::string>(),c.at("sha256").get<std::string>())});}
        OriginalSoundRuntimePrefixTu15 prefix;
#if HALO2_TEST_ORIGINAL_NATIVE_PREFIX
        auto [fixture,actual_prefix]=PrefixFixture::load(group.at("prefix_source"));prefix=actual_prefix;prefix_sounds+=fixture->native.sounds.size();
        report["canonical_publication"][group.at("map").get<std::string>()]=canonical(fixture,input_bindings,strings);
#endif
        auto runtime=OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,input_bindings,strings,prefix);
        auto translated=OriginalSoundRuntimeBankTu15::create(0x39000000u,8*1024*1024,tags,input_bindings,strings,prefix);
        constexpr std::array<unsigned,11> strides{56,20,4,10,12,16,52,1,12,28,44};
        for(unsigned n=0;n<11;++n)if(!prefix.tables[n].empty()){
            check(std::ranges::equal(runtime->memory(runtime->block(n).address,prefix.tables[n].size()),prefix.tables[n]),"Original append changed an existing native prefix table");
            prefix_rows+=prefix.tables[n].size()/strides[n];prefix_bytes+=prefix.tables[n].size();
        }
        Aliases aliases;const Bytes class_root(runtime->memory(runtime->class_root(),8));check(class_root.u32(0)==54,"Native bank lost its complete original class owner");
        for(unsigned n=0;n<54;++n){const auto expected=prefix.owner?prefix.sound_classes.subspan(n*92u,92):std::span<const std::byte>((*input_bindings.front().bank->metadata().sound_classes)[n]);check(std::ranges::equal(runtime->memory(class_root.u32(4)+n*92u,92),expected),"Native bank changed its retained class owner");++literal_comparisons;}
        for(unsigned i=0;i<input_bindings.size();++i){const auto& binding=input_bindings[i];const auto& bank=*binding.bank;const auto& m=bank.metadata();
            literal_comparisons+=literals(*runtime,binding.sound,bank,*symbols,aliases);
            report["active_sound"]=m.name;const auto primary=runtime->load(binding.sound);const auto relocated=translated->load(binding.sound);check(primary.sound==relocated.sound&&primary.range==relocated.range&&primary.permutations==relocated.permutations,"Native metadata depends on its allocation base");
            for(unsigned pitch=0;pitch<m.pitch_ranges.size();++pitch){const auto& range=m.pitch_ranges[pitch];auto actual=runtime->load(binding.sound,pitch);variations+=variation(direct(bank,pitch),actual);
                for(unsigned n=0;n<range.permutations.size();++n){const auto encoded=runtime->encoded(binding.sound,pitch,n);const auto second=translated->encoded(binding.sound,pitch,n);const auto& original=bank.encoded(pitch,n);
                    check(encoded.data==original.data&&second.data==original.data&&encoded.sample_rate==original.sample_rate&&encoded.channels==original.channels&&encoded.codec==original.codec&&encoded.samples==original.samples&&encoded.sound_class==original.sound_class,"Native encoded route changed original data/format");
                    const auto& p=actual.permutations[n];const auto first=Bytes(p).u16(12),count=Bytes(p).u16(14);check(count==range.permutations[n].chunks.size(),"Native resource interval changed chunk count");
                    for(unsigned c=0;c<count;++c){const auto& source=range.permutations[n].chunks[c];std::vector<std::byte> result(source.sector_bytes);const auto descriptor=runtime->block(8).address+(first+c)*12u;
                        runtime->read_resource(descriptor,0,result);check(std::ranges::equal(result,bank.sectors().subspan(source.offset,source.sector_bytes)),"Native resource read changed original sector/tail");++sectors;
                        const Bytes raw(runtime->memory(descriptor,12));const SourceSoundCacheReadTu15 request{descriptor,raw.u32(0),raw.u32(4),source.sector_bytes};
                        check(runtime->read_cache_resource(request)==result,"Exact native sound-cache request changed original sectors");++cache_reads;
                        auto reject_cache=[&](const auto& input){bool refused=false;try{(void)runtime->read_cache_resource(input);}catch(const FormatError&){refused=true;}check(refused,"Native sound-cache read accepted a foreign descriptor, locator or extent");++cache_refusals;};
                        auto wrong=request;wrong.encoded_offset^=512u;reject_cache(wrong);wrong=request;wrong.size_flags^=0x40000000u;reject_cache(wrong);
                        wrong=request;--wrong.bytes;reject_cache(wrong);wrong=request;++wrong.descriptor;reject_cache(wrong);
                        wrong=request;wrong.descriptor=translated->block(8).address+(first+c)*12u;
                        check(Bytes(translated->memory(wrong.descriptor,8)).u32(0)==request.encoded_offset&&Bytes(translated->memory(wrong.descriptor,8)).u32(4)==request.size_flags,
                            "Independent allocation did not retain the same encoded locator/flags");reject_cache(wrong);++descriptor_collisions;
                    }
                    const std::array<std::string,3> key{m.name,range.name,range.permutations[n].name};const auto& reference=expected.at(key);check(hash(encoded.data)==reference.at("original_encoded_sha256").get<std::string>(),"Native route differs from independent encoded audit");
                    if(decoded.insert(key).second){const auto pcm=decode_sound(encoded);check(pcm.frames()==reference.at("original_decoded_frames")&&hash(std::as_bytes(std::span(pcm.stereo)))==reference.at("original_pcm_sha256").get<std::string>(),"Native route changed same-PC original PCM");}++permutations;
                }
            }
            if(m.promotion){promotion_stores+=promotion(*runtime,binding.sound,bank,group.at("bindings")[i].at("case").at("reference"),constants,report["promotion_selector_to_pitch_coverage"]);promotion_calls+=promotion_burst_calls+promotion_tail_calls;}
            bool refused=false;try{(void)runtime->load({binding.sound.value^0x10000u});}catch(const FormatError&){refused=true;}check(refused,"Native bank accepted a stale salted tag");++rejections;++bindings;
        }
        auto refuse=[&](auto&& f){bool refused=false;try{f();}catch(const std::exception&){refused=true;}check(refused,"Native owner accepted an invalid namespace or extent");++rejections;};
        refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0xfffffffcu,4096,tags,input_bindings,strings);});
        refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,4,tags,input_bindings,strings);});
        auto duplicate=input_bindings;duplicate.push_back(duplicate.front());refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,duplicate,strings);});
        if(group.contains("unowned")){const auto& c=group.at("unowned");auto unowned=input_bindings;unowned.front().bank=PreparedOriginalSoundBank::load(c.at("directory").get<std::string>(),c.at("sha256").get<std::string>());
            check(!unowned.front().bank->metadata().origin,"Offline compatibility control unexpectedly has runtime provenance");
            refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,unowned,strings);});}
        auto aliased_tags=std::make_shared<std::vector<Tag>>(*tags);auto stale=aliased_tags->front();stale.datum^=0x10000u;aliased_tags->push_back(stale);
        refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,aliased_tags,input_bindings,strings);});
        auto mistyped=std::make_shared<std::vector<Tag>>(*tags);mistyped->front().group="effe";
        refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,mistyped,input_bindings,strings);});
        for(unsigned n=1;n<input_bindings.size();++n){const auto found=previous.find(input_bindings[n].bank->metadata().name);if(found==previous.end())continue;
            auto mixed=input_bindings;mixed[n].bank=found->second;refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,mixed,strings);});++mixed_namespace_rejections;break;}
        for(const auto& binding:input_bindings)previous[binding.bank->metadata().name]=binding.bank;
        auto wrong_strings=strings;wrong_strings.lookup=[](auto){return std::string("unmatched");};refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,input_bindings,wrong_strings);});
        if(prefix.owner){
            refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,4,tags,input_bindings,strings,prefix);});
            auto no_owner=prefix;no_owner.owner.reset();refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,input_bindings,strings,no_owner);});
            auto short_class=prefix;short_class.sound_classes=short_class.sound_classes.first(53*92u);refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,input_bindings,strings,short_class);});
            auto partial_row=prefix;partial_row.tables[4]=partial_row.tables[4].first(partial_row.tables[4].size()-1u);refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,input_bindings,strings,partial_row);});
            auto changed_class=std::vector<std::byte>(prefix.sound_classes.begin(),prefix.sound_classes.end());const auto cls=std::to_integer<unsigned>(input_bindings.front().bank->metadata().sound[2]);changed_class[cls*92u]^=std::byte{1};
            auto incompatible=prefix;incompatible.sound_classes=changed_class;refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,input_bindings,strings,incompatible);});
            auto huge=prefix;std::vector<std::byte> scale(128u*20u);huge.tables[1]=scale;refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,input_bindings,strings,huge);});
            std::vector<std::byte> chunks(32768u*12u);huge=prefix;huge.tables[8]=chunks;refuse([&]{(void)OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,input_bindings,strings,huge);});
            // Caller views and their wrapper can be released from resolution;
            // the admitted graph retains the complete prefix lifetime.
            auto lifetime=std::make_shared<std::shared_ptr<const void>>(prefix.owner);std::weak_ptr<std::shared_ptr<const void>> weak=lifetime;
            auto released=prefix;released.owner=lifetime;auto resetting=strings;const auto resolve=strings.resolve;
            resetting.resolve=[&](auto old,auto label){released={};lifetime.reset();return resolve(old,label);};
            auto retained=OriginalSoundRuntimeBankTu15::create(0x41000000u,8*1024*1024,tags,input_bindings,resetting,released);
            check(!weak.expired()&&retained->encoded(input_bindings.front().sound,0,0).data==input_bindings.front().bank->encoded(0,0).data,"Native prefix lost ownership during StringID resolution");
            retained.reset();check(weak.expired(),"Native prefix leaked its retained wrapper");
        }
#if HALO2_TEST_ORIGINAL_NATIVE_PREFIX
        {auto changed=*fixture;auto globals=std::ranges::find_if(changed.tags,[](const auto& t){return t.group=="matg"&&t.address;});globals->size-=4;
            refuse([&]{(void)mcc_sound_class_detail::stage_qualified(changed.view(),fixture->native);});}
        {auto changed=*fixture;const auto globals=std::ranges::find_if(changed.tags,[](const auto& t){return t.group=="matg"&&t.address;});
            const auto sg=Bytes(changed.read(globals->address+0xc0u,8)).u32(4);const auto legacy=Bytes(changed.read(sg,44)).u32(12);const auto tag=std::ranges::find(changed.tags,legacy,&Tag::datum);
            const auto at=Bytes(changed.read(tag->address,8)).u32(4)+15u*120u+119u;auto allocation=changed.allocations.upper_bound(at);--allocation;
            changed.data[allocation->second.offset+at-allocation->second.at]^=std::byte{1};
            refuse([&]{(void)mcc_sound_class_detail::stage_qualified(changed.view(),fixture->native);});}
#endif
        refuse([&]{(void)runtime->memory(runtime->address()-1u,1);});std::array<std::byte,1> byte{};
        refuse([&]{runtime->read_resource(runtime->block(8).address+1u,0,byte);});
        if(prefix.owner){refuse([&]{runtime->read_resource(runtime->block(8).address,0,byte);});
            const auto descriptor=runtime->block(8).address;const Bytes raw(runtime->memory(descriptor,12));
            refuse([&]{(void)runtime->read_cache_resource({descriptor,raw.u32(0),raw.u32(4),512});});}
        const auto chunk_base=runtime->block(8).address;const auto descriptor=chunk_base+runtime->original_chunk_first()*12u;const auto original_locator=Bytes(runtime->memory(descriptor,4)).u32(0);
        put(runtime->memory(descriptor,4),0,original_locator^512u);refuse([&]{runtime->read_resource(descriptor,0,byte);});put(runtime->memory(descriptor,4),0,original_locator);
        const auto chunk_root=runtime->gestalt_root()+64u;const auto chunk_count=runtime->block(8).count;
        put(runtime->memory(chunk_root,8),0,0);put(runtime->memory(chunk_root,8),4,0);
        // Resource operations own the exact admitted physical descriptor,
        // independent of the mutable UGH table selector used by metadata.
        runtime->read_resource(descriptor,0,byte);put(runtime->memory(chunk_root,8),0,std::uint32_t(chunk_count));put(runtime->memory(chunk_root,8),4,chunk_base);
        const auto original_classes=Bytes(runtime->memory(runtime->class_root(),8)).u32(4);
        put(runtime->memory(runtime->class_root(),8),4,0xfffffff0u);refuse([&]{(void)runtime->load(input_bindings.front().sound);});put(runtime->memory(runtime->class_root(),8),4,original_classes);
        report.erase("active_sound");
    }
    check(bindings==908&&decoded.size()==273&&promotion_calls>0&&string_changes>0&&mixed_namespace_rejections>0,"Incomplete native original owner coverage");report.erase("active_map");
    report.update(J{{"passed",true},{"native_namespaces",plan.at("groups").size()},{"sound_bindings",bindings},{"literal_comparisons",literal_comparisons},{"permutation_routes",permutations},{"sector_reads",sectors},{"sound_cache_sector_reads",cache_reads},{"sound_cache_invalid_request_refusals",cache_refusals},{"same_locator_foreign_descriptor_refusals",descriptor_collisions},{"variation_comparisons",variations},{"promotion_calls",promotion_calls},{"promotion_ordered_store_comparisons",promotion_stores},{"unique_original_pcm_comparisons",decoded.size()},{"rejected_invalid_inputs",rejections},{"mixed_namespace_rejections",mixed_namespace_rejections},{"native_string_id_rebases",string_changes},{"independent_allocation_bases",2},{"preserved_prefix_rows",prefix_rows},{"preserved_prefix_bytes",prefix_bytes},{"prefix_sound_definitions",prefix_sounds}});
}catch(const std::exception& error){report["error"]=error.what();}
if(argc==3){std::ofstream output(argv[2]);output<<report.dump(2)<<'\n';if(!output)return 2;}
std::cout<<report.value("error",std::string("Native original metadata/resources, variation and promotion match"))<<'\n';return report.at("passed").get<bool>()?0:1;}
