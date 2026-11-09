#pragma once
#include "halo2/native_tu15_resource_maintenance_kernel.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include "gameplay/weapons/native_tu15_effect_datum_allocate.hpp"
#include <cstdlib>

namespace halo2::native_resource_maintenance_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_RESOURCE_MAINTENANCE");return p&&*p;}();return enabled;}
template<class State>struct Access {
    using P=NativeTu15ResourceMaintenancePhase;State&s;
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;}
    static std::uint32_t signed_salt(std::uint32_t value){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));}
    void record(const NativeTu15ResourceMaintenance&d,bool secondary){
        auto&g=s.gp;g[1]=d.record;g[6]=d.datum;s.flags.logic(d.datum,4);if(!secondary)g[5]=d.index;
    }
    std::uint32_t read(P phase,const NativeTu15ResourceMaintenance&d,std::uint32_t address,unsigned width=4){
        auto&g=s.gp;
        switch(phase){
        case P::pool:g[4]=s.flags.minus(g[4],0x14u,4);at(0x1264e3u);s.push(0x1264e3u,g[3]);at(0x1264e4u);break;
        case P::enabled:at(0x1264eau);break;case P::limit:at(0x1264f5u);break;
        case P::primary_stride:at(0x126522u);break;case P::primary_rows:at(0x126525u);break;
        case P::primary_salt:
            g[1]=d.product;s.flags.load(native_effect_datum_allocate_detail::product_flags(d.stride,d.index));at(0x12652bu);break;
        case P::primary_mode:at(0x12653fu);break;case P::primary_enabled:at(0x126545u);break;
        case P::primary_owner:at(0x12654cu);break;case P::primary_handle:at(0x126551u);break;
        case P::primary_delete_pool:at(0x126572u);break;case P::secondary_pool:at(0x126586u);break;
        case P::secondary_stride:at(0x1265a1u);break;case P::secondary_rows:at(0x1265a4u);break;
        case P::secondary_salt:
            g[1]=d.product;s.flags.load(native_effect_datum_allocate_detail::product_flags(d.stride,d.index));at(0x1265aau);break;
        case P::secondary_enabled:at(0x1265c0u);break;case P::secondary_mode:at(0x1265c7u);break;
        case P::secondary_owner:at(0x1265cau);break;case P::secondary_handle:at(0x1265cfu);break;
        case P::secondary_delete_pool:at(0x1265edu);break;
        default:throw std::runtime_error("Invalid resource-maintenance read phase");
        }
        const auto value=physics_step_detail::r(s.memory,address,width);
        switch(phase){
        case P::pool:g[3]=value;break;
        case P::enabled:case P::primary_enabled:case P::secondary_enabled:g[0]=(g[0]&0xffffff00u)|value;s.flags.logic(value,1);break;
        case P::limit:s.flags.minus(value,25u,4);break;
        case P::primary_stride:case P::secondary_stride:g[1]=value;break;
        case P::primary_rows:g[5]=value;break;case P::secondary_rows:g[3]=value;break;
        case P::primary_salt:case P::secondary_salt:g[6]=signed_salt(value);break;
        case P::primary_mode:s.flags.minus(value,1u,4);break;
        case P::primary_owner:case P::secondary_owner:g[1]=value;break;
        case P::primary_handle:case P::secondary_handle:s.flags.minus(value,0xffffffffu,4);break;
        case P::primary_delete_pool:case P::secondary_delete_pool:g[2]=value;break;
        case P::secondary_pool:g[5]=value;break;
        case P::secondary_mode:g[3]=value;break;
        default:break;
        }
        return value;
    }
    std::uint32_t load_count(P phase,const NativeTu15ResourceMaintenance&d){
        if(phase==P::primary_count){record(d,false);at(0x126538u);}
        else if(phase==P::secondary_count)at(0x1265b9u);
        else at(phase==P::primary_increment?0x12657fu:0x1265fau);
        const auto value=physics_step_detail::r(s.memory,s.gp[4]+0x10u,4);
        if(phase==P::primary_count||phase==P::secondary_count)s.flags.minus(value,8u,4);
        else if(phase==P::primary_increment)s.gp[0]=value;
        return value;
    }
    void store_count(P phase,const NativeTu15ResourceMaintenance&,std::uint32_t value){
        if(phase==P::primary_store_count){s.gp[0]=s.flags.increment(value-1u,4,false);at(0x12650du);}
        else if(phase==P::secondary_store_count){s.flags.increment(value-1u,4,false);at(0x1265fau);}
        else at(0x12650du);
        const auto pc=phase==P::secondary_store_count?0x1265fau:0x12650du;
        physics_step_detail::w(s.memory,s.observe,pc,s.gp[4]+0x10u,4,value);
    }
    std::uint32_t load_cursor(const NativeTu15ResourceMaintenance&){
        at(0x126590u);return s.gp[2]=physics_step_detail::r(s.memory,s.gp[4]+0x20u,4);
    }
    void store_cursor(P phase,const NativeTu15ResourceMaintenance&,std::uint32_t value){
        const auto pc=phase==P::secondary_initialize?0x12658cu:0x1265b5u;at(pc);
        physics_step_detail::w(s.memory,s.observe,pc,s.gp[4]+0x20u,4,value);
    }
    void prepare(P phase,const NativeTu15ResourceMaintenance&d){
        auto&g=s.gp;
        switch(phase){
        case P::count_owner:at(0x126502u);s.push(0x126502u,g[5]);break;
        case P::initialize:at(0x126508u);s.push(0x126508u,g[6]);g[5]=s.flags.logic(0xffffffffu,4);at(0x12650cu);s.push(0x12650cu,g[7]);break;
        case P::primary_next:g[2]=g[5]+1u;g[1]=g[3];break;
        case P::secondary_next:g[2]=s.flags.increment(d.secondary_cursor,4,false);g[1]=g[5];break;
        case P::primary_allocate:g[0]=1u;g[7]=g[1];break;
        case P::secondary_allocate:g[0]=g[3];g[7]=g[1];break;
        case P::primary_schedule:case P::secondary_schedule:break;
        case P::primary_delete:case P::secondary_delete:g[0]=g[6];break;
        case P::secondary_initialize:break;
        case P::secondary_store_cursor:record(d,true);break;
        case P::finished:
            if(d.active){at(0x126600u);g[7]=s.pop();at(0x126601u);g[6]=s.pop();at(0x126602u);g[5]=s.pop();}
            at(0x126603u);g[3]=s.pop();g[4]=s.flags.plus(g[4],0x14u,4);at(0x126607u);s.pop();break;
        default:throw std::runtime_error("Invalid resource-maintenance transport phase");
        }
    }
};
template<class State,class Call>bool bridge(State&s,Call call){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        if(s.observe&&!c->observation_only_stores)return false;
        for(const auto site:c->service_sites)if(site>=0x1264e0u&&site<=0x126607u)return false;
    }
    Access<State>access{s};NativeTu15ResourceMaintenance data;
    struct Owners {
        State&s;Call&call;Access<State>&memory;
        void invoke(std::uint32_t target,std::uint32_t site,std::uint32_t continuation){memory.at(site);s.push(site,continuation);call(s,target,site,false);}
        NativeTu15ResourceMaintenanceCursor primary_cursor()const{return{s.gp[3],s.gp[5]};}
        std::uint32_t secondary_pool()const{return s.gp[5];}
        std::uint32_t secondary_mode()const{return s.gp[3];}
        std::uint32_t count(NativeTu15ResourceMaintenance&){invoke(0x126480u,0x126503u,0x126508u);return s.gp[0];}
        std::uint32_t next(NativeTu15ResourceMaintenance&,std::uint32_t,bool secondary){
            invoke(0x16ea60u,secondary?0x126597u:0x126516u,secondary?0x12659cu:0x12651bu);
            if(!secondary)s.gp[2]=s.flags.logic(0xffffffffu,4);s.flags.minus(s.gp[0],0xffffffffu,4);return s.gp[0];
        }
        std::uint32_t allocate(NativeTu15ResourceMaintenance&d,std::uint32_t,bool secondary){
            invoke(0x126060u,secondary?0x1265d9u:0x12655du,secondary?0x1265deu:0x126562u);
            d.datum=s.gp[6];d.resource=s.gp[7];s.flags.logic(s.gp[0],4);return s.gp[0];
        }
        void schedule(NativeTu15ResourceMaintenance&d,std::uint32_t mode,bool secondary){
            const auto first=secondary?0x1265e2u:0x126566u,second=secondary?0x1265e3u:0x126568u;
            memory.at(first);s.push(first,mode);memory.at(second);s.push(second,0x7000180u);
            invoke(0x1260c0u,secondary?0x1265e8u:0x12656du,secondary?0x1265edu:0x126572u);d.datum=s.gp[6];d.resource=s.gp[7];
        }
        void erase(NativeTu15ResourceMaintenance&d,bool secondary){
            invoke(0x16e8a0u,secondary?0x1265f5u:0x12657au,secondary?0x1265fau:0x12657fu);d.datum=s.gp[6];
        }
    }owners{s,call,access};
    native_tu15_maintain_resources(access,owners,data);return true;
#endif
}
} // namespace halo2::native_resource_maintenance_detail
