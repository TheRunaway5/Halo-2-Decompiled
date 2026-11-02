#include "halo2/native_tu15_island_partition_edges.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
class Edges {
  SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;NativeTu15PhysicsFrameFp&fp_;
  const NativeTu15IslandPartitionEdgeServices&s_;NativeTu15IslandPartitionEdgeStats*stats_;
public:
  Edges(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15IslandPartitionEdgeServices&s,NativeTu15IslandPartitionEdgeStats*stats):m_(m),o_(o),fp_(fp),s_(s),stats_(stats){}
  std::uint32_t read(std::uint32_t p,unsigned size=4){std::array<std::byte,4>b{};m_.read(p,std::span(b).first(size));std::uint32_t v{};for(unsigned n=0;n<size;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
  void write(std::uint32_t site,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8u*n));SourcePhysicsContactRecordStore e{site,p,{},{}};if(o_){e.before.resize(4);m_.read(p,e.before);e.written.assign(b.begin(),b.end());}m_.write(p,b);if(o_)o_(e);}
  template<class F>void require(const F&f,const char*why){if(!f){if(stats_)++stats_->unbound_services;throw std::runtime_error(why);}}
  bool reserve(NativeTu15ArrayHeader header,std::uint32_t current,std::uint32_t needed){
    if(!less(current,needed))return false;auto capacity=current*2u;if(!less(needed,capacity))capacity=needed;
    require(s_.reserve_array,"Native TU1.5 partition edges require actual2E31E0 reserve");if(stats_)++stats_->array_reserves;s_.reserve_array(header,capacity,8u,fp_);return true;
  }
  void run(NativeTu15PhysicsIsland island,NativeTu15ArrayHeader output,NativeTu15IslandPartitionEdgeScratch scratch){
    if(stats_)++stats_->calls;write(0x321ee1u,output.identity+4u,0);write(0x321ee4u,scratch.action_bodies.identity,scratch.inline_bodies);write(0x321ee8u,scratch.action_bodies.identity+4u,0);write(0x321eecu,scratch.action_bodies.identity+8u,0x8000000au);
    auto count=read(island.identity+0x50u);std::uint32_t action_index{},action_data{},action_capacity=0x8000000au;
    if(less(0,count)){
      do{
        const auto entries=read(island.identity+0x4cu);write(0x321f0au,scratch.action_bodies.identity+4u,0);
        const auto action=read(entries+action_index*4u),table=read(action);const NativeTu15PhysicsVirtualOwner owner{read(table+0x18u)};
        require(s_.enumerate_action_bodies,"Native TU1.5 partition requires actual action body enumeration");if(stats_)++stats_->actions;s_.enumerate_action_bodies(action,owner,scratch.action_bodies,fp_);
        auto body_count=read(scratch.action_bodies.identity+4u);action_data=read(scratch.action_bodies.identity);std::uint32_t candidate{},anchor=0xffffffffu;
        while(less(candidate,body_count)){
          const auto body=read(action_data+candidate*4u);if(!read(body+0x40u,1))anchor=candidate;++candidate;if(anchor!=0xffffffffu)break;
        }
        if(anchor!=0xffffffffu&&less(candidate,body_count))do{
          const auto body=read(action_data+candidate*4u);
          if(!read(body+0x40u,1)){
            const auto needed=read(output.identity+4u)+1u,capacity=read(output.identity+8u)&0x7fffffffu;
            if(reserve(output,capacity,needed))action_data=read(scratch.action_bodies.identity);
            const auto data=read(output.identity);write(0x321f76u,output.identity+4u,needed);
            const auto first=read(action_data+anchor*4u),destination=data+needed*8u-8u,first_index=read(first+0x58u);
            write(0x321f87u,destination,first_index);action_data=read(scratch.action_bodies.identity);
            const auto second=read(action_data+candidate*4u),second_index=read(second+0x58u);write(0x321f93u,destination+4u,second_index);
            body_count=read(scratch.action_bodies.identity+4u);action_data=read(scratch.action_bodies.identity);if(stats_)++stats_->action_pairs;
          }
          ++candidate;
        }while(less(candidate,body_count));
        count=read(island.identity+0x50u);++action_index;
      }while(less(action_index,count));
      action_capacity=read(scratch.action_bodies.identity+8u);
    }
    if(!less(action_capacity,0)){
      const auto allocator=read(0x48c938u),table=read(allocator);const NativeTu15ArrayReleaseRequest request{allocator,read(table+0x14u),action_data,(action_capacity&0x7fffffffu)*4u,0x12u};
      require(s_.allocation.release,"Native TU1.5 partition action scratch requires actual owned-array release");if(stats_)++stats_->action_array_releases;s_.allocation.release(request,fp_);
    }
    count=read(island.identity+0x5cu);std::uint32_t index{};
    if(less(0,count))do{
      const auto entries=read(island.identity+0x58u),constraint=read(entries+index*4u),first=read(constraint+0xcu);
      if(!read(first+0x40u,1)){
        const auto second=read(constraint+0x10u);if(!read(second+0x40u,1)){
          const auto needed=read(output.identity+4u)+1u,capacity=read(output.identity+8u)&0x7fffffffu;reserve(output,capacity,needed);
          const auto data=read(output.identity);write(0x322034u,output.identity+4u,needed);
          const auto current_entries=read(island.identity+0x58u),current_constraint=read(current_entries+index*4u),current_first=read(current_constraint+0xcu),first_index=read(current_first+0x58u),destination=data+needed*8u-8u;
          write(0x32204bu,destination,first_index);const auto next_entries=read(island.identity+0x58u),next_constraint=read(next_entries+index*4u),current_second=read(next_constraint+0x10u),second_index=read(current_second+0x58u);
          write(0x322059u,destination+4u,second_index);if(stats_)++stats_->constraint_pairs;
        }
      }
      count=read(island.identity+0x5cu);++index;
    }while(less(index,count));
    count=read(island.identity+0x6cu);index=0;std::uint32_t offset{};
    if(less(0,count))do{
      const auto entries=read(island.identity+0x68u),first_payload=read(entries+offset),first=read(first_payload+0x10u),second_payload=read(entries+offset+4u),second=read(second_payload+0x10u);
      if(!read(first+0x40u,1)&&!read(second+0x40u,1)){
        const auto needed=read(output.identity+4u)+1u,capacity=read(output.identity+8u)&0x7fffffffu;reserve(output,capacity,needed);
        const auto data=read(output.identity);write(0x3220bau,output.identity+4u,needed);const auto first_index=read(first+0x58u),destination=data+needed*8u-8u;
        write(0x3220c8u,destination,first_index);const auto second_index=read(second+0x58u);write(0x3220cdu,destination+4u,second_index);if(stats_)++stats_->contact_pairs;
      }
      count=read(island.identity+0x6cu);++index;offset+=16u;
    }while(less(index,count));
  }
};
}
void native_tu15_collect_island_partition_edges(SourcePhysicsContactMemory&m,NativeTu15PhysicsIsland island,
 NativeTu15ArrayHeader output,NativeTu15IslandPartitionEdgeScratch scratch,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15IslandPartitionEdgeServices&s,const SourcePhysicsContactRecordObserver&o,
 NativeTu15IslandPartitionEdgeStats*stats){Edges(m,o,fp,s,stats).run(island,output,scratch);}
} // namespace halo2
