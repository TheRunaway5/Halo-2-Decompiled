#pragma once
#include <cstdint>

namespace halo2::native_unit_motion_parameters {
inline constexpr std::uint32_t game_globals=0x4ecc1cu,one=0x46a0d0u,default_value=0x45695cu;
inline constexpr std::uint32_t motion_table=0x134u,matrix_words=13u;

namespace input_field {
inline constexpr std::uint32_t selector=0x2cu,matrix=0x34u,vector=0x74u;
}
namespace output_field {
inline constexpr std::uint32_t active=0x04u,descriptor=0x08u,options=0x14u,selector=0x54u,matrix=0x58u,
    vector=0x8cu,scale=0x98u,position=0x9cu,option=0xa8u,base_position=0x10cu,direction=0xdcu,
    fallback_position=0xf4u,speed=0x138u,lateral=0x13cu,vertical=0x140u,limit=0x144u,padding_value=0x148u;
}
namespace option {
inline constexpr std::uint32_t fixed_blend=4u,preserve_limit=8u;
}
namespace tuning_field {
inline constexpr std::uint32_t override_speed=0x24u,positive_speed=0x2cu,negative_speed=0x30u,
    lateral=0x34u,limit=0x38u,positive_blend=0x3cu,negative_blend=0x40u,
    lateral_blend=0x44u,limit_blend=0x48u,padding_value=0x4cu;
}

enum class Access {
    selector,selector_store,matrix,matrix_store,
    vector_x,vector_x_store,vector_y,vector_y_store,vector_z,vector_z_store,
    option_arg,option_store,fallback_x,fallback_x_store,fallback_y,fallback_z,fallback_y_store,fallback_z_store,
    options,scale_store,active_store,blend_arg,descriptor,descriptor_options,skip_arg,
    globals,tuning,
    direction_x,direction_x_store,direction_y,direction_z,direction_x_reload,direction_y_store,direction_y_reload,
    direction_z_store,direction_z_reload,one_load,length_store,length_reload,reciprocal_dividend,normalize_x,
    normalized_x_store,normalized_x_reload,normalize_y,normalized_y_store,normalized_y_reload,
    forward_speed,forward_blend,reverse_speed,reverse_blend,speed_store,lateral,lateral_blend,lateral_store,
    vertical_store,limit,limit_blend,limit_store,padding,padding_store,override_arg,override_speed,
    override_speed_store,override_lateral_store,speed_reload,speed_scaled_store,lateral_reload,lateral_scaled_store,
    position_x,position_y,position_z,position_x_store,position_y_store,position_z_store,default_limit,default_limit_store,
    count,
};
enum class Local : std::uint32_t { direction_x=0x0cu,direction_y=0x10u,direction_z=0x14u,
    skip=0x1cu,override_speed=0x20u,option_or_length=0x24u,blend=0x28u };

template<class Memory,class Frame>
void prepare(Memory &memory,Frame &frame,std::uint32_t output,std::uint32_t input,std::uint32_t scale) {
    using enum Access;
    const auto selector_value=memory.load32(selector,input+input_field::selector);
    memory.store32(selector_store,output+output_field::selector,selector_value);
    frame.begin_matrix(input+input_field::matrix,output+output_field::matrix,matrix_words);
    auto source=input+input_field::matrix;
    auto target=output+output_field::matrix;
    const auto step=frame.matrix_step();
    for(std::uint32_t word=0;word<matrix_words;++word) {
        const auto bits=memory.load32(matrix,source);
        memory.store32(matrix_store,target,bits);
        source+=step;
        target+=step;
        frame.advance_matrix(source,target);
    }
    const auto input_vector=input+input_field::vector;
    const auto output_vector=output+output_field::vector;
    const auto x=memory.load32(vector_x,input_vector);
    memory.store32(vector_x_store,output_vector,x);
    const auto y=memory.load32(vector_y,input_vector+4u);
    memory.store32(vector_y_store,output_vector+4u,y);
    const auto z=memory.load32(vector_z,input_vector+8u);
    memory.store32(vector_z_store,output_vector+8u,z);
    const auto argument=memory.load8(option_arg,frame.local(Local::option_or_length));
    memory.store8(option_store,output+output_field::option,argument);

    const auto fallback=output+output_field::fallback_position;
    const auto position=output+output_field::position;
    const auto fallback_position_x=memory.load32(fallback_x,fallback);
    memory.store32(fallback_x_store,position,fallback_position_x);
    const auto fallback_position_y=memory.load32(fallback_y,fallback+4u);
    const auto fallback_position_z=memory.load32(fallback_z,fallback+8u);
    memory.store32(fallback_y_store,position+4u,fallback_position_y);
    memory.store32(fallback_z_store,position+8u,fallback_position_z);
    const auto raw_options=memory.load32(options,output+output_field::options);
    const bool fixed_blend=frame.use_fixed_blend(raw_options);
    memory.store32(scale_store,output+output_field::scale,scale);
    memory.store8(active_store,output+output_field::active,1u);
    frame.zero_blend();
    if(!fixed_blend)frame.set_blend(memory.load32(blend_arg,frame.local(Local::blend)));

    const auto descriptor_address=memory.load32(descriptor,output+output_field::descriptor);
    const auto descriptor_flags=memory.load32(descriptor_options,descriptor_address);
    if(!frame.motion_enabled(descriptor_flags)||
       frame.motion_skipped(memory.load8(skip_arg,frame.local(Local::skip)))) {
        if(frame.needs_default_limit(raw_options)) {
            const auto value=memory.load32(default_limit,default_value);
            memory.store32(default_limit_store,output+output_field::limit,value);
        }
        frame.finish_without_motion();
        return;
    }

    const auto game=memory.load32(globals,game_globals);
    const auto table=memory.load32(tuning,game+motion_table);
    const auto direction=output+output_field::direction;
    const auto direction_x_word=memory.load32(direction_x,direction);
    memory.store32(direction_x_store,frame.local(Local::direction_x),direction_x_word);
    const auto direction_y_word=memory.load32(direction_y,direction+4u);
    const auto direction_z_word=memory.load32(direction_z,direction+8u);
    frame.load_axis_x(memory.load32(direction_x_reload,frame.local(Local::direction_x)));
    memory.store32(direction_y_store,frame.local(Local::direction_y),direction_y_word);
    frame.load_axis_y(memory.load32(direction_y_reload,frame.local(Local::direction_y)));
    memory.store32(direction_z_store,frame.local(Local::direction_z),direction_z_word);
    const auto length=frame.squared_length(memory.load32(direction_z_reload,frame.local(Local::direction_z)));
    const auto unit=memory.load32(one_load,one);
    const bool normalize=frame.needs_normalization(unit);
    memory.store32(length_store,frame.local(Local::option_or_length),length);
    if(normalize) {
        frame.load_length(memory.load32(length_reload,frame.local(Local::option_or_length)));
        frame.inverse_length(memory.load32(reciprocal_dividend,one));
        const auto normalized_x=frame.scaled_x(memory.load32(normalize_x,frame.local(Local::direction_x)));
        memory.store32(normalized_x_store,frame.local(Local::direction_x),normalized_x);
        frame.pop_extended();
        frame.load_axis_x(memory.load32(normalized_x_reload,frame.local(Local::direction_x)));
        const auto normalized_y=frame.scaled_y(memory.load32(normalize_y,frame.local(Local::direction_y)));
        memory.store32(normalized_y_store,frame.local(Local::direction_y),normalized_y);
        frame.pop_extended();
        frame.load_axis_y(memory.load32(normalized_y_reload,frame.local(Local::direction_y)));
    }

    const bool forward=frame.begin_blend();
    const auto speed=memory.load32(forward?forward_speed:reverse_speed,
        table+(forward?tuning_field::positive_speed:tuning_field::negative_speed));
    const auto blend_speed=memory.load32(forward?forward_blend:reverse_blend,
        table+(forward?tuning_field::positive_blend:tuning_field::negative_blend));
    memory.store32(speed_store,output+output_field::speed,frame.blend(speed,blend_speed));
    const auto side=memory.load32(lateral,table+tuning_field::lateral);
    const auto blend_side=memory.load32(lateral_blend,table+tuning_field::lateral_blend);
    memory.store32(lateral_store,output+output_field::lateral,frame.blend(side,blend_side));
    memory.store32(vertical_store,output+output_field::vertical,0u);
    frame.prepare_limit(memory.load32(limit,table+tuning_field::limit));
    const auto limit_value=frame.finish_limit(memory.load32(limit_blend,table+tuning_field::limit_blend));
    memory.store32(limit_store,output+output_field::limit,limit_value);
    const auto table_value=memory.load32(padding,table+tuning_field::padding_value);
    memory.store32(padding_store,output+output_field::padding_value,table_value);

    if(frame.overrides_speed(memory.load8(override_arg,frame.local(Local::override_speed)))) {
        const auto value=frame.forward_override()?memory.load32(override_speed,table+tuning_field::override_speed):0u;
        memory.store32(override_speed_store,output+output_field::speed,frame.set_override(value));
        memory.store32(override_lateral_store,output+output_field::lateral,0u);
    }
    const auto forward_scaled=frame.scale_speed(memory.load32(speed_reload,output+output_field::speed));
    memory.store32(speed_scaled_store,output+output_field::speed,forward_scaled);
    const auto lateral_scaled=frame.scale_lateral(memory.load32(lateral_reload,output+output_field::lateral));
    memory.store32(lateral_scaled_store,output+output_field::lateral,lateral_scaled);

    const auto base=frame.position_address(output);
    const auto position_x_word=memory.load32(position_x,base);
    const auto position_y_word=memory.load32(position_y,base+4u);
    const auto position_z_word=memory.load32(position_z,base+8u);
    memory.store32(position_x_store,position,position_x_word);
    memory.store32(position_y_store,position+4u,position_y_word);
    frame.restore_before_position_z();
    memory.store32(position_z_store,position+8u,position_z_word);
    frame.finish_motion();
}
} // namespace halo2::native_unit_motion_parameters
