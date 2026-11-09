#pragma once
#include "halo2/native_tu15_object_node_rebuild_kernel.hpp"
#include "physics/runtime/native_tu15_object_node_hierarchy_abi.hpp"
#include "physics/runtime/native_tu15_object_node_admission.hpp"

namespace halo2::native_object_node_rebuild_detail {
using P=NativeTu15ObjectNodeRebuildPhase;
using F=NativeTu15ObjectNodeFrame;
using Data=NativeTu15ObjectNodeRebuild;
using Noop=native_object_node_hierarchy_detail::Noop;
inline bool reference_mode(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_NODE_REBUILD");return p&&*p;}();return value;}
struct Activity {std::uint64_t entries{};};
inline Activity&activity(){thread_local Activity value;return value;}
inline std::uint32_t signed_byte(std::uint32_t x){return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x))));}
inline std::uint32_t signed_half(std::uint32_t x){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));}

// Retained full-controller transport. Pending arguments belong to real
// caller storage: later logical slots must be acquired after those writes.
template<class State,class Before=Noop>struct Access {
    State &s;Before before{};unsigned pending{};
    std::uint32_t site{},return_pc{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    void push(std::uint32_t pc,std::uint32_t value){at(pc);s.push(pc,value);pending+=4u;}
    bool backwards()const{return (s.other_flags&1024u)!=0u;}
    std::uint32_t logical(F slot,unsigned offset=0)const{return s.gp[4]+pending+std::uint32_t(slot)+offset;}
    void call_at(std::uint32_t pc,std::uint32_t next){site=pc;return_pc=next;}
    std::uint32_t load(std::uint32_t pc,std::uint32_t address,unsigned width){at(pc);return physics_step_detail::r(s.memory,address,width);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){at(pc);physics_step_detail::w(s.memory,s.observe,pc,address,width,value);}
    std::uint32_t frame_address(P phase,const Data&,F slot){
        auto &g=s.gp;
        if(phase==P::physical_mask){push(0xbf1d0u,g[2]);g[0]=logical(slot);return g[0];}
        if(phase==P::prediction_position){g[6]=logical(slot);return g[6];}
        if(phase==P::prediction_up){g[6]=logical(slot);return g[6];}
        if(phase==P::prediction_forward){g[7]=logical(slot);return g[7];}
        if(phase==P::root_output){g[1]=logical(slot);return g[1];}
        if(phase==P::root_local_output){push(0xbf505u,g[0]);g[1]=logical(slot);return g[1];}
        return logical(slot);
    }
    std::uint32_t read(P phase,const Data&,std::uint32_t address,unsigned width){
        auto &g=s.gp;std::uint32_t pc{};
        switch(phase){
        case P::tag_array:g[0]=s.flags.logic(g[0]&0xffffu,4);pc=0xbf0d6u;break;
        case P::object_pool:g[6]=g[0]*3u;pc=0xbf0dfu;break;
        case P::object_rows:pc=0xbf0e4u;break;
        case P::body:g[6]=s.flags.shift(g[6],2u,4);pc=0xbf0eau;break;
        case P::definition_tag:pc=0xbf0eeu;break;
        case P::definition:g[2]=s.flags.logic(g[2]&0xffffu,4);g[2]=s.flags.shift(g[2],4u,4);pc=0xbf0f9u;break;
        case P::matrix_bytes:pc=0xbf0fdu;break;
        case P::matrix_offset:{g[0]=0x4ec4ec4fu;const auto value=std::uint64_t(g[0])*g[2];g[0]=std::uint32_t(value);g[2]=std::uint32_t(value>>32u);s.flags.cf=s.flags.of=g[2]!=0u;pc=0xbf10bu;break;}
        case P::body_flags:pc=0xbf118u;break;
        case P::definition_type:pc=0xbf123u;break;
        case P::subtype_pool:pc=0xbf13eu;break;
        case P::subtype_rows:pc=0xbf144u;break;
        case P::subtype_body:pc=0xbf147u;break;
        case P::subtype:pc=0xbf14bu;break;
        case P::physical_handle:pc=0xbf175u;break;
        case P::physical_enabled:pc=0xbf180u;break;
        case P::physical_type:pc=0xbf192u;break;
        case P::physical_pool:pc=0xbf194u;break;
        case P::physical_rows:pc=0xbf19au;break;
        case P::tag_array_after_physical:pc=0xbf1dcu;break;
        case P::live_pool:pc=0xbf1e2u;break;
        case P::live_rows:pc=0xbf1e8u;break;
        case P::live_body:pc=0xbf1ebu;break;
        case P::live_orientations:pc=0xbf1efu;break;
        case P::orientation_offset:pc=0xbf1fdu;break;
        case P::hlmt_datum:pc=0xbf21du;break;
        case P::hlmt:g[0]=s.flags.logic(g[0]&0xffffu,4);g[0]=s.flags.shift(g[0],4u,4);pc=0xbf231u;break;
        case P::model_datum:pc=0xbf239u;break;
        case P::parent:pc=0xbf245u;break;
        case P::model:g[0]=s.flags.logic(g[0]&0xffffu,4);g[0]=s.flags.shift(g[0],4u,4);s.flags.minus(g[6],0xffffffffu,4);pc=0xbf253u;break;
        case P::parent_node:pc=0xbf26fu;break;
        case P::parent_body:g[2]=g[6]*3u;pc=0xbf289u;break;
        case P::parent_flags:pc=0xbf28du;break;
        case P::live_flags:pc=0xbf29du;break;
        case P::animation_type:pc=0xbf2b0u;break;
        case P::animation_subtype:pc=0xbf2bdu;break;
        case P::manager_offset:pc=0xbf2fbu;break;
        case P::manager_selection:pc=0xbf302u;break;
        case P::mask_initialized:pc=0xbf311u;break;
        case P::mask_initialize:pc=0xbf31au;break;
        case P::mask_copy_read:pc=0xbf351u;break;
        case P::selection_live:pc=0xbf39au;break;
        case P::animation_flags:pc=0xbf3b2u;break;
        case P::model_count:pc=0xbf3c1u;break;
        case P::identity_source:pc=0xbf3c8u;break;
        case P::identity_read:pc=0xbf3d7u;break;
        case P::position_x:g[1]=g[3]+0x64u;pc=0xbf403u;break;
        case P::position_y:pc=0xbf405u;break;
        case P::position_z:pc=0xbf408u;break;
        case P::forward_x:g[2]=g[3]+0x70u;pc=0xbf416u;break;
        case P::forward_y:pc=0xbf420u;break;
        case P::forward_z:pc=0xbf423u;break;
        case P::up_x:pc=0xbf42du;break;
        case P::up_y:pc=0xbf433u;break;
        case P::up_z:pc=0xbf436u;break;
        case P::network_entity:pc=0xbf43du;break;
        case P::network_enabled:pc=0xbf44eu;break;
        case P::root_flags:pc=0xbf4cbu;break;
        case P::root_scale:pc=0xbf4dau;break;
        case P::no_model_parent:pc=0xbf5c3u;break;
        case P::no_model_rows:g[0]=g[6];g[0]=s.flags.logic(g[0]&0xffffu,4);g[2]=g[0]*3u;pc=0xbf5d9u;break;
        case P::no_model_parent_body:pc=0xbf5dcu;break;
        case P::no_model_node:pc=0xbf5e0u;break;
        case P::no_model_parent_offset:pc=0xbf5e4u;break;
        case P::no_model_parent_flags:g[2]=s.flags.product(g[2],0x34u);g[2]=s.flags.plus(g[2],g[1],4);g[0]=s.flags.plus(g[0],g[2],4);pc=0xbf5f2u;break;
        case P::no_model_scale:pc=0xbf600u;break;
        case P::no_model_flags:pc=0xbf607u;break;
        default:throw std::runtime_error("Invalid native BF0C0 field read phase");
        }
        const auto value=load(pc,address,width);
        switch(phase){
        case P::tag_array:case P::tag_array_after_physical:g[5]=value;break;
        case P::object_pool:case P::subtype_body:case P::hlmt:case P::no_model_rows:case P::root_flags:g[0]=value;break;
        case P::object_rows:case P::definition:case P::subtype_pool:case P::live_pool:g[1]=value;break;
        case P::body:g[3]=value;break;
        case P::definition_tag:case P::subtype_rows:case P::physical_pool:case P::live_body:g[2]=value;break;
        case P::matrix_bytes:g[2]=signed_half(value);break;
        case P::matrix_offset:case P::orientation_offset:g[0]=signed_half(value);break;
        case P::body_flags:s.flags.logic(value&0xbfffffffu,4);break;
        case P::definition_type:case P::physical_type:s.set(1,value,1,0);break;
        case P::subtype:s.flags.minus(value,2u,1);break;
        case P::physical_handle:g[0]=value;s.flags.minus(g[0],0xffffffffu,4);break;
        case P::physical_enabled:s.set(1,value,1,0);s.set(1,s.flags.right(s.reg(1,1,0),6u,1,false),1,0);s.flags.logic(s.reg(1,1,0)&1u,1);break;
        case P::physical_rows:
            g[5]=value;g[0]=s.flags.logic(g[0]&0xffffu,4);g[7]=g[0]*5u;g[0]=1u;g[0]=s.flags.shift(g[0],s.reg(1,1,0),4);
            g[7]=s.flags.shift(g[7],5u,4);g[7]=s.flags.plus(g[7],g[5],4);s.flags.logic(s.reg(0,1,0)&1u,1);break;
        case P::live_rows:g[7]=value;break;
        case P::live_orientations:s.flags.minus(value,0xffffu,2);break;
        case P::hlmt_datum:case P::model_datum:g[0]=value;s.flags.minus(value,0xffffffffu,4);break;
        case P::parent:g[6]=value;break;
        case P::model:g[5]=value;break;
        case P::parent_node:s.set(1,signed_byte(value),2,0);break;
        case P::parent_body:g[0]=value;break;
        case P::parent_flags:g[0]=value;break;
        case P::live_flags:g[1]=value;g[1]=s.flags.right(g[1],29u,4,false);s.flags.logic(s.reg(1,1,0)&1u,1);break;
        case P::animation_type:s.set(1,value,1,0);g[0]=1u;g[0]=s.flags.shift(g[0],s.reg(1,1,0),4);s.flags.logic(s.reg(0,1,0)&1u,1);break;
        case P::animation_subtype:s.set(0,value,1,0);s.flags.logic(s.reg(0,1,0),1);break;
        case P::manager_offset:g[5]=signed_half(value);break;
        case P::manager_selection:g[0]=value;g[5]=s.flags.plus(g[5],g[3],4);s.flags.minus(g[0],0xffffffffu,4);break;
        case P::mask_initialized:s.flags.logic(value&1u,1);break;
        case P::mask_initialize:s.flags.logic(value|1u,4);break;
        case P::selection_live:s.flags.minus(value,0xffffffffu,4);break;
        case P::animation_flags:s.set(0,value,1,0);s.set(0,s.flags.right(s.reg(0,1,0),1u,1,false),1,0);s.set(0,s.flags.logic(s.reg(0,1,0)&1u,1),1,0);break;
        case P::model_count:g[0]=value;s.flags.logic(g[0],4);break;
        case P::identity_source:g[6]=value;break;
        case P::position_x:g[2]=value;break;
        case P::position_y:case P::forward_x:case P::up_z:g[0]=value;break;
        case P::position_z:case P::forward_y:case P::up_x:g[1]=value;break;
        case P::forward_z:case P::up_y:g[2]=value;break;
        case P::network_entity:s.flags.minus(value,0xffffffffu,4);break;
        case P::network_enabled:s.flags.logic(value&1u,1);break;
        case P::root_scale:case P::no_model_scale:g[1]=value;break;
        case P::no_model_parent:g[6]=value;g[0]=s.flags.logic(0u,4);s.set(2,s.flags.logic(0u,1),1,0);s.flags.minus(g[6],0xffffffffu,4);break;
        case P::no_model_parent_body:g[1]=value;break;
        case P::no_model_node:g[2]=signed_byte(value);break;
        case P::no_model_parent_offset:g[0]=signed_half(value);break;
        case P::no_model_parent_flags:g[2]=s.flags.right(value,10u,4,false);s.set(2,s.flags.logic(s.reg(2,1,0)&1u,1),1,0);break;
        case P::no_model_flags:g[0]=value;break;
        default:break;
        }
        return value;
    }
    void write(P phase,const Data&,std::uint32_t address,std::uint32_t value,unsigned width){
        std::uint32_t pc{};
        switch(phase){
        case P::body_flags_publish:pc=0xbf118u;break;
        case P::mask_clear:pc=0xbf16cu;break;
        case P::mask_initialize:pc=0xbf31au;break;
        case P::mask_copy_write:pc=0xbf351u;break;
        case P::identity_write:pc=0xbf3d7u;break;
        default:throw std::runtime_error("Invalid native BF0C0 field publication phase");
        }
        store(pc,address,value,width);
    }
    std::uint32_t frame_read(P phase,const Data&,F slot,unsigned offset,unsigned width){
        auto &g=s.gp;std::uint32_t pc{};
        switch(phase){
        case P::object:pc=0xbf0cau;break;
        case P::physical_definition:pc=0xbf18eu;break;
        case P::height_object:pc=0xbf1b5u;break;
        case P::height_input:pc=0xbf1ccu;break;
        case P::model_definition:pc=0xbf219u;break;
        case P::parent_live_body:pc=0xbf290u;break;
        case P::animation_definition:pc=0xbf2acu;break;
        case P::bind_orientations:pc=0xbf2cbu;break;
        case P::animation_object:pc=0xbf2d6u;break;
        case P::local_owned:pc=0xbf321u;break;
        case P::hlmt_reload:pc=0xbf32bu;break;
        case P::mask_count:pc=0xbf358u;break;
        case P::animation_orientations:pc=0xbf36bu;break;
        case P::animation_model:pc=0xbf36fu;break;
        case P::animation_handle:pc=0xbf373u;break;
        case P::animation_count:pc=0xbf37bu;break;
        case P::model_reload:pc=0xbf3bdu;break;
        case P::identity_destination:pc=0xbf3ceu;break;
        case P::root_mask:pc=0xbf3d9u;break;
        case P::world_relative:pc=0xbf3e4u;break;
        case P::world_orientation:pc=0xbf3ecu;break;
        case P::world_matrix:pc=0xbf3f0u;break;
        case P::prediction_object:pc=0xbf457u;break;
        case P::position_scratch_x:pc=0xbf46bu;break;
        case P::position_scratch_y:pc=0xbf46fu;break;
        case P::position_scratch_z:pc=0xbf473u;break;
        case P::prediction_axes_object:pc=0xbf483u;break;
        case P::axes_forward_x:pc=0xbf49bu;break;
        case P::axes_forward_y:pc=0xbf49fu;break;
        case P::axes_forward_z:pc=0xbf4a3u;break;
        case P::axes_up_x:pc=0xbf4abu;break;
        case P::axes_up_y:pc=0xbf4b3u;break;
        case P::axes_up_z:pc=0xbf4bbu;break;
        case P::root_parent:pc=0xbf4ceu;break;
        case P::root_parent_mirror:pc=0xbf4e0u;break;
        case P::root_orientation:pc=0xbf501u;break;
        case P::root_product_output:pc=0xbf513u;break;
        case P::notify_matrix:pc=0xbf52bu;break;
        case P::notify_object:pc=0xbf52fu;break;
        case P::no_model_output:pc=0xbf5fbu;break;
        default:throw std::runtime_error("Invalid native BF0C0 caller acquisition phase");
        }
        const auto value=load(pc,logical(slot,offset),width);
        switch(phase){
        case P::object:case P::height_object:case P::model_definition:case P::animation_orientations:case P::mask_count:case P::animation_count:
        case P::world_matrix:case P::position_scratch_z:case P::axes_forward_z:case P::axes_up_z:case P::root_orientation:case P::notify_matrix:g[0]=value;break;
        case P::physical_definition:case P::animation_definition:case P::animation_model:case P::prediction_object:case P::prediction_axes_object:
        case P::position_scratch_x:case P::axes_forward_x:case P::axes_up_x:case P::no_model_output:g[1]=value;break;
        case P::height_input:case P::parent_live_body:case P::bind_orientations:case P::animation_handle:case P::world_orientation:
        case P::position_scratch_y:case P::axes_forward_y:case P::axes_up_y:case P::root_parent:case P::root_product_output:g[2]=value;break;
        case P::animation_object:g[6]=value;break;
        case P::local_owned:s.set(0,value,1,0);g[6]=s.flags.logic(0u,4);s.flags.logic(s.reg(0,1,0),1);break;
        case P::hlmt_reload:g[7]=value;break;
        case P::model_reload:g[5]=value;break;
        case P::identity_destination:g[7]=value;break;
        case P::root_mask:s.flags.logic(value&1u,1);break;
        case P::world_relative:s.set(0,value,1,0);s.flags.logic(s.reg(0,1,0),1);break;
        case P::root_parent_mirror:s.set(3,value,1,0);break;
        case P::notify_object:g[3]=value;break;
        default:break;
        }
        return value;
    }
    void frame_write(P phase,const Data&,F slot,std::uint32_t value,unsigned offset,unsigned width){
        auto &g=s.gp;std::uint32_t pc{};
        switch(phase){
        case P::matrices_publish:g[0]=s.flags.plus(g[0],g[3],4);pc=0xbf114u;break;
        case P::definition_publish:pc=0xbf11fu;break;
        case P::count_publish:g[0]=1u;g[0]=s.flags.shift(g[0],s.reg(1,1,0),4);g[7]=s.flags.right(g[2],4u,4,false);pc=0xbf131u;break;
        case P::local_owned_publish:s.flags.logic(s.reg(0,1,0)&1u,1);pc=0xbf137u;break;
        case P::height_publish:pc=0xbf1c6u;break;
        case P::body_publish:pc=0xbf1f7u;break;
        case P::orientations_publish:g[0]=s.flags.plus(g[0],g[3],4);pc=0xbf206u;break;
        case P::orientations_default:pc=0xbf20cu;break;
        case P::local_unowned_publish:pc=0xbf214u;break;
        case P::hlmt_publish:pc=0xbf235u;break;
        case P::model_publish:pc=0xbf257u;break;
        case P::parent_reset:pc=0xbf25bu;break;
        case P::parent_mirror_reset:pc=0xbf263u;break;
        case P::world_reset:pc=0xbf268u;break;
        case P::parent_publish:g[6]=s.flags.logic(g[6]&0xffffu,4);pc=0xbf282u;break;
        case P::parent_mirror_publish:g[0]=s.flags.right(g[0],10u,4,false);s.set(0,s.flags.logic(s.reg(0,1,0)&1u,1),1,0);pc=0xbf299u;break;
        case P::world_publish:pc=0xbf3b9u;break;
        case P::position_x_publish:pc=0xbf40bu;break;
        case P::position_y_publish:pc=0xbf40fu;break;
        case P::forward_x_publish:pc=0xbf418u;break;
        case P::position_z_publish:pc=0xbf41cu;break;
        case P::forward_y_publish:g[0]=g[3]+0x7cu;pc=0xbf429u;break;
        case P::forward_z_publish:pc=0xbf42fu;break;
        case P::up_z_publish:pc=0xbf439u;break;
        case P::up_x_publish:pc=0xbf444u;break;
        case P::up_y_publish:pc=0xbf448u;break;
        case P::position_override_x:pc=0xbf477u;break;
        case P::position_override_y:pc=0xbf47bu;break;
        case P::position_override_z:pc=0xbf47fu;break;
        case P::forward_override_x:pc=0xbf4a7u;break;
        case P::forward_override_y:pc=0xbf4afu;break;
        case P::forward_override_z:pc=0xbf4b7u;break;
        case P::up_override_x:pc=0xbf4bfu;break;
        case P::up_override_y:pc=0xbf4c3u;break;
        case P::up_override_z:pc=0xbf4c7u;break;
        default:throw std::runtime_error("Invalid native BF0C0 caller publication phase");
        }
        store(pc,logical(slot,offset),value,width);
    }
    void prepare(P phase,const Data&){
        auto &g=s.gp;
        switch(phase){
        case P::begin:g[4]=s.flags.minus(g[4],0xe8u,4);for(auto reg:{3u,5u,6u,7u}){at(0xbf0c6u+(reg==3?0u:reg==5?1u:reg==6?2u:3u));s.push(0xbf0c6u+(reg==3?0u:reg==5?1u:reg==6?2u:3u),g[reg]);}break;
        case P::mask_begin:g[1]=g[7]+31u;g[1]=s.flags.right(g[1],5u,4,true);g[1]=s.flags.shift(g[1],2u,4);g[2]=g[1];g[1]=s.flags.right(g[1],2u,4,false);g[0]=s.flags.logic(0u,4);g[7]=logical(F::mask);break;
        case P::mask_advance:g[7]+=backwards()?0xfffffffcu:4u;--g[1];break;
        case P::mask_end:g[1]=s.flags.logic(g[2]&3u,4);break;
        case P::height_call:call_at(0xbf1bcu,0xbf1c1u);break;
        case P::height_zero:s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);break;
        case P::physical_call:push(0xbf1d5u,g[0]);push(0xbf1d6u,g[7]);call_at(0xbf1d7u,0xbf1dcu);break;
        case P::parent_call:g[0]=g[6];push(0xbf276u,g[1]);call_at(0xbf277u,0xbf27cu);break;
        case P::bind_call:push(0xbf2cfu,g[2]);push(0xbf2d0u,g[5]);call_at(0xbf2d1u,0xbf2d6u);break;
        case P::has_animation_call:g[0]=g[6];call_at(0xbf2dfu,0xbf2e4u);break;
        case P::frozen_call:g[1]=g[6];call_at(0xbf2eeu,0xbf2f3u);break;
        case P::mask_empty_call:g[7]=s.flags.plus(g[7],0xa4u,4);g[1]=g[7];call_at(0xbf337u,0xbf33cu);break;
        case P::mask_copy_begin:g[6]=g[7];g[1]=8u;g[7]=g[0]=0x56eb8cu;break;
        case P::copy_advance:g[6]+=backwards()?0xfffffffcu:4u;g[7]+=backwards()?0xfffffffcu:4u;--g[1];break;
        case P::invert_mask_call:call_at(0xbf353u,0xbf358u);break;
        case P::trim_mask_call:g[6]=0x56eb8cu;call_at(0xbf361u,0xbf366u);break;
        case P::animation_call:push(0xbf37au,g[0]);call_at(0xbf383u,0xbf388u);break;
        case P::render_mask_call:s.flags.logic(g[6],4);s.set(2,!s.flags.zf,1,0);g[1]=30u;g[0]=g[3]+4u;call_at(0xbf395u,0xbf39au);break;
        case P::selection_present_call:g[0]=g[5];call_at(0xbf3a2u,0xbf3a7u);break;
        case P::selection_call:g[1]=g[5];call_at(0xbf3adu,0xbf3b2u);break;
        case P::identity_begin:g[1]=8u;break;
        case P::world_local_call:push(0xbf3f4u,g[2]);push(0xbf3f5u,g[0]);call_at(0xbf3f6u,0xbf3fbu);break;
        case P::prediction_position_call:call_at(0xbf462u,0xbf467u);break;
        case P::prediction_axes_call:call_at(0xbf492u,0xbf497u);break;
        case P::root_begin:push(0xbf4d9u,g[1]);break;
        case P::root_arguments:
            push(0xbf4e4u,g[2]);g[0]=s.flags.right(g[0],10u,4,false);g[0]=s.flags.logic(g[0]&0xffffff01u,4);push(0xbf4edu,g[0]);push(0xbf4eeu,g[1]);
            g[2]=logical(F::position);push(0xbf4f3u,g[2]);g[0]=logical(F::up);g[1]=logical(F::forward);break;
        case P::root_call:call_at(0xbf4fcu,0xbf501u);break;
        case P::root_local_call:push(0xbf50du,g[1]);call_at(0xbf50eu,0xbf513u);break;
        case P::root_product_call:push(0xbf517u,g[2]);g[2]=logical(F::local_matrix);g[1]=logical(F::root_matrix);call_at(0xbf526u,0xbf52bu);break;
        case P::notify_call:push(0xbf536u,g[0]);call_at(0xbf537u,0xbf53cu);break;
        case P::no_model_begin:push(0xbf5ffu,g[1]);break;
        case P::no_model_parent_argument:push(0xbf606u,g[0]);break;
        case P::no_model_arguments:
            g[0]=s.flags.right(g[0],10u,4,false);g[0]=s.flags.logic(g[0]&0xffffff01u,4);push(0xbf612u,g[0]);push(0xbf613u,g[1]);
            g[0]=g[3]+0x7cu;g[1]=g[3]+0x70u;g[3]=s.flags.plus(g[3],0x64u,4);push(0xbf61du,g[3]);s.set(3,s.reg(2,1,0),1,0);call_at(0xbf620u,0xbf625u);break;
        default:break;
        }
    }
    void complete(P phase,const Data&){
        if(phase!=P::finish_common)throw std::runtime_error("Invalid native BF0C0 completion phase");
        auto &g=s.gp;at(0xbf625u);g[7]=s.pop();at(0xbf626u);g[6]=s.pop();at(0xbf627u);g[5]=s.pop();at(0xbf628u);g[3]=s.pop();
        g[4]=s.flags.plus(g[4],0xe8u,4);at(0xbf62fu);s.pop();g[4]+=4u;
    }
};

