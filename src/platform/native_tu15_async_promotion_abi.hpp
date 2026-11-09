#pragma once
#include "halo2/native_tu15_async_promotion_kernel.hpp"
#include "platform/native_tu15_async_order.hpp"

namespace halo2::native_async_promotion_abi_detail {
template<class State,class Insert,class Before>struct Frame:native_async_queue_abi_detail::Storage<State,Before> {
    using Base=native_async_queue_abi_detail::Storage<State,Before>;
    using Base::s;using Base::at;using Base::word;Insert&insertion;
    Frame(State&state,Insert&insert,Before&before,native_raw_x87_detail::Scope<State>&scope):Base{state,0x1236a0u,before,scope},insertion(insert){}
    void begin(const NativeAsyncPromotion&){
        at(0);s.gp[0]=word(0x4ecc2cu);this->push(5,s.gp[3]);this->push(6,0xffffffffu);this->push(8,s.gp[0]);
        s.gp[3]&=0xffffff00u;s.flags.logic(0,1);this->push(0xb,0x1236b0u);
        this->scope.child([&]{s.required(0x1236abu,0x2e0478u,false);});
    }
    std::uint32_t job(const NativeAsyncPromotion&)const{return s.gp[6];}
    std::uint32_t node(const NativeAsyncPromotion&d){return s.gp[0]=s.flags.plus((d.job&255u)<<6u,0x4ecc38u,4);}
    std::uint32_t read(NativeAsyncPromotionRead kind,const NativeAsyncPromotion&d){
        using R=NativeAsyncPromotionRead;unsigned offset{};std::uint32_t address{};int destination=-1;
        switch(kind){
        case R::owner:offset=0x1f;address=d.node+4u;break;
        case R::head:offset=0x24;address=0x4ef1bcu;destination=1;break;
        case R::head_next:s.flags.minus(d.head,d.node,4);offset=0x2e;address=d.node+0x3cu;destination=1;break;
        case R::initial_link:s.flags.minus(d.head,d.node,4);offset=0x39;address=d.previous+0x3cu;break;
        case R::advance:offset=0x40;address=d.previous+0x3cu;destination=1;break;
        case R::advanced_link:offset=0x43;address=d.previous+0x3cu;break;
        case R::node_next:offset=0x48;address=d.node+0x3cu;destination=2;break;
        case R::priority:offset=0x4e;address=s.gp[4]+8u;destination=1;break;
        }
        at(offset);const auto value=word(address);if(destination>=0)s.gp[unsigned(destination)]=value;
        if(kind==R::owner)s.flags.minus(value,d.job,4);
        else if(kind==R::initial_link||kind==R::advanced_link)s.flags.minus(value,d.node,4);
        return value;
    }
    void write(NativeAsyncPromotionWrite kind,const NativeAsyncPromotion&d,std::uint32_t value){
        using W=NativeAsyncPromotionWrite;
        switch(kind){
        case W::head:this->store(0x31,0x4ef1bcu,value);break;
        case W::previous_next:this->store(0x4b,d.previous+0x3cu,value);break;
        case W::priority:this->store(0x53,s.gp[0],value);break;
        }
    }
    void prepare_insert(const NativeAsyncPromotion&){this->push(0x52,s.gp[0]);}
    void insert(const NativeAsyncPromotion&){this->push(0x55,0x1236fau);this->scope.child([&]{insertion(s);});}
    void promoted(const NativeAsyncPromotion&){s.gp[3]=(s.gp[3]&0xffffff00u)|1u;}
    void finish(const NativeAsyncPromotion&){
        at(0x5c);s.gp[2]=word(0x4ecc2cu);this->push(0x62,s.gp[2]);this->push(0x63,0x123708u);
        this->scope.child([&]{s.required(0x123703u,0x2e0342u,false);});
        s.gp[0]=(s.gp[0]&0xffffff00u)|(s.gp[3]&255u);s.gp[3]=this->pop(0x6a);this->pop(0x6b);s.gp[4]+=4u;
    }
};
template<class State,class Insert,class Before=native_async_queue_abi_detail::Noop>
bool promote(State&s,Insert&&insert,Before before={}){
    if(!native_async_queue_abi_detail::admitted(s))return false;
    native_raw_x87_detail::Scope scope{s};Frame<State,Insert,Before>frame{s,insert,before,scope};
    NativeAsyncPromotion data;native_async_promote(frame,data);return true;
}
} // namespace halo2::native_async_promotion_abi_detail
