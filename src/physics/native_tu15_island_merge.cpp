#include "halo2/native_tu15_island_merge.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
class Merge {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
    const NativeTu15IslandMergeServices&s_;NativeTu15PhysicsFrameFp&fp_;NativeTu15IslandMergeStats*stats_;
public:
    Merge(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
      const NativeTu15IslandMergeServices&s,NativeTu15PhysicsFrameFp&fp,NativeTu15IslandMergeStats*stats)
      :m_(m),o_(o),s_(s),fp_(fp),stats_(stats){}
    std::uint32_t read(std::uint32_t p,unsigned width=4){std::array<std::byte,4>b{};m_.read(p,std::span(b).first(width));
        std::uint32_t v{};for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
    void store(std::uint32_t pc,std::uint32_t p,std::uint32_t v,unsigned width=4){
        std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(v>>(8u*n));SourcePhysicsContactRecordStore e{pc,p,{},{}};
        if(o_){e.before.resize(width);m_.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}m_.write(p,std::span(b).first(width));if(o_)o_(e);
    }
    template<class F>void require(const F&f,const char*message){if(!f){if(stats_)++stats_->unbound_services;throw std::runtime_error(message);}}
    void reserve(std::uint32_t header,std::uint32_t current,std::uint32_t needed,std::uint32_t stride){
        if(!less(current,needed))return;auto capacity=current*2u;if(!less(needed,capacity))capacity=needed;
        require(s_.reserve_array,"Native TU1.5 island merge requires actual2E31E0 array reserve");
        if(stats_)++stats_->array_reserves;s_.reserve_array({header},capacity,stride,fp_);
    }
    NativeTu15PhysicsIsland run(NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland first,NativeTu15PhysicsIsland second){
        if(stats_)++stats_->calls;const auto a=first.identity,b=second.identity;
        if(a==b){if(stats_)++stats_->same_island_skips;return first;}
        if(!read(a+0x29u,1)){require(s_.activate_island,"Native TU1.5 island merge requires actual316D40 activation");
            if(stats_)++stats_->island_activations;s_.activate_island(world,first,fp_);}
        if(!read(b+0x29u,1)){require(s_.activate_island,"Native TU1.5 island merge requires actual316D40 activation");
            if(stats_)++stats_->island_activations;s_.activate_island(world,second,fp_);}
        require(s_.merge_contact_records,"Native TU1.5 island merge requires actual318DE0 contact records");
        if(stats_)++stats_->contact_merges;s_.merge_contact_records({a+0x68u},{b+0x68u},fp_);
        auto target_index=read(a+0x40u);const auto added_bodies=read(b+0x40u),body_capacity=read(a+0x44u)&0x7fffffffu;
        const auto total_bodies=target_index+added_bodies;reserve(a+0x3cu,body_capacity,total_bodies,4);
        store(0x319141u,a+0x40u,total_bodies);auto count=read(b+0x40u);std::uint32_t index{};
        if(less(0,count))do{
            auto source=read(b+0x3cu),body=read(source+index*4u);const auto destination=read(a+0x3cu);
            store(0x319159u,destination+target_index*4u,body);
            source=read(b+0x3cu);body=read(source+index*4u);store(0x319162u,body+0x44u,a);
            source=read(b+0x3cu);body=read(source+index*4u);store(0x31916bu,body+0x58u,target_index);
            count=read(b+0x40u);++index;++target_index;if(stats_)++stats_->moved_bodies;
        }while(less(index,count));
        auto offset=read(a+0x50u);const auto added_actions=read(b+0x50u),action_capacity=read(a+0x54u)&0x7fffffffu;
        const auto action_header=a+0x4cu,total_actions=offset+added_actions;reserve(action_header,action_capacity,total_actions,4);
        store(0x3191aau,action_header+4u,total_actions);count=read(b+0x50u);index=0;
        if(less(0,count)){offset*=4u;do{
            auto source=read(b+0x4cu),action=read(source+index*4u);const auto destination=read(action_header);
            store(0x3191c9u,destination+offset,action);source=read(b+0x4cu);action=read(source+index*4u);
            store(0x3191d2u,action+0x10u,a);count=read(b+0x50u);++index;offset+=4u;if(stats_)++stats_->moved_actions;
        }while(less(index,count));}
        offset=read(a+0x5cu);const auto added_constraints=read(b+0x5cu),constraint_capacity=read(a+0x60u)&0x7fffffffu;
        const auto constraint_header=a+0x58u,total_constraints=offset+added_constraints;reserve(constraint_header,constraint_capacity,total_constraints,4);
        store(0x319213u,constraint_header+4u,total_constraints);count=read(b+0x5cu);index=0;
        if(less(0,count)){offset*=4u;do{
            const auto source=read(b+0x58u),constraint=read(source+index*4u),destination=read(constraint_header);
            store(0x31922fu,destination+offset,constraint);store(0x319232u,constraint+0x14u,a);
            count=read(b+0x5cu);++index;offset+=4u;if(stats_)++stats_->moved_constraints;
        }while(less(index,count));}
        auto maximum=read(b+8u);const auto first_maximum=read(a+8u);if(less(maximum,first_maximum))maximum=first_maximum;
        auto accumulated=read(a+0xcu),next=read(a+0x10u);store(0x319252u,a+8u,maximum);
        accumulated+=read(b+0xcu);auto third=read(a+0x18u);store(0x31925du,a+0xcu,accumulated);
        next+=read(b+0x10u);auto fourth=read(a+0x14u);store(0x319268u,a+0x10u,next);
        third+=read(b+0x18u);offset=read(a+0x78u);store(0x319273u,a+0x18u,third);
        fourth+=read(b+0x14u);store(0x31927bu,a+0x14u,fourth);
        const auto added_rows=read(b+0x78u),row_capacity=read(a+0x7cu)&0x7fffffffu,total_rows=offset+added_rows;
        reserve(a+0x74u,row_capacity,total_rows,8);store(0x3192aeu,a+0x78u,total_rows);count=read(b+0x78u);index=0;
        if(less(0,count)){offset*=8u;do{
            const auto source=read(b+0x74u),low=read(source+index*8u),destination=read(a+0x74u);
            store(0x3192c9u,destination+offset,low);const auto high=read(source+index*8u+4u);
            store(0x3192d0u,destination+offset+4u,high);count=read(b+0x78u);++index;offset+=8u;if(stats_)++stats_->moved_rows;
        }while(less(index,count));}
        const auto active=read(world.identity+8u),active_count=read(world.identity+0xcu),last=read(active+active_count*4u-4u);
        const auto removed_index=read(b+0x24u);store(0x3192f0u,active+removed_index*4u,last);
        const auto current_index=read(b+0x24u),current_active=read(world.identity+8u),moved=read(current_active+current_index*4u);
        store(0x3192fcu,moved+0x24u,current_index);store(0x3192ffu,world.identity+0xcu,read(world.identity+0xcu)-1u);
        auto partitioned=read(a+0x28u,1)!=0;if(!partitioned)partitioned=read(b+0x28u,1)!=0;
        store(0x319317u,a+0x28u,partitioned?1u:0u,1);if(read(b+0xaeu,1))store(0x319324u,a+0xaeu,1u,1);
        const auto table=read(b);const NativeTu15PhysicsVirtualOwner owner{read(table)};
        require(s_.destroy_island,"Native TU1.5 island merge requires its actual island destructor");
        if(stats_)++stats_->island_destructions;s_.destroy_island(second,owner,1u,fp_);return first;
    }
};
}
NativeTu15PhysicsIsland native_tu15_merge_physics_islands(SourcePhysicsContactMemory&m,
 NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland first,NativeTu15PhysicsIsland second,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandMergeServices&s,const SourcePhysicsContactRecordObserver&o,
 NativeTu15IslandMergeStats*stats){return Merge(m,o,s,fp,stats).run(world,first,second);}
} // namespace halo2
