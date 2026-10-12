#include "halo2/source_vector_contact.hpp"
#include "halo2/source_object_posed_target.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

using namespace halo2;
namespace {
void require(bool value,const std::string& message){if(!value)throw FormatError(message);}
std::vector<std::uint32_t> words(const std::string& line){std::istringstream in(line);std::vector<std::uint32_t> out;std::uint32_t v;while(in>>std::hex>>v)out.push_back(v);return out;}
struct Input {
    const std::vector<std::uint32_t>& data;std::size_t at{};
    std::uint32_t take(){require(at<data.size(),"truncated source vector fixture");return data[at++];}
    float scalar(){return std::bit_cast<float>(take());}
    Vec3 vec(){return {scalar(),scalar(),scalar()};}
    Transform transform(){const auto s=scalar();const auto a=vec(),b=vec(),c=vec(),p=vec();return{s,{a,b,c},p};}
    std::vector<std::byte> raw(unsigned count){require(count%4==0,"unaligned original collision block");std::vector<std::byte> out;for(unsigned i=0;i<count/4;++i){const auto w=take();for(unsigned j=0;j<4;++j)out.push_back(std::byte(w>>(j*8)));}return out;}
    void done(){require(at==data.size(),"trailing source vector fixture fields");}
};
struct Bytes {
    std::vector<std::uint8_t> data;std::size_t at{};
    std::uint32_t word(std::size_t p,unsigned count=4)const{require(p+count<=data.size(),"raw source snapshot field outside bank");std::uint32_t v=0;for(unsigned i=0;i<count;++i)v|=std::uint32_t(data[p+i])<<(i*8);return v;}
    std::uint32_t take(){const auto v=word(at);at+=4;return v;}
    Bytes block(){const auto size=take();require(at+size<=data.size(),"truncated source snapshot bank");Bytes out;out.data.assign(data.begin()+at,data.begin()+at+size);at+=size;return out;}
};
void pool(SourceObjectSpatialDatumPool& out,const Bytes& header,const Bytes& raw,const Bytes& occupied){
    require(header.data.size()==76 && occupied.data.size()==256 && header.word(0x20)==2048,"invalid literal source spatial header");
    const auto stride=header.word(0x24);require(stride==12 || stride==32,"invalid literal source spatial stride");
    require(raw.data.size()==stride*2048,"incomplete original spatial record bank");
    out.record_words=std::uint8_t(stride/4);out.cursor=header.word(0x34);out.highwater=header.word(0x38);out.active_count=header.word(0x3c);
    out.next_generation=std::uint16_t(header.word(0x40,2));out.debug_fill=(header.word(0x2a,2)&8)!=0;
    for(unsigned i=0;i<2048;++i)for(unsigned j=0;j<stride/4;++j)out.records[i][j]=raw.word(i*stride+j*4);
    for(unsigned i=0;i<64;++i)out.occupied[i]=occupied.word(i*4);
}
struct Snapshot {
    SourceObjectSpatialPool collidable{512};std::vector<SourceObjectVectorOwner> owners;
};
Snapshot snapshot(const std::filesystem::path& path){
    std::ifstream file(path,std::ios::binary);require(bool(file),"raw source snapshot unavailable");Bytes b;b.data.assign(std::istreambuf_iterator<char>(file),{});
    require(b.data.size()>=16 && std::string(b.data.begin(),b.data.begin()+8)=="H2OASNP1","unsupported literal source snapshot");b.at=8;
    require(b.take()==55 && b.take()==17,"unexpected snapshot map/bank counts");
    const auto header=b.block(),handles=b.block(),occupied=b.block();
    require(header.word(0x20)==2048 && header.word(0x24)==12 && header.word(0x3c)==202,"original2048slot/202owner bank missing");
    Snapshot out;
    for(unsigned side=0;side<2;++side){
        const auto heads=b.block();require(heads.data.size()==2048,"all512source heads required");
        if(!side)for(unsigned i=0;i<512;++i)out.collidable.heads[i]=heads.word(i*4);
        for(unsigned kind=0;kind<2;++kind){const auto h=b.block(),r=b.block(),o=b.block();if(!side)pool(kind?out.collidable.references:out.collidable.entries,h,r,o);}
    }
    const auto count=b.take();require(count==202,"all202actual owners required");unsigned raw_bytes=0;
    for(unsigned i=0;i<count;++i){
        const auto id=b.take(),address=b.take();const auto raw=b.block();const auto slot=id&65535u;
        require(slot<2048 && (occupied.word((slot/32)*4)&(1u<<(slot&31))),"actual source owner slot not occupied");
        require(handles.word(slot*12,2)==id>>16 && handles.word(slot*12+8)==address && handles.word(slot*12+6,2)==raw.data.size(),"raw source handle/address/size mismatch");
        const auto f=[&](unsigned p){return std::bit_cast<float>(raw.word(p));};
        out.owners.push_back({id,raw.word(4),std::uint8_t(handles.word(slot*12+2,1)),std::uint8_t(handles.word(slot*12+3,1)),
            raw.word(0xc),raw.word(0x10),std::uint8_t(raw.word(0x10a,1)),std::uint8_t(raw.data.size()>0x348?raw.word(0x348,1):0),
            raw.data.size()>=0x130?raw.word(0x12c):0,{{f(0x40),f(0x44),f(0x48)},f(0x4c)}});
        raw_bytes+=unsigned(raw.data.size());
    }
    require(b.at==b.data.size() && raw_bytes==127296,"incomplete raw owner export");return out;
}
struct Service {std::uint32_t requested{};std::optional<PosedCollisionHit> hit;};
std::vector<Service> services(const std::vector<std::uint32_t>& values){
    Input in{values};std::vector<Service> out;const auto count=in.take();
    for(unsigned i=0;i<count;++i){Service s;s.requested=in.take();const auto hit=in.take();require(hit<=1,"invalid source target status");if(hit){
        PosedCollisionHit h;h.fraction=in.scalar();h.plane.normal=in.vec();h.plane.distance=in.scalar();h.packed_bsp=in.take();
        h.leaf=std::int32_t(in.take());h.surface=std::int32_t(in.take());h.signed_plane=std::int32_t(in.take());h.region=std::int16_t(in.take());h.node=std::int16_t(in.take());
        h.model_material=std::int16_t(in.take());h.global_material=std::int16_t(in.take());h.flags=std::uint8_t(in.take());h.breakable=std::uint8_t(in.take());s.hit=h;
    }out.push_back(s);}in.done();return out;
}
RawCollisionBsp collision_bsp(Input& r){
    RawCollisionBsp out;out.node=std::int16_t(r.take());constexpr unsigned strides[]{8,16,4,4,16,8,12,16};
    for(unsigned k=0;k<8;++k){const auto n=r.take();const auto raw=r.raw(n*strides[k]);const halo2::Bytes b(raw);
        const auto byte=[&](unsigned p){return std::to_integer<std::uint8_t>(b.slice(p,1)[0]);};
        const auto sh=[&](unsigned p){return std::bit_cast<std::int16_t>(b.u16(p));};
        const auto point=[&](unsigned p){return Vec3{b.f32(p),b.f32(p+4),b.f32(p+8)};};
        const auto child=[&](unsigned p){const auto v=std::uint32_t(byte(p))|(std::uint32_t(byte(p+1))<<8)|(std::uint32_t(byte(p+2))<<16);return std::bit_cast<std::int32_t>(v|(v&0x800000u?0xff000000u:0));};
        for(unsigned i=0;i<n;++i){const auto p=i*strides[k];switch(k){
            case 0:out.nodes.push_back({sh(p),{child(p+2),child(p+5)}});break;
            case 1:out.planes.push_back({point(p),b.f32(p+12)});break;
            case 2:out.leaves.push_back({byte(p),byte(p+1),b.u16(p+2)});break;
            case 3:out.references.push_back({sh(p),sh(p+2)});break;
            case 4:out.nodes2.push_back({{b.f32(p),b.f32(p+4)},b.f32(p+8),{sh(p+12),sh(p+14)}});break;
            case 5:out.surfaces.push_back({sh(p),b.u16(p+2),byte(p+4),byte(p+5),sh(p+6)});break;
            case 6:out.edges.push_back({{b.u16(p),b.u16(p+2)},{b.u16(p+4),b.u16(p+6)},{b.u16(p+8),b.u16(p+10)}});break;
            case 7:out.vertices.push_back(point(p));break;
        }}
    }return out;
}
struct PosedOwner {std::uint32_t model{};std::vector<Transform> nodes;std::vector<std::uint8_t> active;};
struct PosedBanks {
    std::unordered_map<std::uint32_t,ModelCollision> models;
    std::unordered_map<std::uint16_t,PosedOwner> owners;
    std::unordered_set<std::uint16_t> lookup_misses;
};
PosedBanks posed_banks(const std::filesystem::path& path){
    std::ifstream file(path);require(bool(file),"literal original posed-target banks unavailable");PosedBanks out;
    for(std::string line;std::getline(file,line);){if(line.empty() || line[0]=='#' || line[0]=='Q' || line[0]=='X')continue;
        const auto data=words(line.substr(2));Input r{data};
        if(line[0]=='M'){
            ModelCollision m;m.model=r.take();m.collision=r.take();
            for(auto n=r.take();n;--n){const auto d=std::int16_t(r.take()),g=std::int16_t(r.take());m.materials.push_back({d,g});}
            for(auto n=r.take();n;--n){ModelCollision::Region region{std::uint8_t(r.take()),{}};for(auto p=r.take();p;--p)region.permutations.push_back(std::uint8_t(r.take()));m.model_regions.push_back(std::move(region));}
            for(auto n=r.take();n;--n){std::vector<ModelCollision::Permutation> region;for(auto p=r.take();p;--p){ModelCollision::Permutation permutation;for(auto b=r.take();b;--b)permutation.push_back(collision_bsp(r));region.push_back(std::move(permutation));}m.regions.push_back(std::move(region));}
            const auto id=m.model;require(out.models.emplace(id,std::move(m)).second,"duplicate original collision model");
        }else if(line[0]=='O'){
            const auto id=r.take();PosedOwner o;o.model=r.take();for(auto n=r.take();n;--n)o.nodes.push_back(r.transform());for(auto n=r.take();n;--n)o.active.push_back(std::uint8_t(r.take()));
            require(out.owners.emplace(std::uint16_t(id),std::move(o)).second,"duplicate original posed owner slot");
        }else if(line[0]=='N'){require(out.lookup_misses.insert(std::uint16_t(r.take())).second,"duplicate source lookup miss");}
        else throw FormatError("unknown posed-target bank row");r.done();
    }
    require(out.models.size()==39 && out.owners.size()==158 && out.lookup_misses.size()==44,"incomplete actual posed metadata/owner bank");return out;
}
std::vector<std::uint32_t> target_packet(const PosedCollisionHit& h){return {
    std::bit_cast<std::uint32_t>(h.fraction),std::bit_cast<std::uint32_t>(h.plane.normal.x),std::bit_cast<std::uint32_t>(h.plane.normal.y),std::bit_cast<std::uint32_t>(h.plane.normal.z),std::bit_cast<std::uint32_t>(h.plane.distance),h.packed_bsp,
    std::uint32_t(h.leaf),std::uint32_t(h.surface),std::uint32_t(h.signed_plane),std::uint32_t(h.region),std::uint32_t(h.node),std::uint32_t(h.model_material),std::uint32_t(h.global_material),h.flags,h.breakable};}
}
int main(int argc,char** argv){try{
    require(argc==3,"expected literal snapshot and vector fixture paths");const std::filesystem::path root=HALO2_SOURCE_DIR;
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(root/"assets/disc/maps/zanzibar.map")){std::cout<<"original source snapshot/map assets unavailable\n";return 77;}
    const auto state=snapshot(argv[1]);auto bsp=Cache(root/"assets/disc/maps/zanzibar.map").load_bsp_surface_lighting();
    const auto posed=posed_banks(root/"tests/fixtures/retail_source_object_posed_target.tsv");
    std::ifstream fixture(argv[2]);require(bool(fixture),"source vector contact fixture missing");
    SourceVectorContactBank bank{bsp};std::vector<std::int16_t> materials;std::vector<std::int8_t> designators;std::vector<SourceVectorFogPlane> fog;std::vector<std::uint32_t> broken;
    std::string line;unsigned cases=0,packet_words=0,target_count=0,hit_targets=0,dynamic_cases=0,static_cases=0,controls=0,fog_controls=0;bool bank_loaded=false;
    while(std::getline(fixture,line)){
        if(line.empty() || line[0]=='#')continue;
        if(line.starts_with("B ")){materials.clear();designators.clear();fog.clear();broken.clear();const auto data=words(line.substr(2));Input in{data};
            bank.bsp_index=std::int16_t(in.take());bank.default_material=std::int16_t(in.take());auto n=in.take();for(unsigned i=0;i<n;++i)materials.push_back(std::int16_t(in.take()));
            n=in.take();for(unsigned i=0;i<n;++i)designators.push_back(std::bit_cast<std::int8_t>(std::uint8_t(in.take())));
            n=in.take();for(unsigned i=0;i<n;++i){SourceVectorFogPlane f;f.material=std::int16_t(in.take());f.plane.normal=in.vec();f.plane.distance=in.scalar();fog.push_back(f);}
            n=in.take();for(unsigned i=0;i<n;++i)broken.push_back(in.take());in.done();
            bank.global_materials=materials;bank.cluster_fog=designators;bank.fog_planes=fog;bank.breakable_bits=broken;
            require(bsp.bsp==bank.bsp_index && designators.size()==bsp.clusters.size(),"snapshot active authored BSP identity mismatch");bank_loaded=true;continue;
        }
        require(bank_loaded,"missing source vector bank");const auto tab=line.find('\t'),bar=line.find('|'),bar2=line.find('|',bar+1);
        require(tab!=std::string::npos && bar!=std::string::npos && bar2!=std::string::npos,"invalid source vector row");const auto label=line.substr(0,tab);
        const auto input=words(line.substr(tab+1,bar-tab-1)),expected=words(line.substr(bar2+1));
        const auto outcomes=services(words(line.substr(bar+1,bar2-bar-1)));
        Input in{input};SourceObjectVectorInput query;query.flags=in.take();query.excluded_a=in.take();query.excluded_b=in.take();query.origin=in.vec();query.delta=in.vec();
        SourceVectorContactPacket caller;for(auto& v:caller)v=in.take();in.done();std::size_t used=0;
        const auto actual=source_vector_contact(bank,state.collidable,state.owners,query,[&](std::uint32_t id,const auto&,const auto& forwarded){
            require(used<outcomes.size(),label+": unexpected native posed target");const auto& s=outcomes[used++];require(s.requested==id,label+": source target order differs");
            require(forwarded.origin.x==query.origin.x && forwarded.delta.z==query.delta.z,label+": query segment changed");return s.hit;
        },caller);
        require(used==outcomes.size(),label+": native traversal skipped an original target");
        used=0;
        const auto integrated=source_vector_contact(bank,state.collidable,state.owners,query,[&](std::uint32_t id,const auto&,const auto& forwarded)->std::optional<PosedCollisionHit>{
            require(used<outcomes.size(),label+": unexpected integrated posed target");const auto& expected_target=outcomes[used++];require(expected_target.requested==id,label+": integrated target order differs");
            const auto slot=std::uint16_t(id);std::optional<PosedCollisionHit> hit;
            if(!posed.lookup_misses.contains(slot)){
                const auto owner=posed.owners.find(slot);require(owner!=posed.owners.end(),label+": actual posed owner bank missing");const auto& o=owner->second;
                hit=source_object_posed_target(posed.models.at(o.model),o.nodes,std::span<const std::uint8_t>(o.active),forwarded.origin,forwarded.delta,forwarded.lower_flags);
            }
            require(bool(hit)==bool(expected_target.hit),label+": integrated target source status differs");
            if(hit)require(target_packet(*hit)==target_packet(*expected_target.hit),label+": integrated target source15field packet differs");return hit;
        },caller);
        require(used==outcomes.size() && integrated.hit==actual.hit && integrated.packet==actual.packet && integrated.targets==actual.targets,label+": native complete map+posed-target dispatcher differs");
        Input exp{expected};require(bool(exp.take())==actual.hit,label+": source hit status differs");
        for(unsigned i=0;i<24;++i){const auto e=exp.take();if(e!=actual.packet[i]){std::cerr<<label<<" word"<<i<<" actual"<<std::hex<<actual.packet[i]<<" expected"<<e<<'\n';throw FormatError("ordinary source96byte contact packet differs");}}
        // Retained169510 wrapper-entry observations are diagnostic. The exact
        // admitted1EFB40 target order is compared without reconstructing them.
        auto n=exp.take();for(unsigned i=0;i<n*2;++i)exp.take();n=exp.take();require(n==actual.targets.size(),label+": source target count differs");
        for(auto id:actual.targets)require(exp.take()==id,label+": source target sequence differs");exp.done();
        ++cases;packet_words+=24;target_count+=unsigned(outcomes.size());for(const auto& s:outcomes)hit_targets+=bool(s.hit);
        dynamic_cases+=label.starts_with("dynamic_");static_cases+=label.starts_with("static_");controls+=label.starts_with("control_");fog_controls+=label.starts_with("fog_");
    }
    require(dynamic_cases==3792 && static_cases==948 && controls>0 && fog_controls>0,"original contact coverage changed");
    bool rejected=false;try{SourceObjectVectorInput q;q.flags=0x20000000;source_vector_contact(bank,state.collidable,state.owners,q,{});}catch(const FormatError&){rejected=true;}
    require(rejected,"unproved source location repair admitted");
    std::cout<<std::dec<<cases<<" original ordinary map+dynamic dispatcher cases / "<<packet_words<<" packet words exact in both callback-boundary and complete native posed-target passes; "<<dynamic_cases<<" actual-pool dynamic + "<<static_cases<<" static + "<<controls<<" flags/sentinel + "<<fog_controls<<" synthetic fog-input controls; "<<target_count<<" ordered source targets / "<<hit_targets<<" independently executed target hits; no native registry activation\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
