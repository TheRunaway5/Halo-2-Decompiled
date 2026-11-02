#include "halo2/native_tu15_camera_view_layout.hpp"
#include <bit>
#include <limits>
namespace halo2 {
NativeTu15CameraViewLayoutDivideError::NativeTu15CameraViewLayoutDivideError(
    std::int32_t dividend,std::int32_t divisor,std::uint32_t site):
    std::runtime_error("Native camera layout reached original signed divide trap"),
    dividend_(dividend),divisor_(divisor),site_(site){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
std::int32_t signed_value(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=2) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(8u*n));
    memory.write(address,event.written);if(observe)observe(event);
}
struct Division {std::int32_t quotient,remainder;};
Division divide(std::int32_t dividend,std::int32_t divisor,std::uint32_t site) {
    if(!divisor||(dividend==std::numeric_limits<std::int32_t>::min()&&divisor==-1))
        throw NativeTu15CameraViewLayoutDivideError(dividend,divisor,site);
    return {dividend/divisor,dividend%divisor};
}
std::uint32_t pair(std::uint16_t first,std::uint16_t second){return std::uint32_t(first)|(std::uint32_t(second)<<16u);}
void inset(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::int32_t delta) {
    const auto before=read(memory,address,2);publish(memory,observe,site,address,before+std::uint32_t(delta));
}
}
NativeTu15CameraViewGrid native_tu15_camera_view_grid(std::int32_t count,std::int32_t split_mode) {
    std::uint32_t columns=1,rows=1;
    if(count>1)do {
        const bool grow_column=split_mode==1?signed_value(columns)<signed_value(rows):signed_value(columns)<=signed_value(rows);
        if(grow_column)++columns;else{columns=1;++rows;}
    }while(signed_value(columns*rows)<count);
    return {std::uint16_t(columns),std::uint16_t(rows)};
}
void native_tu15_camera_view_grid_publish(SourcePhysicsContactMemory &memory,std::int32_t count,
    std::int32_t split_mode,std::uint32_t destination,const SourcePhysicsContactRecordObserver &observe) {
    const auto grid=native_tu15_camera_view_grid(count,split_mode);
    publish(memory,observe,0x2f617u,destination,grid.columns);
    publish(memory,observe,0x2f61au,destination+2u,grid.rows);
}
void native_tu15_camera_view_tile(SourcePhysicsContactMemory &memory,std::int32_t ordinal,
    std::uint32_t grid,std::uint32_t span_output,const NativeTu15CameraViewTileArguments &argument,
    const SourcePhysicsContactRecordObserver &observe) {
    publish(memory,observe,0x2f622u,span_output+2u,1);
    publish(memory,observe,0x2f628u,span_output,1);
    const auto width=signed_word(read(memory,grid,2)),height=signed_word(read(memory,grid+2u,2));
    const auto capacity=signed_value(std::uint32_t(width)*std::uint32_t(height));std::int32_t extra{};
    if(argument.count()<capacity) {
        if(argument.split_mode()==1) {
            if(!ordinal)publish(memory,observe,0x2f64du,span_output,2);else extra=1;
        }else {
            if(!ordinal)publish(memory,observe,0x2f65fu,span_output+2u,2);
            else extra=divide(ordinal,width,0x2f66au).quotient;
        }
    }
    const auto position=signed_value(std::uint32_t(ordinal)+std::uint32_t(extra));
    const auto first_width=signed_word(read(memory,grid,2));
    const auto first=divide(position,first_width,0x2f675u);
    const auto output=argument.tile_output();
    publish(memory,observe,0x2f67du,output,std::uint32_t(first.remainder));
    const auto second_width=signed_word(read(memory,grid,2));
    const auto second=divide(position,second_width,0x2f684u);
    publish(memory,observe,0x2f686u,output+2u,std::uint32_t(second.quotient));
}
void native_tu15_camera_view_rectangles(SourcePhysicsContactMemory &memory,std::uint32_t tile,
    std::uint32_t inset_output,const NativeTu15CameraViewRectangleArguments &argument,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto window_end=read(memory,0x4922aeu),safe_end=read(memory,0x4922b6u);
    const auto window_begin=read(memory,0x4922aau),safe_begin=read(memory,0x4922b2u);
    const std::array<std::uint16_t,4> window{std::uint16_t(window_begin),std::uint16_t(window_begin>>16u),
        std::uint16_t(window_end),std::uint16_t(window_end>>16u)};
    const std::array<std::uint16_t,4> safe{std::uint16_t(safe_begin),std::uint16_t(safe_begin>>16u),
        std::uint16_t(safe_end),std::uint16_t(safe_end>>16u)};
    const auto grid_width=signed_word(read(memory,argument.grid(),2));
    const auto width=std::uint16_t(divide(signed_word(safe[3])-signed_word(safe[1]),grid_width,0x2f6d4u).quotient);
    const auto grid_height=signed_word(read(memory,argument.grid()+2u,2));
    const auto height=std::uint16_t(divide(signed_word(safe[2])-signed_word(safe[0]),grid_height,0x2f6eeu).quotient);
    const auto column=read(memory,tile,2);const auto left=std::uint16_t(column*width+safe[1]);
    const auto span=argument.span();const auto column_end=std::uint16_t(read(memory,span,2)+column);
    const auto right=std::uint16_t(std::uint32_t(column_end)*width+safe[1]);
    const auto row=read(memory,tile+2u,2);const auto top=std::uint16_t(row*height+safe[0]);
    const auto row_end=std::uint16_t(read(memory,span+2u,2)+row);
    const auto bottom=std::uint16_t(std::uint32_t(row_end)*height+safe[0]);
    const auto outer=argument.rectangle_output();
    const auto begin=pair(top,left),end=pair(bottom,right);
    publish(memory,observe,0x2f755u,outer,begin,4);publish(memory,observe,0x2f757u,outer+4u,end,4);
    publish(memory,observe,0x2f75au,inset_output,begin,4);publish(memory,observe,0x2f75cu,inset_output+4u,end,4);
    if(!read(memory,tile,2))publish(memory,observe,0x2f76au,outer+2u,window[1]);
    else inset(memory,observe,0x2f770u,inset_output+2u,4);
    const auto columns=signed_word(read(memory,span,2))+signed_word(read(memory,tile,2));
    const auto last_grid=argument.grid();const auto columns_total=signed_word(read(memory,last_grid,2));
    if(columns>=columns_total)publish(memory,observe,0x2f78du,outer+6u,window[3]);
    else inset(memory,observe,0x2f793u,inset_output+6u,-4);
    if(!read(memory,tile+2u,2))publish(memory,observe,0x2f7a4u,outer,window[0]);
    else inset(memory,observe,0x2f7a9u,inset_output,4);
    const auto span_rows=signed_word(read(memory,span+2u,2));
    const auto tile_row=signed_word(read(memory,tile+2u,2));
    const auto rows_total=signed_word(read(memory,last_grid+2u,2));
    if(span_rows+tile_row>=rows_total)publish(memory,observe,0x2f7c4u,outer+4u,window[2]);
    else inset(memory,observe,0x2f7d0u,inset_output+4u,-4);
}
}
