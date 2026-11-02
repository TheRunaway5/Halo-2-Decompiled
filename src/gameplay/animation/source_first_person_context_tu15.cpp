#include "halo2/source_first_person_context_tu15.hpp"
#include "source_first_person_context_memory.hpp"
#include "halo2/source_first_person_action_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_weapon_animation_query_symbols_tu15.hpp"
#include "halo2/source_sandbox_special_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_animation_composition_tu15.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include "halo2/source_vehicle_director_tu15.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_map_world_support_tu15.hpp"
#include "halo2/source_effect_light_view_tu15.hpp"
#include "halo2/physics_phantom_world_tu15.hpp"
#include "halo2/source_first_person_effect_retirement_tu15.hpp"
#include "halo2/source_first_person_vector_pack_tu15.hpp"
#include "halo2/unit_animation_graph_metadata.hpp"
#include "halo2/source_unit_animation_graph_header.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <map>
#include <x86intrin.h>
namespace halo2 {
namespace {
bool same_native_first_person_map_prefixes(std::span<const std::byte,0x300> authored,
    std::span<const std::byte,0x300> received) {
    const Bytes native(authored),control(received);
    // TU1.5 16AD45 ->1D4060 ->1E5770 writes only count DWORDs for
    // F4/FC and F8/1FC. 16A4DD consumes those same counted domains;
    // the remaining capacity retains earlier bindings' opaque source bytes.
    for(const auto &[count_at,map_at]:{std::pair{0xf4u,0xfcu},std::pair{0xf8u,0x1fcu}}) {
        const auto count=native.u32(count_at);
        if(count>64u || control.u32(count_at)!=count)return false;
        if(!std::equal(authored.begin()+map_at,authored.begin()+map_at+count*4u,
            received.begin()+map_at))return false;
    }
    return true;
}
bool same_native_first_person_hand_binding(std::span<const std::byte,0x300> authored,
    std::span<const std::byte,0x300> received,std::uint32_t weapon) {
    const Bytes native(authored),control(received);
    if(control.u32(4)!=weapon || native.u32(4)!=weapon ||
        control.u32(0xf4)>64u || control.u32(0xf8)>64u ||
        native.u32(0xf4)>64u || native.u32(0xf8)>64u)return false;
    //16AD45 clears active/completeness bits0..2 and16B1F5 writes NONE,
    // then the NONE branch skips the graph/map builders. An empty hand's
    // earlier graph/counts/maps are opaque continuation bytes, unlike the
    // cleared bank of a fresh168BC5 context. They cannot authorize sampling.
    if(weapon==0xffffffffu)return !(control.u32(0)&7u) && !(native.u32(0)&7u);
    return control.u32(8)==native.u32(8) &&
        same_native_first_person_map_prefixes(authored,received);
}
std::uint32_t datum_row(const SourcePhysicsContactMemory &memory,std::uint32_t pool,
    std::uint32_t datum,std::uint32_t stride) {
    std::array<std::byte,4> storage;
    SourceRawDatumPoolServices services;
    services.read=[&](std::uint32_t at,std::size_t count)->std::span<const std::byte> {
        if(count>storage.size())throw FormatError("Native FP identity lookup exceeded its source operand");
        memory.read(at,std::span(storage).first(count));return std::span(storage).first(count);
    };
    memory.read(pool+0x24u,storage);
    if(Bytes(storage).u32(0)!=stride)throw FormatError("Native FP identity pool stride differs");
    const auto row=source_raw_datum_lookup(pool,datum,services);
    if(!row)throw FormatError("Native FP context requires a live full-salt datum");
    return row;
}
using first_person_context_detail::ContextMemory;
}
struct SourceFirstPersonContextTu15::Impl {
    const Cache &cache;
    std::shared_ptr<const SourceRuntimeResources> resources;
    ContextMemory memory;
    std::vector<std::uint32_t> geometry_owners;
    SourcePhysicsWorldRuntimeServices cpu;
    SourceFirstPersonTemporalStateTu15 published;
    std::optional<SourceFirstPersonTemporalStateTu15> sampling_inputs;
    bool initialized{};
    std::optional<std::uint32_t> last_advanced_tick;
    std::unique_ptr<Impl> receiver_context;
    std::array<std::uint32_t,10> owner_roots{};
    std::uint32_t caller_stack{};
    static constexpr std::array<std::uint32_t,10> root_globals{
        0x4f6498u,0x4f6494u,0x4f3cd8u,0x4f0414u,0x4f0418u,
        0x4f041cu,0x4f0420u,0x4f0424u,0x51df54u,0x4f54f0u};
    void retain_roots(const SourcePhysicsWorldRuntimeAbi &caller) {
        for(unsigned n=0;n<root_globals.size();++n)owner_roots[n]=memory.word(root_globals[n]);
        caller_stack=caller.gp[4];
    }
    struct ReadJob {std::uint32_t graph,index,descriptor,destination,bytes;};
    std::map<std::uint32_t,ReadJob> jobs;
    std::uint64_t next_job=0xf8000001u;
    struct BitmapPredictionTrace {
        std::uint32_t instruction{},record{},type{},datum{},frame{},table{},root{};
        unsigned stage{};
        bool reading{};
    } bitmap_trace;
    bool trace_bitmap_requests{};
    static constexpr SourcePhysicsFactoryFloatingPoint fp{0x27f,0x1f80};
    explicit Impl(const Cache &c,std::shared_ptr<const SourceRuntimeResources> r,
        SourcePhysicsWorldRuntimeServices supplied_cpu={}):cache(c),resources(std::move(r)) {
        cpu.reciprocal_sqrt=std::move(supplied_cpu.reciprocal_sqrt);
        cpu.timestamp_counter=std::move(supplied_cpu.timestamp_counter);
        if(!resources)resources=std::make_shared<SourceRuntimeResources>(cache);
        if(!cpu.reciprocal_sqrt)cpu.reciprocal_sqrt=[](auto bits){return std::bit_cast<std::uint32_t>(
            _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));};
        if(!cpu.timestamp_counter)cpu.timestamp_counter=[](){return __rdtsc();};
        const auto* trace=std::getenv("HALO2_SHADOW_TRANSPORT_TRACE");
        trace_bitmap_requests=trace&&*trace;
        if(trace_bitmap_requests)memory.observe_reads([this](auto at,auto bytes){trace_bitmap_read(at,bytes);});
    }
    void trace_bitmap_read(std::uint32_t at,std::span<const std::byte> bytes) {
        auto& trace=bitmap_trace;
        if(trace.reading)return;
        struct Reading {bool& value;explicit Reading(bool&v):value(v){value=true;}~Reading(){value=false;}} reading(trace.reading);
        if(trace.instruction==0x171463u && bytes.size()==2u) {
            trace.record=at;trace.type=Bytes(bytes).u16(0);trace.stage=1u;
        } else if(trace.instruction==0x1714b6u && bytes.size()==4u && trace.stage==1u &&
            trace.type==0u && at==trace.record+4u) {
            trace.datum=Bytes(bytes).u32(0);trace.stage=2u;
        } else if(trace.instruction==0x1714b9u && bytes.size()==2u && trace.stage==2u && at==trace.record+2u) {
            trace.frame=Bytes(bytes).u16(0);trace.stage=3u;
        } else if(trace.instruction==0x1714bdu && bytes.size()==4u && trace.stage==3u && at==0x4f0414u) {
            trace.table=Bytes(bytes).u32(0);trace.stage=4u;
        } else if(trace.instruction==0x1714cfu && bytes.size()==4u && trace.stage==4u &&
            std::uint64_t(at)==std::uint64_t(trace.table)+(trace.datum&65535u)*16u+8u) {
            trace.root=Bytes(bytes).u32(0);trace.stage=5u;
        } else if(trace.instruction==0x12f733u && bytes.size()==4u && trace.stage==5u) {
            trace.stage=0u;
            // Keep the bounded receipt for the actual cold-handle branch
            // that can allocate a prediction row, rather than resident touches.
            if(Bytes(bytes).u32(0)!=0xffffffffu)return;
            static std::atomic<unsigned> receipts{};
            if(receipts.fetch_add(1,std::memory_order_relaxed)>=16u)return;
            try {
                const auto row=trace.table+(trace.datum&65535u)*16u;
                const auto first=memory.word(trace.root+0x48u),count=memory.word(trace.root+0x44u);
                const auto signed_frame=std::bit_cast<std::int16_t>(std::uint16_t(trace.frame));
                const auto* tag=cache.find_tag({trace.datum},tag_groups::bitmap);
                const bool fresh=tag && memory.word(row)==tag_groups::bitmap.value &&
                    memory.word(row+4u)==trace.datum && memory.word(row+8u)==trace.root &&
                    memory.word(row+12u)==tag->size && count==cache.bitmap_count(trace.datum);
                const auto native=std::int64_t(first)+std::int64_t(signed_frame)*116;
                const auto authored=std::int64_t(first)+std::int64_t(signed_frame)*(cache.is_mcc()?168:116);
                const auto image=std::uint64_t(at)-0x28u;
                const char* classification=!fresh?"unqualified_directory":
                    (image==authored?"canonical_authored_image":
                    (cache.is_mcc() && image==native?"canonical_116_stride_MCC_interior":"unqualified_foreign_or_derived_image"));
                std::fprintf(stderr,"[native-fp-bitmap-prediction] {\"site\":1512662,\"type\":%u,\"record\":%u,\"datum\":%u,\"frame\":%d,\"canonical_root\":%u,\"count\":%u,\"first\":%u,\"actual_image\":%llu,\"expected_native116\":%lld,\"expected_authored\":%lld,\"native_handle_word\":%u,\"fresh_cache_full_salt\":%s,\"classification\":\"%s\",\"world_projection_receipt\":false}\n",
                    trace.type,trace.record,trace.datum,signed_frame,trace.root,count,first,
                    static_cast<unsigned long long>(image),static_cast<long long>(native),
                    static_cast<long long>(authored),Bytes(bytes).u32(0),fresh?"true":"false",classification);
            }catch(const std::exception& error) {
                std::fprintf(stderr,"[native-fp-bitmap-prediction] {\"diagnostic_error\":true,\"datum\":%u,\"actual_operand\":%u}\n",trace.datum,at);
                (void)error;
            }
        }
    }
    struct Borrow {
        Impl &owner;
        SourceWeaponAnimationQuerySymbolsTu15 symbols;
        Borrow(Impl &o,const SourcePhysicsContactMemory &source):owner(o),
            symbols(o.memory,o.cache.is_mcc(),[&o](auto id){return o.cache.string_id(id);}) {owner.memory.borrow(&source);}
        ~Borrow(){owner.memory.borrow(nullptr);}
    };
    void pin_range(std::uint32_t at,std::uint32_t bytes) {
        memory.pin_range(at,bytes);
    }
    void pin_pool(std::uint32_t pool,std::uint32_t capacity,std::uint32_t stride) {
        if(memory.word(pool+0x20u)!=capacity || memory.word(pool+0x24u)!=stride ||
            memory.word(pool+0x38u)>capacity)
            throw FormatError("Native FP animation-cache pool differs from its original constructor");
        pin_range(pool,0x4cu);
        pin_range(memory.word(pool+0x44u),capacity*stride);
        pin_range(memory.word(pool+0x48u),((capacity+31u)/32u)*4u);
    }
    std::vector<std::uint32_t> bitmap_prediction_headers(const SourcePhysicsContactMemory &source)const {
        // MCC's168-byte records have a separate runtime-view owner. This
        // continuation admits only actual original116-byte authored images.
        if(cache.is_mcc())return {};
        const auto word=[&](std::uint32_t at) {
            std::array<std::byte,4> bytes;source.read(at,bytes);return Bytes(bytes).u32(0);
        };
        const auto table=word(0x4f0414u);std::vector<std::uint32_t> result;
        for(const auto& tag:cache.select_tags({.group=tag_groups::bitmap,.require_address=true})) {
            const auto row=table+(tag.datum&65535u)*16u,root=word(row+8u);
            if(tag.size<0x4cu || word(row)!=tag_groups::bitmap.value || word(row+4u)!=tag.datum ||
                word(row+12u)!=tag.size || !root || std::uint64_t(root)+tag.size>std::uint64_t(UINT32_MAX)+1u)
                throw FormatError("Native FP bitmap prediction lost its full-salt fresh Cache tag owner");
            const auto count=word(root+0x44u),first=word(root+0x48u);
            if(count!=cache.bitmap_count(tag.datum) || (count && (first<root ||
                std::uint64_t(first)-root+std::uint64_t(count)*116u>tag.size)))
                throw FormatError("Native FP bitmap prediction escaped its complete authored image bank");
            for(unsigned image=0;image<count;++image)result.push_back(first+image*116u);
        }
        std::sort(result.begin(),result.end());
        if(std::adjacent_find(result.begin(),result.end())!=result.end())
            throw FormatError("Native FP bitmap prediction has aliased authored image owners");
        return result;
    }
    std::vector<std::uint32_t> geometry_prediction_headers(const SourcePhysicsContactMemory &source)const {
        const auto word=[&](std::uint32_t at) {
            std::array<std::byte,4> bytes;source.read(at,bytes);return Bytes(bytes).u32(0);
        };
        const auto table=word(0x4f0414u);std::vector<std::uint32_t> result;
        for(const auto& tag:cache.select_tags({.group=TagGroup::from_string("mode"),.require_address=true})) {
            const auto row=table+(tag.datum&65535u)*16u,root=word(row+8u);
            if(tag.size<0x2cu || word(row)!=TagGroup::from_string("mode").value || word(row+4u)!=tag.datum ||
                word(row+12u)!=tag.size || !root || std::uint64_t(root)+tag.size>std::uint64_t(UINT32_MAX)+1u)
                throw FormatError("Native FP geometry prediction lost its full-salt fresh Cache model owner");
            const auto count=word(root+0x24u),first=word(root+0x28u);
            if(count && (first<root || std::uint64_t(first)-root+std::uint64_t(count)*0x5cu>tag.size))
                throw FormatError("Native FP geometry prediction escaped its authored section bank");
            for(unsigned section=0;section<count;++section)result.push_back(first+section*0x5cu+0x38u);
        }
        // The same native1303C0 queue also serves the selected BSP's449D0
        // headers. Its actual published root, full-salt Cache table row and
        // complete authored tag extent qualify cluster/instance definitions;
        // neither a queued pointer nor the archive can nominate another bank.
        const auto current_bsp=word(0x4ecc18u);
        bool current_bsp_found=!current_bsp;
        for(const auto& bsp:cache.bsps()) {
            const auto* tag=cache.find_tag({bsp.datum},tag_groups::scenario_structure_bsp);
            if(!tag)throw FormatError("Native FP geometry prediction lost its Cache BSP owner");
            const auto row=table+(tag->datum&65535u)*16u,root=word(row+8u);
            if(root!=current_bsp)continue;
            //1255C0 publishes only table+8, from the complete allocation
            // header at556D94. Separate BSP table sizes remain zero in the
            // canonical delivery; authenticate that real allocation instead
            // of treating its resident table row as an ordinary MODE tag.
            const auto allocation=word(0x556d94u);
            const auto authored=Bytes(cache.source_runtime_cache_bytes()).slice(bsp.file_offset,bsp.size);
            const Bytes header(authored);
            const auto authored_root=header.u32(4);
            if(bsp.size<16u || authored_root<bsp.memory_address ||
                std::uint64_t(authored_root)-bsp.memory_address>=bsp.size ||
                header.u32(0)!=bsp.size || (tag->address && authored_root!=tag->address))
                throw FormatError("Native FP geometry prediction lost its authored BSP allocation");
            const auto offset=authored_root-bsp.memory_address;
            const auto extent=tag->size?tag->size:bsp.size-offset;
            if(current_bsp_found || extent<0x140u || std::uint64_t(offset)+extent>bsp.size ||
                word(row)!=tag_groups::scenario_structure_bsp.value || word(row+4u)!=tag->datum ||
                (word(row+12u)!=0u && word(row+12u)!=tag->size) || !allocation ||
                std::uint64_t(allocation)+bsp.size>std::uint64_t(UINT32_MAX)+1u ||
                std::uint64_t(allocation)+offset!=root || word(allocation)!=bsp.size ||
                word(allocation+4u)!=root || word(allocation+12u)!=tag_groups::scenario_structure_bsp.value)
                throw FormatError("Native FP geometry prediction lost its full-salt current BSP allocation");
            current_bsp_found=true;
            for(const auto [offset,stride]:{std::pair{0x9cu,0xb0u},std::pair{0x138u,0xc8u}}) {
                const auto count=word(root+offset),first=word(root+offset+4u);
                if(count && (first<root || std::uint64_t(first)-root+std::uint64_t(count)*stride>extent))
                    throw FormatError("Native FP geometry prediction escaped its authored BSP section bank");
                for(unsigned section=0;section<count;++section)result.push_back(first+section*stride+0x28u);
            }
            //1716F0/171750 nominate the selected lightmap's incident-stream
            // bucket+C, not an sbsp section. Authenticate its separate full
            // salt and allocation/root publication before admitting only the
            // same56-byte records decoded by Cache::load_bsp_lighting.
            const auto* scenario=cache.find_tag(cache.scenario_tag(),tag_groups::scenario);
            if(!scenario || !scenario->address || scenario->size<0x218u)
                throw FormatError("Native FP lightmap prediction lacks its Cache scenario mapping");
            const Bytes scnr(cache.read_tag_bytes(scenario->address,scenario->size));
            const auto selected=std::size_t(&bsp-cache.bsps().data());
            if(scnr.u32(0x210u)!=cache.bsps().size())
                throw FormatError("Native FP lightmap prediction lost its exact scenario BSP count");
            const Bytes mapping(cache.read_tag_bytes(scnr.u32(0x214u)+selected*0x44u,0x44u));
            // The MCC Cache keeps zero delivery fields here; its owning
            // MccRuntimeCacheDelivery supplies the normalized allocation.
            // Both formats retain the actual full-salt sbsp/ltmp links.
            if(mapping.u32(0x14u)!=bsp.datum || (!cache.is_mcc() &&
                (mapping.u32(0)!=bsp.file_offset || mapping.u32(4)!=bsp.size || mapping.u32(8)!=bsp.memory_address)))
                {std::ostringstream message;message<<"Native FP lightmap prediction lost its exact scenario BSP allocation mcc="<<cache.is_mcc()<<std::hex<<" mapping=["<<mapping.u32(0)<<","<<mapping.u32(4)<<","<<mapping.u32(8)<<","<<mapping.u32(0x14u)<<"] allocation=["<<bsp.file_offset<<","<<bsp.size<<","<<bsp.memory_address<<","<<bsp.datum<<"]";throw FormatError(message.str());}
            const auto lightmap=word(0x4ecc14u),authored_lightmap=header.u32(8),lightmap_datum=mapping.u32(0x1cu);
            if(lightmap_datum!=0xffffffffu) {
                const auto* tag=cache.find_tag({lightmap_datum},tag_groups::scenario_structure_lightmap);
                if(!tag || authored_lightmap<bsp.memory_address ||
                    std::uint64_t(authored_lightmap)-bsp.memory_address>=bsp.size ||
                    (tag->address && tag->address!=authored_lightmap))
                    throw FormatError("Native FP lightmap prediction lost its authored ltmp allocation");
                const auto offset=authored_lightmap-bsp.memory_address;
                const auto extent=tag->size?tag->size:bsp.size-offset;
                const auto row=table+(lightmap_datum&65535u)*16u;
                if(extent<0x88u || std::uint64_t(offset)+extent>bsp.size ||
                    std::uint64_t(allocation)+offset!=lightmap || word(allocation+8u)!=lightmap ||
                    word(row)!=tag_groups::scenario_structure_lightmap.value || word(row+4u)!=lightmap_datum ||
                    word(row+8u)!=lightmap || (word(row+12u)!=0u && word(row+12u)!=tag->size))
                    throw FormatError("Native FP lightmap prediction lost its full-salt selected ltmp publication");
                const auto count=word(lightmap+0x80u),groups=word(lightmap+0x84u);
                const auto authored_count=header.u32(offset+0x80u),authored_groups=header.u32(offset+0x84u);
                if(count!=authored_count || (count && (authored_groups<bsp.memory_address ||
                    std::uint64_t(authored_groups)-bsp.memory_address+std::uint64_t(count)*0x68u>bsp.size ||
                    std::uint64_t(allocation)+authored_groups-bsp.memory_address!=groups)))
                    throw FormatError("Native FP lightmap groups differ from their independent Cache count/offset");
                if(count && (groups<lightmap || std::uint64_t(groups)-lightmap+std::uint64_t(count)*0x68u>extent))
                    throw FormatError("Native FP lightmap groups escaped their actual ltmp owner");
                // Both actual native consumers select group0 and reject stale
                // geometry checksum or an absent authored bitmap binding.
                if(count && word(groups+0x1cu)!=0xffffffffu && word(groups+4u)==word(root+8u)) {
                    const auto authored_group=authored_groups-bsp.memory_address;
                    if(word(groups+0x1cu)!=header.u32(authored_group+0x1cu) ||
                        word(groups+4u)!=header.u32(authored_group+4u) ||
                        word(root+8u)!=header.u32(authored_root-bsp.memory_address+8u))
                        throw FormatError("Native FP lightmap prediction changed its authored bitmap/checksum binding");
                    const auto count=word(groups+0x40u),buckets=word(groups+0x44u);
                    const auto authored_buckets=header.u32(authored_group+0x44u);
                    if(count!=header.u32(authored_group+0x40u) || (count &&
                        (authored_buckets<bsp.memory_address ||
                        std::uint64_t(authored_buckets)-bsp.memory_address+std::uint64_t(count)*0x38u>bsp.size ||
                        std::uint64_t(allocation)+authored_buckets-bsp.memory_address!=buckets)))
                        throw FormatError("Native FP lightmap buckets differ from their independent Cache count/offset");
                    if(count && (buckets<lightmap ||
                        std::uint64_t(buckets)-lightmap+std::uint64_t(count)*0x38u>extent))
                        throw FormatError("Native FP lightmap bucket requests escaped their authored record bank");
                    for(unsigned bucket=0;bucket<count;++bucket) {
                        const auto actual=buckets+bucket*0x38u+0xcu;
                        const auto authored=authored_buckets-bsp.memory_address+bucket*0x38u+0xcu;
                        for(unsigned at:{0u,4u,8u,0xcu,0x10u,0x18u})
                            if(word(actual+at)!=header.u32(authored+at))
                                throw FormatError("Native FP lightmap prediction changed immutable resource metadata");
                        const auto pointer=header.u32(authored+0x14u);
                        if((pointer && (pointer<bsp.memory_address ||
                            std::uint64_t(pointer)-bsp.memory_address>=bsp.size ||
                            std::uint64_t(allocation)+pointer-bsp.memory_address!=word(actual+0x14u))) ||
                            (!pointer && word(actual+0x14u)) ||
                            (word(actual+0x1cu)&65535u)!=header.u16(authored+0x1cu))
                            throw FormatError("Native FP lightmap prediction changed its resource pointer/static fields");
                        result.push_back(actual);
                    }
                }
            } else if(lightmap)throw FormatError("Native FP lightmap prediction has a foreign published ltmp root");
        }
        if(!current_bsp_found)throw FormatError("Native FP geometry prediction lacks its selected Cache BSP publication");
        std::sort(result.begin(),result.end());
        if(std::adjacent_find(result.begin(),result.end())!=result.end())
            throw FormatError("Native FP geometry prediction has aliased authored section owners");
        return result;
    }
    std::array<std::byte,0xc4> animation_graph_header(std::uint32_t graph) const {
        static const bool copy_graph=[]{const auto *p=std::getenv("HALO2_REFERENCE_COPY_FP_GRAPH_METADATA");return p && *p;}();
        if(!copy_graph)return load_source_unit_animation_graph_header(cache,graph);
        const auto metadata=cache.load_source_unit_animation_graph_metadata(graph);
        std::array<std::byte,0xc4> header;
        std::copy(metadata.header().begin(),metadata.header().end(),header.begin());
        return header;
    }
    void pin_animation_cache() {
        // These are the complete native owners constructed by125C90/125D40.
        // Private allocations must never mix with a later authority free list,
        // readiness byte, descriptor handle, or overwritten resident resource.
        // Original178A13/178A18 and178A4C/178A51 construct these exact
        // effect instance/emitter pools. Native17B262/17A230 retirement must
        // retain complete private owners as external effect caches evolve.
        pin_range(0x4f7208u,8u);
        pin_pool(memory.word(0x4f7208u),0x200u,0x3cu);
        pin_pool(memory.word(0x4f720cu),0x100u,0x190u);
        pin_range(0x4f0418u,0x18u);pin_range(0x51df20u,2u);
        pin_pool(memory.word(0x4f0418u),0x200u,0x10u);
        pin_pool(memory.word(0x4f041cu),0x80u,0xcu);
        // Original130426 constructs the bounded predicted-resource queue.
        const auto predicted_geometry=memory.word(0x4f2d60u);
        pin_pool(predicted_geometry,0x15eu,8u);
        const auto geometry_headers=geometry_prediction_headers(memory);
        const auto geometry_rows=memory.word(predicted_geometry+0x44u),geometry_bits=memory.word(predicted_geometry+0x48u);
        for(unsigned index=0;index<0x15eu;++index)if(memory.word(geometry_bits+(index/32u)*4u)&(1u<<(index%32u))) {
            const auto header=memory.word(geometry_rows+index*8u+4u);
            if(!std::binary_search(geometry_headers.begin(),geometry_headers.end(),header)) {
                std::ostringstream message;message<<"Native FP inherited geometry request lacks its actual authored section owner header0x"<<std::hex<<header<<" queue0x"<<predicted_geometry<<" index0x"<<index<<" full-row0x"<<memory.word(geometry_rows+index*8u)<<" table0x"<<memory.word(0x4f0414u)<<" bsp0x"<<memory.word(0x4ecc18u)<<" allocation0x"<<memory.word(0x556d94u);
                const auto next=std::lower_bound(geometry_headers.begin(),geometry_headers.end(),header);
                if(next!=geometry_headers.end())message<<" next-authored0x"<<*next;
                if(next!=geometry_headers.begin())message<<" previous-authored0x"<<*std::prev(next);
                const auto allocation=memory.word(0x556d94u);
                if(allocation)for(unsigned at=0;at<16u;at+=4u)message<<" allocation+"<<at<<"=0x"<<memory.word(allocation+at);
                throw FormatError(message.str());
            }
            pin_range(header+0x1fu,1u);
        }
        if(!cache.is_mcc()) {
            //12EC46 constructs this separate200-row global prediction pool.
            // Only an authored image's queued bit belongs to this context;
            // hardware handles, pixels and completion remain borrowed.
            const auto predicted_bitmaps=memory.word(0x4f2d28u);
            pin_pool(predicted_bitmaps,0xc8u,8u);
            const auto images=bitmap_prediction_headers(memory);
            const auto rows=memory.word(predicted_bitmaps+0x44u),bits=memory.word(predicted_bitmaps+0x48u);
            for(unsigned image=0;image<0xc8u;++image)if(memory.word(bits+(image/32u)*4u)&(1u<<(image%32u))) {
                const auto header=memory.word(rows+image*8u+4u);
                if(!std::binary_search(images.begin(),images.end(),header))
                    throw FormatError("Native FP inherited bitmap request lacks its authored116-byte image owner");
                pin_range(header+0xfu,1u);
            }
        }
        const auto allocator=memory.word(0x4f0424u);
        pin_range(allocator,0x30fcu);
        const auto pool=memory.word(allocator+0x64u);
        if(pool!=allocator+0x70u)throw FormatError("Native FP animation allocator lost its original inline pool");
        pin_pool(pool,0x200u,0x18u);
        const auto rows=memory.word(pool+0x44u),shift=memory.word(allocator+0x34u);
        const auto data=memory.word(0x4f0420u),bytes=memory.word(0x4f0428u);
        if(shift!=11u || !data || !bytes || bytes>0x400000u)
            throw FormatError("Native FP animation cache data differs from125D40");
        for(unsigned i=0;i<memory.word(pool+0x38u);++i) {
            const auto row=rows+i*0x18u;
            if(!(memory.word(row)&65535u))continue;
            const auto begin=std::uint64_t(memory.word(row+8u))<<shift;
            const auto count=std::uint64_t(memory.word(row+4u))<<shift;
            if(!count || begin>bytes || count>bytes-begin)
                throw FormatError("Native FP animation allocation escaped its actual resource owner");
            pin_range(data+std::uint32_t(begin),std::uint32_t(count));
        }
        const auto table=memory.word(0x4f0414u);
        for(const auto &tag:cache.select_tags({.group=tag_groups::model_animation_graph,.require_address=true})) {
            const auto root=memory.word(table+(tag.datum&65535u)*16u+8u);
            if(!root)continue;
            const auto count=memory.word(root+0xacu);
            const auto metadata=animation_graph_header(tag.datum);
            if(count!=Bytes(metadata).u32(0xac))
                throw FormatError("Native FP resource descriptors differ from their admitted Cache graph");
            if(count)pin_range(memory.word(root+0xb0u),count*20u);
        }
    }
    SourcePhysicsWorldRuntimeServices services() {
        SourcePhysicsWorldRuntimeServices result;
        if(trace_bitmap_requests)result.instruction=[this](auto pc){bitmap_trace.instruction=pc;};
        result.reciprocal_sqrt=cpu.reciprocal_sqrt;
        result.timestamp_counter=cpu.timestamp_counter;
        result.required_owner=[this](auto &bank,const auto &call,auto &abi,const auto &stores) {
            if(call.instruction==0x126139u && call.target==0x1a3b70u && !call.tail) {
                resource_read(abi);return;
            }
            auto nested=services();
            auto control=abi.floating_point.value_or(fp);
            // Original17B262 retires the retained first-person effect parts.
            // The complete17A230 owner validates its full effect identity and
            // runs16E8A0 for each actual linked record before clearing slots.
            if(source_first_person_effect_retirement_tu15(bank,call,abi,control,nested,stores))return;
            if(source_first_person_vector_pack_tu15(bank,call,abi,control,nested,stores))return;
            if(call.target==0xe8d50u || call.target==0x24570u || call.target==0x32a660u) {
                source_run_physics_phantom_world_tu15_runtime(bank,call.target,abi,control,nested,stores);
                abi.floating_point=control;return;
            }
            using Run=void(*)(SourcePhysicsContactMemory&,std::uint32_t,SourcePhysicsWorldRuntimeAbi&,
                SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,
                const SourcePhysicsContactRecordObserver&);
            const std::pair<bool(*)(std::uint32_t),Run> owners[]{
                {source_sandbox_special_tu15_owns_entry,source_run_sandbox_special_tu15},
                {source_physics_live_support_tu15_owns_entry,source_run_physics_live_support_tu15},
                {source_animation_composition_tu15_owns_entry,source_run_animation_composition_tu15},
                {source_model_render_tu15_owns_entry,source_run_model_render_tu15},
                {source_scene_device_constructor_tu15_owns_entry,source_run_scene_device_constructor_tu15},
                {source_vehicle_director_tu15_owns_entry,source_run_vehicle_director_tu15},
                {source_campaign_camera_tu15_owns_entry,source_run_campaign_camera_tu15},
                {source_map_world_support_tu15_owns_entry,source_run_map_world_support_tu15},
                {source_effect_light_view_tu15_owns_entry,source_run_effect_light_view_tu15},
                {source_combat_action_tu15_owns_entry,source_run_combat_action_tu15}};
            for(const auto &[owns,run]:owners)if(owns(call.target)) {
                run(bank,call.target,abi,control,nested,stores);return;
            }
            std::ostringstream message;message<<"Native FP context requires unowned callee0x"<<
                std::hex<<call.target<<" at0x"<<call.instruction;throw FormatError(message.str());
        };
        return result;
    }
    void resource_read(SourcePhysicsWorldRuntimeAbi &abi) {
        const auto sp=abi.gp[4],tracking=abi.gp[7],descriptor=memory.word(tracking+0xcu);
        const auto size=memory.word(sp+4u),destination=memory.word(sp+8u);
        if(memory.word(sp)!=0x12613eu || memory.word(sp+12u)!=0 ||
            memory.word(sp+16u)!=tracking+4u || memory.word(sp+20u)!=6u)
            throw FormatError("Native FP animation resource caller differs from126139");
        const auto table=memory.word(0x4f0414u);
        for(const auto &tag:cache.select_tags({.group=tag_groups::model_animation_graph,.require_address=true})) {
            const auto native_root=memory.word(table+(tag.datum&65535u)*16u+8u);
            if(!native_root)continue;
            const auto count=memory.word(native_root+0xacu),first=memory.word(native_root+0xb0u);
            if(descriptor<first || std::uint64_t(descriptor)-first>=std::uint64_t(count)*20u ||
                (descriptor-first)%20u)continue;
            const auto index=(descriptor-first)/20u;
            const auto metadata=animation_graph_header(tag.datum);
            const Bytes header(metadata);
            if(header.u32(0xac)!=count)throw FormatError("Native FP resource group count differs from its Cache");
            const auto resource=cache.load_source_runtime_animation_image(header.u32(0xb0)+index*20u);
            std::array<std::byte,12> current;memory.read(descriptor,current);
            if(!std::equal(current.begin(),current.end(),resource.descriptor.begin()) ||
                abi.gp[1]!=Bytes(resource.descriptor).u32(8) || size!=((resource.bytes.size()+511u)&~511u))
                throw FormatError("Native FP read differs from its authored animation resource");
            const auto bytes=resources->load(abi.gp[1],size);
            if(bytes.size()!=size)throw FormatError("Native FP resource owner returned a partial read");
            memory.write(destination,bytes);
            if(next_job>UINT32_MAX)throw FormatError("Native FP private animation job namespace exhausted");
            const auto job=std::uint32_t(next_job++);
            jobs.emplace(job,ReadJob{tag.datum,index,descriptor,destination,size});
            const std::array done{std::byte{1}};memory.write(tracking+4u,done);
            abi.gp[0]=job;abi.gp[4]+=28u;return;
        }
        throw FormatError("Native FP resource has no admitted local Cache owner");
    }
    void call(const SourcePhysicsWorldRuntimeAbi &caller,std::uint32_t entry,
        std::initializer_list<std::uint32_t> arguments,std::optional<std::uint32_t> eax={},
        std::optional<std::uint32_t> ebx={},std::optional<std::uint32_t> ecx={}) {
        // F0DF1 retains the actual arithmetic status, including MXCSR's six
        // sticky exception flags. Validate controls without clearing those
        // source inputs; the native owners consume the complete caller ABI.
        if(!caller.floating_point || caller.floating_point->x87_control_word!=fp.x87_control_word ||
            (caller.floating_point->mxcsr&~0x3fu)!=fp.mxcsr || !caller.gp[4] || caller.gp[4]>0xffffffdfu)
            throw FormatError("Native FP context lacks the retained native caller ABI");
        const auto control=*caller.floating_point;
        auto abi=caller;unsigned n=1;
        for(const auto value:arguments)memory.put(caller.gp[4]+4u*n++,value);
        if(eax)abi.gp[0]=*eax;if(ebx)abi.gp[3]=*ebx;if(ecx)abi.gp[1]=*ecx;
        const auto native_services=services();
        if(source_physics_live_support_tu15_owns_entry(entry))
            source_run_physics_live_support_tu15(memory,entry,abi,control,native_services);
        else if(source_sandbox_special_tu15_owns_entry(entry))
            source_run_sandbox_special_tu15(memory,entry,abi,control,native_services);
        else if(source_vehicle_director_tu15_owns_entry(entry))
            source_run_vehicle_director_tu15(memory,entry,abi,control,native_services);
        else throw FormatError("Native FP context entry has no closed original owner");
    }
    std::vector<std::uint32_t> geometry_headers(const SourcePhysicsContactMemory &source,
        const SourceFirstPersonContextIdentityTu15 &identity)const {
        ContextMemory fresh;fresh.borrow(&source);
        const auto fresh_word=[&](std::uint32_t at){return fresh.word(at);};
        const auto fresh_body=[&](std::uint32_t datum) {
            const auto row=datum_row(fresh,fresh_word(0x4ecbd0u),datum,12u);
            const auto body=fresh_word(row+8u);
            if(!body)throw FormatError("Native FP geometry binding lost its actual object body");
            return body;
        };
        std::vector<std::uint32_t> queued_geometry;
        const auto tag_table=fresh_word(0x4f0414u);
        const auto tag_extent=[&](std::uint32_t datum,TagGroup group,std::uint32_t minimum) {
            const auto *tag=cache.find_tag(TagHandle{datum},group);
            const auto row=tag_table+(datum&65535u)*16u;
            if(!tag || !tag->address || tag->size<minimum || fresh_word(row)!=group.value ||
                fresh_word(row+4u)!=datum || fresh_word(row+12u)!=tag->size ||
                !fresh_word(row+8u) || std::uint64_t(fresh_word(row+8u))+tag->size>std::uint64_t(UINT32_MAX)+1u)
                throw FormatError("Native FP geometry binding differs from its fresh Cache tag owner");
            return std::pair{fresh_word(row+8u),std::uint32_t(tag->size)};
        };
        const auto block=[&](std::pair<std::uint32_t,std::uint32_t> tag,std::uint32_t offset,
            std::uint32_t stride) {
            const auto count=fresh_word(tag.first+offset),first=fresh_word(tag.first+offset+4u);
            if(count && (first<tag.first || std::uint64_t(first)-tag.first+std::uint64_t(count)*stride>tag.second))
                throw FormatError("Native FP geometry binding block escaped its immutable tag extent");
            return std::pair{count,first};
        };
        const auto model=[&](std::uint32_t datum) {
            if(datum==0xffffffffu)return;
            const auto extent=tag_extent(datum,TagGroup::from_string("mode"),0x2cu);
            const auto [count,first]=block(extent,0x24u,0x5cu);
            for(unsigned n=0;n<count;++n) {
                const auto header=first+n*0x5cu+0x38u;
                if(std::find(queued_geometry.begin(),queued_geometry.end(),header)==queued_geometry.end()) {
                    queued_geometry.push_back(header);
                }
            }
        };
        const auto globals=tag_extent(cache.globals_tag().value,tag_groups::globals,0x140u);
        if(fresh_word(0x4ecc1cu)!=globals.first)
            throw FormatError("Native FP representations differ from their fresh globals owner");
        const auto [representation_count,representations]=block(globals,0x138u,0xbcu);
        const auto inventory=[&](std::uint32_t unit,std::uint32_t representation) {
            if(representation>=representation_count)
                throw FormatError("Native FP geometry representation escaped its authored block");
            model(fresh_word(representations+representation*0xbcu+4u));
            const auto body=fresh_body(unit);
            for(unsigned slot=0;slot<4u;++slot) {
                const auto weapon=fresh_word(body+0x218u+slot*4u);if(weapon==0xffffffffu)continue;
                const auto definition=fresh_word(fresh_body(weapon));
                const auto extent=tag_extent(definition,TagGroup::from_string("weap"),0x2b0u);
                const auto [count,first]=block(extent,0x2a8u,0x10u);if(!count)continue;
                const unsigned preferred=representation==1u || representation==3u?1u:0u;
                model(fresh_word(first+(count>preferred?preferred:0u)*0x10u+4u));
            }
        };
        inventory(identity.unit,std::uint32_t(identity.representation));
        for(unsigned view=0;view<4u;++view) {
            const auto at=fresh_word(0x4f6498u)+view*0x20ccu,unit=fresh_word(at+4u);
            if(unit!=0xffffffffu)inventory(unit,fresh_word(at+8u));
        }
        return queued_geometry;
    }
    void retire_obsolete_geometry(const SourcePhysicsContactMemory &source,
        const SourcePhysicsWorldRuntimeAbi &caller,const SourceFirstPersonContextIdentityTu15 &identity) {
        const auto current=geometry_headers(source,identity);
        std::optional<SourceFirstPersonTemporalStateTu15> temporal_before;
        std::array<std::byte,8> random_before;
        // A private graphics request has no gameplay/event clock. At a
        // genuine binding transition, cancel obsolete private requests using
        // the same complete16E8A0 row retirement as130943/13079E. Relinquish
        // only the old queued byte to World; never free resident geometry or
        // drain IO, and never serialize an unverifiable former asset owner.
        for(auto header:geometry_owners)if(memory.owns(header+0x1fu) &&
            std::find(current.begin(),current.end(),header)==current.end()) {
            if(!temporal_before) {
                temporal_before=SourceFirstPersonContextTu15::capture_local(cache,memory,0u,identity,
                    published.generation,published.accepted_action_sequence);
                memory.read(memory.word(0x4f3cd8u),random_before);
            }
            const auto queue=memory.word(0x4f2d60u),rows=memory.word(queue+0x44u),bitmap=memory.word(queue+0x48u);
            if(memory.word(queue+0x20u)!=0x15eu || memory.word(queue+0x24u)!=8u || memory.word(queue+0x38u)>0x15eu)
                throw FormatError("Native FP obsolete graphics request lost its predicted queue owner");
            for(unsigned index=0;index<0x15eu;++index)if((memory.word(bitmap+(index/32u)*4u)&(1u<<(index%32u))) &&
                memory.word(rows+index*8u+4u)==header) {
                const auto salt=memory.word(rows+index*8u)&65535u;
                if(!salt)throw FormatError("Native FP obsolete graphics request lost its full row salt");
                auto abi=caller;auto control=caller.floating_point.value_or(fp);
                abi.gp[0]=(salt<<16u)|index;abi.gp[2]=queue;
                source_run_physics_vehicle_environment_state_runtime(memory,0x16e8a0u,abi,control,services());
            }
            memory.relinquish(header+0x1fu);
        }
        if(temporal_before) {
            std::array<std::byte,8> random_after;memory.read(memory.word(0x4f3cd8u),random_after);
            if(random_before!=random_after || *temporal_before!=SourceFirstPersonContextTu15::capture_local(
                cache,memory,0u,identity,published.generation,published.accepted_action_sequence))
                throw FormatError("Native private graphics retirement changed temporal controls or RNG");
        }
        geometry_owners=current;
    }
    std::uint32_t body(std::uint32_t datum)const {
        if(datum==0xffffffffu)throw FormatError("Native FP context requires an actual object identity");
        const auto row=datum_row(memory,memory.word(0x4ecbd0u),datum,12u);
        const auto result=memory.word(row+8u);
        if(!result)throw FormatError("Native FP object pool lost its body storage");
        return result;
    }
    void validate_identity(const SourceFirstPersonContextIdentityTu15 &identity)const {
        if(identity.player==0xffffffffu || (identity.representation!=2 && identity.representation!=3) ||
            memory.word(body(identity.unit))!=identity.unit_definition)
            throw FormatError("Native FP context unit identity differs from its input owner");
        const auto at=body(identity.unit);
        const auto player=datum_row(memory,memory.word(0x4f54f4u),identity.player,0x21cu);
        if(memory.word(at+0x13cu)!=identity.player || memory.word(player+0x2cu)!=identity.unit)
            throw FormatError("Native FP context player differs from the actual unit owner");
        for(unsigned hand=0;hand<2;++hand) {
            std::array<std::byte,1> slot;memory.read(at+0x212u+hand,slot);
            const auto index=std::bit_cast<std::int8_t>(slot[0]);
            if(index < -1 || index>=4)throw FormatError("Native FP context inventory slot escaped its original capacity");
            const auto actual=index==-1?0xffffffffu:memory.word(at+0x218u+std::uint32_t(index)*4u);
            if(actual!=identity.weapons[hand])throw FormatError("Native FP context weapon differs from its actual inventory");
            if(actual!=0xffffffffu && memory.word(body(actual))!=identity.weapon_definitions[hand])
                throw FormatError("Native FP context weapon definition differs from its input owner");
            if(actual==0xffffffffu && identity.weapon_definitions[hand]!=0xffffffffu)
                throw FormatError("Native FP context absent hand carries a definition");
        }
    }
    void bind(const SourcePhysicsWorldRuntimeAbi &caller,const SourceFirstPersonContextIdentityTu15 &identity) {
        validate_identity(identity);
        call(caller,0x16b16au,{0u,std::uint32_t(identity.representation)},identity.unit);
        const auto view=memory.word(0x4f6498u);
        if(memory.word(view+4u)!=identity.unit)throw FormatError("Native FP constructor rejected its unit binding");
        for(unsigned hand=0;hand<2;++hand) {
            // Original168D38 compares each actual inventory hand before16AD45.
            //16B16A alone returns immediately while the unit identity matches.
            if(memory.word(view+0x10u+hand*0x1010u)!=identity.weapons[hand])
                call(caller,0x16ad45u,{0u,hand});
            if(memory.word(view+0x10u+hand*0x1010u)!=identity.weapons[hand])
                throw FormatError("Native FP constructor rejected its weapon binding");
        }
    }
    unsigned graph_nodes(std::uint32_t graph)const {
        if(!cache.find_tag(TagHandle{graph},TagGroup::from_string("jmad")))
            throw FormatError("Native FP selected a graph outside its admitted Cache");
        const auto root=memory.word(memory.word(0x4f0414u)+(graph&65535u)*16u+8u);
        const auto nodes=memory.word(root+0xcu)&65535u;
        if(!nodes || nodes>64)throw FormatError("Native FP graph node count escaped its original signed capacity");
        return nodes;
    }
    void construct_view(const SourcePhysicsWorldRuntimeAbi &caller,
        const SourceFirstPersonContextIdentityTu15 &identity) {
        call(caller,0x168bc5u,{});
        call(caller,0x157eb0u,{}); //Complete original director constructor, never the host's camera bank.
        const auto roster=memory.word(0x4f54f0u);pin_range(roster,0x1cu);
        for(unsigned view=0;view<4;++view) {
            const auto old=memory.word(roster+0xcu+view*4u);
            if(old!=0xffffffffu)call(caller,0x151c90u,{old,0xffffffffu});
        }
        call(caller,0x151c90u,{identity.player,0u});
        bind(caller,identity);
    }
    void normalize_roster(const SourceFirstPersonContextIdentityTu15 &identity) {
        // Authority clone normalization: these are precisely151C90's typed
        // local-view datum/count and player+28 fields. No live owner is driven
        // while cloning completed local temporal state into private view0.
        const auto roster=memory.word(0x4f54f0u);pin_range(roster,0x1cu);
        for(unsigned view=0;view<4;++view) {
            const auto previous=memory.word(roster+0xcu+view*4u);
            if(previous!=0xffffffffu) {
                const auto player=datum_row(memory,memory.word(0x4f54f4u),previous,0x21cu);
                const std::array none{std::byte{0xff},std::byte{0xff}};memory.write(player+0x28u,none);
            }
            memory.put(roster+0xcu+view*4u,view?0xffffffffu:identity.player);
        }
        const std::array one{std::byte{1},std::byte{0}};memory.write(roster+8u,one);
        const auto player=datum_row(memory,memory.word(0x4f54f4u),identity.player,0x21cu);
        const std::array zero{std::byte{0},std::byte{0}};memory.write(player+0x28u,zero);
    }
    void capture(const SourceFirstPersonContextIdentityTu15 &identity) {
        const auto view=memory.word(0x4f6498u),locals=memory.word(0x4f6494u);
        published.identity=identity;published.source_tick=memory.word(memory.word(0x51df54u)+8u);
        published.view_flags=memory.word(view);
        memory.read(memory.word(0x51df54u),published.clock_controls);
        memory.read(view+0x202cu,published.view_feedback);
        published.camera_node=std::bit_cast<std::int32_t>(memory.word(view+0x2094u));
        for(unsigned hand=0;hand<2;++hand) {
            auto &out=published.hands[hand];const auto h=view+0xcu+hand*0x1010u;
            memory.read(h,out.control);
            const auto graph=memory.word(h+8u);unsigned nodes{};
            if(identity.weapons[hand]!=0xffffffffu) {
                //16AD45 already chose this hand's graph from the actual native
                // weapon/representation binding. Avoid decoding every authored
                // action/clip again merely to copy its transition-local bank.
                nodes=graph_nodes(graph);
            }
            out.saved_locals.resize(nodes);
            if(nodes)memory.read(locals+hand*0x1000u+0x800u,std::as_writable_bytes(std::span(out.saved_locals)));
        }
    }
};
SourceFirstPersonContextTu15::SourceFirstPersonContextTu15(const Cache &cache,
    std::shared_ptr<const SourceRuntimeResources> resources,SourcePhysicsWorldRuntimeServices cpu):
    impl_(std::make_unique<Impl>(cache,std::move(resources),std::move(cpu))){}
