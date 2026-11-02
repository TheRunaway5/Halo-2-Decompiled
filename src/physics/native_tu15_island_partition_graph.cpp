#include "halo2/native_tu15_island_partition_graph.hpp"
#include <bit>
namespace halo2 {
namespace {
bool negative(std::uint32_t n){return std::bit_cast<std::int32_t>(n)<0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
class GraphMemory {
  SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
  NativeTu15IslandPartitionGraphStats*stats_;
public:
  GraphMemory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    NativeTu15IslandPartitionGraphStats*stats):m_(m),o_(o),stats_(stats){}
  std::uint32_t read(std::uint32_t p){std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t n{};
    for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;}
  void write(std::uint32_t site,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};
    for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(8u*k));SourcePhysicsContactRecordStore e{site,p,{},{}};
    if(o_){e.before.resize(4);m_.read(p,e.before);e.written.assign(b.begin(),b.end());}
    m_.write(p,b);if(stats_)++stats_->parent_publications;if(o_)o_(e);
  }
};
}
NativeTu15IslandPartitionGraph native_tu15_construct_island_partition_graph(
 SourcePhysicsContactMemory&memory,NativeTu15IslandPartitionGraph graph,std::uint32_t parents,
 std::uint32_t edge_roots,const SourcePhysicsContactRecordObserver&observe,NativeTu15IslandPartitionGraphStats*stats){
  if(stats)++stats->constructors;GraphMemory m(memory,observe,stats);
  m.write(0x37c17au,graph.identity,parents);m.write(0x37c17cu,graph.identity+4u,edge_roots);return graph;
}
std::uint32_t native_tu15_union_island_partition_vertices(SourcePhysicsContactMemory&memory,
 NativeTu15IslandPartitionGraph graph,std::uint32_t first,std::uint32_t second,
 const SourcePhysicsContactRecordObserver&observe,NativeTu15IslandPartitionGraphStats*stats){
  if(stats)++stats->unions;GraphMemory m(memory,observe,stats);
  auto first_header=m.read(graph.identity),parents=m.read(first_header);
  const auto first_parent=m.read(parents+first*4u);auto first_root=first;
  if(!negative(first_parent)){
    auto next=first_parent;
    do{first_root=next;next=m.read(parents+first_root*4u);}while(!negative(next));
    if(!negative(first_parent)){
      auto cursor=first;
      do{const auto data=m.read(first_header),slot=data+cursor*4u;cursor=m.read(slot);
        m.write(0x37c1bbu,slot,first_root);first_header=m.read(graph.identity);parents=m.read(first_header);
      }while(!negative(m.read(parents+cursor*4u)));
      first_root=cursor;
    }
  }
  auto second_header=m.read(graph.identity);parents=m.read(second_header);
  const auto second_parent=m.read(parents+second*4u);auto second_root=second;
  if(!negative(second_parent)){
    auto next=second_parent;
    do{second_root=next;next=m.read(parents+second_root*4u);}while(!negative(next));
    if(!negative(second_parent)){
      auto cursor=second;
      do{const auto data=m.read(second_header),slot=data+cursor*4u;cursor=m.read(slot);
        m.write(0x37c1f9u,slot,second_root);second_header=m.read(graph.identity);parents=m.read(second_header);
      }while(!negative(m.read(parents+cursor*4u)));
      second_root=cursor;
    }
  }
  if(first_root!=second_root){
    const auto header=m.read(graph.identity),data=m.read(header),first_size=m.read(data+first_root*4u);
    if(less(first_root,second_root)){
      const auto current_size=m.read(data+first_root*4u),second_size=m.read(data+second_root*4u);
      m.write(0x37c21fu,data+first_root*4u,current_size+second_size);
      const auto current_header=m.read(graph.identity),current_data=m.read(current_header);
      m.write(0x37c225u,current_data+second_root*4u,first_root);
    }else{
      const auto second_size=m.read(data+second_root*4u);
      m.write(0x37c232u,data+second_root*4u,second_size+first_size);
      const auto current_header=m.read(graph.identity),current_data=m.read(current_header);
      m.write(0x37c238u,current_data+first_root*4u,second_root);
    }
  }
  return less(first_root,second_root)?first_root:second_root;
}
void native_tu15_compress_island_partition_graph(SourcePhysicsContactMemory&memory,
 NativeTu15IslandPartitionGraph graph,const SourcePhysicsContactRecordObserver&observe,
 NativeTu15IslandPartitionGraphStats*stats){
  if(stats)++stats->compressions;GraphMemory m(memory,observe,stats);
  const auto header=m.read(graph.identity);auto parents=m.read(header);const auto count=m.read(header+4u);
  auto cursor=parents;const auto end=parents+count*4u;
  if(cursor!=end)do{
    const auto parent=m.read(cursor);
    if(!negative(parent)&&!negative(m.read(parents+parent*4u))){
      do{const auto current=m.read(cursor),next=m.read(parents+current*4u);
        m.write(0x37c275u,cursor,next);const auto current_header=m.read(graph.identity);parents=m.read(current_header);
        const auto updated=m.read(cursor);if(negative(m.read(parents+updated*4u)))break;
      }while(true);
    }
    cursor+=4u;
  }while(cursor!=end);
  const auto edges_header=m.read(graph.identity+4u),edges=m.read(edges_header),edge_count=m.read(edges_header+4u);
  cursor=edges;const auto edges_end=edges+edge_count*4u;
  if(cursor!=edges_end){
    const auto current_header=m.read(graph.identity);parents=m.read(current_header);
    do{
    const auto root=m.read(cursor);
    if(!negative(m.read(parents+root*4u))){
      do{const auto current=m.read(cursor),next=m.read(parents+current*4u);
        m.write(0x37c2adu,cursor,next);const auto current_parent_header=m.read(graph.identity);parents=m.read(current_parent_header);
        const auto updated=m.read(cursor);if(negative(m.read(parents+updated*4u)))break;
      }while(true);
    }
    cursor+=4u;
  }while(cursor!=edges_end);
  }
}
} // namespace halo2
