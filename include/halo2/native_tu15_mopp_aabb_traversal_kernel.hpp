#pragma once
#include "halo2/native_tu15_mopp_aabb_projection_kernel.hpp"
#include <bit>

namespace halo2::native_mopp_aabb_detail {
enum class Read {
    opcode,encoded_byte,projection_upper,projection_lower,
    scale_translation,scale_shift,scale_lower,scale_upper,scale_local_z_lower,scale_local_z_upper,
    scale_property,scale_key,key_base,preserved_property,
    single_upper,single_lower,offset_lower,offset_upper,offset_saved_low_left,offset_saved_lower_left,
    offset_saved_lower_right,offset_saved_low_right,slab_upper,slab_lower,wide_upper,wide_lower,
    leaf_base,leaf_array,leaf_count,leaf_capacity,leaf_index,leaf_data,leaf_current
};
enum class Write {scale_translation,scale_shift,scale_lower,scale_upper,scale_property,scale_key,
    key_base,property,preserved_property,offset_low,offset_lower,leaf_key,leaf_count};
enum class Event {initial,decoded,advance,projection_read,projection_ready,projection_jump,
    single_ready,single_jump,single_test,offset_ready,offset_left_test,offset_right_test,
    slab_first,slab_second,wide_first,wide_second,scale_shift_ready,scale_commit,
    key_local,property_begin,property_local,leaf_begin,leaf_test,leaf_count_ready,finish,leaf_finish};
enum class Encoding {none,jump,key_base,property,leaf,wide_low,wide_high,scale,single,offset};
enum class Flow {initial,scale,jump,key_base,project_left,project_right,project_recursed,
    project_axis_right,project_axis_recursed,project_axis_continue,
    single_left,single_right,single_recursed,offset_left,offset_right,offset_recursed,slab,wide,property};
struct Continuation {std::uint32_t cache{},variables{},program{},jump{},key{},array{};};
struct Progress {
    std::uint32_t cache{},variables{},program{},opcode{},previous_program{};
    Encoding encoding{Encoding::none};Flow flow{Flow::initial};
    std::array<std::uint32_t,4>bytes{};unsigned byte_index{},byte_offset{},byte_count{};
    std::uint32_t encoded{},value{},axis{},shift{},quantization{};
    std::array<std::uint32_t,3>translation{};
    std::uint32_t upper{},lower{},low{},high{},jump{},left{},right{},child_program{},property{},saved_property{};
    std::uint32_t key{},base{},array{},count{},capacity{},data{},current{};
    Projection projection{};ProjectionRead projection_read{ProjectionRead::low_threshold};unsigned projection_axis{};
};
inline std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
inline std::uint32_t arithmetic_right(std::uint32_t value,std::uint32_t count){return std::uint32_t(signed_word(value)>>(count&31u));}

// The authored MOPP DATA program drives geometric clipping and key delivery.
// Frame supplies live input fields, real recursive/growth continuations and
// private lifetime. Numerical decisions never inspect a CPU register file or
// emulated flags. The public value API and retained caller adapter share this
// controller while keeping their distinct ownership contracts explicit.
template<class Frame>class Traversal {
    Frame&frame_;Progress d_{};
    std::uint32_t read(Read phase,unsigned index=0){d_.axis=index;return frame_.read(phase,d_,index);}
    void write(Write phase,std::uint32_t value,unsigned index=0){d_.axis=index;d_.value=value;frame_.write(phase,d_,value,index);}
    void event(Event phase){frame_.event(phase,d_);}
    std::uint32_t byte(Encoding kind,unsigned offset,unsigned index=0){
        d_.encoding=kind;d_.byte_offset=offset;d_.byte_index=index;
        const auto value=read(Read::encoded_byte);d_.bytes[index]=value;return value;
    }
    std::uint32_t decode(Encoding kind,unsigned offset,unsigned count){
        d_.encoding=kind;d_.byte_count=count;d_.encoded=0;d_.bytes={};
        for(unsigned index=0;index<count;++index)d_.encoded=(d_.encoded<<8u)+byte(kind,offset+index,index);
        return d_.encoded;
    }
    void advance(std::uint32_t program,Flow flow){
        d_.previous_program=d_.program;d_.program=program;d_.flow=flow;event(Event::advance);
    }
    void recurse(Flow flow){
        d_.flow=flow;const auto resumed=frame_.recurse(d_);
        d_.cache=resumed.cache;d_.variables=resumed.variables;d_.program=resumed.program;d_.jump=resumed.jump;
    }
    void leaf(){
        event(Event::leaf_begin);d_.base=read(Read::leaf_base);d_.key+=d_.base;
        d_.array=read(Read::leaf_array);d_.count=read(Read::leaf_count);d_.capacity=read(Read::leaf_capacity)&0x7fffffffu;
        event(Event::leaf_test);
        if(d_.count==d_.capacity){const auto resumed=frame_.grow(d_);d_.key=resumed.key;d_.array=resumed.array;}
        d_.count=read(Read::leaf_index);d_.data=read(Read::leaf_data);write(Write::leaf_key,d_.key);
        d_.current=read(Read::leaf_current);event(Event::leaf_count_ready);write(Write::leaf_count,d_.current+1u);event(Event::leaf_finish);
    }
public:
    explicit Traversal(Frame&frame):frame_(frame){}
    void run(){
        frame_.begin(d_);event(Event::initial);
        for(;;){
            d_.opcode=read(Read::opcode);event(Event::decoded);
            if(d_.opcode==0){event(Event::finish);return;}
            if(d_.opcode>=1u&&d_.opcode<=4u){
                for(unsigned axis=0;axis<3;++axis){
                    d_.value=byte(Encoding::scale,axis+1u,axis);const auto old=read(Read::scale_translation,axis);
                    d_.translation[axis]=(old+d_.value)<<d_.opcode;write(Write::scale_translation,d_.translation[axis],axis);
                }
                d_.quantization=read(Read::scale_shift)+d_.opcode;write(Write::scale_shift,d_.quantization);
                d_.shift=16u-d_.quantization;event(Event::scale_shift_ready);
                for(unsigned axis=0;axis<3;++axis){
                    d_.value=arithmetic_right(read(Read::scale_lower,axis),d_.shift);
                    if(axis==2)d_.translation[axis]=read(Read::scale_local_z_lower);
                    write(Write::scale_lower,d_.value-d_.translation[axis],axis);
                }
                d_.value=arithmetic_right(read(Read::scale_upper,0),d_.shift)+1u-d_.translation[0];
                d_.upper=arithmetic_right(read(Read::scale_upper,1),d_.shift);
                write(Write::scale_upper,d_.value,0);write(Write::scale_upper,d_.upper+1u-d_.translation[1],1);
                d_.upper=arithmetic_right(read(Read::scale_upper,2),d_.shift);
                d_.translation[2]=read(Read::scale_local_z_upper);write(Write::scale_upper,d_.upper+1u-d_.translation[2],2);
                d_.property=read(Read::scale_property);write(Write::scale_property,d_.property);
                d_.base=read(Read::scale_key);write(Write::scale_key,d_.base);event(Event::scale_commit);
                advance(d_.program+4u,Flow::scale);continue;
            }
            if(d_.opcode>=5u&&d_.opcode<=7u){
                const auto count=d_.opcode-4u;d_.jump=decode(Encoding::jump,1u,count);advance(d_.program+d_.jump+count+1u,Flow::jump);continue;
            }
            if(d_.opcode==9u||d_.opcode==10u||d_.opcode==11u){
                const auto count=d_.opcode==9u?1u:d_.opcode==10u?2u:4u;d_.key=decode(Encoding::key_base,1u,count);
                event(Event::key_local);d_.base=d_.opcode==11u?0u:read(Read::key_base);write(Write::key_base,d_.base+d_.key);
                advance(d_.program+count+1u,Flow::key_base);continue;
            }
            if(d_.opcode>=0x10u&&d_.opcode<=0x1cu){
                struct Fields {
                    Traversal&walk;
                    std::uint32_t upper(unsigned axis){return walk.read(Read::projection_upper,axis);}
                    std::uint32_t lower(unsigned axis){return walk.read(Read::projection_lower,axis);}
                }fields{*this};
                const auto projection=project(d_.opcode,fields,[&](unsigned offset){return byte(Encoding::none,offset,offset-1u);},
                    [&](ProjectionRead phase,unsigned axis,const Projection&progress){d_.projection=progress;d_.projection_read=phase;d_.projection_axis=axis;event(Event::projection_read);});
                d_.projection=projection;d_.upper=projection.upper;d_.lower=projection.lower;d_.low=projection.low;d_.high=projection.high;
                d_.previous_program=d_.program;d_.program+=4u;event(Event::projection_ready);
                if(signed_word(d_.upper)<=signed_word(d_.high)){
                    if(signed_word(d_.lower)>=signed_word(d_.low)){event(Event::finish);return;}
                    advance(d_.program,Flow::project_left);continue;
                }
                event(Event::projection_jump);d_.jump=byte(Encoding::none,0xffffffffu);d_.child_program=d_.program;
                if(d_.opcode<=0x12u){
                    advance(d_.program+d_.jump,Flow::project_axis_right);
                    if(signed_word(d_.lower)<signed_word(d_.low))recurse(Flow::project_axis_recursed);
                    advance(d_.program,Flow::project_axis_continue);continue;
                }
                if(signed_word(d_.lower)<signed_word(d_.low))recurse(Flow::project_recursed);
                advance(d_.program+d_.jump,Flow::project_right);continue;
            }
            if(d_.opcode>=0x20u&&d_.opcode<=0x22u){
                d_.low=byte(Encoding::single,1u);d_.upper=read(Read::single_upper,d_.opcode-0x20u);
                d_.previous_program=d_.program;d_.program+=3u;event(Event::single_ready);
                if(signed_word(d_.upper)<=signed_word(d_.low)){advance(d_.program,Flow::single_left);continue;}
                event(Event::single_jump);d_.jump=byte(Encoding::single,0xffffffffu);d_.lower=read(Read::single_lower,d_.opcode-0x20u);
                d_.child_program=d_.program;d_.previous_program=d_.program;d_.program+=d_.jump;event(Event::single_test);
                if(signed_word(d_.lower)<=signed_word(d_.low))recurse(Flow::single_recursed);
                advance(d_.program,Flow::single_right);continue;
            }
            if(d_.opcode>=0x23u&&d_.opcode<=0x25u){
                d_.low=byte(Encoding::offset,1u,0);d_.left=byte(Encoding::offset,4u,1);d_.high=byte(Encoding::offset,2u,2);
                write(Write::offset_low,d_.low);d_.lower=read(Read::offset_lower,d_.opcode-0x23u);write(Write::offset_lower,d_.lower);
                d_.left+=(byte(Encoding::offset,3u,3)<<8u);d_.upper=read(Read::offset_upper,d_.opcode-0x23u);
                d_.previous_program=d_.program;d_.program+=7u;event(Event::offset_ready);
                if(signed_word(d_.upper)<=signed_word(d_.high)){
                    d_.low=read(Read::offset_saved_low_left);d_.lower=read(Read::offset_saved_lower_left);event(Event::offset_left_test);
                    if(signed_word(d_.lower)>=signed_word(d_.low)){event(Event::finish);return;}
                    advance(d_.program+d_.left,Flow::offset_left);continue;
                }
                d_.right=byte(Encoding::offset,0xfffffffeu,0)<<8u;d_.right+=byte(Encoding::offset,0xffffffffu,1);
                d_.lower=read(Read::offset_saved_lower_right);d_.low=read(Read::offset_saved_low_right);
                d_.child_program=d_.program+d_.left;d_.previous_program=d_.program;d_.program+=d_.right;event(Event::offset_right_test);
                if(signed_word(d_.lower)<signed_word(d_.low))recurse(Flow::offset_recursed);
                advance(d_.program,Flow::offset_right);continue;
            }
            if(d_.opcode>=0x26u&&d_.opcode<=0x28u){
                d_.low=byte(Encoding::single,1u);d_.upper=read(Read::slab_upper,d_.opcode-0x26u);event(Event::slab_first);
                if(signed_word(d_.upper)<signed_word(d_.low)){event(Event::finish);return;}
                d_.high=byte(Encoding::single,2u);d_.lower=read(Read::slab_lower,d_.opcode-0x26u);event(Event::slab_second);
                if(signed_word(d_.lower)>=signed_word(d_.high)){event(Event::finish);return;}
                advance(d_.program+3u,Flow::slab);continue;
            }
            if(d_.opcode>=0x29u&&d_.opcode<=0x2bu){
                d_.low=decode(Encoding::wide_low,1u,3);d_.upper=read(Read::wide_upper,d_.opcode-0x29u);event(Event::wide_first);
                if(signed_word(d_.upper)<signed_word(d_.low)){event(Event::finish);return;}
                d_.high=decode(Encoding::wide_high,4u,3);d_.lower=read(Read::wide_lower,d_.opcode-0x29u);event(Event::wide_second);
                if(signed_word(d_.lower)>signed_word(d_.high)){event(Event::finish);return;}
                advance(d_.program+7u,Flow::wide);continue;
            }
            if(d_.opcode>=0x30u&&d_.opcode<=0x53u){
                d_.key=d_.opcode<0x50u?d_.opcode-0x30u:decode(Encoding::leaf,1u,d_.opcode-0x4fu);leaf();return;
            }
            if(d_.opcode>=0x60u&&d_.opcode<=0x6bu){
                event(Event::property_begin);const auto count=d_.opcode<0x64u?1u:d_.opcode<0x68u?2u:4u;
                d_.property=decode(Encoding::property,1u,count);
                if(count==1)advance(d_.program+2u,Flow::property);
                write(Write::property,d_.property,d_.opcode&3u);
                if(count!=1)advance(d_.program+count+1u,Flow::property);
                d_.saved_property=read(Read::preserved_property);event(Event::property_local);write(Write::preserved_property,d_.saved_property);
                continue;
            }
            frame_.diagnostic(d_);event(Event::finish);return;
        }
    }
};
} // namespace halo2::native_mopp_aabb_detail
