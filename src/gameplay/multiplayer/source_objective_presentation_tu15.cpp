#include "halo2/source_objective_presentation_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_multiplayer_engines_tu15.hpp"
#include "halo2/source_ctf_activation_tu15.hpp"
#include "halo2/source_ctf_configuration_tu15.hpp"
#include "halo2/source_ctf_activation_spawn_tu15.hpp"
#include "halo2/source_ctf_player_iterator_tu15.hpp"
#include "halo2/source_ctf_flag_position_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_sandbox_special_tu15.hpp"
#include "halo2/source_vehicle_occupancy_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "gameplay/multiplayer/source_objective_output_tu15_internal.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <cstdlib>

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));std::uint32_t out{};
    for(unsigned i=0;i<width;++i)out|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);return out;
}
void put(SourcePhysicsContactMemory& memory,std::uint32_t at,std::uint32_t value) {
    std::array<std::byte,4> bytes;for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8));memory.write(at,bytes);
}
class Caller final:public SourcePhysicsContactMemory {
    const SourceRuntimeMemory& source_;
    std::vector<std::byte> storage_;
    std::uint32_t base_{};
    struct Overlay {std::uint32_t at;std::vector<std::byte> bytes;};
    std::vector<Overlay> overlays_;
public:
    std::uint32_t last_instruction{};
    Caller(const SourceRuntimeMemory& source,std::uint32_t player,std::size_t extent=0x10000):source_(source),storage_(extent) {
        const auto base=source.find_unowned_range(0x70000000u,0x7f000000u,storage_.size(),0x10000u);
        if(!base)throw std::runtime_error("Objective presentation has no private caller range");base_=*base;
        for(unsigned i=0;i<4;++i)put(*this,base_+0x200cu+i*4u,i==0?player:0xffffffffu);
    }
    std::uint32_t base()const{return base_;}
    // The native font resource boundary supplies authenticated logical metrics.
    // Legacy disk cache handles/readiness are neither published nor fabricated.
    std::array<std::uint32_t,2> bind_fonts() {
        std::array<std::uint32_t,2> headers{};
        for(unsigned i=0;i<2;++i) {
            const auto selector=i+5;const auto metrics=gameplay_font_reference_line_metrics(selector);
            if(!metrics)throw std::runtime_error("Objective waypoint has no authored original font layout");
            headers[i]=base_+0x2200u+i*0x20u;
            write(headers[i]+4,std::as_bytes(std::span(*metrics)));
            overlay(0x4ef1c4u+selector*4u,std::as_bytes(std::span(&selector,1)));
        }
        return headers;
    }
    void overlay(std::uint32_t at,std::span<const std::byte> bytes) {
        overlays_.push_back({at,{bytes.begin(),bytes.end()}});
    }
    void read(std::uint32_t at,std::span<std::byte> bytes)const override {
        for(const auto& o:overlays_)if(at>=o.at && std::uint64_t(at)+bytes.size()<=std::uint64_t(o.at)+o.bytes.size()) {
            std::copy_n(o.bytes.begin()+at-o.at,bytes.size(),bytes.begin());return;
        }
        if(at==0x4f54f0u && bytes.size()==4) {
            const auto value=base_+0x2000u;for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8));return;
        }
        if(at==0x4c66f8u && bytes.size()==4){std::fill(bytes.begin(),bytes.end(),std::byte{});return;}
        if(at>=base_ && std::uint64_t(at)+bytes.size()<=std::uint64_t(base_)+storage_.size()) {
            std::copy_n(storage_.begin()+at-base_,bytes.size(),bytes.begin());return;
        }
        try {source_.read(at,bytes);}
        catch(const std::out_of_range&) {
            std::ostringstream error;error<<"Objective presentation read unavailable at0x"<<std::hex<<at<<" after0x"<<last_instruction;
            throw std::runtime_error(error.str());
        }
    }
    void write(std::uint32_t at,std::span<const std::byte> bytes)override {
        for(auto& o:overlays_)if(at>=o.at && std::uint64_t(at)+bytes.size()<=std::uint64_t(o.at)+o.bytes.size()) {
            std::copy(bytes.begin(),bytes.end(),o.bytes.begin()+at-o.at);return;
        }
        if(at<base_ || std::uint64_t(at)+bytes.size()>std::uint64_t(base_)+storage_.size())
            throw std::runtime_error("Objective presentation attempted a world store");
        std::copy(bytes.begin(),bytes.end(),storage_.begin()+at-base_);
    }
};
}
SourceObjectivePresentationTu15 source_objective_presentation_tu15(
    const SourceRuntimeMemory& source,std::uint32_t player,SourcePhysicsFactoryFloatingPoint fp) {
    Caller memory(source,player);SourceObjectivePresentationTu15 output;
    const auto options=word(memory,0x4f3218u),type=word(memory,options+0x180u);
    if(type!=1 && type!=9)return output;
    const auto object=word(memory,0x56dfa0u+type*4u);
    if(!object || player==0xffffffffu || (player&65535u)>=16)
        throw std::runtime_error("Objective presentation has no selected original player/engine");
    const auto vtable=word(memory,object),stack=memory.base()+0x8000u;
    if(word(memory,vtable+13u*4u)!=0x24aa90u || word(memory,vtable+14u*4u)!=0x24acb0u)
        throw std::runtime_error("Objective presentation registration differs");
    SourcePhysicsWorldRuntimeServices services;
    if(std::getenv("HALO2_OBJECTIVE_TRACE_QUERY"))services.instruction=[&](auto pc){memory.last_instruction=pc;};
    services.required_owner=[&](auto& m,const auto& call,auto& abi,const auto& observer) {
        if(call.target==0x41480u) {
            const auto sp=abi.gp[4];
            if(word(m,sp+12u)!=0x44200000u || word(m,sp+16u)!=0xd7100u || word(m,sp+20u)!=0x24d760u || word(m,sp+24u)!=0x24d900u ||
                word(m,sp+8u)!=0xffffffffu || word(m,sp+4u)!=0 || output.goals.size()>=18)
                throw std::runtime_error("Objective goal output has a different original render contract");
            const auto designator=std::bit_cast<std::int32_t>(word(m,sp+28u));
            const auto bank=word(m,0x52d710u),scenario=word(m,0x4ecc20u);
            const auto index=designator>=0?designator:~designator;
            if(index<0 || index>=9)throw std::runtime_error("Objective goal designator differs");
            const auto marker=std::bit_cast<std::int16_t>(std::uint16_t(word(m,bank+(designator>=0?0x14u:0x26u)+unsigned(index)*2u,2)));
            if(marker<0 || unsigned(marker)>=word(m,scenario+0x118u))throw std::runtime_error("Objective goal has no authored marker");
            const auto position=word(m,scenario+0x11cu)+unsigned(marker)*32u,color=word(m,0x474c70u);
            const auto point=[&](std::uint32_t at){return Vec3{std::bit_cast<float>(word(m,at)),std::bit_cast<float>(word(m,at+4u)),std::bit_cast<float>(word(m,at+8u))};};
            output.goals.push_back({abi.gp[0],designator,point(position),point(color),
                std::bit_cast<float>(word(m,bank+(designator>=0?0x3cu:0xa8u)+unsigned(index)*12u)),
                std::bit_cast<float>(word(m,0x48c7b4u))});
            abi.gp[4]+=36u;return;
        }
#define H2_QUERY_OWNER(name) if(source_##name##_tu15_owns_entry(call.target)){source_run_##name##_tu15(m,call.target,abi,fp,services,observer);return;}
        H2_QUERY_OWNER(multiplayer_engines)
        H2_QUERY_OWNER(ctf_activation)
        H2_QUERY_OWNER(ctf_configuration)
        H2_QUERY_OWNER(ctf_activation_spawn)
        H2_QUERY_OWNER(ctf_player_iterator)
        H2_QUERY_OWNER(ctf_flag_position)
        H2_QUERY_OWNER(physics_live_support)
        H2_QUERY_OWNER(sandbox_special)
        H2_QUERY_OWNER(vehicle_occupancy)
        H2_QUERY_OWNER(combat_action)
        H2_QUERY_OWNER(model_render)
        H2_QUERY_OWNER(scene_device_constructor)
#undef H2_QUERY_OWNER
        std::ostringstream error;error<<"Objective presentation unresolved original helper "<<std::hex<<call.target;
        throw std::runtime_error(error.str());
    };
    source_objective_output_tu15_detail::Scope outputs(output,memory.bind_fonts());
    for(const auto entry:{0x24aa90u,0x24acb0u}) {
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[1]=object;abi.gp[4]=stack;abi.eflags=2;abi.floating_point=fp;
        put(memory,stack,memory.base()+0xf000u);put(memory,stack+4u,entry==0x24aa90u?player:0u);
        source_run_multiplayer_engines_tu15(memory,entry,abi,fp,services);
        if(abi.gp[4]!=stack+8u)throw std::runtime_error("Objective presentation caller return extent differs");
    }
    return output;
}

