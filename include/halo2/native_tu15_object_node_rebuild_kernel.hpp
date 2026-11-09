#pragma once
#include <array>
#include <bit>
#include <cstdint>

namespace halo2 {
// Logical BF0C0 caller storage. Access acquires each field at its real use
// boundary, accounting for arguments already pushed by the retained transport.
enum class NativeTu15ObjectNodeFrame : std::uint32_t {
    parent_mirror=0x11,local_owned=0x12,world_relative=0x13,model_or_height=0x14,
    orientations=0x18,matrices=0x1c,definition=0x20,count=0x24,parent_matrix=0x28,
    scratch=0x2c,up=0x38,position=0x44,forward=0x50,body=0x5c,hlmt=0x60,
    axes_scratch=0x64,mask=0x70,local_matrix=0x90,root_matrix=0xc4,object=0xfc
};
enum class NativeTu15ObjectNodeRebuildPhase {
    begin,object,tag_array,object_pool,object_rows,body,definition_tag,definition,
    matrix_bytes,matrix_offset,matrices_publish,body_flags,body_flags_publish,
    definition_publish,definition_type,count_publish,local_owned_publish,
    subtype_pool,subtype_rows,subtype_body,subtype,mask_begin,mask_clear,mask_advance,mask_end,
    physical_handle,physical_enabled,physical_definition,physical_type,physical_pool,
    physical_rows,height_object,height_call,height_zero,height_publish,height_input,
    physical_mask,physical_call,tag_array_after_physical,live_pool,live_rows,live_body,
    live_orientations,body_publish,orientation_offset,orientations_publish,
    orientations_default,local_unowned_publish,model_definition,hlmt_datum,hlmt,
    hlmt_publish,model_datum,parent,model,model_publish,parent_reset,parent_mirror_reset,
    world_reset,parent_node,parent_call,parent_publish,parent_body,parent_flags,parent_live_body,
    parent_mirror_publish,live_flags,animation_definition,animation_type,animation_subtype,
    bind_orientations,bind_call,animation_object,has_animation_call,frozen_call,
    manager_offset,manager_selection,mask_initialized,mask_initialize,local_owned,
    hlmt_reload,mask_empty_call,mask_copy_begin,mask_copy_read,mask_copy_write,
    copy_advance,invert_mask_call,mask_count,trim_mask_call,animation_orientations,
    animation_model,animation_handle,animation_count,animation_call,render_mask_call,
    selection_live,selection_present_call,selection_call,animation_flags,world_publish,
    model_reload,model_count,identity_source,identity_destination,identity_begin,
    identity_read,identity_write,root_mask,world_relative,world_orientation,world_matrix,
    world_local_call,position_x,position_y,position_z,position_x_publish,position_y_publish,
    forward_x,forward_x_publish,position_z_publish,forward_y,forward_z,forward_y_publish,
    up_x,forward_z_publish,up_y,up_z,up_z_publish,network_entity,up_x_publish,up_y_publish,
    network_enabled,prediction_object,prediction_position,prediction_position_call,
    position_scratch_x,position_scratch_y,position_scratch_z,position_override_x,
    position_override_y,position_override_z,prediction_axes_object,prediction_up,
    prediction_forward,prediction_axes_call,axes_forward_x,axes_forward_y,axes_forward_z,
    forward_override_x,axes_up_x,forward_override_y,axes_up_y,forward_override_z,
    axes_up_z,up_override_x,up_override_y,up_override_z,root_flags,root_parent,root_output,
    root_begin,root_scale,root_parent_mirror,root_arguments,root_call,root_orientation,
    root_local_output,root_local_call,root_product_output,root_product_call,notify_matrix,
    notify_object,notify_call,no_model_parent,no_model_rows,no_model_parent_body,
    no_model_node,no_model_parent_offset,no_model_parent_flags,no_model_output,
    no_model_begin,no_model_scale,no_model_parent_argument,no_model_flags,
    no_model_arguments,no_model_call,finish_common
};
struct NativeTu15ObjectNodeRebuild {
    std::uint32_t object{},object_offset{},tag_array{},object_pool{},object_rows{},body{},
        definition_tag{},definition{},definition_type{},matrix_bytes{},matrix_offset{},
        matrices{},count{},physical_handle{},physical_pool{},physical_rows{},component{},height{},
        live_body{},local_offset{},orientations{},hlmt_datum{},hlmt{},model_datum{},model{},
        parent{},parent_node{},parent_matrix{},parent_body{},parent_mirror{},flags{},manager{},
        selection{},mask_source{},blend_mask{},mask_count{},mask{},copy_source{},copy_destination{},
        copy_remaining{},copy_word{},model_count{},network_entity{},scale{},mirror_flags{},
        up_pointer{},forward_pointer{},position_pointer{},output{},local_matrix{},root_matrix{};
    std::array<std::uint32_t,3> position{},forward{},up{},temporary{};
    bool biped{},local_owned{},world_relative{};
};

// Complete BF0C0 DATA controller. Services execute genuine owners and update
// the semantic cached fields actually resumed by them. In particular, body,
// model/manager and saved child cursors must not be replaced by entry values.
// The last callable composes the shared native hierarchy stage, including its
// own real epilogue. It is an internal native stage, not a supplied child result.
template<class Access,class Services,class Hierarchy>
void native_tu15_rebuild_object_nodes(Access &memory,NativeTu15ObjectNodeRebuild &s,
    Services &services,Hierarchy publish_hierarchy){
    using P=NativeTu15ObjectNodeRebuildPhase;using F=NativeTu15ObjectNodeFrame;
    const auto read=[&](P phase,std::uint32_t at,unsigned width=4u){return memory.read(phase,s,at,width);};
    const auto put=[&](P phase,std::uint32_t at,std::uint32_t value,unsigned width=4u){memory.write(phase,s,at,value,width);};
    const auto frame=[&](P phase,F slot,unsigned offset=0u,unsigned width=4u){return memory.frame_read(phase,s,slot,offset,width);};
    const auto save=[&](P phase,F slot,std::uint32_t value,unsigned offset=0u,unsigned width=4u){memory.frame_write(phase,s,slot,value,offset,width);};
    const auto address=[&](P phase,F slot){return memory.frame_address(phase,s,slot);};
    const auto sx16=[](std::uint32_t x){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));};
    const auto sx8=[](std::uint32_t x){return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x))));};
    const auto is_biped=[](std::uint32_t type){return (type&31u)==0u;};
    const auto copy_words=[&](P read_phase,P write_phase){
        const auto delta=memory.backwards()?0xfffffffcu:4u;
        while(s.copy_remaining){
            s.copy_word=read(read_phase,s.copy_source);put(write_phase,s.copy_destination,s.copy_word);
            s.copy_source+=delta;s.copy_destination+=delta;--s.copy_remaining;
            memory.prepare(P::copy_advance,s);
        }
    };
    memory.prepare(P::begin,s);
    s.object=frame(P::object,F::object);s.object_offset=(s.object&0xffffu)*12u;
    s.tag_array=read(P::tag_array,0x4f0414u);s.object_pool=read(P::object_pool,0x4ecbd0u);
    s.object_rows=read(P::object_rows,s.object_pool+0x44u);s.body=read(P::body,s.object_rows+s.object_offset+8u);
    s.definition_tag=read(P::definition_tag,s.body);s.definition=read(P::definition,s.tag_array+(s.definition_tag&0xffffu)*16u+8u);
    s.matrix_bytes=sx16(read(P::matrix_bytes,s.body+0x114u,2u));
    s.count=std::uint32_t((std::uint64_t(s.matrix_bytes)*0x4ec4ec4fu)>>36u);
    s.matrix_offset=sx16(read(P::matrix_offset,s.body+0x116u,2u));s.matrices=s.body+s.matrix_offset;
    save(P::matrices_publish,F::matrices,s.matrices);
    s.flags=read(P::body_flags,s.body+4u)&0xbfffffffu;put(P::body_flags_publish,s.body+4u,s.flags);
    save(P::definition_publish,F::definition,s.definition);
    s.definition_type=read(P::definition_type,s.definition,1u);s.biped=is_biped(s.definition_type);
    save(P::count_publish,F::count,s.count);save(P::local_owned_publish,F::local_owned,1u,0u,1u);
    if(s.biped){
        s.object_pool=read(P::subtype_pool,0x4ecbd0u);s.object_rows=read(P::subtype_rows,s.object_pool+0x44u);
        s.live_body=read(P::subtype_body,s.object_rows+s.object_offset+8u);
        if(read(P::subtype,s.live_body+0x34bu,1u)==2u){memory.complete(P::finish_common,s);return;}
    }
    s.mask=address(P::mask_begin,F::mask);s.copy_destination=s.mask;
    s.copy_remaining=(s.count+31u)>>5u;memory.prepare(P::mask_begin,s);
    while(s.copy_remaining){put(P::mask_clear,s.copy_destination,0u);s.copy_destination+=memory.backwards()?0xfffffffcu:4u;--s.copy_remaining;memory.prepare(P::mask_advance,s);}
    memory.prepare(P::mask_end,s);
    s.physical_handle=read(P::physical_handle,s.body+0xb4u);
    if(s.physical_handle!=0xffffffffu&&(read(P::physical_enabled,s.body+0xc0u,1u)&0x40u)){
        s.definition=frame(P::physical_definition,F::definition);
        s.definition_type=read(P::physical_type,s.definition,1u);
        s.physical_pool=read(P::physical_pool,0x52be5cu);s.physical_rows=read(P::physical_rows,s.physical_pool+0x44u);
        s.component=s.physical_rows+(s.physical_handle&0xffffu)*160u;
        if(is_biped(s.definition_type)){s.object=frame(P::height_object,F::object);memory.prepare(P::height_call,s);s.height=services.height(s);}
        else{s.height=0u;memory.prepare(P::height_zero,s);}
        save(P::height_publish,F::model_or_height,s.height);s.height=frame(P::height_input,F::model_or_height);
        s.mask=address(P::physical_mask,F::mask);memory.prepare(P::physical_call,s);services.deliver_physics(s);
        s.tag_array=read(P::tag_array_after_physical,0x4f0414u);
    }
    s.object_pool=read(P::live_pool,0x4ecbd0u);s.object_rows=read(P::live_rows,s.object_pool+0x44u);
    s.live_body=read(P::live_body,s.object_rows+s.object_offset+8u);
    s.local_owned=read(P::live_orientations,s.live_body+0x112u,2u)!=0xffffu;
    save(P::body_publish,F::body,s.live_body);
    if(s.local_owned){s.local_offset=sx16(read(P::orientation_offset,s.body+0x112u,2u));s.orientations=s.body+s.local_offset;save(P::orientations_publish,F::orientations,s.orientations);}
    else{s.orientations=0x4e8bc0u;save(P::orientations_default,F::orientations,s.orientations);save(P::local_unowned_publish,F::local_owned,0u,0u,1u);}
    s.definition=frame(P::model_definition,F::definition);s.hlmt_datum=read(P::hlmt_datum,s.definition+0x38u);
    bool has_model=false;
    if(s.hlmt_datum!=0xffffffffu){
        s.hlmt=read(P::hlmt,s.tag_array+(s.hlmt_datum&0xffffu)*16u+8u);save(P::hlmt_publish,F::hlmt,s.hlmt);
        s.model_datum=read(P::model_datum,s.hlmt+4u);
        if(s.model_datum!=0xffffffffu){
            // The parent handle is acquired before the render-model pointer.
            s.parent=read(P::parent,s.body+0x14u);s.model=read(P::model,s.tag_array+(s.model_datum&0xffffu)*16u+8u);has_model=true;
        }
    }
    if(!has_model){
        s.parent=read(P::no_model_parent,s.body+0x14u);s.parent_matrix=0u;s.parent_mirror=0u;
        if(s.parent!=0xffffffffu){
            s.object_rows=read(P::no_model_rows,s.object_pool+0x44u);
            s.parent_body=read(P::no_model_parent_body,s.object_rows+(s.parent&0xffffu)*12u+8u);
            s.parent_node=sx8(read(P::no_model_node,s.body+0x18u,1u));
            s.matrix_offset=sx16(read(P::no_model_parent_offset,s.parent_body+0x116u,2u));
            s.parent_matrix=s.parent_body+s.parent_node*0x34u+s.matrix_offset;
            s.parent_mirror=(read(P::no_model_parent_flags,s.parent_body+4u)>>10u)&1u;
        }
        s.output=frame(P::no_model_output,F::matrices);memory.prepare(P::no_model_begin,s);
        s.scale=read(P::no_model_scale,s.body+0xa0u);memory.prepare(P::no_model_parent_argument,s);
        s.flags=read(P::no_model_flags,s.body+4u);s.mirror_flags=(s.flags>>10u)&0xffffff01u;
        s.up_pointer=s.body+0x7cu;s.forward_pointer=s.body+0x70u;s.position_pointer=s.body+0x64u;
        memory.prepare(P::no_model_arguments,s);services.root_transform(s);memory.complete(P::finish_common,s);return;
    }
    save(P::model_publish,F::model_or_height,s.model);s.parent_matrix=0u;save(P::parent_reset,F::parent_matrix,0u);
    s.parent_mirror=0u;s.world_relative=false;save(P::parent_mirror_reset,F::parent_mirror,0u,0u,1u);save(P::world_reset,F::world_relative,0u,0u,1u);
    if(s.parent!=0xffffffffu){
        s.parent_node=sx8(read(P::parent_node,s.body+0x18u,1u));memory.prepare(P::parent_call,s);s.parent_matrix=services.parent_matrix(s);
        save(P::parent_publish,F::parent_matrix,s.parent_matrix);
        s.parent_body=read(P::parent_body,s.object_rows+(s.parent&0xffffu)*12u+8u);
        s.flags=read(P::parent_flags,s.parent_body+4u);s.live_body=frame(P::parent_live_body,F::body);
        s.parent_mirror=(s.flags>>10u)&1u;save(P::parent_mirror_publish,F::parent_mirror,s.parent_mirror,0u,1u);
    }
    s.flags=read(P::live_flags,s.live_body+4u);
    bool animate=!(s.flags&0x20000000u);
    if(animate){
        s.definition=frame(P::animation_definition,F::definition);s.definition_type=read(P::animation_type,s.definition,1u);
        if(is_biped(s.definition_type)&&read(P::animation_subtype,s.live_body+0x34bu,1u))animate=false;
    }
    if(animate){
        s.orientations=frame(P::bind_orientations,F::orientations);memory.prepare(P::bind_call,s);services.bind_pose(s);
        s.object=frame(P::animation_object,F::object);memory.prepare(P::has_animation_call,s);
        if(services.has_animation(s)){
            memory.prepare(P::frozen_call,s);
            if(!services.animation_frozen(s)){
                s.manager=s.body+sx16(read(P::manager_offset,s.body+0x12au,2u));s.selection=read(P::manager_selection,s.manager+0x68u);
                if(s.selection!=0xffffffffu){
                    if(!(read(P::mask_initialized,0x56ebacu,1u)&1u))put(P::mask_initialize,0x56ebacu,read(P::mask_initialize,0x56ebacu)|1u);
                    s.local_owned=frame(P::local_owned,F::local_owned,0u,1u)!=0u;s.blend_mask=0u;
                    if(s.local_owned){
                        s.hlmt=frame(P::hlmt_reload,F::hlmt);s.mask_source=s.hlmt+0xa4u;memory.prepare(P::mask_empty_call,s);
                        if(!services.mask_empty(s)){
                            s.copy_source=s.mask_source;s.copy_destination=0x56eb8cu;s.copy_remaining=8u;memory.prepare(P::mask_copy_begin,s);
                            copy_words(P::mask_copy_read,P::mask_copy_write);memory.prepare(P::invert_mask_call,s);services.invert_mask(s);
                            s.mask_count=frame(P::mask_count,F::count);memory.prepare(P::trim_mask_call,s);services.trim_mask(s);s.blend_mask=0x56eb8cu;
                        }
                    }
                    s.orientations=frame(P::animation_orientations,F::orientations);s.model=frame(P::animation_model,F::model_or_height);
                    s.object=frame(P::animation_handle,F::object);memory.prepare(P::animation_call,s);
                    s.count=frame(P::animation_count,F::count);services.animate(s);
                    memory.prepare(P::render_mask_call,s);services.publish_render_mask(s);
                    s.selection=read(P::selection_live,s.manager+0x68u);
                    if(s.selection!=0xffffffffu){
                        memory.prepare(P::selection_present_call,s);
                        if(services.selection_present(s)){
                            memory.prepare(P::selection_call,s);s.selection=services.selected_animation(s);
                            s.world_relative=(read(P::animation_flags,s.selection+0x16u,1u)>>1u)&1u;
                            save(P::world_publish,F::world_relative,s.world_relative,0u,1u);
                        }
                    }
                }
                s.model=frame(P::model_reload,F::model_or_height);
            }
        }
    }
    s.model_count=read(P::model_count,s.model+0x48u);
    if(s.model_count==0u){
        s.copy_source=read(P::identity_source,0x474d38u);s.copy_destination=frame(P::identity_destination,F::orientations);
        s.copy_remaining=8u;memory.prepare(P::identity_begin,s);copy_words(P::identity_read,P::identity_write);
    }
    if(!(frame(P::root_mask,F::mask,0u,1u)&1u)){
        s.world_relative=frame(P::world_relative,F::world_relative,0u,1u)!=0u;
        if(s.world_relative){
            s.orientations=frame(P::world_orientation,F::orientations);s.output=frame(P::world_matrix,F::matrices);
            memory.prepare(P::world_local_call,s);services.local_transform(s);
        }else{
            s.position[0]=read(P::position_x,s.body+0x64u);s.position[1]=read(P::position_y,s.body+0x68u);s.position[2]=read(P::position_z,s.body+0x6cu);
            save(P::position_x_publish,F::position,s.position[0]);save(P::position_y_publish,F::position,s.position[1],4u);
            s.forward[0]=read(P::forward_x,s.body+0x70u);save(P::forward_x_publish,F::forward,s.forward[0]);save(P::position_z_publish,F::position,s.position[2],8u);
            s.forward[1]=read(P::forward_y,s.body+0x74u);s.forward[2]=read(P::forward_z,s.body+0x78u);save(P::forward_y_publish,F::forward,s.forward[1],4u);
            s.up[0]=read(P::up_x,s.body+0x7cu);save(P::forward_z_publish,F::forward,s.forward[2],8u);
            s.up[1]=read(P::up_y,s.body+0x80u);s.up[2]=read(P::up_z,s.body+0x84u);save(P::up_z_publish,F::up,s.up[2],8u);
            s.network_entity=read(P::network_entity,s.body+0xd4u);save(P::up_x_publish,F::up,s.up[0]);save(P::up_y_publish,F::up,s.up[1],4u);
            if(s.network_entity!=0xffffffffu&&(read(P::network_enabled,s.body+0xd8u,1u)&1u)){
                s.object=frame(P::prediction_object,F::object);s.position_pointer=address(P::prediction_position,F::scratch);memory.prepare(P::prediction_position_call,s);
                if(services.predict_position(s)){
                    s.temporary[0]=frame(P::position_scratch_x,F::scratch);s.temporary[1]=frame(P::position_scratch_y,F::scratch,4u);s.temporary[2]=frame(P::position_scratch_z,F::scratch,8u);
                    save(P::position_override_x,F::position,s.temporary[0]);save(P::position_override_y,F::position,s.temporary[1],4u);save(P::position_override_z,F::position,s.temporary[2],8u);
                }
                s.object=frame(P::prediction_axes_object,F::object);s.up_pointer=address(P::prediction_up,F::axes_scratch);s.forward_pointer=address(P::prediction_forward,F::scratch);
                memory.prepare(P::prediction_axes_call,s);
                if(services.predict_axes(s)){
                    s.temporary[0]=frame(P::axes_forward_x,F::scratch);s.temporary[1]=frame(P::axes_forward_y,F::scratch,4u);s.temporary[2]=frame(P::axes_forward_z,F::scratch,8u);
                    save(P::forward_override_x,F::forward,s.temporary[0]);s.up[0]=frame(P::axes_up_x,F::axes_scratch);
                    save(P::forward_override_y,F::forward,s.temporary[1],4u);s.up[1]=frame(P::axes_up_y,F::axes_scratch,4u);
                    save(P::forward_override_z,F::forward,s.temporary[2],8u);s.up[2]=frame(P::axes_up_z,F::axes_scratch,8u);
                    save(P::up_override_x,F::up,s.up[0]);save(P::up_override_y,F::up,s.up[1],4u);save(P::up_override_z,F::up,s.up[2],8u);
                }
            }
            s.flags=read(P::root_flags,s.body+4u);s.parent_matrix=frame(P::root_parent,F::parent_matrix);s.output=address(P::root_output,F::root_matrix);
            memory.prepare(P::root_begin,s);s.scale=read(P::root_scale,s.body+0xa0u);s.parent_mirror=frame(P::root_parent_mirror,F::parent_mirror,0u,1u);
            s.mirror_flags=(s.flags>>10u)&0xffffff01u;
            s.up_pointer=address(P::root_arguments,F::up);s.forward_pointer=address(P::root_arguments,F::forward);s.position_pointer=address(P::root_arguments,F::position);
            memory.prepare(P::root_arguments,s);memory.prepare(P::root_call,s);services.root_transform(s);
            s.orientations=frame(P::root_orientation,F::orientations);s.output=address(P::root_local_output,F::local_matrix);
            memory.prepare(P::root_local_call,s);services.local_transform(s);
            s.output=frame(P::root_product_output,F::matrices);s.local_matrix=address(P::root_product_call,F::local_matrix);s.root_matrix=address(P::root_product_call,F::root_matrix);
            memory.prepare(P::root_product_call,s);services.root_product(s);
        }
        s.output=frame(P::notify_matrix,F::matrices);s.object=frame(P::notify_object,F::object);memory.prepare(P::notify_call,s);services.notify_type(s);
    }
    publish_hierarchy(s);
}
} // namespace halo2
