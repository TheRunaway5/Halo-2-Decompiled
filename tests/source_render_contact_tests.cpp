#include "halo2/source_render_contact.hpp"
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
using Words=std::vector<std::uint32_t>;
struct Reader {
    Words words;std::size_t at{};
    explicit Reader(std::string_view line) {
        std::istringstream in{std::string(line)};std::uint32_t value;
        while(in>>std::hex>>value)words.push_back(value);
        if(!in.eof())throw FormatError("invalid attribution fixture word");
    }
    std::uint32_t take() {
        if(at==words.size())throw FormatError("short attribution fixture");
        return words[at++];
    }
    float scalar(){return std::bit_cast<float>(take());}
    Vec3 point(){const auto x=scalar(),y=scalar(),z=scalar();return {x,y,z};}
    Transform transform(){const auto s=scalar();const auto a=point(),b=point(),c=point(),p=point();return {s,{a,b,c},p};}
    std::vector<std::byte> bytes() {
        const auto count=take();std::vector<std::byte> out;out.reserve(count);
        for(std::uint32_t i=0;i<(count+3)/4;++i) {
            const auto value=take();
            for(unsigned b=0;b<4 && out.size()<count;++b)out.push_back(std::byte(value>>(b*8)));
        }
        return out;
    }
    void done() const {if(at!=words.size())throw FormatError("extra attribution fixture words");}
};
struct AttributionModel {SourceObjectLightAttributionModel render;ModelCollision collision;};
AttributionModel load_attribution_model(Reader& r) {
    AttributionModel model;auto& m=model.render;m.render_model=r.take();
    for(auto& v:m.position_bounds)v=r.scalar();
    for(auto n=r.take();n;--n)m.inverse_binds.push_back(r.transform());
    for(auto n=r.take();n;--n) {
        std::vector<std::int16_t> region;
        for(auto p=r.take();p;--p)region.push_back(std::int16_t(r.take()));
        m.highest_sections.push_back(std::move(region));
    }
    for(auto n=r.take();n;--n) {
        SourceObjectLightAttributionModel::Section section;
        section.resident=bool(r.take());section.positions.format=std::uint8_t(r.take());
        section.positions.vertex_count=r.take();section.positions.bytes=r.bytes();
        for(auto k=r.take();k;--k)section.indices.push_back(std::uint16_t(r.take()));
        for(auto v:r.bytes())section.leaves.nodes.push_back(std::to_integer<std::uint8_t>(v));
        for(auto k=r.take();k;--k) {
            ObjectLightRenderLeaves::NodeMap map;
            for(auto l=r.take();l;--l) {
                const auto cluster=std::int16_t(r.take()),count=std::int16_t(r.take());
                const auto first=std::int32_t(r.take());map.leaves.push_back({cluster,count,first});
            }
            for(auto f=r.take();f;--f) {
                const auto strip=std::uint16_t(r.take()),triangle=std::uint16_t(r.take());
                const auto node=std::int32_t(r.take());map.references.push_back({strip,triangle,node});
            }
            section.leaves.maps.push_back(std::move(map));
        }
        m.sections.push_back(std::move(section));
    }
    for(auto n=r.take();n;--n) {
        std::vector<ModelCollision::Permutation> region;
        for(auto p=r.take();p;--p) {
            ModelCollision::Permutation permutation;
            for(auto b=r.take();b;--b) {
                RawCollisionBsp bsp;bsp.leaves.resize(r.take());
                for(auto v=r.take();v;--v)
                    bsp.nodes.push_back({std::bit_cast<std::int16_t>(std::uint16_t(r.take())),{}});
                permutation.push_back(std::move(bsp));
            }
            region.push_back(std::move(permutation));
        }
        model.collision.regions.push_back(std::move(region));
    }
    r.done();return model;
}
}
int main(int argc,char** argv){try{
    require(argc==3,"expected literal snapshot and render fixture paths");const std::filesystem::path root=HALO2_SOURCE_DIR;
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(root/"assets/disc/maps/zanzibar.map")){std::cout<<"original source snapshot/map assets unavailable\n";return 77;}
    const auto state=snapshot(argv[1]);auto bsp=Cache(root/"assets/disc/maps/zanzibar.map").load_bsp_surface_lighting();
    const auto posed=posed_banks(root/"tests/fixtures/retail_source_object_posed_target.tsv");
    std::unordered_map<std::uint32_t,AttributionModel> models;
    {std::ifstream file(root/"tests/fixtures/retail_source_object_light_attribution.tsv");require(bool(file),"literal attribution bank missing");
    for(std::string l;std::getline(file,l);)if(l.starts_with("M ")){Reader r{std::string_view(l).substr(2)};auto m=load_attribution_model(r);const auto id=m.render.render_model;models.emplace(id,std::move(m));}}
    require(models.size()==3,"all3literal attribution model banks required");
    std::ifstream fixture(argv[2]);require(bool(fixture),"source render contact fixture missing");
    SourceVectorContactBank bank{bsp};std::vector<std::int16_t> materials;std::vector<std::int8_t> designators;std::vector<SourceVectorFogPlane> fog;std::vector<std::uint32_t> broken;
    std::vector<std::uint8_t> cluster_resident,definition_resident;std::vector<SourceRenderContactOwner> render_owners;
    unsigned cases=0,blocked=0,packet_words=0,contact_words=0,attempts=0,target_count=0,retry_cases=0,guard_cases=0;
    for(std::string line;std::getline(fixture,line);){
        if(line.empty() || line[0]=='#')continue;
        if(line.starts_with("B ")){const auto data=words(line.substr(2));Input in{data};
            bank.bsp_index=std::int16_t(in.take());bank.default_material=std::int16_t(in.take());auto n=in.take();for(unsigned i=0;i<n;++i)materials.push_back(std::int16_t(in.take()));
            n=in.take();for(unsigned i=0;i<n;++i)designators.push_back(std::bit_cast<std::int8_t>(std::uint8_t(in.take())));
            n=in.take();for(unsigned i=0;i<n;++i){SourceVectorFogPlane f;f.material=std::int16_t(in.take());f.plane.normal=in.vec();f.plane.distance=in.scalar();fog.push_back(f);}
            n=in.take();for(unsigned i=0;i<n;++i)broken.push_back(in.take());in.done();
            bank.global_materials=materials;bank.cluster_fog=designators;bank.fog_planes=fog;bank.breakable_bits=broken;
            require(bsp.bsp==bank.bsp_index && designators.size()==bsp.clusters.size(),"snapshot active authored BSP identity mismatch");continue;
        }
        if(line.starts_with("L ")){const auto data=words(line.substr(2));Input in{data};for(auto n=in.take();n;--n)cluster_resident.push_back(std::uint8_t(in.take()));for(auto n=in.take();n;--n)definition_resident.push_back(std::uint8_t(in.take()));in.done();continue;}
        if(line.starts_with("O ")){const auto data=words(line.substr(2));Input in{data};SourceRenderContactOwner o;o.full_handle=in.take();o.current_type=std::int8_t(in.take());auto& d=o.definition;
            d.datum=in.take();d.model=in.take();d.render_model=in.take();d.collision=in.take();d.prt_count=in.take();d.section_render_leaves_count=in.take();d.scenery_policy=std::int16_t(in.take());in.done();
            if(const auto m=models.find(d.render_model);m!=models.end())o.render=&m->second.render;
            if(const auto m=posed.models.find(d.model);m!=posed.models.end())o.collision=&m->second;
            if(const auto own=posed.owners.find(std::uint16_t(o.full_handle));own!=posed.owners.end()){o.active_permutations=own->second.active;o.world_nodes=own->second.nodes;}
            render_owners.push_back(o);continue;
        }
        require(render_owners.size()==202 && cluster_resident.size()==55 && definition_resident.size()==19,"actual render/query banks incomplete");
        const auto tab=line.find('\t'),bar=line.find('|');require(tab!=std::string::npos && bar!=std::string::npos,"invalid upper query row");const auto label=line.substr(0,tab);
        const auto input=words(line.substr(tab+1,bar-tab-1)),expected=words(line.substr(bar+1));Input in{input};SourceRenderContactInput query;
        query.bank_valid=bool(in.take());query.initial_query=bool(in.take());query.excluded=in.take();query.origin=in.vec();query.delta=in.vec();for(auto& v:query.caller_packet)v=in.take();for(auto& v:query.collision_packet)v=in.take();in.done();
        const auto target=[&](std::uint32_t id,const auto&,const auto& forwarded)->std::optional<PosedCollisionHit>{const auto slot=std::uint16_t(id);
            if(posed.lookup_misses.contains(slot))return {};const auto owner=posed.owners.find(slot);require(owner!=posed.owners.end(),label+": actual posed target owner missing");const auto& o=owner->second;
            return source_object_posed_target(posed.models.at(o.model),o.nodes,std::span<const std::uint8_t>(o.active),forwarded.origin,forwarded.delta,forwarded.lower_flags);};
        if(line.starts_with("U ")){bool rejected=false;try{source_render_contact(bank,state.collidable,state.owners,render_owners,cluster_resident,definition_resident,target,query);}catch(const FormatError& e){
            require(std::string(e.what()).find("residency scheduling unresolved")!=std::string::npos,label+": unexpected missing-input rejection: "+e.what());rejected=true;}
            require(rejected,label+": unknown original residency silently admitted");++blocked;continue;}
        const auto actual=source_render_contact(bank,state.collidable,state.owners,render_owners,cluster_resident,definition_resident,target,query);
        Input exp{expected};require(bool(exp.take())==actual.hit,label+": source attribution status differs");const auto start=exp.vec();
        require(std::bit_cast<std::uint32_t>(start.x)==std::bit_cast<std::uint32_t>(actual.origin.x) && std::bit_cast<std::uint32_t>(start.y)==std::bit_cast<std::uint32_t>(actual.origin.y) && std::bit_cast<std::uint32_t>(start.z)==std::bit_cast<std::uint32_t>(actual.origin.z),label+": original current query start differs");
        for(unsigned i=0;i<14;++i){const auto e=exp.take();if(e!=actual.packet[i]){std::cerr<<label<<" attributionword"<<i<<" actual"<<std::hex<<actual.packet[i]<<" expected"<<e<<'\n';throw FormatError("complete original56byte attribution packet differs");}}
        const auto count=exp.take();require(count==actual.attempts.size(),label+": original retry count differs");
        for(const auto& a:actual.attempts){require(exp.take()==a.flags && exp.take()==a.excluded,label+": original retry flags/exclusion differs");const auto origin=exp.vec();
            require(std::bit_cast<std::uint32_t>(origin.x)==std::bit_cast<std::uint32_t>(a.origin.x) && std::bit_cast<std::uint32_t>(origin.y)==std::bit_cast<std::uint32_t>(a.origin.y) && std::bit_cast<std::uint32_t>(origin.z)==std::bit_cast<std::uint32_t>(a.origin.z),label+": original retry origin differs");
            require(bool(exp.take())==a.vector.hit,label+": original vector status differs");for(unsigned i=0;i<24;++i){const auto e=exp.take();if(e!=a.vector.packet[i]){std::cerr<<label<<" vectorword"<<i<<" actual"<<std::hex<<a.vector.packet[i]<<" expected"<<e<<'\n';throw FormatError("original retry vector packet differs");}}
            const auto targets=exp.take();require(targets==a.vector.targets.size(),label+": original target count differs");for(const auto id:a.vector.targets)require(exp.take()==id,label+": original target order differs");target_count+=targets;
        }exp.done();++cases;guard_cases+=!query.bank_valid;retry_cases+=count>1;attempts+=count;packet_words+=14;contact_words+=count*24;
    }
    require(cases==3758 && blocked==154 && guard_cases==60,"whole original upper-query coverage changed");
    std::cout<<std::dec<<cases<<" unchanged complete14AF40 cases / "<<packet_words<<" attribution words / "<<contact_words<<" vector words / "<<target_count<<" ordered targets exact over actual posed banks; "<<retry_cases<<" retry cases, "<<guard_cases<<" guard cases; "<<blocked<<" original async residency cases explicitly rejected; no live activation or invented residency\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
