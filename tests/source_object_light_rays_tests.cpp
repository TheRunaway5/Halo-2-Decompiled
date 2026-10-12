#include "halo2/source_object_light_rays.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
constexpr std::uint32_t none=0xffffffffu;
void require(bool v,const std::string& message){if(!v)throw FormatError(message);}
struct Reader {
    std::vector<std::uint32_t> values;std::size_t at{};
    explicit Reader(const std::string& s){std::istringstream in(s);std::uint32_t v;while(in>>std::hex>>v)values.push_back(v);require(in.eof(),"invalid light ray fixture word");}
    std::uint32_t take(){require(at<values.size(),"truncated light ray fixture");return values[at++];}
    float scalar(){return std::bit_cast<float>(take());}
    Vec3 vec(){return {scalar(),scalar(),scalar()};}
    template<std::size_t N> std::array<std::uint32_t,N> array(){std::array<std::uint32_t,N> out;for(auto& v:out)v=take();return out;}
    void done()const{require(at==values.size(),"trailing light ray fixture words");}
};
std::array<std::uint32_t,3> bits(Vec3 p){return{std::bit_cast<std::uint32_t>(p.x),std::bit_cast<std::uint32_t>(p.y),std::bit_cast<std::uint32_t>(p.z)};}
std::array<std::uint32_t,5> bits(const ObjectLightmapSample& s){const auto p=bits(s.incident);return{p[0],p[1],p[2],s.diffuse_rgb,s.lightmap_rgb};}
ObjectLightmapSample sample(const std::array<std::uint32_t,5>& w){return{{std::bit_cast<float>(w[0]),std::bit_cast<float>(w[1]),std::bit_cast<float>(w[2])},w[3],w[4]};}
struct Step {
    Vec3 origin,delta;std::uint32_t excluded{},initial{};
    SourceRenderContactPacket incoming{},outgoing{};
    std::uint32_t hit{},status{},sample_flags{},sample_initial{};
    std::array<std::uint32_t,5> output{},input{};
};
struct UnknownService {bool sample;};
void guards(){
    SourceObjectLightRaysInput in;in.flags=4;
    ObjectLightmapSample out{{-1,2,-3},0x12345678,0xabcdef01};const auto original=bits(out);
    const auto result=source_object_light_rays(in,{}, {},out);
    require(result.status==2 && result.steps.empty() && bits(out)==original,"invalid bank must preserve output and bypass missing services");
    in.bank_valid=true;bool rejected=false;
    try{source_object_light_rays(in,{}, {},out);}catch(const FormatError&){rejected=true;}
    require(rejected,"missing selected custom ray must remain an explicit boundary");
    in.flags=0;rejected=false;
    try{source_object_light_rays(in,{}, {},out);}catch(const FormatError&){rejected=true;}
    require(rejected,"missing contact service must remain an explicit boundary");
    rejected=false;
    try{source_object_light_rays(in,[](const auto&,auto&){return true;},{},out);}catch(const FormatError&){rejected=true;}
    require(rejected,"missing sample service must remain an explicit boundary");
    unsigned calls=0;const auto miss=source_object_light_rays(in,[&](const auto&,auto&){++calls;return false;},{},out);
    require(calls==1 && miss.status==2 && bits(out)==original,"uncontacted ray must not require a sample service");
    // Whole uint32 flags are forwarded, while their literal bit0/bit2 select rays.
    in.flags=0x010005u;calls=0;
    const auto five=source_object_light_rays(in,[&](const auto& q,auto&){require(q.ordinal==calls++,"five ray ordinal");return true;},
        [&](const auto&,auto flags,bool,auto&){require(flags==in.flags,"sample flags truncated");return 2;},out);
    require(calls==5 && five.steps.size()==5,"bit0 must select five and bypass missing custom ray");
}
}
int main(int argc,char** argv){try{
    require(argc==2,"expected original light ray fixture");std::ifstream file(argv[1]);require(bool(file),"original light ray fixture unavailable");guards();
    std::size_t complete=0,unknown=0,suffix=0,cache_empty=0,contacts=0,samples=0,words=0;unsigned statuses[4]{};
    for(std::string line;std::getline(file,line);){if(line.empty() || line[0]=='#')continue;
        require(line[0]=='Q' || line[0]=='U',"unknown light ray fixture row");const bool unresolved=line[0]=='U';
        const auto tab=line.find('\t'),bar=line.find('|');require(tab!=std::string::npos && bar!=std::string::npos,"malformed light ray fixture row");
        const auto name=line.substr(2,tab-2);Reader r(line.substr(tab+1,bar-tab-1)),e(line.substr(bar+1));
        SourceObjectLightRaysInput in;in.bank_valid=r.take()!=0;in.flags=r.take();in.excluded=r.take();in.initial_query=r.take()!=0;in.origin=r.vec();
        const bool custom=r.take()!=0;const auto ray=r.vec();if(custom)in.custom_delta=ray;in.caller_contact=r.array<14>();auto output=sample(r.array<5>());r.done();
        const auto status=e.take();const auto expected_sample=e.array<5>();const auto expected_contact=e.array<14>();std::vector<Step> expected;
        for(auto n=e.take();n;--n){Step s;s.origin=e.vec();s.delta=e.vec();s.excluded=e.take();s.initial=e.take();s.incoming=e.array<14>();s.hit=e.take();s.outgoing=e.array<14>();s.status=e.take();s.output=e.array<5>();s.sample_flags=e.take();s.sample_initial=e.take();s.input=e.array<5>();expected.push_back(s);}e.done();
        std::size_t next=0;bool awaiting_sample=false,threw=false;SourceObjectLightRaysResult result;
        const auto contact=[&](const SourceObjectLightRayRequest& q,SourceRenderContactPacket& p){
            require(!awaiting_sample && next<expected.size(),name+": unexpected contact call");const auto& s=expected[next++];
            require(q.ordinal==next-1 && bits(q.origin)==bits(s.origin) && bits(q.delta)==bits(s.delta) && q.excluded==s.excluded && q.initial_query==(s.initial!=0) && p==s.incoming,name+": contact request/input differs");
            contacts++;words+=22;
            if(s.hit==none)throw UnknownService{false};require(s.hit<=1,name+": invalid source bool");p=s.outgoing;awaiting_sample=s.hit!=0;words+=14;return s.hit!=0;
        };
        const auto sampler=[&](const SourceRenderContactPacket& p,std::uint32_t flags,bool initial,ObjectLightmapSample& value){
            require(awaiting_sample && next!=0,name+": unexpected sample call");const auto& s=expected[next-1];awaiting_sample=false;
            require(p==s.outgoing && flags==s.sample_flags && initial==(s.sample_initial!=0) && bits(value)==s.input,name+": sample packet/arguments/prior output differ");samples++;words+=21;
            if(s.status==none)throw UnknownService{true};value=sample(s.output);words+=5;return std::int32_t(s.status);
        };
        try{result=source_object_light_rays(in,contact,sampler,output);}catch(const UnknownService& boundary){
            threw=true;require(unresolved && next==expected.size(),name+": unexpected service boundary");const auto& s=expected.back();require(boundary.sample?(s.hit==1 && s.status==none):s.hit==none,name+": wrong unknown service stage");
        }
        require(next==expected.size() && !awaiting_sample,name+": source call count differs");
        if(unresolved){require(threw && status==none,name+": missing kernel must not become a source miss/status");++unknown;continue;}
        require(!threw && std::uint32_t(result.status)==status && bits(output)==expected_sample && result.contact==expected_contact,name+": final status/output/contact differs");
        require(result.steps.size()==expected.size(),name+": result trace count differs");
        for(std::size_t j=0;j<expected.size();++j){const auto& a=result.steps[j];const auto& b=expected[j];require(a.contacted==(b.hit!=0) && a.contact==b.outgoing && (b.hit? a.sample_status && std::uint32_t(*a.sample_status)==b.status:!a.sample_status),name+": result trace differs");words+=16;}
        require(status<4,name+": unexpected source status");statuses[status]++;words+=20;++complete;
        suffix+=name.starts_with("suffix_");cache_empty+=name.starts_with("empty_cache_");
    }
    require(complete==3383 && unknown==407 && suffix==32 && cache_empty==26 && contacts==4110 && samples==2670,"incomplete original light ray corpus");
    require(statuses[0]==2476 && statuses[1]==34 && statuses[2]==865 && statuses[3]==8,"original source/suffix status census changed");
    std::cout<<complete-suffix<<" complete original D2A50 calls ("<<cache_empty<<" count-zero current-PRT controls), "<<suffix
        <<" separately supplied D2B72 return-state controls; "<<unknown<<" unknown kernel paths rejected; "<<contacts<<" contact and "<<samples<<" sample entries, "<<words<<" words exact\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
