#include "halo2/source_model_resources.hpp"
#include "halo2/source_render_cache.hpp"
#include "halo2/object_lighting.hpp"
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
template<class T>T read(const SourceModelResourceWorld& w,std::uint32_t at) {
    if(!w.banks.read)throw FormatError("source model resource reader absent");
    const auto raw=w.banks.read(at,sizeof(T));
    if(raw.size()!=sizeof(T))throw FormatError("source model resource read truncated");
    T v;std::memcpy(&v,raw.data(),sizeof(T));return v;
}
template<class T>void put(const SourceModelResourceWorld& w,std::uint32_t at,T v) {
    if(!w.write)throw FormatError("source model resource writer absent");
    auto raw=w.write(at,sizeof(T));
    if(raw.size()!=sizeof(T))throw FormatError("source model resource write truncated");
    std::memcpy(raw.data(),&v,sizeof(T));
}
std::uint32_t metadata(const SourceModelResourceWorld& w,std::uint32_t model) {
    return read<std::uint32_t>(w,w.banks.tag_table+(model&65535u)*16+8);
}
std::uint32_t permutation_section(const SourceModelResourceWorld& w,
    std::uint32_t region,std::uint32_t permutation,std::int32_t lod) {
    return read<std::uint32_t>(w,region+0xc)+permutation*16+std::uint32_t(lod)*2+4;
}
bool request(const SourceModelResourceWorld& w,std::uint32_t mode,std::uint32_t at) {
    if(!w.banks.request)throw FormatError("source model resource publication owner absent");
    return w.banks.request(mode,at);
}
std::uint32_t resource(const SourceModelResourceWorld& w,std::uint32_t model,std::uint32_t section_word) {
    const auto section=read<std::int16_t>(w,section_word);
    return read<std::uint32_t>(w,model+0x28)+std::uint32_t(std::int32_t(section))*0x5c+0x38;
}
bool valid_view(const SourceModelResourceWorld& w){return w.view>=0 && w.view<4;}
std::uint32_t acquire(const SourceModelResourceWorld& w,std::uint32_t owner) {
    const auto result=source_model_acquire_cache(owner,w);
    if(w.cache_acquired)w.cache_acquired(owner,result);
    return result;
}
std::uint32_t cache_record(const SourceModelResourceWorld& w,std::uint32_t token) {
    if((token&65535u)>=256)throw FormatError("source model resource cache token escapes256 records");
    return read<std::uint32_t>(w,w.render_cache_pool+0x44)+(token&65535u)*256;
}
}
void source_model_build_inputs(std::uint32_t cached,std::uint32_t cache_index,
    std::uint32_t flags,std::uint32_t source_owner,std::uint32_t model,
    std::uint32_t transforms,std::uint32_t count,std::uint32_t owner,
    std::uint32_t cached_models,const SourceModelResourceWorld& w) {
    if(cached&255u) {
        const auto index=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(cache_index)));
        const auto entry=cached_models+std::uint32_t(index)*0xd0c;
        const auto tag=read<std::uint32_t>(w,entry),m=metadata(w,tag);
        put(w,model,tag);put(w,transforms,entry+12);
        put(w,count,std::min(read<std::uint32_t>(w,m+0x48),64u));
        put(w,flags,read<std::uint32_t>(w,entry+8));
        put(w,source_owner,read<std::uint32_t>(w,entry+4));
    } else {
        const auto headers=read<std::uint32_t>(w,w.banks.objects+0x44);
        const auto at=headers+(owner&65535u)*12+8;
        const auto body=read<std::uint32_t>(w,at);
        const auto definition=metadata(w,read<std::uint32_t>(w,body));
        const auto hlmt=metadata(w,read<std::uint32_t>(w,definition+0x38));
        const auto tag=read<std::uint32_t>(w,hlmt+4),m=metadata(w,tag);
        put(w,model,tag);
        // The original rereads the pool body after the model output, then
        // publishes an unsigned quotient before reading the signed offset.
        const auto current_headers=read<std::uint32_t>(w,w.banks.objects+0x44);
        const auto current_body=read<std::uint32_t>(w,current_headers+(owner&65535u)*12+8);
        const auto bytes=std::uint32_t(std::int32_t(read<std::int16_t>(w,current_body+0x114)));
        const auto intermediate=std::uint32_t((std::uint64_t(bytes)*0x4ec4ec4fu)>>32)>>4;
        put(w,count,intermediate);
        put(w,transforms,current_body+std::uint32_t(std::int32_t(read<std::int16_t>(w,current_body+0x116))));
        put(w,count,read<std::uint32_t>(w,m+0x48));
        put<std::uint32_t>(w,flags,0);put(w,source_owner,owner);
    }
}
std::int32_t source_model_geometry_node_count(std::uint32_t model,std::int32_t lod,
    std::uint32_t selected,const SourceModelResourceWorld& w) {
    const auto m=metadata(w,model);std::uint32_t count=1;
    if(!(read<std::uint8_t>(w,m+4)&4)) {
        const auto group=read<std::int8_t>(w,m+std::uint32_t(lod)+0x3c);
        const auto groups=read<std::uint32_t>(w,m+0x38);
        count=read<std::uint32_t>(w,groups+std::uint32_t(std::int32_t(group))*12+4)+read<std::uint32_t>(w,m+0x48);
    }
    const auto regions=read<std::int32_t>(w,m+0x1c);
    for(std::int32_t i=0;i<regions;++i) {
        const auto section=read<std::uint8_t>(w,selected+std::uint32_t(i));
        if(section!=255)count+=read<std::uint8_t>(w,read<std::uint32_t>(w,m+0x28)+std::uint32_t(section)*0x5c+0x25);
    }
    return std::bit_cast<std::int32_t>(count);
}
void source_model_refresh_colors(std::uint32_t owner,std::uint32_t force,const SourceModelResourceWorld& w) {
    const auto headers=read<std::uint32_t>(w,w.banks.objects+0x44);
    const auto body=read<std::uint32_t>(w,headers+(owner&65535u)*12+8);
    const auto token=read<std::uint32_t>(w,body+0xcc);
    if(token==0xffffffffu)return;
    if(!(force&255u) && !(read<std::uint8_t>(w,metadata(w,read<std::uint32_t>(w,body))+0x1c)&1))return;
    const auto record=cache_record(w,token);
    put<std::uint8_t>(w,record+0xb8,0);
    for(std::uint32_t i=0;i<4;++i) {
        const auto current_headers=read<std::uint32_t>(w,w.banks.objects+0x44);
        const auto current_body=read<std::uint32_t>(w,current_headers+(owner&65535u)*12+8);
        const auto offset=std::uint32_t(std::int32_t(read<std::int16_t>(w,current_body+0x126)));
        // BAD50 uses unsigned MUL after sign-extending the byte extent.
        const auto extent=std::uint32_t(std::int32_t(read<std::int16_t>(w,current_body+0x124)));
        const auto count=std::uint32_t((std::uint64_t(extent)*0xaaaaaaabu)>>32)>>4;
        if(i<count) {
            const auto rgb=read<Vec3>(w,current_body+offset+(count+i)*12);
            put(w,record+0xa8+i*4,pack_object_lighting_rgb(rgb));
            put(w,record+0xb8,std::uint8_t(read<std::uint8_t>(w,record+0xb8)+1));
        }
    }
}
std::uint32_t source_model_acquire_cache(std::uint32_t owner,const SourceModelResourceWorld& w) {
    const auto headers=read<std::uint32_t>(w,w.banks.objects+0x44);
    const auto body=read<std::uint32_t>(w,headers+(owner&65535u)*12+8);
    auto token=read<std::uint32_t>(w,body+0xcc);
    const auto records=read<std::uint32_t>(w,w.render_cache_pool+0x44);
    // Source does not inspect occupancy/salt, pool counters or object regions
    // and colors when the full owner matches. In particular stale region
    // extents on this path must not become eager native input validation.
    if(token!=0xffffffffu && read<std::uint32_t>(w,cache_record(w,token)+4)==owner)return token;
    const auto pool=w.render_cache_pool;
    if(read<std::uint32_t>(w,pool+0x20)!=256 || read<std::uint32_t>(w,pool+0x24)!=256)
        throw FormatError("source model cache requires its original256-record pool and stride");
    SourceRenderCachePool p;
    p.context_callback=w.context_callback;
    p.cursor=read<std::uint32_t>(w,pool+0x34);p.highwater=read<std::uint32_t>(w,pool+0x38);
    p.active_count=read<std::uint32_t>(w,pool+0x3c);p.next_generation=read<std::uint16_t>(w,pool+0x40);
    const auto bits=read<std::uint32_t>(w,pool+0x48);
    const auto copy_in=[&](std::uint32_t at,auto& value) {
        const auto bytes=w.banks.read(at,sizeof(value));
        if(bytes.size()!=sizeof(value))throw FormatError("source model cache input bank truncated");
        std::memcpy(&value,bytes.data(),sizeof(value));
    };
    copy_in(records,p.records);copy_in(bits,p.occupied);
    // With the scan exhausted and no occupied record, neither allocation nor
    // pressure rebind can succeed. Source returns before reading body inputs.
    if(p.cursor==256 && p.highwater==256 && std::ranges::all_of(p.occupied,[](auto word){return word==0;}))
        return 0xffffffffu;
    const auto region_count=read<std::int16_t>(w,body+0x118)/10;
    if(region_count<0 || region_count>16)throw FormatError("source model cache region extent exceeds16 slots");
    const auto regions_at=body+std::uint32_t(std::int32_t(read<std::int16_t>(w,body+0x11a)));
    std::array<std::uint8_t,16> regions{};
    if(region_count) {
        const auto bytes=w.banks.read(regions_at,std::size_t(region_count));
        if(bytes.size()!=std::size_t(region_count))throw FormatError("source model cache regions truncated");
        std::memcpy(regions.data(),bytes.data(),bytes.size());
    }
    const auto result=p.acquire(owner,token,std::span(regions).first(region_count),
        {},w.render_frame);
    if(result==0xffffffffu)return result;
    const auto copy_out=[&](std::uint32_t at,const auto& value) {
        auto bytes=w.write(at,sizeof(value));
        if(bytes.size()!=sizeof(value))throw FormatError("source model cache output bank truncated");
        std::memcpy(bytes.data(),&value,sizeof(value));
    };
    copy_out(records,p.records);copy_out(bits,p.occupied);
    put(w,pool+0x34,p.cursor);put(w,pool+0x38,p.highwater);put(w,pool+0x3c,p.active_count);
    put(w,pool+0x40,p.next_generation);put(w,body+0xcc,token);
    source_model_refresh_colors(owner,1,w);
    return result;
}
bool source_model_resource_sections(std::uint32_t model,std::uint32_t active,
    std::uint32_t cached,std::int32_t lod,std::uint32_t do_request,
    std::uint32_t selected,std::uint32_t changed,const SourceModelResourceWorld& w) {
    const auto m=metadata(w,model);put<std::uint8_t>(w,changed,0);
    std::uint32_t admitted=0;auto count=read<std::int32_t>(w,m+0x1c);
    for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<count;++i) {
        const auto permutation=read<std::uint8_t>(w,active+i);
        const auto region=read<std::uint32_t>(w,m+0x20)+i*16;
        // The source's MOVZX followed by CMP -1 does not exclude byteFF.
        // Signed permutation-count comparison is the actual admission gate.
        if(std::int32_t(permutation)<read<std::int32_t>(w,region+8)) {
            const auto section=permutation_section(w,region,permutation,lod);
            if(request(w,(do_request&255u)?3u:0u,resource(w,m,section)))
                admitted|=1u<<(i&31u);
        }
        count=read<std::int32_t>(w,m+0x1c);
    }
    admitted&=65535u;bool success=true;count=read<std::int32_t>(w,m+0x1c);
    for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<count;++i) {
        const auto region=read<std::uint32_t>(w,m+0x20)+i*16;
        if(admitted&(1u<<(i&31u))) {
            const auto permutation=read<std::int8_t>(w,active+i);
            const auto section=permutation_section(w,region,std::uint32_t(std::int32_t(permutation)),lod);
            put(w,selected+i,read<std::uint8_t>(w,section));
            put(w,cached+i,read<std::uint8_t>(w,active+i));
        } else if(read<std::uint8_t>(w,active+i)==255)put<std::uint8_t>(w,selected+i,255);
        else if(const auto prior=read<std::uint8_t>(w,cached+i);prior!=255) {
            const auto section=permutation_section(w,region,
                std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(prior))),lod);
            if(request(w,0,resource(w,m,section))) {
                put(w,selected+i,read<std::uint8_t>(w,section));put<std::uint8_t>(w,changed,1);
            } else success=false; // Selected and cached bytes survive failure.
        } else {
            put<std::uint8_t>(w,changed,1);put<std::uint8_t>(w,selected+i,255);
        }
        count=read<std::int32_t>(w,m+0x1c);
    }
    return success;
}
std::uint32_t source_model_cached_regions(std::uint32_t owner,const SourceModelResourceWorld& w) {
    if(!valid_view(w))return 0;
    const auto token=acquire(w,owner);
    if(token==0xffffffffu)return 0;
    return cache_record(w,token)+0xc0+std::uint32_t(w.view)*16;
}
std::int8_t source_model_prior_lod(std::uint32_t owner,const SourceModelResourceWorld& w) {
    if(!valid_view(w))return -1;
    const auto token=acquire(w,owner);
    if(token==0xffffffffu)throw FormatError("source prior LOD requires an allocated cache record");
    return read<std::int8_t>(w,cache_record(w,token)+0xbc+std::uint32_t(w.view));
}
void source_model_cache_lod(std::uint32_t owner,std::uint32_t lod,const SourceModelResourceWorld& w) {
    if(!valid_view(w))return;
    const auto token=acquire(w,owner);
    if(token==0xffffffffu)throw FormatError("source4C640 requires an actual allocated cache record");
    put(w,cache_record(w,token)+0xbc+std::uint32_t(w.view),std::uint8_t(lod));
}
bool source_model_resources(std::uint32_t owner,std::uint32_t first_person,
    std::uint32_t model,std::uint32_t region_count,std::uint32_t lod,
    std::uint32_t prior_lod,std::uint32_t selected,std::uint32_t changed,
    const SourceModelResourceWorld& w) {
    const auto m=metadata(w,model);std::uint32_t active,cached;
    if(!(first_person&255u)) {
        const auto headers=read<std::uint32_t>(w,w.banks.objects+0x44);
        const auto body=read<std::uint32_t>(w,headers+(owner&65535u)*12+8);
        const auto bytes=read<std::int16_t>(w,body+0x118);
        put(w,region_count,std::int32_t(bytes));
        active=body+std::uint32_t(std::int32_t(read<std::int16_t>(w,body+0x11a)));
        put(w,region_count,std::int32_t(bytes/10));
        cached=source_model_cached_regions(owner,w);
    } else {
        if(!w.temporary_regions)throw FormatError("source first-person resource region bank absent");
        active=cached=w.temporary_regions;
        const auto count=read<std::int32_t>(w,m+0x1c);
        put<std::uint32_t>(w,active,0);put<std::uint32_t>(w,active+4,0);
        put(w,region_count,count);
        put<std::uint32_t>(w,active+8,0);put<std::uint32_t>(w,active+12,0);
    }
    bool ok=source_model_resource_sections(model,active,cached,read<std::int16_t>(w,lod),1,selected,changed,w);
    if(!ok && (prior_lod&255u)!=255u) {
        const auto prior=std::bit_cast<std::int8_t>(std::uint8_t(prior_lod));
        ok=source_model_resource_sections(model,active,cached,prior,0,selected,changed,w);
        if(ok)put(w,lod,std::int16_t(prior));
    } else source_model_cache_lod(owner,read<std::uint8_t>(w,lod),w);
    if(!ok)source_model_cache_lod(owner,0xffffffffu,w);
    return ok;
}
}
