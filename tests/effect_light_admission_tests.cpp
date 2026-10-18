#include "halo2/effect_light_admission.hpp"
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void require(bool condition,const std::string& message){if(!condition)throw FormatError(message);}
SourceEffectLightRuntime runtime(std::span<const std::byte> bytes,std::uint32_t identity) {
    const Bytes raw(bytes);const auto vector=[&](unsigned at){return Vec3{raw.f32(at),raw.f32(at+4),raw.f32(at+8)};};
    return {identity,raw.u32(0x4c),std::uint8_t(raw.u16(2)),
        std::int16_t(raw.u16(0x54)),std::int16_t(raw.u16(0x48)),std::bit_cast<std::int32_t>(raw.u32(8)),
        raw.f32(0x24),raw.f32(0x100),vector(0x18),vector(0x84),vector(0xac),vector(0xb8),
        {raw.f32(0xc4),raw.f32(0xcc),vector(0xd4),vector(0xe0),raw.f32(0xec)}};
}
}
int main(int argc,char**argv){try {
    if(argc==3 && std::string(argv[1])=="--collection") {
        std::ifstream file(argv[2],std::ios::binary);require(bool(file),"Original light collection fixture missing");
        const auto word=[&](){std::uint32_t v{};file.read(reinterpret_cast<char*>(&v),4);require(bool(file),"Truncated light collection word");return v;};
        const auto scalar=[&](){return std::bit_cast<float>(word());};
        const auto read=[&](auto& bytes){file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());require(bool(file),"Truncated light collection bytes");};
        require(word()==0x314c4c48,"Invalid original light collection fixture");const auto cases=word();
        SourceClusterFrustumSet frustums;frustums.group_count=std::int16_t(word());const auto clusters=word(),count=word();
        require(clusters==8 && count==2,"Original collection frustum scope changed");
        for(unsigned i=0;i<clusters;++i) {
            SourceClusterFrustumRanges range;range.source_cluster=std::int16_t(word());
            for(auto& x:range.counts)x=std::int16_t(word());for(auto& x:range.first_indices)x=std::int16_t(word());frustums.clusters.push_back(range);
        }
        for(unsigned i=0;i<count;++i) {
            SourceSphereFrustum f;for(auto& x:f.bounds)x=scalar();for(auto& p:f.planes)for(auto& x:p)x=scalar();frustums.frustums.push_back(f);
        }
        unsigned appended=0,stamped=0,compared=0,priority_total=0;
        for(unsigned n=0;n<cases;++n) {
            SourceEffectLightCollectionContext context;context.visibility.epoch=word();
            context.visibility.query_mode=std::bit_cast<std::int32_t>(word());context.flags=word();
            context.pass_mode=std::bit_cast<std::int32_t>(word());context.containing_cluster=std::int16_t(word());context.local_player=word();
            const auto local=word(),source_count=word();context.visibility.frustums=&frustums;
            context.visibility.sphere_center={scalar(),scalar(),scalar()};context.visibility.sphere_radius=scalar();
            const auto capacity=word(),before_count=word(),expected_priority=word();
            require(source_count<=34 && capacity<=64,"Original collection scope changed");
            std::vector<EffectLightDefinition> definitions;definitions.reserve(source_count);
            std::vector<SourceEffectLightVisibilitySource> sources;sources.reserve(source_count);
            for(unsigned i=0;i<source_count;++i) {
                const auto identity=word();std::array<std::byte,228> d;std::array<std::byte,272> r;read(d);read(r);
                definitions.push_back(source_effect_light_definition(d));const Bytes raw(r);
                sources.push_back({runtime(r,identity),raw.u32(4)==0xffffffffu?nullptr:&definitions.back(),raw.u32(12)});
            }
            std::vector<std::vector<std::uint32_t>> lists(8);
            for(auto& ids:lists){const auto size=word();require(size<12,"Original collection spatial list scope changed");for(unsigned i=0;i<size;++i)ids.push_back(word());}
            std::vector<SourceBspAdmissionRecord> storage(capacity);
            for(auto& row:storage){row.id=word();row.flags=std::uint16_t(word());row.other=word();row.association=std::uint16_t(word());}
            SourceEffectLightVisibleQueue queue{capacity,std::uint16_t(before_count),storage};
            unsigned priorities=0;context.owners.local_priority=[&](std::uint32_t owner){
                require(owner==0xe0000001u || owner==0xe0000002u || owner==0xffffffffu,"Original collection priority owner changed");
                ++priorities;return std::uint8_t(owner==0xe0000001u?local:0);
            };
            std::vector<std::uint32_t> before_epochs;for(const auto& s:sources)before_epochs.push_back(s.last_seen_epoch);
            source_effect_light_collect_visible(sources,lists,queue,context);
            const auto expected_count=word();
            require(queue.count==expected_count && priorities==expected_priority,"Original light collection count/owner calls differ at case "+std::to_string(n));
            appended+=queue.count-before_count;priority_total+=priorities;
            for(unsigned i=0;i<sources.size();++i) {
                const auto expected=word();require(sources[i].last_seen_epoch==expected,"Original light collection epoch differs at case "+std::to_string(n));
                stamped+=sources[i].last_seen_epoch!=before_epochs[i];
            }
            for(const auto& row:storage)for(auto got:std::array{row.id,std::uint32_t(row.flags),row.other,std::uint32_t(row.association)}) {
                const auto expected=word();require(got==expected,"Original light collection storage differs at case "+std::to_string(n));++compared;
            }
        }
        require(cases==256 && appended==286 && stamped==286 && priority_total==1239,"Original light collection branch coverage missing");
        require(file.peek()==std::char_traits<char>::eof(),"Trailing original light collection data");
        std::cout<<"PASS "<<cases<<" whole original132220 light traversals, "<<appended<<" appends/epoch stamps, "<<compared<<" exact retained queue words, "<<priority_total<<" owner-priority calls\n";return 0;
    }
    if(argc==3 && std::string(argv[1])=="--visibility") {
        std::ifstream file(argv[2],std::ios::binary);require(bool(file),"Original light visibility fixture missing");
        const auto word=[&](){std::uint32_t v{};file.read(reinterpret_cast<char*>(&v),4);require(bool(file),"Truncated light visibility word");return v;};
        const auto scalar=[&](){return std::bit_cast<float>(word());};
        require(word()==0x31564c48,"Invalid original light visibility fixture");const auto cases=word();
        SourceClusterFrustumSet frustums;frustums.group_count=std::int16_t(word());const auto clusters=word(),count=word();
        require(clusters==8 && count==2,"Original light visibility frustum scope changed");
        for(unsigned i=0;i<clusters;++i) {
            SourceClusterFrustumRanges range;range.source_cluster=std::int16_t(word());
            for(auto& x:range.counts)x=std::int16_t(word());for(auto& x:range.first_indices)x=std::int16_t(word());frustums.clusters.push_back(range);
        }
        for(unsigned i=0;i<count;++i) {
            SourceSphereFrustum f;for(auto& x:f.bounds)x=scalar();for(auto& p:f.planes)for(auto& x:p)x=scalar();frustums.frustums.push_back(f);
        }
        unsigned appended=0,compared=0;
        for(unsigned n=0;n<cases;++n) {
            std::array<std::byte,272> bytes;file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());require(bool(file),"Truncated original light visibility record");
            const auto record=runtime(bytes,word());auto last_seen=word();SourceEffectLightVisibilityContext context;
            context.epoch=word();context.query_mode=std::bit_cast<std::int32_t>(word());context.rejection_override=std::int8_t(word());
            const auto ordinal=word(),force=word();context.frustums=&frustums;context.sphere_center={scalar(),scalar(),scalar()};context.sphere_radius=scalar();
            const auto capacity=word(),before_count=word();require(capacity<=16,"Original visibility queue fixture scope changed");
            std::vector<SourceBspAdmissionRecord> storage(capacity);
            for(auto& row:storage){row.id=word();row.flags=std::uint16_t(word());row.other=word();row.association=std::uint16_t(word());}
            SourceEffectLightVisibleQueue queue{capacity,std::uint16_t(before_count),storage};
            const auto expected_count=word(),expected_epoch=word();
            const bool result=source_effect_light_visibility_visit(record,last_seen,queue,context,ordinal,std::uint8_t(force));
            require(queue.count==expected_count && last_seen==expected_epoch && result==(queue.count!=before_count),"Original light visibility visit differs at case "+std::to_string(n));
            appended+=result;
            for(const auto& row:storage)for(auto got:std::array{row.id,std::uint32_t(row.flags),row.other,std::uint32_t(row.association)}) {
                const auto expected=word();require(got==expected,"Original light visibility storage differs at case "+std::to_string(n));++compared;
            }
        }
        require(cases==4096 && appended>100 && compared>10000,"Original light visibility branch coverage missing");
        std::cout<<"PASS "<<cases<<" whole original132A30 calls, "<<appended<<" appends/epoch stamps, "<<compared<<" exact retained queue words\n";return 0;
    }
    if(argc==3 && std::string(argv[1])=="--candidates") {
        std::ifstream file(argv[2],std::ios::binary);require(bool(file),"Original light candidate fixture missing");
        const auto word=[&](){std::uint32_t v{};file.read(reinterpret_cast<char*>(&v),4);require(bool(file),"Truncated light candidate word");return v;};
        const auto scalar=[&](){return std::bit_cast<float>(word());};
        const auto read=[&](auto& bytes){file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());require(bool(file),"Truncated light candidate bytes");};
        require(word()==0x31434c48,"Invalid original light candidate fixture");const auto cases=word();unsigned compared=0,full=0,empty=0;
        for(unsigned n=0;n<cases;++n) {
            SourceEffectLightCandidateContext context;context.admission.renderer_enabled=std::uint8_t(word());
            context.admission.debug_gate=std::bit_cast<std::int32_t>(word());context.admission.debug_flags=word();context.admission.view_type=std::int16_t(word());
            const auto local=word(),kind=word();for(auto& x:context.depth_plane)x=scalar();context.pixel_scale=scalar();
            context.admission.local_priority=[&](std::uint32_t owner){require(owner==0xe0000001u || owner==0xe0000002u || owner==0xffffffffu,"Original candidate priority owner changed");return std::uint8_t(owner==0xffffffffu?0:local);};
            context.root_owner=[](std::uint32_t owner){require(owner==0xe0000001u || owner==0xe0000002u,"Original candidate root owner changed");return 0xe0000001u;};
            context.owner_kind=[&](std::uint32_t owner){require(owner==0xe0000001u || owner==0xe0000002u,"Original candidate header owner changed");return std::uint8_t(owner==0xe0000002u?2:kind);};
            const auto count=word(),queue_count=word();require(count<=190 && queue_count<=400,"Original candidate input bank exceeds fixture scope");
            std::vector<EffectLightDefinition> definitions;definitions.reserve(count);
            std::vector<SourceEffectLightCandidateSource> sources;sources.reserve(count);
            for(unsigned i=0;i<count;++i) {
                const auto identity=word();std::array<std::byte,228> d;std::array<std::byte,272> r;read(d);read(r);
                definitions.push_back(source_effect_light_definition(d));sources.push_back({runtime(r,identity),&definitions.back()});
            }
            std::vector<std::uint32_t> queue;for(unsigned i=0;i<queue_count;++i)queue.push_back(word());
            const auto expected_count=word();const auto result=source_effect_light_candidates(sources,queue,context);
            require(result.size()==expected_count,"Original candidate count differs at case "+std::to_string(n));
            unsigned offset=0;
            for(const auto& candidate:result)for(auto got:std::array{candidate.flags,candidate.identity,
                std::bit_cast<std::uint32_t>(candidate.owner),std::bit_cast<std::uint32_t>(candidate.requested_fade),std::bit_cast<std::uint32_t>(candidate.coverage)}) {
                const auto expected=word();if(got!=expected)std::cerr<<"case="<<n<<" candidate_word="<<offset<<" native="<<std::hex<<got<<" original="<<expected<<std::dec<<'\n';
                require(got==expected,"Original candidate word differs at case "+std::to_string(n));++compared;++offset;
            }
            full+=result.size()==128;empty+=result.empty();
        }
        require(cases==192 && compared>10000 && full>10 && empty>10,"Original candidate branch coverage missing");
        std::cout<<"PASS "<<cases<<" original31910 preselection queues, "<<compared<<" exact candidate words, "<<full<<" capacity cases, "<<empty<<" empty cases\n";return 0;
    }
    require(argc==2,"Supply original light admission fixture");
    std::ifstream file(argv[1],std::ios::binary);require(bool(file),"Original light admission fixture missing");
    const auto read=[&](auto& raw){file.read(reinterpret_cast<char*>(raw.data()),raw.size());require(bool(file),"Truncated light admission bytes");};
    const auto word=[&](){std::uint32_t value{};file.read(reinterpret_cast<char*>(&value),4);require(bool(file),"Truncated light admission word");return value;};
    require(word()==0x31414c48,"Invalid original light admission fixture");const auto cases=word();
    unsigned admitted=0,shapes=0,priority_calls=0;
    for(unsigned n=0;n<cases;++n) {
        std::array<std::byte,228> definition_bytes;read(definition_bytes);
        std::array<std::byte,272> record_bytes;read(record_bytes);
        const auto record=runtime(record_bytes,word());
        SourceEffectLightRenderContext context;context.renderer_enabled=std::uint8_t(word());
        context.debug_gate=std::bit_cast<std::int32_t>(word());context.debug_flags=word();context.view_type=std::int16_t(word());
        const auto local=word();context.local_priority=[&](std::uint32_t owner){require(owner==record.owner,"Original light owner changed");++priority_calls;return std::uint8_t(local);};
        const bool eligible=word(),produced=word();std::array<std::byte,124> expected;read(expected);
        const auto definition=source_effect_light_definition(definition_bytes);
        const auto result=source_effect_light_render_admission(definition,record,context);
        require(result.eligible==eligible && bool(result.sphere)==produced,"Original light admission differs at case "+std::to_string(n));
        if(produced) {
            const Bytes output(expected);for(unsigned i=0;i<31;++i)
                require(result.sphere->shape[i]==output.u32(i*4),"Original light admission shape differs at case "+std::to_string(n));
        }
        admitted+=eligible;shapes+=produced;
    }
    require(cases==4096 && admitted>100 && shapes>300 && priority_calls>500,"Original light admission branch coverage missing");
    // Unconsumed services must not be demanded by original early returns.
    EffectLightDefinition definition;SourceEffectLightRuntime record{};SourceEffectLightRenderContext context{};
    record.identity=0xffffffffu;require(!source_effect_light_render_admission(definition,record,context).eligible,"NONE light admitted");
    record.identity=1;record.flags=2;record.spatial_radius=record.overall_fade=1;record.state.diffuse={1,1,1};
    record.attached_node=0;bool rejected=false;try{(void)source_effect_light_render_admission(definition,record,context);}catch(const FormatError&){rejected=true;}
    require(rejected,"Missing original local-priority service admitted");
    record.attached_node=-1;record.scenario_placement=0;context.renderer_enabled=1;rejected=false;
    try{(void)source_effect_light_render_admission(definition,record,context);}catch(const FormatError&){rejected=true;}
    require(rejected,"Missing original scenario placement shape service admitted");
    std::cout<<"PASS "<<cases<<" whole original31590 calls, "<<admitted<<" admitted, "<<shapes<<" exact 31-word shapes, "<<priority_calls<<" owner-priority calls\n";
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
