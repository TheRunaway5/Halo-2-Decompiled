#pragma once
#include "halo2/native_tu15_page_retire_kernel.hpp"
#include "physics/runtime/native_tu15_page_window_abi.hpp"
#include "physics/runtime/native_tu15_datum_delete_abi.hpp"
#include <map>

namespace halo2::native_page_retire_detail {
struct Activity {std::uint64_t calls{},native_erases{},reference_erases{};std::map<std::uint32_t,std::uint64_t> callbacks,erasures;};
inline Activity &activity(){thread_local Activity value;return value;}
template<class State,class Before=native_page_window_detail::Noop>struct Access {
    using P=NativeTu15PageRetirePhase;State &s;Before before{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    std::uint32_t read(P phase,const NativeTu15PageRetirement &data,std::uint32_t address){
        auto &g=s.gp;
        switch(phase){
        case P::pool:at(0x140310u);break;case P::rows:at(0x140313u);break;
        case P::callback:
            g[0]=s.flags.logic(g[3]&0xffffu,4);g[0]*=3u;at(0x140320u);s.push(0x140320u,g[6]);g[6]=g[2]+g[0]*8u;at(0x140324u);break;
        case P::previous:at(0x14032eu);break;
        case P::previous_pool:at(0x140336u);break;case P::previous_rows:at(0x140339u);break;case P::next_for_previous:at(0x14033cu);break;
        case P::next_for_head:at(0x14034du);break;case P::next:at(0x140353u);break;
        case P::next_pool:at(0x14035bu);break;case P::next_rows:at(0x14035eu);break;case P::previous_for_next:at(0x140361u);break;
        case P::previous_for_tail:at(0x14037bu);break;
        case P::delete_pool:at(data.next==0xffffffffu?0x140381u:0x140370u);break;
        default:throw std::runtime_error("Invalid native page retirement read boundary");
        }
        const auto value=physics_step_detail::r(s.memory,address,4);
        switch(phase){
        case P::pool:case P::previous_pool:case P::next_pool:g[1]=value;break;
        case P::rows:case P::previous_rows:case P::next_rows:case P::next_for_head:case P::previous_for_tail:case P::delete_pool:g[2]=value;break;
        case P::callback:g[0]=value;s.flags.logic(value,4);break;
        case P::previous:case P::next:g[0]=value;s.flags.minus(value,0xffffffffu,4);break;
        case P::next_for_previous:case P::previous_for_next:g[1]=value;g[0]=s.flags.logic(g[0]&0xffffu,4)*3u;break;
        default:break;
        }
        return value;
    }
    void write(P phase,const NativeTu15PageRetirement&,std::uint32_t address,std::uint32_t value){
        std::uint32_t pc{};switch(phase){case P::previous_store:pc=0x140347u;break;case P::head_store:pc=0x140350u;break;case P::next_store:pc=0x14036cu;break;case P::tail_store:pc=0x14037eu;break;default:throw std::runtime_error("Invalid page retirement store boundary");}
        at(pc);physics_step_detail::w(s.memory,s.observe,pc,address,4,value);
    }
    void prepare(P,const NativeTu15PageRetirement &data){
        s.gp[0]=s.gp[3];at(data.next==0xffffffffu?0x140386u:0x140375u);s.gp[6]=s.pop();at(data.next==0xffffffffu?0x140387u:0x140376u);
    }
};
template<class State,class Call,class Before=native_page_window_detail::Noop>bool run(State &s,Call call,Before before={}){
    if(native_page_window_detail::reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().calls;NativeTu15PageRetirement data;data.owner=s.gp[7];data.datum=s.gp[3];Access<State,Before> access{s,before};
    struct Services {
        State &s;Call &call;Access<State,Before> &access;
        NativeTu15PageRetireResume callback(std::uint32_t owner,std::uint32_t,const NativeTu15PageRetirement&){
            ++activity().callbacks[owner];access.at(0x14032bu);s.push(0x14032bu,s.gp[3]);access.at(0x14032cu);s.push(0x14032cu,0x14032eu);call(s,owner,0x14032cu,false);
            return{s.gp[7],s.gp[3],s.gp[6]};
        }
        void erase(std::uint32_t,std::uint32_t,const NativeTu15PageRetirement &value){
            ++activity().erasures[0x16e8a0u];access.before(s,0x16e8a0u);
            if(native_datum_delete_detail::run(s)){++activity().native_erases;return;}
            ++activity().reference_erases;call(s,0x16e8a0u,value.next==0xffffffffu?0x140387u:0x140376u,true);
        }
    }services{s,call,access};
    native_tu15_retire_page(access,data,services);return true;
}
} // namespace halo2::native_page_retire_detail
