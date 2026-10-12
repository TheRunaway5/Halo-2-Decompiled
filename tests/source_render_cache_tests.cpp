#include "halo2/source_render_cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
Words parse(std::string_view line) {
    std::istringstream s{std::string(line)};Words out;std::uint32_t v;
    while(s>>std::hex>>v)out.push_back(v);
    if(!s.eof())throw FormatError("invalid render cache fixture word");
    return out;
}
std::uint32_t hash(const SourceRenderCachePool& p) {
    std::uint32_t h=2166136261;
    for(const auto& r:p.records)for(auto w:r.words)
        for(unsigned b=0;b<4;++b){h^=std::uint8_t(w>>(b*8));h*=16777619;}
    return h;
}
Words execute(char kind,const Words& w) {
    std::size_t at=0;
    auto take=[&](){if(at>=w.size())throw FormatError("short render cache fixture");return w[at++];};
    auto done=[&](){if(at!=w.size())throw FormatError("extra render cache fixture words");};
    if(kind=='C') {
        SourceRenderContext c;c.words.fill(take());done();source_render_context_initialize(c);
        return {c.words.begin(),c.words.end()};
    }
    if(kind=='R') {
        SourceRenderCachePool p;const auto fill=take(),debug=take();done();
        for(auto& r:p.records)r.words.fill(fill);
        p.occupied.fill(0xffffffff);p.cursor=p.highwater=p.active_count=256;
        p.next_generation=0xfffe;p.activate(debug!=0);
        return {p.cursor,p.highwater,p.active_count,p.next_generation,p.active,
                p.occupied[0],p.occupied[7],p.records[0].words[0],p.records[0].words[1],
                p.records[255].words[0],p.records[255].words[63],hash(p)};
    }
    if(kind=='P' || kind=='L') {
        SourceRenderCachePool p;
        p.cursor=take();p.highwater=take();p.active_count=take();p.next_generation=std::uint16_t(take());
        const auto fill=take(),owner=take();auto token=take();const auto frame=take();
        const auto regions=take(),colors=take();
        std::array<std::uint8_t,16> region{};
        for(unsigned i=0;i<4;++i){const auto word=take();for(unsigned b=0;b<4;++b)region[i*4+b]=std::uint8_t(word>>(8*b));}
        std::array<Vec3,4> rgb{};
        for(auto& color:rgb)color={std::bit_cast<float>(take()),std::bit_cast<float>(take()),std::bit_cast<float>(take())};
        for(auto& r:p.records)r.words.fill(fill);
        const auto count=take();
        for(unsigned i=0;i<count;++i) {
            const auto index=take(),salt=take(),record_owner=take(),last=take(),record_fill=take();
            if(index>=256)throw FormatError("invalid warm source cache index");
            auto& r=p.records[index];r.words.fill(record_fill);
            r.words[0]=(record_fill&0xffff0000)|(salt&0xffff);r.words[1]=record_owner;r.words[3]=last;
            p.occupied[index/32]|=1u<<(index&31);
        }
        std::int32_t view=-1;std::int8_t lod=-1;
        if(kind=='L'){view=std::bit_cast<std::int32_t>(take());lod=std::int8_t(take());}
        done();if(regions>16 || colors>4)throw FormatError("invalid source color/region count");
        std::uint32_t result;
        if(kind=='L') {
            p.set_view_lod(owner,token,std::span(region).first(regions),std::span(rgb).first(colors),frame,view,lod);
            result=token;
        } else result=p.acquire(owner,token,std::span(region).first(regions),std::span(rgb).first(colors),frame);
        const auto index=result==0xffffffff?0:result&0xffff;
        Words out={result,token,p.cursor,p.highwater,p.active_count,p.next_generation};
        out.insert(out.end(),p.occupied.begin(),p.occupied.end());out.push_back(index);
        out.insert(out.end(),p.records[index].words.begin(),p.records[index].words.end());out.push_back(hash(p));return out;
    }
    if(kind=='B') {
        const bool present=take()!=0;const auto token=take(),stamp=take();
        const auto prior=std::int8_t(take()),requested=std::int8_t(take());
        const auto frame=take();const auto view=std::bit_cast<std::int32_t>(take());done();
        const auto out=source_render_geometry_begin(present,token,stamp,prior,requested,frame,view);
        return {out.reuse_candidate,std::uint32_t(std::int32_t(out.selected_lod))};
    }
    if(kind=='G') {
        SourceRenderGeometryInput in;in.begin.reuse_candidate=take()!=0;
        in.requested_lod=std::int8_t(take());in.resource_succeeded=take()!=0;in.region_changed=take()!=0;
        in.actual_lod=std::int16_t(take());in.node_count=std::bit_cast<std::int32_t>(take());
        in.source_model_ordinal=std::bit_cast<std::int32_t>(take());in.first_person=take()!=0;in.context_present=take()!=0;
        in.context_token=take();in.context_stamp=take();in.frame=take();in.view=std::bit_cast<std::int32_t>(take());
        auto cursor=take();done();const auto out=source_render_geometry_allocate(in,cursor);
        return {std::uint32_t(out.continuation),out.reused,out.context_written,out.node_count,
                out.token,cursor,out.context_token,out.context_stamp};
    }
    if(kind=='T') {
        const bool reuse=take(),success=take(),context=take(),prt=take(),prt_success=take();done();
        const auto out=source_render_geometry_tail(reuse,success,context,prt,prt_success);
        return {out.call_prt,out.call_colors,out.call_owner_query,out.accepted};
    }
    if(kind=='M') {
        const auto owner=take(),token=take(),record_owner=take(),before=take(),frame=take();done();
        SourceRenderCachePool p;
        const unsigned index=token==0xffffffff?0:token&0xffff;
        if(index>=256)throw FormatError("invalid mark-rendered fixture token");
        p.records[index].words[1]=record_owner;p.records[index].words[3]=before;
        p.mark_rendered(owner,token,frame);return {p.records[index].words[3]};
    }
    if(kind=='D') {
        const auto requested=std::int16_t(take());const auto prior=std::int8_t(take());
        const bool initial=take(),fallback=take();done();
        const auto out=source_render_resource_decision(requested,prior,initial,fallback);
        return {out.fallback_requested,out.succeeded,std::uint32_t(std::int32_t(out.actual_lod)),
                out.cache_lod_write_count,std::uint32_t(std::int32_t(out.cache_lod_writes[0])),
                std::uint32_t(std::int32_t(out.cache_lod_writes[1]))};
    }
    if(kind=='F') {
        auto frame=take(),cursor=take();done();source_render_frame_begin(frame,cursor);return {frame,cursor};
    }
    throw FormatError("unknown render cache fixture kind");
}
void print(const Words& words) {
    for(std::size_t i=0;i<words.size();++i)std::cout<<(i?" ":"")<<std::hex<<words[i];
    std::cout<<'\n';
}
}
int main(int argc,char** argv) {try {
    if(argc==2 && std::string_view(argv[1])=="--probe") {
        std::string line;while(std::getline(std::cin,line)) {
            if(line.size()<2)throw FormatError("short render cache probe");
            print(execute(line[0],parse(std::string_view(line).substr(2))));
        }return 0;
    }
    if(argc!=2)throw FormatError("render cache test requires original fixtures");
    std::ifstream f(argv[1]);if(!f)throw FormatError("missing original render cache fixtures");
    std::string line;std::array<unsigned,256> counts{};unsigned cases=0,words=0;
    while(std::getline(f,line)) {
        if(line.empty() || line[0]=='#')continue;
        const auto split=line.find('|');if(split==std::string::npos)throw FormatError("missing render cache fixture separator");
        const auto input=parse(std::string_view(line).substr(2,split-2));
        const auto expected=parse(std::string_view(line).substr(split+1));
        const auto actual=execute(line[0],input);
        if(actual!=expected) {
            std::size_t word=0;while(word<actual.size() && word<expected.size() && actual[word]==expected[word])++word;
            throw FormatError("source render cache fixture differs at case "+std::to_string(cases)+" word "+std::to_string(word));
        }
        ++counts[std::uint8_t(line[0])];++cases;words+=expected.size();
    }
    if(!f.eof() || counts['C']<8 || counts['R']<8 || counts['P']<100 || counts['L']<20 || counts['M']<20 || counts['D']<100 || counts['F']<20 || counts['B']<100 || counts['G']<200 || counts['T']<20)
        throw FormatError("incomplete original render cache fixture coverage");
    SourceRenderCachePool bounded;auto token=0xffffffffu;bool rejected=false;
    try{std::array<std::uint8_t,17> bad{};bounded.acquire(1,token,bad,{},0);}catch(const FormatError&){rejected=true;}
    if(!rejected)throw FormatError("unbounded source region bank accepted");
    rejected=false;try{bounded.owner_record(1,0xe1630100);}catch(const FormatError&){rejected=true;}
    if(!rejected)throw FormatError("unbounded source cache token accepted");
    std::cout<<cases<<" original render-cache fixtures passed; "<<words<<" exact words\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
