#include "halo2/native_tu15_island_regroup.hpp"
#include "halo2/native_tu15_contact_collector_info.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
class Regroup {
  SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;NativeTu15PhysicsFrameFp&fp_;
  const NativeTu15IslandRegroupServices&s_;NativeTu15IslandRegroupScratch scratch_;NativeTu15IslandRegroupStats*stats_;
public:
  Regroup(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15IslandRegroupServices&s,NativeTu15IslandRegroupScratch scratch,NativeTu15IslandRegroupStats*stats)
    :m_(m),o_(o),fp_(fp),s_(s),scratch_(scratch),stats_(stats){}
  std::uint32_t read(std::uint32_t p,unsigned size=4){std::array<std::byte,4>b{};m_.read(p,std::span(b).first(size));std::uint32_t v{};for(unsigned n=0;n<size;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
  void write(std::uint32_t site,std::uint32_t p,std::uint32_t v,unsigned size=4){std::array<std::byte,4>b{};for(unsigned n=0;n<size;++n)b[n]=std::byte(v>>(8u*n));SourcePhysicsContactRecordStore e{site,p,{},{}};if(o_){e.before.resize(size);m_.read(p,e.before);e.written.assign(b.begin(),b.begin()+size);}m_.write(p,std::span(b).first(size));if(o_)o_(e);}
  template<class F>void require(const F&f,const char*why){if(!f){if(stats_)++stats_->unbound_services;throw std::runtime_error(why);}}
  void reserve(std::uint32_t header,std::uint32_t capacity,std::uint32_t stride){require(s_.partition.reserve_array,"Native TU1.5 regroup requires actual array reserve");s_.partition.reserve_array({header},capacity,stride,fp_);}
  void ensure(std::uint32_t header,std::uint32_t current,std::uint32_t needed,std::uint32_t stride){if(less(current,needed)){auto capacity=current*2u;if(!less(needed,capacity))capacity=needed;reserve(header,capacity,stride);}}
  void grow(std::uint32_t header,std::uint32_t stride){NativeTu15ArrayGrowthMemory array(m_,{header},o_);native_tu15_grow_array(array,stride,fp_,s_.lifetime.allocation);if(stats_)++stats_->array_growths;}
  void destroy(NativeTu15PhysicsIsland island){const auto table=read(island.identity);const NativeTu15PhysicsVirtualOwner owner{read(table)};
    if(owner.identity==0x322100u)(void)native_tu15_destroy_physics_island(m_,island,1u,fp_,s_.lifetime,o_,stats_?&stats_->lifetime:nullptr);
    else{require(s_.destroy_island,"Native TU1.5 regroup requires actual island destructor");s_.destroy_island(island,owner,1u,fp_);}}
  std::uint32_t weight(std::uint32_t motion,NativeTu15PhysicsVirtualOwner owner){if(stats_)++stats_->motion_queries;if(owner.identity==0x323420u){if(stats_)++stats_->native_motion_queries;return native_tu15_default_motion_partition_weight();}require(s_.motion_weight,"Native TU1.5 regroup requires actual motion weight owner");return s_.motion_weight(motion,owner,fp_);}
  void enumerate(std::uint32_t action){const auto table=read(action);const NativeTu15PhysicsVirtualOwner owner{read(table+0x18u)};require(s_.edges.enumerate_action_bodies,"Native TU1.5 regroup requires actual action body enumeration");s_.edges.enumerate_action_bodies(action,owner,scratch_.action_bodies,fp_);}
  void constraint_info(std::uint32_t constraint,std::uint32_t table){const NativeTu15PhysicsVirtualOwner owner{read(table+0x28u)};if(stats_)++stats_->info_queries;
    if(owner.identity==0x325e90u){native_tu15_get_contact_collector_info(m_,constraint,scratch_.info,o_);if(stats_)++stats_->native_info_queries;}
    else{require(s_.constraint_info,"Native TU1.5 regroup requires actual constraint info owner");s_.constraint_info(constraint,owner,scratch_.info,fp_);}}
  void update_info(NativeTu15PhysicsIsland island,std::uint32_t constraint,bool adding){const auto table=read(island.identity);const NativeTu15PhysicsVirtualOwner owner{read(table+(adding?8u:12u))};
    if(adding&&owner.identity==0x3200b0u)native_tu15_add_island_constraint_info(m_,island,scratch_.info,o_);
    else if(!adding&&owner.identity==0x320100u)native_tu15_remove_island_constraint_info(m_,island,scratch_.info,o_);
    else{const auto&callback=adding?s_.add_info:s_.remove_info;require(callback,"Native TU1.5 regroup requires actual island info update owner");callback(island,owner,constraint,scratch_.info,fp_);}}
  void create_groups(NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland source){
    auto groups=read(scratch_.groups.identity+4u);std::uint32_t group=1u,offset=12u;
    if(less(1u,groups))do{
      const auto allocator=read(0x48c938u),table=read(allocator);const NativeTu15ArrayAllocationRequest request{allocator,read(table+0x10u),0xb0u,0x2du};
      require(s_.lifetime.allocation.allocate,"Native TU1.5 regroup requires actual new-island allocation");const auto data=s_.lifetime.allocation.allocate(request,fp_);write(0x318932u,data+4u,0xb0u,2);
      const auto target=native_tu15_construct_physics_island(m_,{data},world,o_);if(stats_)++stats_->created_islands;
      const auto capacity=read(world.identity+0x10u)&0x7fffffffu,header=world.identity+8u,count=read(header+4u);if(count==capacity)grow(header,4u);
      const auto index=read(header+4u),active=read(header);write(0x31897du,active+index*4u,target.identity);write(0x318980u,header+4u,read(header+4u)+1u);
      write(0x318987u,target.identity+0x24u,read(world.identity+0xcu)-1u);write(0x31898du,target.identity+0x30u,read(source.identity+0x30u,1),1);
      auto records=read(scratch_.groups.identity);auto body_capacity=read(target.identity+0x44u)&0x7fffffffu;auto wanted=read(records+offset+4u);const auto body_header=target.identity+0x3cu;
      if(less(body_capacity,wanted))reserve(body_header,wanted,4u);
      records=read(scratch_.groups.identity);body_capacity=read(body_header+8u)&0x7fffffffu;wanted=read(records+offset+4u);ensure(body_header,body_capacity,wanted,4u);write(0x3189e4u,body_header+4u,wanted);
      if(read(source.identity+0xaeu,1))write(0x3189f1u,target.identity+0xaeu,1u,1);
      records=read(scratch_.groups.identity);auto body_count=read(records+offset+4u);std::uint32_t index_in_group{};
      if(less(0,body_count))do{
        const auto indices=read(records+offset),body_index=read(indices+index_in_group*4u),bodies=read(source.identity+0x3cu),body=read(bodies+body_index*4u),motion=read(body+0x3cu);
        write(0x318a15u,body+0x44u,target.identity);write(0x318a18u,body+0x58u,index_in_group);const auto motion_table=read(motion);const auto amount=weight(motion,{read(motion_table+0x14u)});
        write(0x318a20u,source.identity+0x14u,read(source.identity+0x14u)-amount);write(0x318a2cu,target.identity+0x14u,read(target.identity+0x14u)+amount);
        const auto destination=read(target.identity+0x3cu);write(0x318a32u,destination+index_in_group*4u,body);records=read(scratch_.groups.identity);body_count=read(records+offset+4u);++index_in_group;if(stats_)++stats_->moved_bodies;
      }while(less(index_in_group,body_count));
      groups=read(scratch_.groups.identity+4u);++group;offset+=12u;
    }while(less(group,groups));
  }
  void compact_bodies(NativeTu15PhysicsIsland source){auto count=read(source.identity+0x40u);std::uint32_t index{};if(!less(0,count))return;
    do{const auto bodies=read(source.identity+0x3cu),body=read(bodies+index*4u);
      if(read(body+0x44u)!=source.identity){const auto last=read(source.identity+0x40u)-1u;write(0x318a7fu,source.identity+0x40u,last);const auto current=read(source.identity+0x3cu),retained=read(current+last*4u);write(0x318a8au,current+index*4u,retained);if(stats_)++stats_->compacted_bodies;}
      else{write(0x318a8fu,body+0x58u,index);++index;}count=read(source.identity+0x40u);
    }while(less(index,count));
  }
  void move_actions(NativeTu15PhysicsIsland source){auto index=read(source.identity+0x50u)-1u;if(less(index,0))return;
    do{
      write(0x318aa9u,scratch_.action_bodies.identity,scratch_.inline_bodies);write(0x318ab0u,scratch_.action_bodies.identity+4u,0);write(0x318abbu,scratch_.action_bodies.identity+8u,0x80000010u);
      auto actions=read(source.identity+0x4cu);auto action=read(actions+index*4u);enumerate(action);const auto count=read(scratch_.action_bodies.identity+4u);std::uint32_t target{},body_index{};
      if(less(0,count)){const auto bodies=read(scratch_.action_bodies.identity);do{const auto body=read(bodies+body_index*4u);target=read(body+0x44u);if(read(target+0x24u)!=0xffffffffu)break;++body_index;}while(less(body_index,count));}
      actions=read(source.identity+0x4cu);action=read(actions+index*4u);write(0x318b1cu,action+0x10u,target);
      actions=read(source.identity+0x4cu);const auto retained_slot=actions+index*4u;action=read(retained_slot);target=read(action+0x10u);
      if(target!=source.identity){const auto capacity=read(target+0x54u)&0x7fffffffu,count=read(target+0x50u),header=target+0x4cu;if(count==capacity)grow(header,4u);
        const auto destination_index=read(header+4u),retained=read(retained_slot),destination=read(header);write(0x318b55u,destination+destination_index*4u,retained);write(0x318b58u,header+4u,read(header+4u)+1u);
        const auto last=read(source.identity+0x50u)-1u;write(0x318b5fu,source.identity+0x50u,last);const auto current=read(source.identity+0x4cu),replacement=read(current+last*4u);write(0x318b6au,current+index*4u,replacement);if(stats_)++stats_->moved_actions;
      }
      const auto capacity=read(scratch_.action_bodies.identity+8u);if(!less(capacity,0)){const auto allocator=read(0x48c938u),table=read(allocator),data=read(scratch_.action_bodies.identity);const NativeTu15ArrayReleaseRequest request{allocator,read(table+0x14u),data,(capacity&0x7fffffffu)*4u,0x12u};require(s_.lifetime.allocation.release,"Native TU1.5 regroup requires actual action scratch release");s_.lifetime.allocation.release(request,fp_);}
      --index;
    }while(!less(index,0));
  }
  void move_constraints(NativeTu15PhysicsIsland source){auto index=read(source.identity+0x5cu)-1u;if(less(index,0))return;
    do{const auto entries=read(source.identity+0x58u),slot=entries+index*4u,constraint=read(slot),first=read(constraint+0xcu);auto target=read(first+0x44u);
      if(read(target+0x24u)==0xffffffffu){const auto second=read(constraint+0x10u);target=read(second+0x44u);}
      if(target!=source.identity){const auto retained=read(slot),last=read(source.identity+0x5cu)-1u;write(0x318bdcu,source.identity+0x5cu,last);const auto current=read(source.identity+0x58u),replacement=read(current+last*4u);write(0x318be7u,current+index*4u,replacement);
        const auto capacity=read(target+0x60u)&0x7fffffffu,count=read(target+0x5cu),header=target+0x58u;if(count==capacity)grow(header,4u);
        const auto destination_index=read(header+4u),destination=read(header);write(0x318c0cu,destination+destination_index*4u,retained);const auto next=read(header+4u)+1u;write(0x318c17u,header+4u,next);const auto info_table=read(retained);write(0x318c1fu,retained+0x14u,target);
        constraint_info(retained,info_table);update_info({target},retained,true);update_info(source,retained,false);if(stats_)++stats_->moved_constraints;
      }
      --index;
    }while(!less(index,0));
  }
  void move_contacts(NativeTu15PhysicsIsland source){auto count=read(source.identity+0x6cu);const auto header=source.identity+0x68u;std::uint32_t kept{},index{},source_offset{},retained_offset{};
    if(less(0,count))do{const auto data=read(header),first_payload=read(data+source_offset),record=data+source_offset,first=read(first_payload-0x10u+0x20u);auto target=read(first+0x44u);
      if(read(target+0x24u)==0xffffffffu){const auto second_payload=read(record+4u),second=read(second_payload-0x10u+0x20u);target=read(second+0x44u);}
      if(target!=source.identity){const auto target_count=read(target+0x6cu),target_header=target+0x68u,capacity=read(target_header+8u)&0x7fffffffu;if(target_count==capacity)grow(target_header,16u);
        const auto current_count=read(target_header+4u),destination_data=read(target_header),destination=destination_data+current_count*16u;
        write(0x318ccau,destination,read(record));write(0x318ccfu,destination+4u,read(record+4u));write(0x318cd5u,destination+8u,read(record+8u));write(0x318cdbu,destination+12u,read(record+12u));write(0x318cdeu,target_header+4u,read(target_header+4u)+1u);if(stats_)++stats_->moved_contacts;
      }else{const auto destination=data+retained_offset;write(0x318cefu,destination,read(record));write(0x318cf4u,destination+4u,read(record+4u));write(0x318cfau,destination+8u,read(record+8u));write(0x318d04u,destination+12u,read(record+12u));++kept;retained_offset+=16u;if(stats_)++stats_->retained_contacts;}
      count=read(header+4u);++index;source_offset+=16u;
    }while(less(index,count));
    const auto capacity=read(header+8u)&0x7fffffffu;ensure(header,capacity,kept,16u);write(0x318d52u,header+4u,kept);
  }
  void partition(NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland island){
    const auto actions=read(island.identity+0x50u),contacts=read(island.identity+0x6cu),constraints=read(island.identity+0x5cu),edge_capacity=contacts+actions*4u+constraints;
    native_tu15_construct_island_partition_temporary(m_,scratch_.edges,edge_capacity,NativeTu15IslandPartitionTemporaryKind::edges,fp_,s_.lifetime,o_,stats_?&stats_->lifetime:nullptr);
    native_tu15_collect_island_partition_edges(m_,island,scratch_.edges,scratch_.edge_actions,fp_,s_.edges,o_,stats_?&stats_->edges:nullptr);
    const auto vertices=read(island.identity+0x40u);native_tu15_construct_island_partition_temporary(m_,scratch_.parents,vertices,NativeTu15IslandPartitionTemporaryKind::indices,fp_,s_.lifetime,o_,stats_?&stats_->lifetime:nullptr);
    const auto edges=read(scratch_.edges.identity+4u);native_tu15_construct_island_partition_temporary(m_,scratch_.edge_roots,edges,NativeTu15IslandPartitionTemporaryKind::indices,fp_,s_.lifetime,o_,stats_?&stats_->lifetime:nullptr);
    native_tu15_construct_island_partition_graph(m_,scratch_.graph,scratch_.parents.identity,scratch_.edge_roots.identity,o_,stats_?&stats_->partition.graph:nullptr);
    const auto count=read(island.identity+0x40u);native_tu15_build_island_partition_graph(m_,scratch_.graph,scratch_.edges,count,fp_,s_.partition,o_,stats_?&stats_->partition:nullptr);
    const auto parents_header=read(scratch_.graph.identity),vertex_count=read(parents_header+4u),data=read(parents_header),root=read(data);
    if(root!=0u-vertex_count){
      write(0x3188d0u,scratch_.groups.identity,0);write(0x3188d4u,scratch_.groups.identity+4u,0);write(0x3188d8u,scratch_.groups.identity+8u,0x80000000u);
      native_tu15_extract_island_partition_groups(m_,scratch_.graph,scratch_.groups,fp_,s_.partition,o_,stats_?&stats_->partition:nullptr);
      create_groups(world,island);compact_bodies(island);move_actions(island);move_constraints(island);move_contacts(island);
      native_tu15_destroy_island_partition_groups(m_,scratch_.groups,fp_,s_.partition,o_,stats_?&stats_->partition:nullptr);
    }else if(stats_)++stats_->fully_connected;
    write(0x318d6eu,island.identity+0x28u,0,1);
    native_tu15_destroy_island_partition_temporary(m_,scratch_.edge_roots,NativeTu15IslandPartitionTemporaryKind::indices,fp_,s_.lifetime,o_,stats_?&stats_->lifetime:nullptr);
    native_tu15_destroy_island_partition_temporary(m_,scratch_.parents,NativeTu15IslandPartitionTemporaryKind::indices,fp_,s_.lifetime,o_,stats_?&stats_->lifetime:nullptr);
    native_tu15_destroy_island_partition_temporary(m_,scratch_.edges,NativeTu15IslandPartitionTemporaryKind::edges,fp_,s_.lifetime,o_,stats_?&stats_->lifetime:nullptr);
  }
  void run(NativeTu15PhysicsWorld world){if(stats_)++stats_->calls;auto index=read(world.identity+0xcu)-1u;if(less(index,0))return;
    do{const auto active=read(world.identity+8u);const NativeTu15PhysicsIsland island{read(active+index*4u)};if(stats_)++stats_->active_visits;
      if(read(island.identity+0x40u)==0){const auto current=read(world.identity+8u),count=read(world.identity+0xcu),last=read(current+count*4u-4u);write(0x31880cu,current+index*4u,last);const auto next_active=read(world.identity+8u),moved=read(next_active+index*4u);write(0x318815u,moved+0x24u,index);write(0x318818u,world.identity+0xcu,read(world.identity+0xcu)-1u);destroy(island);if(stats_)++stats_->empty_islands;}
      else if(read(island.identity+0x28u,1))partition(world,island);else if(stats_)++stats_->flag_skips;
      --index;
    }while(!less(index,0));
  }
};
}
void native_tu15_regroup_physics_islands(SourcePhysicsContactMemory&m,NativeTu15PhysicsWorld world,
 NativeTu15IslandRegroupScratch scratch,NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandRegroupServices&s,
 const SourcePhysicsContactRecordObserver&o,NativeTu15IslandRegroupStats*stats){Regroup(m,o,fp,s,scratch,stats).run(world);}
} // namespace halo2
