#include "halo2/native_tu15_island_constraint_detach.hpp"
#include "halo2/native_tu15_contact_collector_info.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
class Detach {
  SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;NativeTu15IslandConstraintDetachStats*stats_;
public:
  Detach(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,NativeTu15IslandConstraintDetachStats*stats):m_(m),o_(o),stats_(stats){}
  std::uint32_t read(std::uint32_t p,unsigned size=4){std::array<std::byte,4>b{};m_.read(p,std::span(b).first(size));std::uint32_t v{};for(unsigned n=0;n<size;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
  void write(std::uint32_t site,std::uint32_t p,std::uint32_t v,unsigned size=4){std::array<std::byte,4>b{};for(unsigned n=0;n<size;++n)b[n]=std::byte(v>>(8u*n));SourcePhysicsContactRecordStore e{site,p,{},{}};if(o_){e.before.resize(size);m_.read(p,e.before);e.written.assign(b.begin(),b.begin()+size);}m_.write(p,std::span(b).first(size));if(o_)o_(e);}
  template<class F>void require(const F&f,const char*why){if(!f){if(stats_)++stats_->unbound_services;throw std::runtime_error(why);}}
  void run(NativeTu15PhysicsIsland island,std::uint32_t constraint,NativeTu15IslandConstraintInfo info,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandConstraintDetachServices&s){
    if(stats_)++stats_->calls;const auto count=read(island.identity+0x5cu);std::uint32_t index{};bool found=false;
    if(less(0,count)){const auto entries=read(island.identity+0x58u);auto cursor=entries;do{if(read(cursor)==constraint){found=true;break;}++index;cursor+=4u;}while(less(index,count));}
    if(!found)index=0xffffffffu;
    const auto last=read(island.identity+0x5cu)-1u;write(0x3201dfu,island.identity+0x5cu,last);
    if(less(index,last))do{const auto entries=read(island.identity+0x58u),next=read(entries+index*4u+4u);write(0x3201eeu,entries+index*4u,next);const auto current=read(island.identity+0x5cu);++index;if(stats_)++stats_->shifted_entries;if(!less(index,current))break;}while(true);
    const auto constraint_table=read(constraint);const NativeTu15PhysicsVirtualOwner info_owner{read(constraint_table+0x28u)};
    if(stats_)++stats_->info_queries;
    if(info_owner.identity==0x325e90u){native_tu15_get_contact_collector_info(m_,constraint,info,o_);if(stats_)++stats_->native_info_queries;}
    else{require(s.get_info,"Native TU1.5 island detach requires actual constraint info owner");s.get_info(constraint,info_owner,info,fp);}
    const auto island_table=read(island.identity);const NativeTu15PhysicsVirtualOwner remove_owner{read(island_table+0xcu)};
    if(stats_)++stats_->metadata_removals;
    if(remove_owner.identity==0x320100u){native_tu15_remove_island_constraint_info(m_,island,info,o_);if(stats_)++stats_->native_metadata_removals;}
    else{require(s.remove_info,"Native TU1.5 island detach requires actual metadata removal owner");s.remove_info(island,remove_owner,constraint,info,fp);}
    write(0x320211u,island.identity+0x28u,1u,1);write(0x320215u,constraint+6u,(read(constraint+6u,2)-1u)&0xffffu,2);if(stats_)++stats_->reference_releases;
    const auto references=read(constraint+6u,2);write(0x32021eu,constraint+0x14u,0);if(references)return;
    //283450 is the source's generic reserved-physics-object range test; it
    // reads global bounds only. The existing typed helper has body spelling.
    if(stats_)++stats_->protected_queries;
    if(native_tu15_reserved_body_reference(m_,{constraint})){write(0x320234u,constraint+6u,1u,2);if(stats_)++stats_->protected_references;return;}
    const auto table=read(constraint);const NativeTu15PhysicsVirtualOwner destructor{read(table)};
    require(s.destroy_constraint,"Native TU1.5 island detach requires actual zero-reference destruction owner");if(stats_)++stats_->destructions;s.destroy_constraint(constraint,destructor,1u,fp);
  }
};
}
void native_tu15_detach_island_constraint(SourcePhysicsContactMemory&m,NativeTu15PhysicsIsland island,
 std::uint32_t constraint,NativeTu15IslandConstraintInfo info,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15IslandConstraintDetachServices&s,const SourcePhysicsContactRecordObserver&o,
 NativeTu15IslandConstraintDetachStats*stats){Detach(m,o,stats).run(island,constraint,info,fp,s);}
} // namespace halo2
