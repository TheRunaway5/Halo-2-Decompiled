#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/mcc_model_node_map.hpp"
#include "halo2/mcc_animation.hpp"
#include "halo2/mcc_physics.hpp"
#include "halo2/mcc_ordinary_pointer.hpp"
#include "halo2/mcc_sound_tag_layout.hpp"
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/mcc_sound_vanilla_reference.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <type_traits>
#include <sstream>

namespace {
using namespace halo2;
static_assert(!std::is_default_constructible_v<MccRuntimeCacheDelivery>);
static_assert(std::is_move_constructible_v<MccRuntimeCacheDelivery>);
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F action){try{action();}catch(const FormatError&){return;}throw std::runtime_error("Unsupported canonical delivery was published");}
std::uint64_t hash(std::span<const std::byte> bytes){std::uint64_t v=14695981039346656037ull;for(auto b:bytes)v=(v^std::to_integer<unsigned>(b))*1099511628211ull;return v;}
void reservation_fixture(){
    std::vector<std::byte> data(8192);
    const auto word=[&](std::size_t at,std::uint32_t value){for(unsigned n=0;n<4;++n)data.at(at+n)=std::byte(value>>(8*n));};
    const auto text=[&](std::size_t at,std::string_view value){for(auto c:value)data.at(at++)=std::byte(c);};
    word(0,0x68656164);word(4,8);word(8,data.size());word(2044,0x666f6f74);
    word(16,2048);word(20,1024);word(24,1024);word(0x1c,0x4000);
    text(288,"02.09.27.09809");text(408,"reservation");
    word(2048,0x80061020);word(2052,1);word(2056,0x8006102c);
    word(2060,0xffffffffu);word(2064,0xe0000000u);word(2072,1);word(2076,0x74616773);
    word(2080,tag_groups::globals.value);word(2084,0xffffffffu);word(2088,0xffffffffu);
    word(2092,tag_groups::globals.value);word(2096,0xe0000000u);word(2100,0x80062000);word(2104,16);
    word(708,4096);word(712,64);word(716,4160);text(4096,"globals\\reservation");
    struct File {std::filesystem::path path;~File(){std::error_code e;std::filesystem::remove(path,e);}} file{
        std::filesystem::temp_directory_path()/("halo2-reservation-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".map")};
    {std::ofstream out(file.path,std::ios::binary);out.write(reinterpret_cast<const char*>(data.data()),data.size());check(bool(out),"Reservation fixture write failed");}
    Cache cache(file.path);
    const auto view=cache.view_source_runtime_cache_tags();const auto image=cache.load_source_runtime_cache_tag_image();
    check(view.reservation_bytes==0x4000&&image.reservation_bytes==view.reservation_bytes,"Owned Xbox reservation changed header+1C");
    check(view.index==0x80061000&&view.regions[1].address==0x80062000,"Reservation changed existing logical ranges");
    for(unsigned n=0;n<2;++n)check(image.regions[n].address==view.regions[n].address&&std::ranges::equal(image.regions[n].bytes,view.regions[n].bytes),"Owned reservation changed cache delivery bytes");
}
}
int main(int argc,char**argv)try{
    using namespace halo2;
    reservation_fixture();
    const std::array groups{TagGroupDefinition{tag_groups::globals,{},{}},TagGroupDefinition{tag_groups::scenario,{},{}}};
    std::array<Tag,4> tags{{{"scnr","scenarios\\original",0xe1000000u,0x80100000u,64},
        {"matg","globals\\original",0xe2000001u,0x80100100u,128},
        {"","",0xffffffffu,0,0},{"matg","globals\\empty_alias",0xe3000003u,0x80100120u,0}}};
    const auto index=build_canonical_runtime_cache_index(groups,tags,{tags[0].datum},{tags[1].datum},0x80061000u);
    const Bytes bytes(index.region.bytes);const auto table=std::size_t(index.table-index.region.address);
    check(index.region.bytes.size()==120&&index.table==0x80061038u,"Canonical index extent/table differs");
    check(bytes.u32(0)==0x80061020u&&bytes.u32(4)==2&&bytes.u32(8)==index.table&&
        bytes.u32(12)==0xe1000000u&&bytes.u32(16)==0xe2000001u&&bytes.u32(24)==4&&bytes.u32(28)==0x74616773u,"Canonical index header differs");
    for(unsigned n=0;n<tags.size();++n){
        check(bytes.u32(table+n*16+4)==tags[n].datum&&bytes.u32(table+n*16+8)==tags[n].address&&bytes.u32(table+n*16+12)==tags[n].size,"Canonical identity/address/extent changed");
        check(bytes.u32(table+n*16)==(tags[n].handle().is_null()?0xffffffffu:tags[n].type().value),"Canonical group omitted");
    }
    check(reinterpret_cast<std::uintptr_t>(index.region.bytes.data())>0xffffffffull,"Canonical index backing was narrowed");
    auto invalid=tags;invalid[0].datum=0xe1000001u;rejects([&]{(void)build_canonical_runtime_cache_index(groups,invalid,{tags[0].datum},{tags[1].datum},0x80061000u);});
    invalid=tags;invalid[2].address=1;rejects([&]{(void)build_canonical_runtime_cache_index(groups,invalid,{tags[0].datum},{tags[1].datum},0x80061000u);});
    invalid=tags;invalid[1].group="weap";rejects([&]{(void)build_canonical_runtime_cache_index(groups,invalid,{tags[0].datum},{tags[1].datum},0x80061000u);});
    auto bad_groups=groups;std::swap(bad_groups[0],bad_groups[1]);rejects([&]{(void)build_canonical_runtime_cache_index(bad_groups,tags,{tags[0].datum},{tags[1].datum},0x80061000u);});
    bad_groups=groups;bad_groups[0].parent=tag_groups::object;rejects([&]{(void)build_canonical_runtime_cache_index(bad_groups,tags,{tags[0].datum},{tags[1].datum},0x80061000u);});
    rejects([&]{(void)build_canonical_runtime_cache_index(groups,tags,{tags[0].datum},{tags[1].datum},0xfffffff0u);});
    rejects([&]{(void)build_canonical_runtime_cache_index(groups,tags,{tags[0].datum},{tags[1].datum},0x80061001u);});
    const std::array ranges{MccRuntimeRelocation{0x2000,0x80200000u,64},MccRuntimeRelocation{0x1000,0x80100000u,128}};
    for(unsigned n=0;n<128;++n)check(relocate_mcc_runtime_address(ranges,0x1000+n,128-n)==0x80100000u+n,"Piecewise metadata translation differs");
    check(relocate_mcc_runtime_address(ranges,0x2040,0)==0x80200040u,"Empty endpoint alias changed");
    for(auto p:{0u,0xffffffffu,0x1081u,0x1800u})rejects([&]{(void)relocate_mcc_runtime_address(ranges,p,1);});
    rejects([&]{(void)relocate_mcc_runtime_address(ranges,0x107f,2);});
    rejects([&]{(void)relocate_mcc_runtime_address(ranges,0x1000,std::numeric_limits<std::size_t>::max());});
    auto overlapping=ranges;overlapping[0].source=0x107f;rejects([&]{(void)relocate_mcc_runtime_address(overlapping,0x1000,1);});
    overlapping=ranges;overlapping[0].target=0x8010007fu;rejects([&]{(void)relocate_mcc_runtime_address(overlapping,0x1000,1);});
    const std::array empty_locations{MccRuntimeRelocation{0x3000,0x80300000u,0},ranges[0],ranges[1]};
    check(relocate_mcc_runtime_address(empty_locations,0x3000,0)==0x80300000u,"Declared empty location was discarded");
    rejects([&]{(void)relocate_mcc_runtime_address(empty_locations,0x3000,1);});
    auto conflicting=empty_locations;conflicting[0]={0x1040,0x80300000u,0};
    rejects([&]{(void)relocate_mcc_runtime_address(conflicting,0x1040,0);});
    conflicting=empty_locations;conflicting[0]={0x3000,0x80100040u,0};
    rejects([&]{(void)relocate_mcc_runtime_address(conflicting,0x3000,0);});
    conflicting=empty_locations;conflicting[0]={0x1040,0x80100040u,0};
    check(relocate_mcc_runtime_address(conflicting,0x1040,0)==0x80100040u,"Owned empty endpoint alias changed");
    unsigned actual=0,issues=0;std::size_t verified_fields=0,verified_bytes=0;
    if(argc>=2){
        Cache cache(argv[1]);const auto before=hash(cache.source_runtime_cache_bytes());
        unsigned sound_captures=0;
        std::optional<MccSoundRuntimePlan> captured_sound;
        const auto plan=MccRuntimeCacheDelivery::plan(cache,0x80061000u,256u*1024*1024,
            [&](MccSoundRuntimePlan&& value){++sound_captures;captured_sound.emplace(std::move(value));});
        check(sound_captures==1&&captured_sound.has_value(),"Complete delivery did not transfer its sound owner exactly once");
        check(bool(captured_sound->authored_flags),"Sound delivery omitted the authored flag admission result");
        if(cache.name()=="03a_oldmombasa"||cache.name()=="lockout"||cache.name()=="01b_spacestation") {
            const auto expected=cache.name()=="03a_oldmombasa"?1u:0u;
            check(captured_sound->authored_flags->corrections.size()==expected,
                "Required retail authored sound corrections were not admitted");
            unsigned admitted=0;
            for(const auto& owner:captured_sound->authored_flags->owners) {
                check(owner.status!=MccSoundVanillaOwnerStatus::Custom,
                    "Retail sound owner was silently classified custom");
                admitted+=owner.status==MccSoundVanillaOwnerStatus::Authenticated;
            }
            check(admitted==expected,"Required retail authored sound owner was not authenticated");
            std::cout<<"Canonical authored sound admission: corrections="<<expected<<" owners="<<admitted<<'\n';
        }
        if(!plan.issues().empty())
            throw std::runtime_error("Actual MCC delivery has an unclosed schema: "+plan.issues().front().reason);
        for(const auto& issue:plan.issues())
            check(issue.group.string()!="phmo"&&issue.group.string()!="coll"&&issue.group.string()!="jmad",
                "A normalized physics or animation graph remained unclosed");
        check(plan.reservation_bytes()%4096==0&&plan.reservation_bytes()>=plan.index().region.bytes.size(),"Canonical page reservation differs");
        check(plan.index().count==cache.tags().size()&&plan.index().scenario==cache.scenario_tag().value&&plan.index().globals==cache.globals_tag().value,"Canonical selected identities changed");
        const Bytes planned(plan.index().region.bytes);const auto rows=plan.index().table-plan.index().region.address;
        for(std::size_t n=0;n<cache.tags().size();++n){
            const auto& tag=cache.tags()[n];
            check(planned.u32(rows+n*16+4)==tag.datum,"Original salted index changed");
            if(!tag.handle().is_null())check(planned.u32(rows+n*16)==tag.type().value,"Original group changed");
            if(!tag.handle().is_null()&&tag.address&&!tag.size) {
                const auto logical=planned.u32(rows+n*16+8);
                check(logical&&planned.u32(rows+n*16+12)==0,"Empty source alias acquired payload or lost its location");
                check(plan.source_address(logical,0)==tag.address,"Empty source alias lost its exact logical identity");
            }
        }
        for(const auto& retained:cache.retained_tag_allocations()) {
            const auto* owner=cache.find_tag(retained.handle(),retained.type());
            check(owner&&owner->name==retained.name&&owner->address!=retained.address&&retained.size,
                "Retained allocation lost its original typed identity");
            const auto logical=relocate_mcc_runtime_address(plan.relocations(),retained.address,retained.size);
            check(plan.source_address(logical,retained.size)==retained.address,"Retained definition lost its declared owner extent");
        }
        for(const auto& r:plan.relocations()) {
            check(plan.source_address(r.target,r.bytes)==r.source,"Canonical reverse resource mapping differs");
            check(plan.runtime_address(r.source,r.bytes)==r.target,"Canonical forward resource mapping differs");
            if(r.bytes)check(plan.runtime_address(r.source+r.bytes-1,1)==r.target+r.bytes-1,
                "Canonical final payload byte lost its declared allocation owner");
        }
        const auto image=plan.tag_image();std::vector<SourceRuntimeCacheBspImage> bsps;
        for(std::size_t n=0;n<cache.bsps().size();++n)bsps.push_back(plan.bsp_image(n));
        const auto read=[&](std::uint32_t logical,std::size_t bytes) {
            for(const auto& region:image.regions)if(logical>=region.address&&
                std::uint64_t(logical)-region.address+bytes<=region.bytes.size())
                return Bytes(region.bytes).slice(logical-region.address,bytes);
            for(const auto& bsp:bsps)if(logical>=bsp.region.address&&
                std::uint64_t(logical)-bsp.region.address+bytes<=bsp.region.bytes.size())
                return Bytes(bsp.region.bytes).slice(logical-bsp.region.address,bytes);
            throw std::runtime_error("Published pointer has no owned runtime region");
        };
        std::unordered_set<std::uint32_t> patched,sound_root_bytes;
        std::uint64_t sound_address=0;
        for(const auto& r:plan.relocations())
            sound_address=std::max(sound_address,(std::uint64_t(r.target)+(r.bytes?r.bytes:16)+15)&~std::uint64_t(15));
        check(sound_address<=0xffffffffu,"Native sound append address overflowed");
        const auto sound=plan_mcc_sound_runtime_delivery(cache,std::uint32_t(sound_address),
            [&](std::uint32_t at,std::size_t bytes){return plan.runtime_address(at,bytes);});
        check(bool(sound.authored_flags)&&
            sound.authored_flags->corrections.size()==captured_sound->authored_flags->corrections.size()&&
            sound.authored_flags->owners.size()==captured_sound->authored_flags->owners.size(),
            "Captured authored sound admission differs from the independent plan");
        for(std::size_t n=0;n<sound.authored_flags->corrections.size();++n) {
            const auto& a=sound.authored_flags->corrections[n];
            const auto& b=captured_sound->authored_flags->corrections[n];
            check(a.sound==b.sound&&a.gestalt==b.gestalt&&a.source_root==b.source_root&&
                a.source_flags==b.source_flags&&a.native_before==b.native_before&&a.native_after==b.native_after,
                "Captured authored sound correction identity differs");
        }
        for(std::size_t n=0;n<sound.authored_flags->owners.size();++n)
            check(sound.authored_flags->owners[n].gestalt==captured_sound->authored_flags->owners[n].gestalt&&
                sound.authored_flags->owners[n].status==captured_sound->authored_flags->owners[n].status,
                "Captured authored sound owner status differs");
        check(captured_sound->address==sound.address&&captured_sound->bytes==sound.bytes&&
            captured_sound->sounds.size()==sound.sounds.size()&&captured_sound->patches.size()==sound.patches.size(),
            "Captured sound route owner differs from the published metadata");
        for(std::size_t n=0;n<sound.sounds.size();++n) {
            const auto& expected=sound.sounds[n];const auto& owned=captured_sound->sounds[n];
            check(owned.sound==expected.sound&&owned.gestalt==expected.gestalt&&
                owned.source_root==expected.source_root&&owned.native_root==expected.native_root&&owned.file==expected.file&&
                owned.codec.sample_rate==expected.codec.sample_rate&&owned.codec.encoding==expected.codec.encoding&&
                owned.codec.compression==expected.codec.compression&&owned.mcc_extension==expected.mcc_extension&&
                owned.source_flags==expected.source_flags&&owned.native_flags==expected.native_flags,
                "Captured sound identity, roots, format or DAT route differs");
        }
        check(std::ranges::equal(sound.bytes,read(sound.address,sound.bytes.size())),
            "Published native sound tables differ from the complete owned plan");
        for(const auto& patch:sound.patches) {
            check(std::ranges::equal(patch.after,read(plan.runtime_address(patch.source_address,patch.after.size()),patch.after.size())),
                "Published native sound root differs from the complete owned plan");
            for(std::size_t n=0;n<patch.after.size();++n) {
                const auto at=patch.source_address+std::uint32_t(n);
                sound_root_bytes.insert(at);patched.insert(at);
            }
        }
        const auto verify_field=[&](std::uint32_t field,std::uint32_t pointer,std::uint32_t bytes) {
            if(sound_root_bytes.contains(field))return;
            const auto logical=plan.runtime_address(field,4),target=plan.runtime_address(pointer,bytes);
            check(Bytes(read(logical,4)).u32(0)==target,"Published typed pointer differs from its declared relocation");
            for(unsigned n=0;n<4;++n)patched.insert(field+n);
            ++verified_fields;
        };
        for(const auto& native:cache.preserved_native_pointer_maps())for(const auto& [field,target]:native.pointers)
            verify_field(native.address+field,native.address+target,native.bytes-target);
        for(const auto& tag:cache.tags())if(tag.address&&tag.size&&!cache.is_preserved_native_shader(tag)) {
            if(tag.group=="phmo"||tag.group=="coll"||tag.group=="jmad") {
                const auto bytes=cache.read_tag_bytes(tag.address,tag.size);
                const auto fields=tag.group=="phmo"?normalized_mcc_physics_pointer_fields(bytes,tag.address):
                    tag.group=="coll"?normalized_mcc_collision_pointer_fields(bytes,tag.address):
                    normalized_mcc_animation_pointer_fields(bytes,tag.address);
                for(const auto& field:fields)verify_field(tag.address+field.offset,field.address,field.bytes);
            }else {
                const auto fields=ordinary_mcc_pointer_fields(tag.type(),tag.address,
                    [&](std::uint32_t at,std::size_t count){return cache.read_tag_bytes(at,count);},
                    MccOrdinaryPointerFlavor::Mcc,[&](std::uint32_t root,std::size_t bytes) {
                        return std::ranges::any_of(cache.preserved_native_pointer_maps(),[&](const auto& native) {
                            return cache.find_tag(native.owner,tag_groups::globals)&&native.address==root&&bytes<=native.bytes;
                        });
                    });
                check(bool(fields),"A live ordinary definition was published without a field map");
                for(const auto& field:*fields)verify_field(field.field_address,field.address,field.bytes);
            }
        }
        for(const auto& bsp:bsps) {
            const auto source=plan.source_address(bsp.mapping,0x44);
            for(unsigned n=0;n<12;++n)patched.insert(source+n);
            const Bytes row(read(bsp.mapping,0x44));
            check(row.u32(0)==bsp.file_offset&&row.u32(4)==bsp.region.bytes.size()&&row.u32(8)==bsp.region.address,
                "Published scenario BSP mapping differs from its owned resource");
            check(std::ranges::equal(bsp.mapping_bytes,read(bsp.mapping,0x44)),"Owned BSP mapping bytes differ from scenario");
            const Bytes header(bsp.region.bytes);
            check(header.u32(12)==0x73627370u&&plan.source_address(header.u32(4),0x23c)==cache.find_tag({bsp.bsp})->address,
                "Published BSP resource root differs from its original identity");
        }
        const auto* globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
        check(globals,"Canonical MCC globals identity was lost");
        const Bytes globals_root(cache.read_tag_bytes(globals->address,0xc8));
        check(globals_root.u32(0xc0)==1,"Sound Globals count differs");
        const auto sound_globals=globals_root.u32(0xc4);
        const auto projected_sound_globals=project_mcc_sound_globals(cache.read_tag_bytes(sound_globals,0x2c));
        check(std::ranges::equal(projected_sound_globals,
            read(plan.runtime_address(sound_globals,0x24),0x24)),"Runtime Sound Globals did not retain the exact legacy references");
        check(cache.find_tag({Bytes(projected_sound_globals).u32(4)},TagGroup::from_string("sncl"))&&
            cache.find_tag({Bytes(projected_sound_globals).u32(0x20)},TagGroup::from_string("ugh!")),
            "Runtime Sound Globals refers to a different salted sound owner");
        for(unsigned n=0;n<projected_sound_globals.size();++n)patched.insert(sound_globals+n);
        for(const auto& map:plan_mcc_model_node_maps(cache)) {
            check(Bytes(cache.read_tag_bytes(map.descriptor_field,4)).u32(0)==0x00010068u,
                "Source MCC node-map descriptor lost its original typed identity");
            check(Bytes(read(plan.runtime_address(map.descriptor_field,4),4)).u32(0)==0x00010064u,
                "Canonical node-map fixup destination differs from Xbox100/channel1");
            for(unsigned n=0;n<4;++n)patched.insert(map.descriptor_field+n);
        }
        for(const auto& range:plan.relocations())if(range.bytes) {
            const auto before_bytes=cache.read_tag_bytes(range.source,range.bytes);
            const auto after_bytes=read(range.target,range.bytes);
            for(std::size_t n=0;n<range.bytes;++n)if(!patched.contains(range.source+std::uint32_t(n)))
                check(before_bytes[n]==after_bytes[n],"Published cache changed an authored nonpointer byte");
            verified_bytes+=range.bytes;
        }
        rejects([&]{(void)plan.bsp_image(cache.bsps().size());});
        rejects([&]{(void)MccRuntimeCacheDelivery::plan(cache,0x80061000u,4096,
            [&](MccSoundRuntimePlan&&){++sound_captures;});});
        check(sound_captures==1,"Failed delivery published a partial sound owner");
        check(hash(cache.source_runtime_cache_bytes())==before,"Planner changed normalized Cache backing");
        actual=1;issues=unsigned(plan.issues().size());
    }
    std::ostringstream report;
    report<<"{\"passed\":true,\"piecewise_address_controls\":129,\"real_MCC_cache_checks\":"<<actual
        <<",\"schema_issues\":"<<issues<<",\"verified_pointer_fields\":"<<verified_fields
        <<",\"verified_owned_bytes\":"<<verified_bytes<<",\"owned_runtime_images_checked\":"<<(actual?"true":"false")<<"}\n";
    if(argc==3){std::ofstream output(argv[2]);output<<report.str();check(bool(output),"MCC delivery report write failed");}
    std::cout<<report.str();
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
