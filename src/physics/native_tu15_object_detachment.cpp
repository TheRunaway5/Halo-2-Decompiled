#include "halo2/native_tu15_object_detachment.hpp"
#include "halo2/native_tu15_datum_delete_kernel.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
}
[[noreturn]] void missing(NativeTu15ObjectDetachmentStats*s,const char* owner){if(s)++s->unbound_services;throw std::runtime_error(owner);}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t site,std::uint32_t at,std::uint32_t value,unsigned width=4){
    std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
    SourcePhysicsContactRecordStore event{site,at,{},{}};
    if(o){event.before.resize(width);m.read(at,event.before);event.written.assign(bytes.begin(),bytes.begin()+width);}
    m.write(at,std::span(bytes).first(width));if(o)o(event);
}
void retire_spatial_datum(SourcePhysicsContactMemory&m,std::uint32_t pool,std::uint32_t datum,
    const SourcePhysicsContactRecordObserver&o){
    struct Access {
        SourcePhysicsContactMemory&memory;const SourcePhysicsContactRecordObserver&observe;
        std::uint32_t read(NativeTu15DatumDeletePhase,const NativeTu15DatumDelete&,std::uint32_t p,unsigned n){return word(memory,p,n);}
        void write(NativeTu15DatumDeletePhase phase,const NativeTu15DatumDelete&,std::uint32_t p,std::uint32_t v,unsigned n){
            using P=NativeTu15DatumDeletePhase;std::uint32_t site{};
            switch(phase){case P::clear_word:site=0x16e8d1u;break;case P::clear_byte:site=0x16e8d8u;break;
            case P::bitmap_store:site=0x16e8f8u;break;case P::salt:site=0x16e8fau;break;
            case P::cursor_store:site=0x16e904u;break;case P::shrink_store:site=0x16e91cu;break;
            case P::occupancy_store:site=0x16e92du;break;default:throw std::logic_error("Unqualified spatial datum store phase");}
            store(memory,observe,site,p,v,n);
        }
        void prepare(NativeTu15DatumDeletePhase,const NativeTu15DatumDelete&){}
        void complete(NativeTu15DatumDeletePhase,const NativeTu15DatumDelete&){}
    }access{m,o};
    NativeTu15DatumDelete state;state.pool=pool;state.datum=datum;native_tu15_delete_datum(access,state);
}
}
void native_tu15_remove_spatial_chain(SourcePhysicsContactMemory&m,std::uint32_t root,
    const NativeTu15SpatialRemovalParameters&parameters,
    const SourcePhysicsContactRecordObserver&o,NativeTu15SpatialRemovalStats*stats){
    if(stats)++stats->calls;auto output=parameters.head_pointer(),datum=word(m,output);
    const auto key=parameters.key();
    const bool nonempty=datum!=0xffffffffu;
    while(datum!=0xffffffffu){
        const auto outer=word(m,root+8u),rows=word(m,outer+0x44u),stride=word(m,outer+0x24u);
        const auto row=rows+(datum&0xffffu)*stride;
        const auto slot=std::bit_cast<std::int16_t>(std::uint16_t(word(m,row+4u,2)));
        const auto next=word(m,row+8u);if(stats)++stats->outer_rows;
        retire_spatial_datum(m,outer,datum,o);if(stats)++stats->outer_deletions;
        auto head=word(m,root)+std::uint32_t(std::int32_t(slot))*4u;
        const auto first=word(m,head),inner=word(m,root+4u);
        if(first!=0xffffffffu){
            const auto inner_stride=word(m,inner+0x24u),inner_rows=word(m,inner+0x44u);
            for(;;){
                const auto record=inner_rows+(word(m,head)&0xffffu)*inner_stride;
                if(stats)++stats->inner_rows;
                if(word(m,record+4u)==key){
                    const auto following=word(m,record+8u),current=word(m,head);
                    retire_spatial_datum(m,inner,current,o);if(stats)++stats->inner_deletions;
                    store(m,o,0x1d3165u,head,following);break;
                }
                head=record+8u;if(word(m,head)==0xffffffffu)break;
            }
        }
        datum=next;
    }
    if(nonempty)output=parameters.head_pointer();
    store(m,o,0x1d3176u,output,0xffffffffu);if(stats)++stats->head_clears;
}
void native_tu15_remove_spatial_chain(SourcePhysicsContactMemory&m,std::uint32_t root,
    std::uint32_t key,const std::function<std::uint32_t()>&head,
    const SourcePhysicsContactRecordObserver&o,NativeTu15SpatialRemovalStats*stats){
    native_tu15_remove_spatial_chain(m,root,{[key]{return key;},head},o,stats);
}
void native_tu15_remove_spatial_chain(SourcePhysicsContactMemory&m,std::uint32_t root,
    std::uint32_t key,std::uint32_t head,const SourcePhysicsContactRecordObserver&o,
    NativeTu15SpatialRemovalStats*stats){
    native_tu15_remove_spatial_chain(m,root,key,[head]{return head;},o,stats);
}
void native_tu15_detach_light_attachment(SourcePhysicsContactMemory&m,std::uint32_t datum,
    const SourcePhysicsContactRecordObserver&o,NativeTu15SpatialRemovalStats*stats){
    if(stats)++stats->attachment_calls;const auto pool=word(m,0x4ecbdcu),rows=word(m,pool+0x44u);
    const auto record=rows+(datum&0xffffu)*0x110u,flags=word(m,record+2u,1);
    if(!(flags&2u)){if(stats)++stats->inactive_attachments;return;}
    native_tu15_remove_spatial_chain(m,0x4ecbe0u,datum,record+0x10u,o,stats);
    store(m,o,0xc5283u,record+2u,word(m,record+2u,1)&0xf7u,1);
    if(stats)++stats->attachment_flag_clears;
}
void native_tu15_detach_object_tree(SourcePhysicsContactMemory& memory,
    NativeTu15ObjectDetachmentRequest request,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ObjectDetachmentServices&s,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ObjectDetachmentStats*stats){
    if(stats)++stats->calls;
    for(;;){
        if(stats)++stats->objects;const auto pool=word(memory,0x4ecbd0u),rows=word(memory,pool+0x44u);
        const auto header=rows+(request.datum&0xffffu)*12u,object=word(memory,header+8u);bool deleting{};
        if(request.current_only&255u){
            if((word(memory,header+2u,1)&0x10u)||(word(memory,object+4u,1)&1u)){if(stats)++stats->guard_skips;return;}
        }else{
            deleting=native_tu15_object_or_ancestor_deleting(memory,request.datum,stats?&stats->ancestor:nullptr);
            if(deleting){if(stats)++stats->guard_skips;return;}
        }
        if(word(memory,object+4u)&0x10u){
            const auto bytes=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(word(memory,object+0x11cu,2))));
            const auto displacement=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(word(memory,object+0x11eu,2))));
            const auto count=std::uint32_t(bytes)>>3u;auto descriptor=object+std::uint32_t(displacement)+4u;
            if(std::bit_cast<std::int32_t>(count)>0)for(std::uint32_t n=0;n<count;++n){
                if(stats)++stats->descriptor_queries;
                if(word(memory,descriptor-4u,1)==0){
                    const auto attachment=word(memory,descriptor);
                    if(attachment!=0xffffffffu&&!deleting){
                        if(request.detach_attachments&255u){
                            if(!s.detach_attachment)missing(stats,"Native object detachment requires actualC5250 attachment owner");
                            if(stats)++stats->attachment_detaches;s.detach_attachment(attachment,fp);
                        }
                        if(request.delete_attachments&255u){
                            const auto current=word(memory,descriptor);
                            if(!s.delete_attachment)missing(stats,"Native object detachment requires actualC4D30 attachment owner");
                            if(stats)++stats->attachment_deletes;s.delete_attachment(current,fp);
                        }
                    }
                }
                descriptor+=8u;
            }
        }
        if(word(memory,object+4u)&0x40u){
            if(!s.detach_children)missing(stats,"Native object detachment requires actualC3750 children owner");
            if(stats)++stats->children_detaches;s.detach_children(request.datum,request.detach_attachments,request.delete_attachments,fp);
        }
        const auto child=word(memory,object+0x10u);
        if(child!=0xffffffffu){
            if(stats)++stats->recursions;
            native_tu15_detach_object_tree(memory,{child,request.detach_attachments,request.delete_attachments,1u,std::uint32_t(!deleting)},fp,s,observe,stats);
        }
        if(!(request.follow_siblings&255u))return;const auto sibling=word(memory,object+0xcu);
        if(sibling==0xffffffffu)return;request.datum=sibling;if(stats)++stats->sibling_steps;
    }
}
}