SourceFirstPersonContextTu15::~SourceFirstPersonContextTu15()=default;
const SourceRuntimeResources::Identity &SourceFirstPersonContextTu15::resources_identity()const noexcept {
    return impl_->resources->identity();
}
void SourceFirstPersonContextTu15::bind_cpu_services(const SourcePhysicsWorldRuntimeServices &cpu) {
    if(!cpu.reciprocal_sqrt || !cpu.timestamp_counter)
        throw FormatError("Native FP context requires both real CPU operand owners");
    impl_->cpu.reciprocal_sqrt=cpu.reciprocal_sqrt;impl_->cpu.timestamp_counter=cpu.timestamp_counter;
}
void SourceFirstPersonContextTu15::reset(const SourcePhysicsContactMemory &source,
    const SourcePhysicsWorldRuntimeAbi &caller,const SourceFirstPersonContextInputTu15 &input) {
    auto &owner=*impl_;owner.memory.clear();owner.jobs.clear();owner.next_job=0xf8000001u;
    owner.geometry_owners.clear();
    owner.initialized=false;owner.last_advanced_tick.reset();const auto generation=owner.published.generation+1u;owner.published={};
    owner.sampling_inputs.reset();
    owner.published.generation=generation;
    Impl::Borrow borrow(owner,source);owner.retain_roots(caller);owner.pin_animation_cache();
    const auto rng=owner.memory.word(0x4f3cd8u);
    owner.memory.write(rng,std::as_bytes(std::span(input.initial_rng)));
    owner.construct_view(caller,input.identity);
    owner.geometry_owners=owner.geometry_headers(source,input.identity);
    owner.capture(input.identity);owner.initialized=true;
}
void SourceFirstPersonContextTu15::apply_action(const SourcePhysicsContactMemory &source,
    const SourcePhysicsWorldRuntimeAbi &caller,const SourceFirstPersonActionRequestTu15 &request) {
    auto &owner=*impl_;
    if(!owner.initialized || request.unit!=owner.published.identity.unit || request.hand>=2u ||
        request.weapon!=owner.published.identity.weapons[request.hand])
        throw FormatError("Native FP action differs from its retained context owner");
    if(request.sequence<=owner.published.accepted_action_sequence)return;
    const auto saved=owner.memory;const auto jobs=owner.jobs;const auto next_job=owner.next_job;
    const auto published=owner.published;
    try {
        Impl::Borrow borrow(owner,source);
        if(request.source_tick!=owner.memory.word(owner.memory.word(0x51df54u)+8u))
            throw FormatError("Native FP action tick differs from its authority input");
        owner.call(caller,0x16b736u,{0u,request.weapon,request.hand},{},std::uint32_t(request.action));
        owner.published.accepted_action_sequence=request.sequence;
        owner.capture(owner.published.identity);
    } catch(...) {
        owner.memory=saved;owner.memory.borrow(nullptr);owner.jobs=jobs;owner.next_job=next_job;owner.published=published;throw;
    }
}
void SourceFirstPersonContextTu15::advance_tick(const SourcePhysicsContactMemory &source,
    const SourcePhysicsWorldRuntimeAbi &caller,const SourceFirstPersonContextInputTu15 &input) {
    auto &owner=*impl_;
    const auto &previous=owner.published.identity;
    if(!owner.initialized || previous.player!=input.identity.player || previous.unit!=input.identity.unit ||
        previous.unit_definition!=input.identity.unit_definition || previous.representation!=input.identity.representation)
        reset(source,caller,input);
    const auto saved=owner.memory;const auto jobs=owner.jobs;const auto next_job=owner.next_job;
    const auto published=owner.published;const auto advanced=owner.last_advanced_tick;
    const auto sampling=owner.sampling_inputs;const auto geometry=owner.geometry_owners;
    try {
        Impl::Borrow borrow(owner,source);
        const auto tick=owner.memory.word(owner.memory.word(0x51df54u)+8u);
        if(owner.last_advanced_tick && tick==*owner.last_advanced_tick)
            throw FormatError("Native FP temporal owner was advanced twice in one authority tick");
        if(std::uint32_t(tick-owner.published.source_tick)>1u)
            throw FormatError("Native FP temporal context lost an authority clock step");
        owner.bind(caller,input.identity); //16AD45 owns each changed hand, retaining the other hand/view.
        owner.retire_obsolete_geometry(source,caller,input.identity);
        if(Bytes(input.look).u32(0)!=input.identity.unit)
            throw FormatError("Native FP look input differs from its actual remote unit");
        owner.memory.write(owner.memory.word(0x4f9b54u)+0x14u,input.look);
        std::array<std::optional<SourceFirstPersonTemporalStateTu15>,2> captures;
        SourceFirstPersonCompositionObserverTu15 composition(owner.memory,
            [&](const auto &memory,const auto &,unsigned view,unsigned hand) {
                if(view!=0u || captures[hand])throw FormatError("Native private FP repeated its composition input boundary");
                captures[hand]=capture_local(owner.cache,memory,view,input.identity,
                    owner.published.generation,owner.published.accepted_action_sequence);
            });
        owner.call(caller,0x169c46u,{0u});
        owner.capture(input.identity);owner.last_advanced_tick=tick;
        owner.sampling_inputs=composition_inputs(owner.published,captures);
    } catch(...) {
        owner.memory=saved;owner.memory.borrow(nullptr);owner.jobs=jobs;owner.next_job=next_job;
        owner.published=published;owner.last_advanced_tick=advanced;owner.sampling_inputs=sampling;
        owner.geometry_owners=geometry;throw;
    }
}
SourceFirstPersonTemporalStateTu15 SourceFirstPersonContextTu15::state()const{return impl_->published;}
SourceFirstPersonTemporalStateTu15 SourceFirstPersonContextTu15::pose_inputs()const {
    if(!impl_->sampling_inputs)throw FormatError("Native FP owner has no genuine composition input boundary");
    return *impl_->sampling_inputs;
}
SourceFirstPersonTemporalStateTu15 SourceFirstPersonContextTu15::composition_inputs(
    const SourceFirstPersonTemporalStateTu15 &completed,
    const std::array<std::optional<SourceFirstPersonTemporalStateTu15>,2> &per_hand) {
    auto result=completed;const SourceFirstPersonTemporalStateTu15 *first{};
    for(unsigned hand=0;hand<2u;++hand) {
        if(!per_hand[hand]) {
            if((Bytes(completed.hands[hand].control).u32(0)&1u) &&
                completed.identity.weapons[hand]!=0xffffffffu)
                throw FormatError("Native FP active hand lost its original composition input boundary");
            continue;
        }
        const auto &input=*per_hand[hand];
        if(input.identity!=completed.identity || input.generation!=completed.generation || input.source_tick!=completed.source_tick ||
            input.clock_controls!=completed.clock_controls ||
            input.accepted_action_sequence!=completed.accepted_action_sequence)
            throw FormatError("Native FP composition input differs from its completed temporal owner");
        //16A4DD may update the shared feedback target via1E14F0. Replaying
        // hands in their original order reproduces that mutation; initialize
        // the shared inputs from the first actual composition call only.
        if(!first) {first=&input;result.view_feedback=input.view_feedback;result.camera_node=input.camera_node;}
        result.hands[hand]=input.hands[hand];
    }
    return result;
}
void SourceFirstPersonContextTu15::with_sampling_inputs(const SourcePhysicsContactMemory &source,
    const SourcePhysicsWorldRuntimeAbi &caller,const SourceFirstPersonTemporalStateTu15 &state,
    const std::function<void(const SourcePhysicsContactMemory&)> &consume,
    const SourcePhysicsWorldRuntimeServices &cpu) {
    if(!consume)throw FormatError("Native FP receiver requires its readonly consumer");
    // Receiver input installation has its own native cache/context. It never
    // changes this owner's authoritative temporal continuation or private RNG.
    if(!impl_->receiver_context)
        impl_->receiver_context=std::make_unique<Impl>(impl_->cache,impl_->resources,impl_->cpu);
    auto &receiver=*impl_->receiver_context;
    if(cpu.reciprocal_sqrt)receiver.cpu.reciprocal_sqrt=cpu.reciprocal_sqrt;
    if(cpu.timestamp_counter)receiver.cpu.timestamp_counter=cpu.timestamp_counter;
    Impl::Borrow borrow(receiver,source);
    if(!receiver.initialized)receiver.pin_animation_cache();
    if(!receiver.initialized || receiver.published.identity.unit!=state.identity.unit ||
        receiver.published.identity.representation!=state.identity.representation) {
        receiver.call(caller,0x168bc5u,{});
        receiver.call(caller,0x158790u,{},1u,{},0u);
        receiver.initialized=false;
    }
    receiver.bind(caller,state.identity);receiver.published.identity=state.identity;receiver.initialized=true;
    auto &memory=receiver.memory;const auto view=memory.word(0x4f6498u),locals=memory.word(0x4f6494u);
    for(unsigned hand=0;hand<2;++hand) {
        const auto h=view+0xcu+hand*0x1010u;const auto &received=state.hands[hand];
        const Bytes control(received.control);
        if(control.u32(4)!=state.identity.weapons[hand])
            throw FormatError("Native FP receiver hand differs from the declared owner");
        std::array<std::byte,0x300> authored;memory.read(h,authored);const Bytes actual(authored);
        // Bound graph/maps come only from the native local binding. Empty
        // hands retain their inactive historical bank without composing it.
        if(!same_native_first_person_hand_binding(authored,received.control,state.identity.weapons[hand]))
            throw FormatError("Native FP receiver controls differ from the admitted authored binding");
        unsigned nodes{};
        if(state.identity.weapons[hand]!=0xffffffffu) {
            nodes=receiver.graph_nodes(actual.u32(8));
        }
        if(nodes>64 || received.saved_locals.size()!=nodes)
            throw FormatError("Native FP receiver transition locals differ from its authored graph");
    }
    const auto install_inputs=[&] {
        for(unsigned hand=0;hand<2;++hand) {
            memory.write(view+0xcu+hand*0x1010u,state.hands[hand].control);
            if(!state.hands[hand].saved_locals.empty())memory.write(locals+hand*0x1000u+0x800u,
                std::as_bytes(std::span(state.hands[hand].saved_locals)));
        }
        memory.put(view,state.view_flags);memory.write(view+0x202cu,state.view_feedback);
        memory.put(view+0x2094u,std::bit_cast<std::uint32_t>(state.camera_node));
        memory.write(memory.word(0x51df54u),state.clock_controls);
        memory.put(memory.word(0x51df54u)+8u,state.source_tick);
    };
    install_inputs();
    for(unsigned hand=0;hand<2;++hand)if(state.identity.weapons[hand]!=0xffffffffu &&
        (Bytes(state.hands[hand].control).u32(0)&1u))
        receiver.call(caller,0x16a4ddu,{0u,hand}); //Only native sampling/real packed-resource admission.
    // Packed-resource admission may set the shared feedback target through
    //1E14F0. The readonly consumer must begin from the declared entry inputs,
    // rather than from an earlier composition's mutated private inputs.
    install_inputs();
    struct Inputs final:SourcePhysicsContactMemory {
        const ContextMemory &memory;std::array<std::pair<std::uint32_t,std::uint32_t>,5> denied;
        Inputs(const ContextMemory &m,std::uint32_t v,std::uint32_t l):memory(m),denied{{
            {v+0xcu+0x304u,0xd04u},{v+0xcu+0x1010u+0x304u,0xd04u},
            {v+0x2098u,0x34u},{l,0x800u},{l+0x1000u,0x800u}}}{}
        void read(std::uint32_t at,std::span<std::byte> bytes)const override {
            for(const auto &[base,count]:denied)
                if(std::uint64_t(at)<std::uint64_t(base)+count && std::uint64_t(base)<std::uint64_t(at)+bytes.size())
                    throw FormatError("Native FP receiver attempted to consume a completed pose output");
            memory.read(at,bytes);
        }
        void write(std::uint32_t,std::span<const std::byte>)override {
            throw FormatError("Native FP receiver input borrow is readonly");
        }
    } inputs(memory,view,locals);
    consume(inputs);
}
SourceFirstPersonTemporalStateTu15 SourceFirstPersonContextTu15::capture_local(const Cache &cache,
    const SourcePhysicsContactMemory &source,unsigned view,const SourceFirstPersonContextIdentityTu15 &identity,
    std::uint64_t generation,std::uint64_t action_sequence) {
    if(view>=4)throw FormatError("Native local FP capture escaped its source view capacity");
    ContextMemory memory;memory.borrow(&source);
    const auto at=memory.word(0x4f6498u)+view*0x20ccu;
    if(memory.word(at+4u)!=identity.unit || memory.word(at+8u)!=std::uint32_t(identity.representation))
        throw FormatError("Native local FP capture differs from its actual view owner");
    const auto unit=datum_row(memory,memory.word(0x4ecbd0u),identity.unit,12u);
    const auto player=datum_row(memory,memory.word(0x4f54f4u),identity.player,0x21cu);
    const auto body=memory.word(unit+8u);
    if(!body || memory.word(body)!=identity.unit_definition || memory.word(body+0x13cu)!=identity.player ||
        memory.word(player+0x2cu)!=identity.unit)
        throw FormatError("Native local FP capture differs from its actual unit/player binding");
    SourceFirstPersonTemporalStateTu15 result;
    result.generation=generation;result.accepted_action_sequence=action_sequence;result.identity=identity;
    result.source_tick=memory.word(memory.word(0x51df54u)+8u);result.view_flags=memory.word(at);
    memory.read(memory.word(0x51df54u),result.clock_controls);
    memory.read(at+0x202cu,result.view_feedback);
    result.camera_node=std::bit_cast<std::int32_t>(memory.word(at+0x2094u));
    const auto locals=memory.word(0x4f6494u);
    for(unsigned hand=0;hand<2;++hand) {
        const auto h=at+0xcu+hand*0x1010u;auto &out=result.hands[hand];memory.read(h,out.control);
        if(Bytes(out.control).u32(4)!=identity.weapons[hand])
            throw FormatError("Native local FP capture hand differs from its published weapon");
        unsigned nodes{};
        if(identity.weapons[hand]!=0xffffffffu) {
            const auto weapon=datum_row(memory,memory.word(0x4ecbd0u),identity.weapons[hand],12u);
            if(memory.word(memory.word(weapon+8u))!=identity.weapon_definitions[hand])
                throw FormatError("Native local FP capture weapon definition differs");
            const auto graph=Bytes(out.control).u32(8);
            if(!cache.find_tag(TagHandle{graph},TagGroup::from_string("jmad")))
                throw FormatError("Native local FP capture graph is outside its admitted Cache");
            const auto root=memory.word(memory.word(0x4f0414u)+(graph&65535u)*16u+8u);
            nodes=memory.word(root+0xcu)&65535u;
            if(!nodes || nodes>64)throw FormatError("Native local FP capture graph node count differs");
        }
        out.saved_locals.resize(nodes);
        if(nodes)memory.read(locals+(view*2u+hand)*0x1000u+0x800u,std::as_writable_bytes(std::span(out.saved_locals)));
    }
    return result;
}
void SourceFirstPersonContextTu15::prepare_identity(const SourcePhysicsContactMemory &source,
    const SourcePhysicsWorldRuntimeAbi &caller,const SourceFirstPersonContextInputTu15 &input) {
    auto &owner=*impl_;const auto &previous=owner.published.identity;
    if(!owner.initialized || previous.player!=input.identity.player || previous.unit!=input.identity.unit ||
        previous.unit_definition!=input.identity.unit_definition || previous.representation!=input.identity.representation) {
        reset(source,caller,input);return;
    }
    const auto memory=owner.memory;const auto jobs=owner.jobs;const auto next=owner.next_job;
    const auto published=owner.published;const auto geometry=owner.geometry_owners;
    try {Impl::Borrow borrow(owner,source);owner.bind(caller,input.identity);
        owner.retire_obsolete_geometry(source,caller,input.identity);owner.capture(input.identity);}
    catch(...) {owner.memory=memory;owner.memory.borrow(nullptr);owner.jobs=jobs;owner.next_job=next;
        owner.published=published;owner.geometry_owners=geometry;throw;}
}
SourceFirstPersonContextTu15::Checkpoint SourceFirstPersonContextTu15::checkpoint()const {
    const auto &owner=*impl_;Checkpoint result;
    result.resources=owner.resources->identity();result.temporal=owner.published;
    result.initialized=owner.initialized;result.last_advanced_tick=owner.last_advanced_tick;
    result.next_job=owner.next_job;result.owner_roots=owner.owner_roots;result.caller_stack=owner.caller_stack;
    result.private_ranges=owner.memory.ranges();
    for(const auto &[identity,job]:owner.jobs)
        result.read_jobs.push_back({identity,job.graph,job.index,job.descriptor,job.destination,job.bytes});
    return result;
}
void SourceFirstPersonContextTu15::prepare_restore(const SourcePhysicsContactMemory &source,
    const SourcePhysicsWorldRuntimeAbi &caller,const Checkpoint &saved) {
    auto &owner=*impl_;
    if(owner.initialized || !owner.memory.ranges().empty())
        throw FormatError("Native FP continuation must prepare a fresh context owner");
    if(saved.resources!=owner.resources->identity())
        throw FormatError("Native FP continuation Cache identity differs");
    if(saved.next_job<0xf8000001u || saved.next_job>std::uint64_t(UINT32_MAX)+1u ||
        saved.read_jobs.size()>0x200u || saved.private_ranges.size()>8192u)
        throw FormatError("Native FP continuation exceeded its original cache capacities");
    Impl::Borrow borrow(owner,source);
    const auto fresh_word=[&](std::uint32_t address) {
        std::array<std::byte,4> bytes;source.read(address,bytes);return Bytes(bytes).u32(0);
    };
    std::vector<std::pair<std::uint32_t,std::uint64_t>> allowed;
    const auto allow=[&](std::uint32_t address,std::uint64_t bytes) {
        if(!address || !bytes || std::uint64_t(address)+bytes>std::uint64_t(UINT32_MAX)+1u)
            throw FormatError("Native FP continuation owner extent wraps or is empty");
        allowed.emplace_back(address,bytes);
    };
    const auto pool_extent=[&](std::uint32_t pool,std::uint32_t capacity,std::uint32_t stride) {
        if(fresh_word(pool+0x20u)!=capacity || fresh_word(pool+0x24u)!=stride)
            throw FormatError("Native FP continuation source cache pool differs");
        allow(pool,0x4cu);allow(fresh_word(pool+0x44u),std::uint64_t(capacity)*stride);
        allow(fresh_word(pool+0x48u),((capacity+31u)/32u)*4u);
    };
    std::vector<std::pair<std::uint32_t,std::uint32_t>> descriptors;
    std::vector<std::uint32_t> cache_pools;
    std::vector<std::uint32_t> queued_geometry;
    std::vector<std::uint32_t> geometry_headers;
    std::vector<std::uint32_t> bitmap_headers;
    if(saved.initialized) {
        // Qualify actor identities against the fresh World before installing
        // any private bytes. A checkpoint cannot manufacture its own owners.
        owner.validate_identity(saved.temporal.identity);
        for(unsigned n=0;n<Impl::root_globals.size();++n)
            if(!saved.owner_roots[n] || fresh_word(Impl::root_globals[n])!=saved.owner_roots[n])
                throw FormatError("Native FP continuation root differs from the restored native World");
        allow(fresh_word(0x4f6498u),0x8330u);allow(fresh_word(0x4f6494u),0x8000u);
        allow(0x4f5508u,0x50cu);allow(fresh_word(0x4f3cd8u),8u);
        // Native animation evaluation's bounded global scratch record,
        // used by authenticated28ADA0 and its selected codec callees.
        allow(0x510ef0u,0xb4u);
        //2835B0 explicitly initializes three256-node bit masks here.
        allow(0x56e000u,0x60u);
        //296540's two explicitly stored translation vectors (six floats).
        allow(0x56e688u,0x18u);
        //2962F0's two codec translation temporaries (six floats), all stored
        // before they are read (loader-zeroed .bss, mounted at56E6A0).
        allow(0x56e6a0u,0x18u);
        // Native12F720/12F761 touches only the usage timestamp (+14) of
        // an existing texture allocation.12EBE0 constructs this distinct
        // allocator at4F2D34; its512x18 inline pool is not the geometry
        // cache at4F2D6C. Keep the texture allocation's remaining metadata,
        // callbacks, pool topology and resource bytes borrowed from World.
        const auto texture=fresh_word(0x4f2d34u);
        if(!texture || texture>UINT32_MAX-0x30fcu ||
            fresh_word(texture+0x64u)!=texture+0x70u || fresh_word(texture+0x34u)!=12u ||
            fresh_word(texture+0x68u)!=0x77656565u || fresh_word(texture+0x9cu)!=0x64407440u ||
            fresh_word(texture+0x90u)!=0x200u || fresh_word(texture+0x94u)!=0x18u ||
            fresh_word(texture+0xa8u)>0x200u || fresh_word(texture+0xb4u)!=texture+0xbcu ||
            fresh_word(texture+0xb8u)!=texture+0x30bcu)
            throw FormatError("Native FP continuation texture usage owner differs from12EBE0");
        for(unsigned index=0;index<0x200u;++index)allow(texture+0xd0u+index*0x18u,4u);
        // Original1303C0 constructs a separate13FC50 geometry allocator.
        // Resident geometry1307C0/130980 touches only each allocation's +14
        // usage stamp (130809/1309A8), just as the distinct texture path above.
        // Authenticate its fresh native inline pool; never import its resource
        // pointers, allocator topology, callbacks or resident geometry bytes.
        const auto geometry=fresh_word(0x4f2d6cu);
        if(!geometry || geometry>UINT32_MAX-0x30fcu ||
            fresh_word(geometry+0x64u)!=geometry+0x70u || fresh_word(geometry+0x34u)!=12u ||
            fresh_word(geometry+0x68u)!=0x77656565u || fresh_word(geometry+0x9cu)!=0x64407440u ||
            fresh_word(geometry+0x90u)!=0x200u || fresh_word(geometry+0x94u)!=0x18u ||
            fresh_word(geometry+0xa8u)>0x200u || fresh_word(geometry+0xb4u)!=geometry+0xbcu ||
            fresh_word(geometry+0xb8u)!=geometry+0x30bcu)
            throw FormatError("Native FP continuation geometry usage owner differs from1303C0/13FC50");
        for(unsigned index=0;index<0x200u;++index)allow(geometry+0xd0u+index*0x18u,4u);
        if(const auto* trace=std::getenv("HALO2_SHADOW_TRANSPORT_TRACE");trace&&*trace) {
            static std::atomic<unsigned> receipts{};
            for(unsigned index=0;index<0x200u;++index) {
                const auto at=geometry+0xd0u+index*0x18u;
                if(std::ranges::any_of(saved.private_ranges,[&](const auto& range){
                    return range.address<=at && std::uint64_t(at)+4u<=std::uint64_t(range.address)+range.bytes.size();})) {
                    if(receipts.fetch_add(1,std::memory_order_relaxed)<16)
                        std::fprintf(stderr,"[native-fp-continuation] {\"geometry_allocator\":%u,\"geometry_rows\":%u,\"first_geometry_usage\":%u,\"geometry_usage_index\":%u}\n",
                            geometry,geometry+0xbcu,at,index);
                    break;
                }
            }
        }
        allow(fresh_word(0x4f9b54u)+0x14u,4u*0x94u);
        allow(fresh_word(0x4f54f0u),0x1cu);
        const auto players=fresh_word(0x4f54f4u),rows=fresh_word(players+0x44u);
        if(fresh_word(players+0x20u)!=16u || fresh_word(players+0x24u)!=0x21cu)
            throw FormatError("Native FP continuation player pool differs");
        for(unsigned n=0;n<16u;++n)allow(rows+n*0x21cu+0x28u,2u);
        //157EB0's only stores through its external camera-registration root.
        const auto camera=fresh_word(0x51df6cu);allow(camera+2u,2u);allow(camera+0x3cu,4u);
        allow(0x4f0418u,0x18u);allow(0x51df20u,2u);
        cache_pools={fresh_word(0x4f0418u),fresh_word(0x4f041cu),fresh_word(0x4f0424u)+0x70u};
        pool_extent(cache_pools[0],0x200u,0x10u);pool_extent(cache_pools[1],0x80u,0xcu);
        // Original13041C/130421 constructs this predicted-resource request
        // queue;130844/13084A allocates it and13085C stores its resource.
        const auto predicted=fresh_word(0x4f2d60u);
        if(!predicted || predicted>UINT32_MAX-0xb68u || fresh_word(predicted+0x2cu)!=0x64407440u ||
            fresh_word(predicted+0x44u)!=predicted+0x4cu || fresh_word(predicted+0x48u)!=predicted+0xb3cu)
            throw FormatError("Native FP geometry prediction owner differs from1303C0/130426");
        pool_extent(predicted,0x15eu,8u);cache_pools.push_back(predicted);
        if(!owner.cache.is_mcc()) {
            const auto bitmaps=fresh_word(0x4f2d28u);
            if(!bitmaps || bitmaps>UINT32_MAX-0x6a8u || fresh_word(bitmaps+0x2cu)!=0x64407440u ||
                fresh_word(bitmaps+0x44u)!=bitmaps+0x4cu || fresh_word(bitmaps+0x48u)!=bitmaps+0x68cu)
                throw FormatError("Native FP bitmap prediction owner differs from12EBE0/12EC46");
            pool_extent(bitmaps,0xc8u,8u);cache_pools.push_back(bitmaps);
            bitmap_headers=owner.bitmap_prediction_headers(source);
            for(const auto image:bitmap_headers)allow(image+0xfu,1u);
        }
        //13085C/130860 publish the exact geometry header and set only its
        // queued bit. The fresh Cache's complete full-salt authored model/BSP
        // banks identify one-byte owners; rows and queued bits must pair below.
        queued_geometry=owner.geometry_headers(source,saved.temporal.identity);
        geometry_headers=owner.geometry_prediction_headers(source);
        for(auto header:geometry_headers)allow(header+0x1fu,1u);
        allow(0x4f7208u,8u);
        const auto emitters=fresh_word(0x4f7208u),effects=fresh_word(0x4f720cu);
        pool_extent(emitters,0x200u,0x3cu);pool_extent(effects,0x100u,0x190u);
        cache_pools.push_back(emitters);cache_pools.push_back(effects);
        const auto allocator=fresh_word(0x4f0424u);
        allow(allocator,0x30fcu);pool_extent(cache_pools[2],0x200u,0x18u);
        if(fresh_word(allocator+0x64u)!=cache_pools[2] || fresh_word(allocator+0x34u)!=11u ||
            !fresh_word(0x4f0428u) || fresh_word(0x4f0428u)>0x400000u)
            throw FormatError("Native FP continuation animation allocator differs");
        allow(fresh_word(0x4f0420u),fresh_word(0x4f0428u));
        const auto table=fresh_word(0x4f0414u);
        for(const auto &tag:owner.cache.select_tags({.group=tag_groups::model_animation_graph,.require_address=true})) {
            const auto root=fresh_word(table+(tag.datum&65535u)*16u+8u);if(!root)continue;
            const auto count=fresh_word(root+0xacu),first=fresh_word(root+0xb0u);
            if(count){allow(first,std::uint64_t(count)*20u);descriptors.emplace_back(first,count);}
        }
        // The actual retained caller, not an address supplied by the archive,
        // identifies its complete native main-stack storage owner.
        const auto *native=dynamic_cast<const SourceRuntimeMemory*>(&source);bool stack_found{};
        if(native)native->visit_committed([&](std::uint32_t at,std::span<const std::byte> bytes) {
            if(std::uint64_t(at)<=caller.gp[4] && caller.gp[4]<std::uint64_t(at)+bytes.size()) {
                if(saved.caller_stack<at || saved.caller_stack>=std::uint64_t(at)+bytes.size())
                    throw FormatError("Native FP continuation caller belongs to another stack owner");
                allow(at,bytes.size());stack_found=true;
            }
        });
        if(!stack_found)throw FormatError("Native FP continuation lacks its actual native caller stack owner");
        std::sort(allowed.begin(),allowed.end());
    }
    std::uint64_t total{},end{};
    for(const auto &range:saved.private_ranges) {
        if(range.bytes.empty() || std::uint64_t(range.address)+range.bytes.size()>std::uint64_t(UINT32_MAX)+1u ||
            (!total?false:range.address<end))
            throw FormatError("Native FP continuation ranges overlap or wrap");
        auto covered=std::uint64_t(range.address),limit=covered+range.bytes.size();
        for(const auto &[address,bytes]:allowed) {
            if(address>covered)break;
            if(std::uint64_t(address)+bytes>covered)covered=std::min(limit,std::uint64_t(address)+bytes);
            if(covered==limit)break;
        }
        if(covered!=limit) {std::ostringstream message;message<<"Native FP continuation range outside typed owners at0x"<<std::hex<<range.address<<" bytes0x"<<range.bytes.size()<<" first-unowned0x"<<covered;for(auto root:Impl::root_globals)message<<" root"<<root<<"="<<fresh_word(root);message<<" input="<<fresh_word(0x4f9b54u)<<" camera="<<fresh_word(0x51df6cu)<<" geometry="<<fresh_word(0x4f2d6cu);throw FormatError(message.str());}
        total+=range.bytes.size();end=std::uint64_t(range.address)+range.bytes.size();
        if(total>8u*1024u*1024u)throw FormatError("Native FP continuation exceeded its bounded private animation owners");
        owner.memory.write(range.address,range.bytes);
    }
    if(saved.initialized) {
        // Runtime cache counters/slots are private; their storage, capacity,
        // assets and allocator topology must remain the actual source owners.
        for(auto pool:cache_pools)for(auto offset:{0x20u,0x24u,0x44u,0x48u})
            if(owner.memory.word(pool+offset)!=fresh_word(pool+offset))
                throw FormatError("Native FP continuation changed immutable cache-pool topology");
        const auto predicted=fresh_word(0x4f2d60u),predicted_rows=fresh_word(predicted+0x44u);
        const auto predicted_bits=fresh_word(predicted+0x48u),upper=owner.memory.word(predicted+0x38u);
        const auto geometry_salt=owner.memory.word(predicted+0x40u)&65535u;
        if(upper>0x15eu || owner.memory.word(predicted+0x34u)>0x15eu ||
            !(geometry_salt&0x8000u) || geometry_salt==65535u)
            throw FormatError("Native FP geometry queue escaped its original capacity or salt domain");
        std::array<std::byte,0x4c> actual_geometry_pool,private_geometry_pool;
        source.read(predicted,actual_geometry_pool);owner.memory.read(predicted,private_geometry_pool);
        for(unsigned n=0;n<actual_geometry_pool.size();++n)if((n<0x34u || n>=0x42u) &&
            actual_geometry_pool[n]!=private_geometry_pool[n])
            throw FormatError("Native FP geometry prediction changed immutable pool metadata");
        std::vector<std::uint32_t> active_geometry;unsigned geometry_high{};
        for(unsigned index=0;index<0x15eu;++index) {
            const auto mask=1u<<(index%32u),bits=predicted_bits+(index/32u)*4u;
            if(!(owner.memory.word(bits)&mask))continue;
            const auto row=predicted_rows+index*8u,header=owner.memory.word(row+4u);
            const auto salt=owner.memory.word(row);
            if(index>=upper || salt>65535u || !(salt&0x8000u) || salt==65535u ||
                !std::binary_search(geometry_headers.begin(),geometry_headers.end(),header))
                throw FormatError("Native FP geometry queue has an invalid active resource row");
            geometry_high=index+1u;
            if(std::find(active_geometry.begin(),active_geometry.end(),header)!=active_geometry.end())
                throw FormatError("Native FP geometry prediction duplicated an authored section request");
            active_geometry.push_back(header);
            std::array<std::byte,1> flag;owner.memory.read(header+0x1fu,flag);
            if((flag[0]&std::byte{4})==std::byte{})
                throw FormatError("Native FP geometry queue row lacks its paired queued flag");
        }
        if(owner.memory.word(predicted+0x3cu)!=active_geometry.size() || geometry_high!=upper ||
            (owner.memory.word(predicted_bits+10u*4u)&0xc0000000u))
            throw FormatError("Native FP geometry queue active count differs from its bitmap");
        for(auto header:geometry_headers) {
            std::array<std::byte,1> actual,restored;source.read(header+0x1fu,actual);
            owner.memory.read(header+0x1fu,restored);
            if(((actual[0]^restored[0])&std::byte{0xfbu})!=std::byte{})
                throw FormatError("Native FP geometry continuation changed an immutable resource flag");
            const bool privately_written=std::any_of(saved.private_ranges.begin(),saved.private_ranges.end(),
                [&](const auto &range){return range.address<=header+0x1fu &&
                    std::uint64_t(header)+0x1fu<std::uint64_t(range.address)+range.bytes.size();});
            if(privately_written && (restored[0]&std::byte{4})!=std::byte{} &&
                std::count(active_geometry.begin(),active_geometry.end(),header)!=1)
                throw FormatError("Native FP queued resource flag lacks its actual predicted-resource row");
        }
        if(!owner.cache.is_mcc()) {
            const auto queue=fresh_word(0x4f2d28u),rows=fresh_word(queue+0x44u),bits=fresh_word(queue+0x48u);
            const auto upper=owner.memory.word(queue+0x38u),salt=owner.memory.word(queue+0x40u)&65535u;
            if(upper>0xc8u || owner.memory.word(queue+0x34u)>0xc8u || !(salt&0x8000u) || salt==65535u)
                throw FormatError("Native FP bitmap queue escaped its original capacity or salt domain");
            std::array<std::byte,0x4c> actual,restored;
            source.read(queue,actual);owner.memory.read(queue,restored);
            for(unsigned n=0;n<actual.size();++n)if((n<0x34u || n>=0x42u) && actual[n]!=restored[n])
                throw FormatError("Native FP bitmap queue changed immutable native pool metadata");
            std::vector<std::uint32_t> active;unsigned high{};
            for(unsigned index=0;index<0xc8u;++index)if(owner.memory.word(bits+(index/32u)*4u)&(1u<<(index%32u))) {
                const auto row=rows+index*8u,image=owner.memory.word(row+4u),salt=owner.memory.word(row);
                if(index>=upper || salt>65535u || !(salt&0x8000u) || salt==65535u ||
                    !std::binary_search(bitmap_headers.begin(),bitmap_headers.end(),image))
                    throw FormatError("Native FP bitmap queue has an invalid full-salt authored image row");
                if(std::find(active.begin(),active.end(),image)!=active.end())
                    throw FormatError("Native FP bitmap queue duplicated an authored image request");
                active.push_back(image);high=index+1u;
                std::array<std::byte,1> flag;owner.memory.read(image+0xfu,flag);
                if((flag[0]&std::byte{4})==std::byte{})
                    throw FormatError("Native FP bitmap queue row lacks its paired authored queued flag");
            }
            if(owner.memory.word(queue+0x3cu)!=active.size() || high!=upper ||
                (owner.memory.word(bits+24u)&0xffffff00u))
                throw FormatError("Native FP bitmap queue count/high-water/tail differs from its exact native bitmap");
            for(const auto image:bitmap_headers) {
                std::array<std::byte,1> actual,restored;source.read(image+0xfu,actual);
                owner.memory.read(image+0xfu,restored);
                if(((actual[0]^restored[0])&std::byte{0xfbu})!=std::byte{})
                    throw FormatError("Native FP bitmap continuation changed an immutable authored image flag");
                const bool retained=std::any_of(saved.private_ranges.begin(),saved.private_ranges.end(),
                    [&](const auto& range){return range.address<=image+0xfu &&
                        std::uint64_t(image)+0xfu<std::uint64_t(range.address)+range.bytes.size();});
                if(retained && (restored[0]&std::byte{4})!=std::byte{} &&
                    std::count(active.begin(),active.end(),image)!=1)
                    throw FormatError("Native FP bitmap queued flag lacks its unique actual native request row");
            }
        }
        for(auto root:{0x4f7208u,0x4f720cu})
            if(owner.memory.word(root)!=fresh_word(root))
                throw FormatError("Native FP continuation changed an effect-cache root");
        const auto allocator=fresh_word(0x4f0424u);
        for(auto offset:{0x20u,0x24u,0x28u,0x30u,0x34u,0x64u,0x68u,0x6cu})
            if(owner.memory.word(allocator+offset)!=fresh_word(allocator+offset))
                throw FormatError("Native FP continuation changed immutable animation allocator input");
        std::array<std::byte,24> actual_globals,private_globals;source.read(0x4f0418u,actual_globals);
        owner.memory.read(0x4f0418u,private_globals);
        if(actual_globals!=private_globals)throw FormatError("Native FP continuation changed animation-cache root inputs");
        for(const auto &[first,count]:descriptors)for(unsigned index=0;index<count;++index) {
            std::array<std::byte,12> actual,restored;
            source.read(first+index*20u,actual);owner.memory.read(first+index*20u,restored);
            if(actual!=restored)throw FormatError("Native FP continuation changed immutable animation descriptor bytes");
        }
        const auto roster=fresh_word(0x4f54f0u),players=fresh_word(0x4f54f4u),rows=fresh_word(players+0x44u);
        if((owner.memory.word(roster+8u)&65535u)!=1u || owner.memory.word(roster+0xcu)!=saved.temporal.identity.player)
            throw FormatError("Native FP continuation changed its declared private local-view binding");
        for(unsigned n=1;n<4u;++n)if(owner.memory.word(roster+0xcu+n*4u)!=0xffffffffu)
            throw FormatError("Native FP continuation contains another local-view owner");
        for(unsigned n=0;n<16u;++n) {
            const auto position=rows+n*0x21cu+0x28u;
            const auto value=owner.memory.word(position)&65535u;
            bool owned{};for(const auto &range:saved.private_ranges)
                owned|=range.address<=position && std::uint64_t(position)+2u<=std::uint64_t(range.address)+range.bytes.size();
            if(owned && value!=(n==(saved.temporal.identity.player&65535u)?0u:65535u))
                throw FormatError("Native FP continuation changed a private player-view selector");
        }
        for(unsigned n=0;n<Impl::root_globals.size();++n) {
            std::array<std::byte,4> actual;source.read(Impl::root_globals[n],actual);
            if(!saved.owner_roots[n] || Bytes(actual).u32(0)!=saved.owner_roots[n])
                throw FormatError("Native FP continuation root differs from the restored native World");
        }
        owner.validate_identity(saved.temporal.identity);
        owner.published=saved.temporal;owner.capture(saved.temporal.identity);
        const auto &actual=owner.published;
        bool same=actual.view_flags==saved.temporal.view_flags && actual.clock_controls==saved.temporal.clock_controls &&
            actual.view_feedback==saved.temporal.view_feedback && actual.camera_node==saved.temporal.camera_node;
        for(unsigned h=0;h<2;++h)same=same && actual.hands[h].control==saved.temporal.hands[h].control &&
            actual.hands[h].saved_locals==saved.temporal.hands[h].saved_locals;
        if(!same || (actual.source_tick!=saved.temporal.source_tick && actual.source_tick!=saved.temporal.source_tick+1u))
            throw FormatError("Native FP continuation does not contain its declared completed temporal state");
    } else if(!saved.private_ranges.empty() || !saved.read_jobs.empty())
        throw FormatError("Inactive native FP continuation carries private owners");
    for(const auto &job:saved.read_jobs) {
        if(job.identity<0xf8000001u || job.identity>=saved.next_job || !job.bytes || job.bytes>0x400000u ||
            !owner.jobs.emplace(job.identity,Impl::ReadJob{job.graph,job.index,job.descriptor,job.destination,job.bytes}).second)
            throw FormatError("Native FP continuation animation read-job identities differ");
        const auto metadata=owner.animation_graph_header(job.graph);const Bytes header(metadata);
        if(job.index>=header.u32(0xac))throw FormatError("Native FP continuation job escaped its authored graph groups");
        const auto table=fresh_word(0x4f0414u),native_graph=fresh_word(table+(job.graph&65535u)*16u+8u);
        if(job.descriptor!=fresh_word(native_graph+0xb0u)+job.index*20u ||
            job.destination<fresh_word(0x4f0420u) ||
            std::uint64_t(job.destination)-fresh_word(0x4f0420u)+job.bytes>fresh_word(0x4f0428u))
            throw FormatError("Native FP continuation job escaped its actual animation owners");
        const auto resource=owner.cache.load_source_runtime_animation_image(header.u32(0xb0)+job.index*20u);
        std::array<std::byte,12> descriptor;owner.memory.read(job.descriptor,descriptor);
        if(!std::equal(descriptor.begin(),descriptor.end(),resource.descriptor.begin()) ||
            job.bytes!=((resource.bytes.size()+511u)&~511u))
            throw FormatError("Native FP continuation job differs from its actual Cache descriptor");
    }
    owner.published=saved.temporal;owner.initialized=saved.initialized;owner.last_advanced_tick=saved.last_advanced_tick;
    owner.owner_roots=saved.owner_roots;owner.caller_stack=saved.caller_stack;owner.next_job=saved.next_job;
    owner.geometry_owners=std::move(queued_geometry);
    (void)caller; //No kernel is entered while preparing the private continuation.
}
void SourceFirstPersonContextTu15::adopt_local(const SourcePhysicsContactMemory &source,
    const SourcePhysicsWorldRuntimeAbi &caller,unsigned view,const SourceFirstPersonContextIdentityTu15 &identity,
    std::uint64_t generation,std::uint64_t action_sequence) {
    const auto captured=capture_local(impl_->cache,source,view,identity,generation,action_sequence);
    auto &owner=*impl_;owner.memory.clear();owner.jobs.clear();owner.next_job=0xf8000001u;
    owner.initialized=false;Impl::Borrow borrow(owner,source);owner.retain_roots(caller);owner.pin_animation_cache();
    owner.normalize_roster(identity);
    const auto fp=owner.memory.word(0x4f6498u),locals=owner.memory.word(0x4f6494u);
    std::vector<std::byte> current_view(0x20ccu);source.read(fp+view*0x20ccu,current_view);owner.memory.write(fp,current_view);
    std::vector<std::byte> current_locals(0x2000u);source.read(locals+view*0x2000u,current_locals);owner.memory.write(locals,current_locals);
    std::array<std::byte,8> rng;source.read(owner.memory.word(0x4f3cd8u),rng);owner.memory.write(owner.memory.word(0x4f3cd8u),rng);
    std::array<std::byte,0x140> director;source.read(0x4f5514u+view*0x140u,director);owner.memory.write(0x4f5514u,director);
    owner.published=captured;owner.last_advanced_tick=captured.source_tick;owner.initialized=true;
    owner.geometry_owners=owner.geometry_headers(source,identity);
    owner.sampling_inputs.reset();
}
void SourceFirstPersonContextTu15::install_local(const Cache &cache,PhysicsLiveWorldTu15 &world,
    unsigned view,const SourceFirstPersonTemporalStateTu15 &continuation,
    const SourceFirstPersonTemporalStateTu15 &sampling_inputs) {
    if(view>=4u)throw FormatError("Native FP local continuation escaped its view capacity");
    if(continuation.identity!=sampling_inputs.identity || continuation.source_tick!=sampling_inputs.source_tick ||
        continuation.generation!=sampling_inputs.generation ||
        continuation.accepted_action_sequence!=sampling_inputs.accepted_action_sequence ||
        continuation.clock_controls!=sampling_inputs.clock_controls)
        throw FormatError("Native FP local sampling inputs differ from their completed continuation");
    ContextMemory input;input.borrow(&world.memory());
    const auto at=input.word(0x4f6498u)+view*0x20ccu,locals=input.word(0x4f6494u);
    const auto &identity=continuation.identity;
    if(input.word(at+4u)!=identity.unit || input.word(at+8u)!=std::uint32_t(identity.representation))
        throw FormatError("Native FP local continuation differs from the configured view owner");
    const auto unit=datum_row(input,input.word(0x4ecbd0u),identity.unit,12u);
    const auto player=datum_row(input,input.word(0x4f54f4u),identity.player,0x21cu),body=input.word(unit+8u);
    if(!body || input.word(body)!=identity.unit_definition || input.word(body+0x13cu)!=identity.player ||
        input.word(player+0x2cu)!=identity.unit || (input.word(player+0x28u)&65535u)!=view)
        throw FormatError("Native FP local continuation differs from its current unit/player binding");
    std::array<std::byte,8> clock;input.read(input.word(0x51df54u),clock);
    if(clock!=continuation.clock_controls)throw FormatError("Native FP local continuation clock controls differ");
    for(const auto *state:{&continuation,&sampling_inputs})for(unsigned hand=0;hand<2;++hand) {
        const auto h=at+0xcu+hand*0x1010u;const auto &received=state->hands[hand];
        std::array<std::byte,0x300> authored;input.read(h,authored);
        const Bytes native(authored);
        if(!same_native_first_person_hand_binding(authored,received.control,identity.weapons[hand])) {
            throw FormatError("Native FP local continuation differs from its authored hand binding: view="+
                std::to_string(view)+" hand="+std::to_string(hand)+
                " state="+(state==&continuation?"POST":"PRE"));
        }
        unsigned nodes{};
        if(identity.weapons[hand]!=0xffffffffu) {
            const auto weapon=datum_row(input,input.word(0x4ecbd0u),identity.weapons[hand],12u);
            if(input.word(input.word(weapon+8u))!=identity.weapon_definitions[hand] ||
                !cache.find_tag(TagHandle{native.u32(8)},TagGroup::from_string("jmad")))
                throw FormatError("Native FP local continuation weapon/graph identity differs");
            const auto root=input.word(input.word(0x4f0414u)+(native.u32(8)&65535u)*16u+8u);
            nodes=input.word(root+0xcu)&65535u;
            if(!nodes || nodes>64u)throw FormatError("Native FP local continuation graph nodes escaped capacity");
        }
        if(received.saved_locals.size()!=nodes)
            throw FormatError("Native FP local continuation transition-local count differs");
    }
    // Every identity/map/extent is qualified before the first staged write.
    // Existing declared-input and native-entry owners preserve real World ABI,
    // resource delivery and CPU operand accounting throughout this restoration.
    const auto install=[&](const SourceFirstPersonTemporalStateTu15 &state) {
        for(unsigned hand=0;hand<2;++hand) {
            world.write_source_call_input(at+0xcu+hand*0x1010u,state.hands[hand].control);
            if(!state.hands[hand].saved_locals.empty())world.write_source_call_input(
                locals+(view*2u+hand)*0x1000u+0x800u,std::as_bytes(std::span(state.hands[hand].saved_locals)));
        }
        world.write_source_call_input(at,std::as_bytes(std::span(&state.view_flags,1)));
        world.write_source_call_input(at+0x202cu,state.view_feedback);
        world.write_source_call_input(at+0x2094u,std::as_bytes(std::span(&state.camera_node,1)));
    };
    install(sampling_inputs);
    for(unsigned hand=0;hand<2;++hand)if(identity.weapons[hand]!=0xffffffffu &&
        (Bytes(sampling_inputs.hands[hand].control).u32(0)&1u)) {
        const std::array arguments{std::uint32_t(view),std::uint32_t(hand)};
        auto caller=world.prepare_source_call(arguments);world.run_actual(0x16a4ddu,caller);
    }
    // Preserve the just-composed current local/matrix outputs, but continue
    // from the genuine completed clock, action timers and transition inputs.
    install(continuation);
}
} // namespace halo2