template<class State,class Call,class Before=Noop>struct Services {
    State &s;Call &call;Access<State,Before> &access;
    void invoke(std::uint32_t entry){
        access.push(access.site,access.return_pc);
        access.before(s,entry);call(s,entry,access.site);access.pending=0;
    }
    void model_resume(Data &d){d.body=s.gp[3];d.model=s.gp[5];}
    void manager_resume(Data &d){d.body=s.gp[3];d.manager=s.gp[5];}
    bool boolean_result(){const auto value=s.reg(0,1,0);s.flags.logic(value,1);return value!=0u;}
    std::uint32_t height(Data &d){
        invoke(0xe3bc0u);d.body=s.gp[3];d.object_offset=s.gp[6];d.component=s.gp[7];
        return native_tu15_leaves::low_bits(s.xmm[0]);
    }
    void deliver_physics(Data &d){invoke(0x1dbbd0u);d.body=s.gp[3];d.object_offset=s.gp[6];}
    std::uint32_t parent_matrix(Data &d){invoke(0xbac00u);model_resume(d);d.parent=s.gp[6];d.object_rows=s.gp[7];return s.gp[0];}
    void bind_pose(Data &d){invoke(0x1707a0u);model_resume(d);}
    bool has_animation(Data &d){invoke(0xc15d0u);model_resume(d);d.object=s.gp[6];return boolean_result();}
    bool animation_frozen(Data &d){invoke(0xc1cc0u);model_resume(d);d.object=s.gp[6];return boolean_result();}
    bool mask_empty(Data &d){invoke(0xc1f40u);manager_resume(d);d.mask_source=s.gp[7];d.blend_mask=s.gp[6];return boolean_result();}
    void invert_mask(Data &d){invoke(0xc1eb0u);manager_resume(d);}
    void trim_mask(Data &d){invoke(0xc1f00u);manager_resume(d);s.gp[6]=0x56eb8cu;d.blend_mask=s.gp[6];}
    void animate(Data &d){
        access.push(0xbf37fu,s.gp[5]);access.push(0xbf380u,s.gp[1]);s.gp[1]=s.gp[6];
        invoke(0xbfb90u);manager_resume(d);d.blend_mask=s.gp[6];
    }
    void publish_render_mask(Data &d){invoke(0xc1e50u);manager_resume(d);d.blend_mask=s.gp[6];}
    bool selection_present(Data &d){invoke(0xb87b0u);manager_resume(d);return boolean_result();}
    std::uint32_t selected_animation(Data &d){invoke(0xb8780u);manager_resume(d);return s.gp[0];}
    bool predict_position(Data &d){invoke(0xb3ae0u);model_resume(d);return boolean_result();}
    bool predict_axes(Data &d){invoke(0xb3b70u);model_resume(d);return boolean_result();}
    void root_transform(Data &d){invoke(0xbfc70u);d.model=s.gp[5];}
    void local_transform(Data &d){invoke(0x144cf0u);d.model=s.gp[5];}
    void root_product(Data &d){invoke(0x145560u);d.model=s.gp[5];}
    void notify_type(Data &d){invoke(0x10e180u);d.model=s.gp[5];}
};

// Internal entry shared by the admitted default and independent validation.
// Call transports the real owner at (entry,callsite), preserving its outcomes.
template<class State,class Call,class Before=Noop>
void run_private(State &s,Call call,Before before={}){
    Access<State,Before> access{s,before};Services<State,Call,Before> services{s,call,access};Data data;
    native_tu15_rebuild_object_nodes(access,data,services,[&](Data&){
        native_object_node_hierarchy_detail::run_private_stage(s,[&](auto &current,std::uint32_t target){
            before(current,target);call(current,target,target==0x144cf0u?0xbf589u:0xbf59cu);
        },before);
    });
}
template<class State,class Call>bool bridge(State&s,Call call){
    if(reference_mode()||!native_object_node_admission_detail::admitted(s,{{0xbf0c0u,0xbf62fu}}))return false;
    ++activity().entries;run_private(s,call);return true;
}
} // namespace halo2::native_object_node_rebuild_detail
