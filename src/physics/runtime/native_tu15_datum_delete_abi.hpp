#pragma once
#include "halo2/native_tu15_datum_delete_kernel.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"

namespace halo2::native_datum_delete_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_DATUM_DELETE");return p&&*p;}();return enabled;}
struct Activity {std::uint64_t calls{};};
inline Activity &activity(){thread_local Activity value;return value;}
template<class State>struct Access {
    using P=NativeTu15DatumDeletePhase;
    State &s;bool canonical_store_sites{};
    std::uint32_t store_site(std::uint32_t pc)const{return canonical_store_sites?pc-0x13e0u:pc;}
    void push(std::uint32_t pc,std::uint32_t value){s.push(store_site(pc),value);}
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;}
    std::uint32_t read(P phase,const NativeTu15DatumDelete&,std::uint32_t address,unsigned width){
        auto &g=s.gp;
        switch(phase){
        case P::rows:at(0x16e8a0u);push(0x16e8a0u,g[3]);at(0x16e8a1u);push(0x16e8a1u,g[6]);g[3]=g[0];at(0x16e8a4u);break;
        case P::stride:at(0x16e8a7u);push(0x16e8a7u,g[7]);at(0x16e8a8u);break;
        case P::debug_flag:g[3]=s.flags.logic(g[3]&0xffffu,4);g[6]=s.flags.product(g[7],g[3]);g[6]=s.flags.plus(g[6],g[0],4);at(0x16e8b8u);break;
        case P::bitmap:at(0x16e8dbu);break;
        case P::bitmap_word:
            g[1]=s.flags.right(g[3],5u,4,true);g[0]+=g[1]*4u;g[1]=s.flags.logic(g[3]&31u,4);g[7]=s.flags.shift(1u,g[1],4);at(0x16e8f2u);break;
        case P::cursor:at(0x16e8ffu);break;
        case P::extent:at(0x16e907u);break;
        case P::shrink_stride:at(0x16e90fu);break;
        case P::shrink_extent:at(0x16e912u);break;
        case P::shrink_salt:at(0x16e921u);break;
        case P::occupancy:at(0x16e927u);break;
        default:throw std::runtime_error("Invalid native datum-delete read boundary");
        }
        const auto value=physics_step_detail::r(s.memory,address,width);
        switch(phase){
        case P::rows:case P::bitmap:case P::shrink_stride:case P::occupancy:g[0]=value;break;
        case P::stride:g[7]=value;break;
        case P::debug_flag:s.set(0,value,1,0);s.set(0,s.flags.right(s.reg(0,1,0),3u,1,false),1,0);s.flags.logic(s.reg(0,1,0)&1u,1);break;
        case P::bitmap_word:g[1]=value;g[7]=~g[7];g[1]=s.flags.logic(g[1]&g[7],4);break;
        case P::cursor:s.flags.minus(g[3],value,4);break;
        case P::extent:g[0]=value;g[3]=s.flags.increment(g[3],4,false);s.flags.minus(g[3],g[0],4);break;
        case P::shrink_extent:g[7]=value;g[6]=s.flags.minus(g[6],g[0],4);g[7]=s.flags.increment(g[7],4,true);g[1]=g[7];s.flags.logic(g[1],4);break;
        case P::shrink_salt:s.flags.minus(value,0,2);break;
        default:break;
        }
        return value;
    }
    void write(P phase,const NativeTu15DatumDelete&,std::uint32_t address,std::uint32_t value,unsigned width){
        std::uint32_t pc{};switch(phase){
        case P::clear_word:pc=0x16e8d1u;break;case P::clear_byte:pc=0x16e8d8u;break;
        case P::bitmap_store:pc=0x16e8f8u;break;case P::salt:pc=0x16e8fau;break;
        case P::cursor_store:pc=0x16e904u;break;case P::shrink_store:pc=0x16e91cu;break;
        case P::occupancy_store:pc=0x16e92du;break;
        default:throw std::runtime_error("Invalid native datum-delete write boundary");
        }
        at(pc);physics_step_detail::w(s.memory,s.observe,store_site(pc),address,width,value);
    }
    void prepare(P phase,const NativeTu15DatumDelete&){
        auto &g=s.gp;switch(phase){
        case P::clear_begin:g[1]=g[7];at(0x16e8c4u);push(0x16e8c4u,g[5]);g[5]=g[1];g[1]=s.flags.right(g[1],2u,4,false);g[0]=0xbabababau;g[7]=g[6];break;
        case P::clear_tail:g[1]=s.flags.logic(g[5]&3u,4);break;
        case P::clear_end:at(0x16e8dau);g[5]=s.pop();break;
        case P::finish:at(0x16e92au);g[7]=s.pop();g[0]=s.flags.increment(g[0],4,true);at(0x16e92cu);g[6]=s.pop();break;
        default:break;
        }
    }
    void complete(P phase,const NativeTu15DatumDelete &data){
        auto &g=s.gp;switch(phase){
        case P::clear_word:g[7]=data.clear_at;g[1]=data.word_count;break;
        case P::clear_byte:g[7]=data.clear_at;g[1]=data.tail_count;break;
        case P::finished:at(0x16e930u);g[3]=s.pop();at(0x16e931u);s.pop();break;
        default:break;
        }
    }
};
template<class State>bool run(State &s,bool canonical_store_sites=false){
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().calls;NativeTu15DatumDelete data;data.pool=s.gp[2];data.datum=s.gp[0];Access<State> access{s,canonical_store_sites};
    native_tu15_delete_datum(access,data,(s.other_flags&1024u)!=0);return true;
}
} // namespace halo2::native_datum_delete_detail
