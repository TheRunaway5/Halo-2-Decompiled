#include "halo2/native_tu15_island_partition_build.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool negative(std::uint32_t n){return std::bit_cast<std::int32_t>(n)<0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
class Partition {
  SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
  NativeTu15PhysicsFrameFp&fp_;const NativeTu15IslandPartitionBuildServices&s_;NativeTu15IslandPartitionBuildStats*stats_;
public:
  Partition(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15IslandPartitionBuildServices&s,NativeTu15IslandPartitionBuildStats*stats):m_(m),o_(o),fp_(fp),s_(s),stats_(stats){}
  std::uint32_t read(std::uint32_t p){std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t n{};
    for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;}
  void write(std::uint32_t site,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};
    for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(8u*k));SourcePhysicsContactRecordStore e{site,p,{},{}};
    if(o_){e.before.resize(4);m_.read(p,e.before);e.written.assign(b.begin(),b.end());}m_.write(p,b);if(o_)o_(e);
  }
  template<class F>void require(const F&f,const char*why){if(!f){if(stats_)++stats_->unbound_services;throw std::runtime_error(why);}}
  void reserve(std::uint32_t header,std::uint32_t current,std::uint32_t needed,std::uint32_t stride){
    if(!less(current,needed))return;auto capacity=current*2u;if(!less(needed,capacity))capacity=needed;
    require(s_.reserve_array,"Native TU1.5 partition requires actual2E31E0 array reserve");
    if(stats_)++stats_->array_reserves;s_.reserve_array({header},capacity,stride,fp_);
  }
  void build(NativeTu15IslandPartitionGraph graph,NativeTu15ArrayHeader edge_header,std::uint32_t vertices){
    if(stats_)++stats_->builds;const auto parents_header=read(graph.identity),capacity=read(parents_header+8u)&0x7fffffffu;
    reserve(parents_header,capacity,vertices,4u);write(0x37c300u,parents_header+4u,vertices);
    for(std::uint32_t index=0;less(index,vertices);++index){const auto header=read(graph.identity),data=read(header);write(0x37c309u,data+index*4u,0xffffffffu);}
    auto roots_header=read(graph.identity+4u);write(0x37c31cu,roots_header+4u,0);
    roots_header=read(graph.identity+4u);const auto roots_capacity=read(roots_header+8u)&0x7fffffffu,edges=read(edge_header.identity+4u);
    reserve(roots_header,roots_capacity,edges,4u);
    roots_header=read(graph.identity+4u);const auto next_capacity=read(roots_header+8u)&0x7fffffffu,edge_count=read(edge_header.identity+4u);
    reserve(roots_header,next_capacity,edge_count,4u);write(0x37c36fu,roots_header+4u,edge_count);
    auto count=read(edge_header.identity+4u);std::uint32_t index{};
    if(less(0,count))do{
      const auto data=read(edge_header.identity),second=read(data+index*8u+4u),root_header=read(graph.identity+4u),root_data=read(root_header),first=read(data+index*8u);
      const auto root=less(first,second)?native_tu15_union_island_partition_vertices(m_,graph,first,second,o_,stats_?&stats_->graph:nullptr):
        native_tu15_union_island_partition_vertices(m_,graph,second,first,o_,stats_?&stats_->graph:nullptr);
      write(0x37c3a5u,root_data+index*4u,root);count=read(edge_header.identity+4u);++index;
    }while(less(index,count));
    native_tu15_compress_island_partition_graph(m_,graph,o_,stats_?&stats_->graph:nullptr);write(0x37c3b7u,graph.identity+8u,0);
  }
  void extract(NativeTu15IslandPartitionGraph graph,NativeTu15ArrayHeader groups){
    if(stats_)++stats_->extractions;auto header=read(graph.identity),count=read(header+4u);std::uint32_t index{};
    if(less(0,count))do{
      const auto data=read(header),parent=read(data+index*4u);
      if(negative(parent)){
        const auto needed=read(groups.identity+4u)+1u,capacity=read(groups.identity+8u)&0x7fffffffu;
        reserve(groups.identity,capacity,needed,12u);const auto records=read(groups.identity);write(0x37c4aeu,groups.identity+4u,needed);
        const auto current_header=read(graph.identity),record=records+needed*12u-12u,parents=read(current_header),size=0u-read(parents+index*4u);
        const auto allocator=read(0x48c938u),table=read(allocator);const NativeTu15PhysicsVirtualOwner owner{read(table)};
        require(s_.allocate_group,"Native TU1.5 partition requires actual raw group allocator");if(stats_)++stats_->group_allocations;
        const auto replacement=s_.allocate_group(allocator,owner,size*4u,1u,fp_);write(0x37c4d2u,record,replacement);
        const auto new_header=read(graph.identity),new_data=read(new_header),new_size=0u-read(new_data+index*4u);
        write(0x37c4deu,record+4u,new_size);write(0x37c4e1u,replacement,index);write(0x37c4e3u,record+8u,1u);
        const auto group_index=read(groups.identity+4u)-1u;header=read(graph.identity);const auto current_data=read(header);
        write(0x37c4f3u,current_data+index*4u,group_index);
      }else{
        const auto parents=read(header),root=read(parents+index*4u),group_index=read(parents+root*4u),records=read(groups.identity);
        const auto offset=group_index*12u,cursor=read(records+offset+8u),record=records+offset,allocation=read(record);
        write(0x37c50eu,allocation+cursor*4u,index);write(0x37c511u,record+8u,read(record+8u)+1u);
      }
      header=read(graph.identity);count=read(header+4u);++index;
    }while(less(index,count));
    const auto parent_header=read(graph.identity),parent_capacity=read(parent_header+8u)&0x7fffffffu;
    // AND7FFFFFFF makes the original JGE unconditional for every source word.
    if(negative(parent_capacity))reserve(parent_header,parent_capacity,0,4u);
    write(0x37c54bu,parent_header+4u,0);const auto roots_header=read(graph.identity+4u),roots_capacity=read(roots_header+8u)&0x7fffffffu;
    if(negative(roots_capacity))reserve(roots_header,roots_capacity,0,4u);write(0x37c573u,roots_header+4u,0);
  }
  void destroy(NativeTu15ArrayHeader groups){
    if(stats_)++stats_->destructions;auto count=read(groups.identity+4u);std::uint32_t index{},offset{};
    if(less(0,count))do{
      const auto records=read(groups.identity),data=read(records+offset),allocator=read(0x48c938u),table=read(allocator);const NativeTu15PhysicsVirtualOwner owner{read(table+4u)};
      require(s_.release_group,"Native TU1.5 partition requires actual raw group release");if(stats_)++stats_->group_releases;s_.release_group(allocator,owner,data,fp_);
      count=read(groups.identity+4u);++index;offset+=12u;
    }while(less(index,count));
    const auto capacity=read(groups.identity+8u);if(!negative(capacity)){
      const auto allocator=read(0x48c938u),table=read(allocator),data=read(groups.identity);
      const NativeTu15ArrayReleaseRequest request{allocator,read(table+0x14u),data,(capacity&0x7fffffffu)*12u,0x12u};
      require(s_.release_array,"Native TU1.5 partition requires actual group-header array release");if(stats_)++stats_->array_releases;s_.release_array(request,fp_);
    }
  }
};
}
void native_tu15_build_island_partition_graph(SourcePhysicsContactMemory&m,NativeTu15IslandPartitionGraph graph,
 NativeTu15ArrayHeader edges,std::uint32_t vertices,NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandPartitionBuildServices&s,
 const SourcePhysicsContactRecordObserver&o,NativeTu15IslandPartitionBuildStats*stats){Partition(m,o,fp,s,stats).build(graph,edges,vertices);}
void native_tu15_extract_island_partition_groups(SourcePhysicsContactMemory&m,NativeTu15IslandPartitionGraph graph,
 NativeTu15ArrayHeader groups,NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandPartitionBuildServices&s,
 const SourcePhysicsContactRecordObserver&o,NativeTu15IslandPartitionBuildStats*stats){Partition(m,o,fp,s,stats).extract(graph,groups);}
void native_tu15_destroy_island_partition_groups(SourcePhysicsContactMemory&m,NativeTu15ArrayHeader groups,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandPartitionBuildServices&s,const SourcePhysicsContactRecordObserver&o,
 NativeTu15IslandPartitionBuildStats*stats){Partition(m,o,fp,s,stats).destroy(groups);}
} // namespace halo2
