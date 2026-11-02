#include "halo2/native_tu15_island_contact_merge.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
class ContactMerge {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
    const NativeTu15IslandContactMergeServices&s_;NativeTu15PhysicsFrameFp&fp_;NativeTu15IslandContactMergeStats*stats_;
public:
    ContactMerge(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
      const NativeTu15IslandContactMergeServices&s,NativeTu15PhysicsFrameFp&fp,NativeTu15IslandContactMergeStats*stats)
      :m_(m),o_(o),s_(s),fp_(fp),stats_(stats){}
    std::uint32_t read(std::uint32_t p){std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t v{};
        for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
    void store(std::uint32_t pc,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};
        for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8u*n));SourcePhysicsContactRecordStore e{pc,p,{},{}};
        if(o_){e.before.resize(4);m_.read(p,e.before);e.written.assign(b.begin(),b.end());}m_.write(p,b);if(o_)o_(e);
    }
    template<class F>void require(const F&f,const char*message){if(!f){if(stats_)++stats_->unbound_services;throw std::runtime_error(message);}}
    void reserve(NativeTu15ArrayHeader h,std::uint32_t capacity,std::uint32_t needed){
        if(!less(capacity,needed))return;auto next=capacity*2u;if(!less(needed,next))next=needed;
        require(s_.reserve_array,"Native TU1.5 contact merge requires actual2E31E0 reserve");
        if(stats_)++stats_->array_reserves;s_.reserve_array(h,next,16u,fp_);
    }
    void copy(std::uint32_t destination,std::uint32_t source,const std::array<std::uint32_t,4>&sites){
        for(unsigned n=0;n<4;++n)store(sites[n],destination+n*4u,read(source+n*4u));
    }
    void run(NativeTu15ArrayHeader first,NativeTu15ArrayHeader second,NativeTu15IslandContactMergeScratch scratch){
        if(stats_)++stats_->calls;const auto a=first.identity,b=second.identity,t=scratch.temporary.identity;
        if(!t)throw std::runtime_error("Native TU1.5 contact merge requires20 caller-owned temporary DATA bytes");
        const auto requested=read(a+4u);native_tu15_construct_island_temporary_records(m_,{t},requested,fp_,s_.lifetime,o_,stats_?&stats_->lifetime:nullptr);
        try{
            const auto temporary_capacity=read(t+8u)&0x7fffffffu,first_count=read(a+4u);
            reserve({t},temporary_capacity,first_count);auto count=read(a+4u);const auto temporary_count=first_count;
            store(0x318e3fu,t+4u,temporary_count);std::uint32_t index{};
            if(less(0,count))do{
                const auto source=read(a),destination=read(t);
                copy(destination+index*16u,source+index*16u,{0x318e53u,0x318e58u,0x318e5eu,0x318e64u});
                count=read(a+4u);++index;if(stats_)++stats_->temporary_copies;
            }while(less(index,count));
            const auto current_first=read(a+4u),second_count=read(b+4u),capacity=read(a+8u)&0x7fffffffu;
            reserve(first,capacity,current_first+second_count);store(0x318eaau,a+4u,current_first+second_count);
            std::uint32_t temp_index{},second_index{},output_index{},duplicates{};
            if(less(0,temporary_count))while(less(second_index,read(b+4u))){
                const auto source=read(b),second_record=source+second_index*16u,second_payload=read(second_record),temporary=read(t),temporary_record=temporary+temp_index*16u;
                const auto temporary_payload=read(temporary_record),temporary_key=read(temporary_payload+4u),second_key=read(second_payload+4u);
                if(stats_)++stats_->comparisons;bool take_first=temporary_key<second_key;
                if(!take_first&&temporary_key==read(second_payload+4u)){
                    const auto temp_other=read(temporary_record+4u),second_other=read(second_record+4u);
                    const auto temp_key=read(temp_other+4u),second_key=read(second_other+4u);take_first=temp_key<second_key;
                }
                if(take_first){
                    const auto destination=read(a);copy(destination+output_index*16u,temporary_record,{0x318f20u,0x318f25u,0x318f2bu,0x318f35u});
                    ++temp_index;++output_index;if(stats_)++stats_->first_moves;
                }else{
                    const auto temp_payload=read(temporary_record),next_payload=read(second_record),temp_key=read(temp_payload+4u),next_key=read(next_payload+4u);
                    bool duplicate=false;
                    if(temp_key==next_key){const auto temp_other=read(temporary_record+4u),next_other=read(second_record+4u);
                        const auto temp_second=read(temp_other+4u),next_second=read(next_other+4u);duplicate=temp_second==next_second;}
                    if(duplicate){
                        const auto agent=read(second_record+8u),table=read(agent),manager=read(agent+8u);const NativeTu15PhysicsVirtualOwner cleanup{read(table+0x14u)};
                        require(s_.cleanup_duplicate_agent,"Native TU1.5 contact merge requires actual duplicate agent cleanup");
                        if(stats_)++stats_->agent_cleanups;s_.cleanup_duplicate_agent(agent,cleanup,fp_);
                        const auto manager_table=read(manager);const NativeTu15PhysicsVirtualOwner release{read(manager_table+0x10u)};
                        require(s_.release_duplicate_manager,"Native TU1.5 contact merge requires actual duplicate manager release");
                        if(stats_)++stats_->manager_releases;s_.release_duplicate_manager(manager,release,fp_);
                        ++second_index;++duplicates;if(stats_)++stats_->duplicates;
                    }else{
                        const auto destination=read(a);copy(destination+output_index*16u,second_record,{0x318fa1u,0x318fa6u,0x318facu,0x318fb2u});
                        ++second_index;++output_index;if(stats_)++stats_->second_moves;
                    }
                }
                if(!less(temp_index,temporary_count))break;
            }
            const auto current_second=read(b+4u),final_count=temporary_count-duplicates+current_second,current_capacity=read(a+8u)&0x7fffffffu;
            reserve(first,current_capacity,final_count);store(0x319020u,a+4u,final_count);
            if(temp_index==temporary_count){
                if(less(second_index,read(b+4u)))do{
                    const auto source=read(b),destination=read(a);
                    copy(destination+output_index*16u,source+second_index*16u,{0x319042u,0x319047u,0x31904du,0x319053u});
                    const auto remaining=read(b+4u);++second_index;++output_index;if(stats_)++stats_->second_moves;
                    if(!less(second_index,remaining))break;
                }while(true);
            }else if(less(temp_index,temporary_count))do{
                const auto source=read(t),destination=read(a);
                copy(destination+output_index*16u,source+temp_index*16u,{0x319084u,0x319089u,0x31908fu,0x319095u});
                ++temp_index;++output_index;if(stats_)++stats_->first_moves;
            }while(less(temp_index,temporary_count));
        }catch(...){native_tu15_destroy_island_temporary_records(m_,{t},fp_,s_.lifetime,o_,stats_?&stats_->lifetime:nullptr);throw;}
        native_tu15_destroy_island_temporary_records(m_,{t},fp_,s_.lifetime,o_,stats_?&stats_->lifetime:nullptr);
    }
};
}
void native_tu15_merge_island_contact_records(SourcePhysicsContactMemory&m,NativeTu15ArrayHeader first,
 NativeTu15ArrayHeader second,NativeTu15IslandContactMergeScratch scratch,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15IslandContactMergeServices&s,const SourcePhysicsContactRecordObserver&o,NativeTu15IslandContactMergeStats*stats){
    ContactMerge(m,o,s,fp,stats).run(first,second,scratch);
}
} // namespace halo2
