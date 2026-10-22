#include "halo2/source_model_admission.hpp"
#include "halo2/source_geometry_lod.hpp"
#include <bit>
#include <cstring>
#include <set>

namespace halo2 {
namespace {
template<class T>T read(const SourceModelResourceWorld& w,std::uint32_t at) {
    if(!w.banks.read)throw FormatError("source model admission reader absent");
    const auto bytes=w.banks.read(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("source model admission bank truncated");
    T value;std::memcpy(&value,bytes.data(),sizeof(T));return value;
}
template<class T>void put(const SourceModelResourceWorld& w,std::uint32_t at,T value) {
    if(!w.write)throw FormatError("source model admission writer absent");
    auto bytes=w.write(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("source model admission output truncated");
    std::memcpy(bytes.data(),&value,sizeof(T));
}
std::uint32_t metadata(const SourceModelResourceWorld& w,std::uint32_t tag) {
    return read<std::uint32_t>(w,w.banks.tag_table+(tag&65535u)*16+8);
}
std::uint32_t body(const SourceModelResourceWorld& w,std::uint32_t owner) {
    return read<std::uint32_t>(w,read<std::uint32_t>(w,w.banks.objects+0x44)+(owner&65535u)*12+8);
}
std::uint32_t model(const SourceModelResourceWorld& w,std::uint32_t owner) {
    return metadata(w,read<std::uint32_t>(w,metadata(w,read<std::uint32_t>(w,body(w,owner)))+0x38));
}
void flags(const SourceModelResourceWorld& w,std::uint32_t out,std::uint32_t set,std::uint32_t clear=0) {
    put(w,out+0x16c,(read<std::uint32_t>(w,out+0x16c)&~clear)|set);
}
SourceGeometryLodDefinition lod_definition(const SourceModelResourceWorld& w,std::uint32_t m) {
    SourceGeometryLodDefinition definition;
    auto& bank=definition.model_bank;
    //4C450 short-circuits the validation loads. The two intervening floats at
    //hlmt+2C/+30 are never inputs, and +28 is fetched only for a valid bank.
    bank[3]=read<float>(w,m+0x34);
    if(!(bank[3]>0.f))return definition;
    for(unsigned i=4;i<8;++i) {
        bank[i]=read<float>(w,m+0x28+i*4);
        if(!(bank[i-1]>=bank[i]))return definition;
    }
    bank[8]=read<float>(w,m+0x48);
    if(bank[8]!=0.f)return definition;
    bank[0]=read<float>(w,m+0x28);
    definition.model_present=true;
    return definition;
}
}
bool source_model_owner_hidden(std::uint32_t owner,const SourceModelResourceWorld& w) {
    const auto headers=read<std::uint32_t>(w,w.banks.objects+0x44);std::set<std::uint32_t> seen;
    do {
        if(!seen.insert(owner).second)throw FormatError("source model admission parent cycle");
        const auto header=headers+(owner&65535u)*12;
        // Source fetches the body pointer even when the header flag rejects.
        const auto header_flags=read<std::uint8_t>(w,header+2);
        const auto object=read<std::uint32_t>(w,header+8);
        if((header_flags&16) || (read<std::uint8_t>(w,object+4)&1))return true;
        owner=read<std::uint32_t>(w,object+0x14);
    } while(owner!=0xffffffffu);
    return false;
}
void source_model_sort_orders(std::uint32_t owner,std::uint32_t out,std::int32_t models,
    const SourceModelResourceWorld& w) {
    const auto object=body(w,owner),m=model(w,owner);
    if(read<std::uint8_t>(w,object+0xb1)!=255 && models==1) {
        auto count=read<std::int32_t>(w,m+0x70);
        for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<count;++i) {
            const auto variant=std::uint32_t(std::int32_t(read<std::int8_t>(w,object+0xb1)))*0x38+
                read<std::uint32_t>(w,m+0x54);
            const auto index=read<std::int8_t>(w,variant+i+4);
            std::uint16_t order=0;
            if(index!=-1)order=read<std::uint16_t>(w,read<std::uint32_t>(w,variant+0x18)+
                std::uint32_t(std::int32_t(index))*20+0x10);
            put(w,out+0xc2+i*2,order);count=read<std::int32_t>(w,m+0x70);
        }
    } else for(std::uint32_t i=0;i<40;++i)put<std::uint32_t>(w,out+0xc2+i*4,0);
}
std::int16_t source_model_render_admission(std::uint32_t owner,float metric,
    std::uint32_t caller_mode,std::uint32_t first_person,std::uint32_t force_world,
    std::uint32_t lod,std::uint32_t out,const SourceModelAdmissionWorld& world) {
    const auto& w=world.memory;put<std::uint32_t>(w,out+0x16c,0);
    const bool hidden=source_model_owner_hidden(owner,w);
    if(first_person)put(w,first_person,std::uint8_t(!hidden && owner==world.first_person_owner));
    if(hidden)return 0;
    if(!first_person)throw FormatError("source model admission visible owner needs first-person output");
    const auto object=body(w,owner);
    const auto definition=metadata(w,read<std::uint32_t>(w,object));
    if(force_world&255u)put<std::uint8_t>(w,first_person,0);
    const auto model_tag=read<std::uint32_t>(w,definition+0x38);
    if(model_tag==0xffffffffu)return 0;
    const auto m=metadata(w,model_tag);
    std::int16_t models;
    if(read<std::uint8_t>(w,first_person))models=world.first_person_models;
    else models=read<std::uint32_t>(w,model(w,owner)+4)!=0xffffffffu;
    if(models>0) {
        if(!caller_mode) {
            const auto render_tag=read<std::uint32_t>(w,m+4);
            if(render_tag!=0xffffffffu) {
                const auto mode=metadata(w,render_tag);
                if(read<std::int32_t>(w,mode+0x74)>0) {
                    if(!w.banks.request)throw FormatError("source model admission12DE70 owner absent");
                    if(w.banks.request(3,read<std::uint32_t>(w,mode+0x78)+0x34))flags(w,out,0x40);
                }
            }
        }
        put(w,lod,std::uint8_t(read<std::uint8_t>(w,first_person)?4:255));
        if(!read<std::uint8_t>(w,first_person)) {
            const auto object_flags=read<std::uint32_t>(w,object+4);
            const auto current_model=model(w,owner);
            if(world.view_modifier)metric*=.3f;
            if(object_flags&0x80000)put<std::uint8_t>(w,lod,5);
            else {
                if(read<std::uint8_t>(w,object+0x10a)&4)metric*=1.7f;
                const auto definition=lod_definition(w,current_model);
                // Source computes the threshold/fraction before acquiring its
                // actual prior LOD. The initializer does not change this bank.
                (void)source_geometry_lod_choice(definition,metric);
                const auto prior=source_model_prior_lod(owner,w);
                const auto choice=source_geometry_lod_choice(definition,metric,prior);
                const auto raw=std::bit_cast<std::int8_t>(std::uint8_t(choice.lod));
                put(w,lod,std::uint8_t(choice.lod));
                const auto maximum=(read<std::uint8_t>(w,out+0x16c)&0x40)?4:world.ordinary_maximum;
                put(w,lod,std::uint8_t(raw<0?0:std::min(std::int32_t(raw),maximum)));
            }
        }
        if(read<std::uint8_t>(w,lod)==255)models=0;
        else {
            bool camouflage=false;
            if(read<std::uint8_t>(w,m+0x9c)&1) {
                flags(w,out,6);put(w,out+0x174,read<std::uint32_t>(w,world.rasterizer+0xd0));
                put<float>(w,out+0x184,.99f);camouflage=true;
            } else {
                const auto current_body=body(w,owner);
                const auto type=read<std::uint8_t>(w,current_body+0xaa);
                if(((1u<<(type&31u))&3u) && read<float>(w,current_body+0x2b0)>0.f) {
                    flags(w,out,2);put(w,out+0x174,read<std::uint32_t>(w,world.rasterizer+0xd0));
                    put(w,out+0x184,read<std::uint32_t>(w,current_body+0x2b0));camouflage=true;
                }
            }
            if(camouflage) {
                const auto mf=read<std::uint32_t>(w,m+0x9c);
                const bool fp=read<std::uint8_t>(w,first_person)!=0;
                flags(w,out,(fp || (mf&4)?8u:0u),8);
                flags(w,out,(!fp && (mf&2)?16u:0u),16);
            }
            if(!caller_mode) {
                if(read<float>(w,object+0xf4)>0.f && read<std::int32_t>(w,m+0x60)>0) {
                    const auto shaders=read<std::uint32_t>(w,m+0x64);
                    const auto shader=read<std::uint32_t>(w,shaders+(read<std::uint8_t>(w,first_person)?0x80:0x88));
                    if(shader!=0xffffffffu) {
                        flags(w,out,1);put(w,out+0x170,shader);put(w,out+0x180,read<std::uint32_t>(w,object+0xf0));
                    }
                }
                if(read<float>(w,object+0xf0)>1.f && read<std::int32_t>(w,m+0x60)>0) {
                    const auto shaders=read<std::uint32_t>(w,m+0x64);
                    const auto shader=read<std::uint32_t>(w,shaders+(read<std::uint8_t>(w,first_person)?0xec:0xf4));
                    if(shader!=0xffffffffu) {
                        flags(w,out,0x20);put(w,out+0x178,shader);put(w,out+0x188,1.f-read<float>(w,object+0xf0));
                    }
                }
                if(read<std::uint32_t>(w,m+0xf4)!=0xffffffffu) {
                    if(!world.scalar)throw FormatError("source model admissionBAB40 scalar owner absent");
                    const auto [enabled,value]=world.scalar(owner,read<std::uint32_t>(w,m+0xf8));
                    if(enabled && value>0.f) {
                        flags(w,out,0x80);put(w,out+0x17c,read<std::uint32_t>(w,m+0xf4));put(w,out+0x18c,value);
                    }
                }
            }
        }
    }
    source_model_sort_orders(owner,out,models,w);return models;
}
}
