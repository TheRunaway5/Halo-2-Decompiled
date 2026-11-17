#pragma once
#include "halo2/native_tu15_object_body_kernel.hpp"
#include <bit>
#include <utility>

namespace halo2::native_object_body_detail {
// The public storage API already owns its pool and argument values. Its view
// preserves that API's lazy provider order without inventing CPU stack stores.
template<class Read>class Value {
    std::uint32_t pool_,datum_,mask_,index_{},selected_{};
    Read&read_;bool admitted_{};
public:
    Value(std::uint32_t pool,std::uint32_t datum,std::uint32_t mask,Read&read)
        :pool_(pool),datum_(datum),mask_(mask),read_(read){}
    std::uint32_t begin()const{return datum_;}
    bool present(std::uint32_t datum)const{return datum!=0xffffffffu;}
    std::uint32_t objects()const{return pool_;}
    std::uint32_t limit(std::uint32_t pool){return read_(pool+0x38u,4u);}
    bool in_bounds(std::uint32_t datum,std::uint32_t limit){
        index_=datum&65535u;
        return std::int32_t(index_)<std::bit_cast<std::int32_t>(limit);
    }
    std::uint32_t record(std::uint32_t pool){
        const auto stride=read_(pool+0x24u,4u);
        return stride*index_+read_(pool+0x44u,4u);
    }
    std::uint16_t salt(std::uint32_t record){return std::uint16_t(read_(record,2u));}
    bool live(std::uint16_t salt)const{return salt!=0u;}
    bool matches(std::uint32_t datum,std::uint16_t salt)const{
        return std::bit_cast<std::int16_t>(salt)==
            std::bit_cast<std::int16_t>(std::uint16_t(datum>>16u));
    }
    void admit(std::uint32_t record){selected_=record;admitted_=true;}
    std::uint32_t finish(){
        if(!admitted_)return 0u;
        if(!(mask_&(1u<<(read_(selected_+3u,1u)&31u))))return 0u;
        return read_(selected_+8u,4u);
    }
};
template<class Read>std::uint32_t value(std::uint32_t pool,std::uint32_t datum,
    std::uint32_t mask,Read&&read){
    Value view{pool,datum,mask,read};return lookup(view);
}
} // namespace halo2::native_object_body_detail
