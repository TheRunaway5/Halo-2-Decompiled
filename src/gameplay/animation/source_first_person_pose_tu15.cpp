#include "halo2/source_first_person_pose_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_weapon_animation_query_symbols_tu15.hpp"
#include "halo2/source_sandbox_special_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_animation_composition_tu15.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include "halo2/source_vehicle_director_tu15.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_map_world_support_tu15.hpp"
#include <bit>
#include <bitset>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <sstream>

namespace halo2 {
namespace {
// An absent byte always comes from the declared input owner. A private page
// does not make its unwritten bytes readable or manufacture zero inputs.
class PoseMemory final:public SourcePhysicsContactMemory {
    static constexpr std::uint32_t page_bytes=4096;
    struct Page {std::array<std::byte,page_bytes> bytes;std::bitset<page_bytes> written;};
    const SourcePhysicsContactMemory &source_;
    const SourceFirstPersonPoseReadObserverTu15 &observe_;
    std::map<std::uint32_t,Page> pages_;
    static void extent(std::uint32_t at,std::size_t bytes) {
        if(bytes>std::uint64_t(std::numeric_limits<std::uint32_t>::max())+1u-at)
            throw FormatError("Native FP pose operand wraps its input address space");
    }
public:
    PoseMemory(const SourcePhysicsContactMemory &source,
        const SourceFirstPersonPoseReadObserverTu15 &observe):source_(source),observe_(observe){}
    void read(std::uint32_t at,std::span<std::byte> bytes)const override {
        extent(at,bytes.size());
        std::size_t n{};
        while(n<bytes.size()) {
            const auto address=at+std::uint32_t(n),page=address/page_bytes;
            const auto offset=address%page_bytes;
            const auto count=std::min<std::size_t>(bytes.size()-n,page_bytes-offset);
            const auto found=pages_.find(page);
            if(found==pages_.end()) {
                source_.read(address,bytes.subspan(n,count));
                if(observe_)observe_(address,count);
                n+=count;continue;
            }
            const auto &owned=found->second;
            std::size_t done{};
            while(done<count) {
                if(owned.written[offset+done]) {
                    bytes[n+done]=owned.bytes[offset+done];++done;continue;
                }
                const auto begin=done++;
                while(done<count && !owned.written[offset+done])++done;
                source_.read(address+std::uint32_t(begin),bytes.subspan(n+begin,done-begin));
                if(observe_)observe_(address+std::uint32_t(begin),done-begin);
            }
            n+=count;
        }
    }
    void write(std::uint32_t at,std::span<const std::byte> bytes)override {
        extent(at,bytes.size());
        std::size_t n{};
        while(n<bytes.size()) {
            const auto address=at+std::uint32_t(n),offset=address%page_bytes;
            const auto count=std::min<std::size_t>(bytes.size()-n,page_bytes-offset);
            auto &owned=pages_[address/page_bytes];
            std::memcpy(owned.bytes.data()+offset,bytes.data()+n,count);
            for(std::size_t k=0;k<count;++k)owned.written.set(offset+k);
            n+=count;
        }
    }
    std::uint32_t word(std::uint32_t at)const {
        std::array<std::byte,4> bytes;read(at,bytes);return Bytes(bytes).u32(0);
    }
    void put(std::uint32_t at,std::uint32_t value) {
        write(at,std::as_bytes(std::span(&value,1)));
    }
};
}

std::vector<SourceFirstPersonModelTu15> source_cached_first_person_models_tu15(
    const Cache &cache,const SourcePhysicsContactMemory &memory,std::uint32_t expected_unit) {
    const auto word=[&](std::uint32_t at) {
        std::array<std::byte,4> bytes;memory.read(at,bytes);return Bytes(bytes).u32(0);
    };
    const auto count=std::bit_cast<std::int32_t>(word(0x4ce3f4u));
    if(count<0 || count>4)throw FormatError("Native FP cached group escaped its original four-record capacity");
    if(count && word(0x4ce3f0u)!=expected_unit)
        throw FormatError("Native FP cached group belongs to another unit");
    std::vector<SourceFirstPersonModelTu15> result;result.reserve(unsigned(count));
    for(unsigned n=0;n<unsigned(count);++n) {
        const auto at=0x4ce3f8u+n*0xd0cu,model=word(at);
        if(model==0xffffffffu || !cache.find_tag(TagHandle{model},TagGroup::from_string("mode")))
            throw FormatError("Native FP cached group returned a model outside its actual Cache");
        const auto metadata=word(word(0x4f0414u)+(model&65535u)*16u+8u);
        const auto nodes=word(metadata+0x48u);
        if(!nodes || nodes>64)throw FormatError("Native FP cached group escaped its source node capacity");
        SourceFirstPersonModelTu15 row{model,word(at+4u),word(at+8u),{}};
        if(row.mode>1)throw FormatError("Native FP cached group has an unknown rendering mode");
        row.nodes.resize(nodes);memory.read(at+0xcu,std::as_writable_bytes(std::span(row.nodes)));
        result.push_back(std::move(row));
    }
    return result;
}

std::vector<SourceFirstPersonModelTu15> source_sample_first_person_pose_tu15(
    const Cache &cache,const SourcePhysicsContactMemory &source,
    const SourcePhysicsWorldRuntimeAbi &caller,unsigned view,
    Vec3 position,Vec3 forward,Vec3 up,const SourcePhysicsWorldRuntimeServices &cpu,
    const SourceFirstPersonPoseReadObserverTu15 &observe) {
    constexpr SourcePhysicsFactoryFloatingPoint expected_controls{0x27f,0x1f80};
    if(view>=4 || !caller.gp[4] || caller.gp[4]>0xffffffebu)
        throw FormatError("Native FP pose requires its actual source view and caller frame");
    // MXCSR's low six bits are incoming sticky exception status, not
    // rounding/mask controls. Preserve every supplied source status bit.
    if(!caller.floating_point || caller.floating_point->x87_control_word!=expected_controls.x87_control_word ||
        (caller.floating_point->mxcsr&~0x3fu)!=expected_controls.mxcsr)
        throw FormatError("Native FP pose caller differs from the live native floating-point ABI");
    const auto floating_point=*caller.floating_point;
    for(const auto v:{position,forward,up})for(const auto component:{v.x,v.y,v.z})
        if(!std::isfinite(component))throw FormatError("Native FP pose camera input is not finite");
    PoseMemory memory(source,observe);
    SourceWeaponAnimationQuerySymbolsTu15 symbols(memory,cache.is_mcc(),
        [&](auto key){return cache.string_id(key);});
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=cpu.instruction;
    services.reciprocal_sqrt=cpu.reciprocal_sqrt;
    services.timestamp_counter=cpu.timestamp_counter;
    using Run=void(*)(SourcePhysicsContactMemory &,std::uint32_t,SourcePhysicsWorldRuntimeAbi &,
        SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices &,
        const SourcePhysicsContactRecordObserver &);
    const std::pair<bool(*)(std::uint32_t),Run> owners[]{
        {source_sandbox_special_tu15_owns_entry,source_run_sandbox_special_tu15},
        {source_physics_live_support_tu15_owns_entry,source_run_physics_live_support_tu15},
        {source_animation_composition_tu15_owns_entry,source_run_animation_composition_tu15},
        {source_model_render_tu15_owns_entry,source_run_model_render_tu15},
        {source_scene_device_constructor_tu15_owns_entry,source_run_scene_device_constructor_tu15},
        {source_vehicle_director_tu15_owns_entry,source_run_vehicle_director_tu15},
        {source_campaign_camera_tu15_owns_entry,source_run_campaign_camera_tu15},
        {source_map_world_support_tu15_owns_entry,source_run_map_world_support_tu15}};
    services.required_owner=[&](auto &bank,const auto &call,auto &abi,const auto &stores) {
        for(const auto &[owns,run]:owners)if(owns(call.target)) {
            run(bank,call.target,abi,abi.floating_point.value_or(floating_point),services,stores);return;
        }
        std::ostringstream message;message<<"Native FP pose requires an unowned callee 0x"<<
            std::hex<<call.target<<" at 0x"<<call.instruction;
        throw FormatError(message.str());
    };
    const auto call=[&](std::uint32_t entry,std::initializer_list<std::uint32_t> arguments) {
        auto abi=caller;unsigned n=1;
        for(const auto value:arguments)memory.put(caller.gp[4]+4u*n++,value);
        source_run_sandbox_special_tu15(memory,entry,abi,floating_point,services);
        return abi.gp[0];
    };
    const auto view_at=memory.word(0x4f6498u)+view*0x20ccu;
    const auto unit=memory.word(view_at+4u);
    if(unit==0xffffffffu)return {};
    for(unsigned hand=0;hand<2;++hand)
        if(memory.word(view_at+0x10u+hand*0x1010u)!=0xffffffffu &&
            (memory.word(view_at+0xcu+hand*0x1010u)&1u))
            call(0x16a4ddu,{view,hand});
    memory.put(0x4c66f8u,view);
    for(const auto &[at,value]:std::array<std::pair<std::uint32_t,Vec3>,3>{{
        {0x4c65c0u,position},{0x4c65ccu,forward},{0x4c65d8u,up}}}) {
        memory.put(at,std::bit_cast<std::uint32_t>(value.x));
        memory.put(at+4u,std::bit_cast<std::uint32_t>(value.y));
        memory.put(at+8u,std::bit_cast<std::uint32_t>(value.z));
    }
    constexpr std::uint32_t output=0x60050000u,capacity=4u,stride=0xd0cu;
    const auto count=call(0x168dedu,{view,unit,capacity,output});
    if(count>capacity)throw FormatError("Native FP pose output escaped its source capacity");
    static_assert(sizeof(Transform)==0x34);
    std::vector<SourceFirstPersonModelTu15> models;models.reserve(count);
    for(unsigned n=0;n<count;++n) {
        const auto at=output+n*stride,model=memory.word(at);
        if(model==0xffffffffu || !cache.find_tag(TagHandle{model},TagGroup::from_string("mode")))
            throw FormatError("Native FP pose returned a model outside its actual Cache");
        const auto metadata=memory.word(memory.word(0x4f0414u)+(model&65535u)*16u+8u);
        const auto nodes=memory.word(metadata+0x48u);
        if(!nodes || nodes>64)throw FormatError("Native FP pose model escaped its source node capacity");
        SourceFirstPersonModelTu15 result{model,memory.word(at+4u),memory.word(at+8u),{}};
        result.nodes.resize(nodes);
        memory.read(at+0xcu,std::as_writable_bytes(std::span(result.nodes)));
        models.push_back(std::move(result));
    }
    return models;
}
}