std::vector<SourceObjectiveQuadTu15> source_objective_waypoint_quads_tu15(
    const SourceRuntimeMemory& source,std::uint32_t player,
    std::span<const SourceObjectiveWaypointTu15> descriptors,const SourceObjectiveViewTu15& view,
    const SourceObjectiveOcclusionTu15& occlusion,SourcePhysicsFactoryFloatingPoint fp) {
    std::size_t extent=0x10000;
    for(const auto& bitmap:view.bitmaps) {
        if(bitmap.frames.size()>256 || extent>0x100000-bitmap.frames.size()*0x74)
            throw std::runtime_error("Objective bitmap caller capacity differs");
        extent+=bitmap.frames.size()*0x74;
    }
    extent=(extent+0xffffu)&~std::size_t(0xffffu);
    Caller memory(source,player,extent);
    std::array<std::byte,0x74> camera{};
    std::memcpy(camera.data(),&view.eye,sizeof(view.eye));
    std::memcpy(camera.data()+0x30,view.outer.data(),8);
    std::memcpy(camera.data()+0x38,view.safe.data(),8);
    memory.overlay(0x4c65c0u,camera);
    memory.overlay(0x4c6634u,std::as_bytes(std::span(&view.projection,1)));
    std::array<std::byte,0x40> inputs{};memory.overlay(0x4f3220u,inputs);
    memory.overlay(0x4f3290u,std::as_bytes(std::span(&view.opacity,1)));
    std::uint32_t rows_address=memory.base()+0x10000u;
    for(const auto& bitmap:view.bitmaps) {
        const auto table=word(source,0x4f0414u),entry=table+(bitmap.datum&65535u)*16u;
        if(word(source,entry+4)!=bitmap.datum)throw std::runtime_error("Objective bitmap lost its full-salt directory owner");
        const auto root=word(source,entry+8);
        if(word(source,root+0x44)!=bitmap.frames.size())throw std::runtime_error("Objective bitmap lost its qualified frame bank");
        const auto row_bytes=bitmap.frames.size()*0x74;
        if(std::uint64_t(rows_address)+row_bytes>std::uint64_t(memory.base())+extent)
            throw std::runtime_error("Objective bitmap view exceeds private caller storage");
        for(unsigned n=0;n<bitmap.frames.size();++n) {
            const auto at=rows_address+n*0x74;
            const std::array<unsigned,4> offsets{4,6,16,18};
            for(unsigned j=0;j<4;++j) {
                const auto value=std::bit_cast<std::uint16_t>(bitmap.frames[n][j]);
                const std::array bytes{std::byte(value),std::byte(value>>8)};memory.write(at+offsets[j],bytes);
            }
        }
        memory.overlay(root+0x48,std::as_bytes(std::span(&rows_address,1)));rows_address+=row_bytes;
    }
    std::vector<SourceObjectiveQuadTu15> output;
    struct Outputs {
        decltype(source_objective_output_tu15_detail::quads) previous;
        std::uint32_t emblem;
        Outputs(std::vector<SourceObjectiveQuadTu15>& q):previous(source_objective_output_tu15_detail::quads),
            emblem(source_objective_output_tu15_detail::emblem_player) {
            source_objective_output_tu15_detail::quads=&q;
            source_objective_output_tu15_detail::emblem_player=0xffffffffu;
        }
        ~Outputs(){source_objective_output_tu15_detail::quads=previous;source_objective_output_tu15_detail::emblem_player=emblem;}
    } outputs(output);
    SourcePhysicsWorldRuntimeServices services;
    if(std::getenv("HALO2_OBJECTIVE_TRACE_QUERY"))services.instruction=[&](auto pc){memory.last_instruction=pc;};
    services.required_owner=[&](auto& m,const auto& call,auto& abi,const auto& observer) {
        if(call.target==0x16cea0u) {
            if(!occlusion)throw std::runtime_error("Objective waypoint requires its actual source collision owner");
            const auto point=[&](std::uint32_t at){return Vec3{std::bit_cast<float>(word(m,at)),std::bit_cast<float>(word(m,at+4)),std::bit_cast<float>(word(m,at+8))};};
            const auto sp=abi.gp[4];
            const bool hit=occlusion(point(abi.gp[0]),point(abi.gp[1]),word(m,sp+4),word(m,sp+8),abi.gp[2]);
            abi.gp[0]=(abi.gp[0]&0xffffff00u)|unsigned(hit);abi.gp[4]+=16;return;
        }
#define H2_QUERY_OWNER(name) if(source_##name##_tu15_owns_entry(call.target)){source_run_##name##_tu15(m,call.target,abi,fp,services,observer);return;}
        H2_QUERY_OWNER(multiplayer_engines)
        H2_QUERY_OWNER(ctf_activation)
        H2_QUERY_OWNER(ctf_configuration)
        H2_QUERY_OWNER(ctf_activation_spawn)
        H2_QUERY_OWNER(ctf_player_iterator)
        H2_QUERY_OWNER(ctf_flag_position)
        H2_QUERY_OWNER(physics_live_support)
        H2_QUERY_OWNER(sandbox_special)
        H2_QUERY_OWNER(vehicle_occupancy)
        H2_QUERY_OWNER(combat_action)
        H2_QUERY_OWNER(model_render)
        H2_QUERY_OWNER(scene_device_constructor)
#undef H2_QUERY_OWNER
        std::ostringstream error;error<<"Objective waypoint unresolved native helper "<<std::hex<<call.target;
        throw std::runtime_error(error.str());
    };
    for(const auto& descriptor:descriptors) {
        const auto stack=memory.base()+0x8000u,input=memory.base()+0x1000u;
        memory.write(input,descriptor.bytes);
        // Friendly-player emblem clearance is a local viewport property.
        // Rebind the original font selector for this view, including replicas.
        if(word(memory,input+0x44)>0 && word(memory,input+0x48)==3) {
            const auto metrics=gameplay_font_reference_line_metrics(view.local_players>1?5u:6u);
            if(!metrics)throw std::runtime_error("Objective local view has no original font layout");
            const float height=float(int((*metrics)[0])+int((*metrics)[1])+int((*metrics)[2]));
            put(memory,input+0x1c,std::bit_cast<std::uint32_t>(height));
        }
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.eflags=2;abi.floating_point=fp;
        put(memory,stack,memory.base()+0xf000u);put(memory,stack+4,input);
        source_run_multiplayer_engines_tu15(memory,0x25c4cfu,abi,fp,services);
        if(abi.gp[4]!=stack+8)throw std::runtime_error("Objective waypoint caller return extent differs");
    }
    return output;
}
} // namespace halo2
