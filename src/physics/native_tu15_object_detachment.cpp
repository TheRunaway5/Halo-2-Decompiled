#include "halo2/native_tu15_object_detachment.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
}
[[noreturn]] void missing(NativeTu15ObjectDetachmentStats*s,const char* owner){if(s)++s->unbound_services;throw std::runtime_error(owner);}
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
