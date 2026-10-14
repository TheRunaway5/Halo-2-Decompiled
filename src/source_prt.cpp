#include "halo2/source_prt.hpp"
#include "halo2/model.hpp"
#include <map>
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
float projection_dot(std::span<const float> a,const std::array<float,9>&b){
    // 23CA0's MULPS/ADDPS reduction, not a sequential scalar dot product.
    const float x=(a[0]*b[0]+a[4]*b[4])+(a[2]*b[2]+a[6]*b[6]);
    const float y=(a[1]*b[1]+a[5]*b[5])+(a[3]*b[3]+a[7]*b[7]);
    return y+(a[8]*b[8]+x);
}

}
std::optional<std::vector<SourcePrtVertex>> source_prt_vertex_stream(const SourcePrtDefinition& d,
    unsigned lod,unsigned section,unsigned count){
    if(lod>5 || d.lods.empty())throw FormatError("invalid source PRT LOD");
    const auto& l=d.lods[std::min<std::size_t>(d.lods.size()-1,5-lod)];
    const auto found=std::find_if(l.sections.begin(),l.sections.end(),[&](auto s){return s.section==section;});
    if(found==l.sections.end())return {};
    if(d.stream_descriptor[0]!=std::byte{57} || d.stream_descriptor[1]!=std::byte{10})
        throw FormatError("unsupported source PRT vertex declaration");
    const auto offset=std::size_t(found->offset/5)*10;const Bytes b(d.stream);b.require(offset,std::size_t(count)*10);
    std::vector<SourcePrtVertex> result(count);
    for(unsigned i=0;i<count;++i){const auto at=offset+i*10;result[i].cluster=float(std::bit_cast<std::int16_t>(b.u16(at)));
        for(unsigned j=0;j<4;++j)result[i].weights[j]=std::max(-1.f,float(std::bit_cast<std::int16_t>(b.u16(at+2+j*2)))/32767.f);}
    return result;
}
std::optional<std::vector<SourcePrtVertex>> Cache::load_source_prt_vertex_stream(std::uint32_t datum,unsigned lod,unsigned section)const {
    const auto d=load_source_prt_definition(datum);if(!d)return {};
    const Bytes b(cache_bytes());const auto t=query_tag({.handle=TagHandle{datum}, .group=tag_groups::render_model, .require_address=true});
    const auto p=metadata_offset(t->address,0x2c);const auto sections=TagAddressSpace(b, metadata_magic_).block(p+0x24, 0x5c, 1000000);
    if(section>=sections.count)throw FormatError("PRT section outside render model");
    return source_prt_vertex_stream(*d,lod,section,b.u16(sections.offset+section*0x5c+4));
}
std::vector<std::uint8_t> Cache::source_prt_section_nodes(std::uint32_t datum,unsigned section)const {
    const Bytes b(cache_bytes());const auto t=query_tag({.handle=TagHandle{datum}, .group=tag_groups::render_model, .require_address=true});
    if(!t)throw FormatError("PRT section render model missing");
    const auto p=metadata_offset(t->address,0x4c);const auto sections=TagAddressSpace(b, metadata_magic_).block(p+0x24, 0x5c, 1000000);
    if(section>=sections.count)throw FormatError("PRT node map section outside model");
    const auto s=sections.offset+section*0x5c;const auto raw=raw_data(b.u32(s+0x38),b.u32(s+0x3c));const Bytes data(raw);
    const auto header=b.u32(s+0x44);if(std::uint64_t(header)+4>raw.size())throw FormatError("invalid PRT section raw header");
    const auto base=raw.size()-header-4;const auto count=data.u16(0x6c);
    if(count){
        const auto resources=TagAddressSpace(b, metadata_magic_).block(s+0x48, 16, 1000000);std::vector<std::uint8_t> nodes;
        for(unsigned i=0;i<resources.count;++i){const auto r=resources.offset+i*16;if(b.u16(r+4)!=100)continue;
            if(!nodes.empty() || b.u32(r+8)<count)throw FormatError("invalid PRT section node map");
            for(const auto v:data.slice(base+b.u32(r+12),count))nodes.push_back(std::to_integer<std::uint8_t>(v));}
        if(nodes.size()!=count)throw FormatError("missing PRT section node map");return nodes;
    }
    //137800's no-map palette: section+24 count, or min(model nodes,40).
    unsigned n=std::to_integer<unsigned>(b.slice(s+0x24,1)[0]);if(!n)n=std::min(b.u32(p+0x48),40u);
    std::vector<std::uint8_t> nodes(n);for(unsigned i=0;i<n;++i)nodes[i]=std::uint8_t(i);return nodes;
}
SourcePrtSh source_prt_node_lighting(const RenderModel&model,unsigned node,const Transform&world_node,const SourcePrtLighting&lighting){
    if(node>=model.bones.size())throw FormatError("PRT section references a missing model node");
    //This helper requires the valid authored bank used by all admitted
    //Zanzibar/Triplicate machine draws.23690's invalid-bank white SH and
    //cached-incident fallback require their explicit retained caller state.
    if(!lighting.structure_matches || lighting.records.empty())throw FormatError("PRT node lighting requires the source active SH bank");
    auto sh=source_prt_sample(lighting.records,world_node.position);const auto& b=model.bones[node];
    const Transform inverse_bind{b.inverse_scale,{{{b.inverse[0],b.inverse[1],b.inverse[2]},
        {b.inverse[3],b.inverse[4],b.inverse[5]},{b.inverse[6],b.inverse[7],b.inverse[8]}}},
        {b.inverse[9],b.inverse[10],b.inverse[11]}};
    const auto palette=compose_transform(world_node,inverse_bind);
    std::array<float,9> matrix{};
    for(unsigned i=0;i<3;++i){matrix[i*3]=palette.basis[i].x*palette.scale;
        matrix[i*3+1]=palette.basis[i].y*palette.scale;matrix[i*3+2]=palette.basis[i].z*palette.scale;}
    const auto inverse=source_prt_inverse_basis(matrix);
    for(auto&channel:sh.coefficients)channel=source_prt_rotate(channel,inverse);
    return sh;
}
bool Cache::apply_source_prt_vertices(RenderModel& model,std::uint32_t datum,unsigned lod)const {
    if(model.vertex_origins.size()!=model.mesh.vertices.size())throw FormatError("PRT source vertex origins missing");
    std::map<unsigned,std::optional<std::vector<SourcePrtVertex>>> streams;bool applied=false;
    for(unsigned i=0;i<model.vertex_origins.size();++i){const auto origin=model.vertex_origins[i];
        auto [it,inserted]=streams.try_emplace(origin.section);
        if(inserted)it->second=load_source_prt_vertex_stream(datum,lod,origin.section);
        if(!it->second)continue;
        if(origin.vertex>=it->second->size())throw FormatError("PRT original vertex outside stream");
        const auto& source=it->second->at(origin.vertex);model.mesh.vertices[i].prt_cluster=source.cluster;
        model.mesh.vertices[i].prt_weights=source.weights;applied=true;
    }return applied;
}
SourcePrtSh source_prt_sample(std::span<const SourcePrtShRecord> records,Vec3 position){
    if(records.empty()){
        // Full146020(0,1,0), order3 and direction(0,0,-1), as selected
        // by22CF0. Incident is left untouched by that original branch.
        SourcePrtSh r;
        constexpr std::array<std::uint32_t,9> bits{0x3f558750,0x80000000,0xbfb8ebd1,0x80000000,0,0,0x3feebb70,0,0};
        for(unsigned channel=0;channel<3;++channel)for(unsigned i=0;i<9;++i)
            r.coefficients[channel][i]=std::bit_cast<float>(bits[i])*(channel==1?1.f:0.f);
        return r;
    }
    if(records.size()==1)return records.front().lighting;
    const auto delta=[&](unsigned i){const auto p=records[i].position;return Vec3{position.x-p.x,position.y-p.y,position.z-p.z};};
    float nearest_distance=1048576.f;unsigned nearest=0;
    for(unsigned i=0;i<records.size();++i){const auto d=delta(i);const float distance=(d.z*d.z+d.x*d.x)+d.y*d.y;
        if(distance<nearest_distance){nearest=i;nearest_distance=distance;}}
    const auto a=delta(nearest);int opposite=-1;float opposite_distance=1048576.f;
    for(unsigned i=0;i<records.size();++i){if(i==nearest)continue;const auto d=delta(i);
        if((d.z*a.z+d.y*a.y)+a.x*d.x<=0){const float distance=(d.z*d.z+d.y*d.y)+d.x*d.x;
            if(distance<opposite_distance){opposite=int(i);opposite_distance=distance;}}}
    if(opposite<0)return records[nearest].lighting;
    const auto n=records[nearest].position,o=records[opposite].position;
    const Vec3 line{n.x-o.x,n.y-o.y,n.z-o.z},query{n.x-position.x,n.y-position.y,n.z-position.z};
    //22C30 rounds the x/y differences before their products. Its z
    //difference remains in x87 after FST and multiplies the rounded z copy.
    //The squared length/sqrt is rounded to float before the SSE reciprocal.
    const float length=float(std::sqrt(((double(n.z)-double(o.z))*double(line.z)+double(line.y)*line.y)+double(line.x)*line.x));
    float weight=0;
    if(length>.0001f){const float inverse=1.f/length;
        weight=(((line.z*inverse)*query.z+(line.y*inverse)*query.y)+(line.x*inverse)*query.x)*inverse;}
    if(!(weight>0))return records[nearest].lighting;
    if(weight>=1)return records[opposite].lighting;
    auto result=records[nearest].lighting;const auto& target=records[opposite].lighting;
    for(unsigned c=0;c<3;++c)for(unsigned i=0;i<9;++i)
        result.coefficients[c][i]=(target.coefficients[c][i]-result.coefficients[c][i])*weight+result.coefficients[c][i];
    result.incident={(target.incident.x-result.incident.x)*weight+result.incident.x,
        (target.incident.y-result.incident.y)*weight+result.incident.y,
        (target.incident.z-result.incident.z)*weight+result.incident.z};return result;
}
std::array<float,9> source_prt_inverse_basis(const std::array<float,9>& m){
    const float determinant=((((m[5]*m[1]*m[6]+m[2]*m[7]*m[3])+m[8]*m[4]*m[0])-
        m[5]*m[7]*m[0])-m[8]*m[1]*m[3])-m[2]*m[4]*m[6];
    if(determinant==0 || !std::isfinite(determinant))throw FormatError("singular source PRT node basis");
    const float inverse=1.f/determinant;std::array<float,9> out{};
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)
        out[row+col*3]=(m[(row+1)%3*3+(col+1)%3]*m[(row+2)%3*3+(col+2)%3]-
            m[(row+1)%3*3+(col+2)%3]*m[(row+2)%3*3+(col+1)%3])*inverse;
    return out;
}
std::array<float,9> source_prt_rotate(std::array<float,9> sh,const std::array<float,9>& m){
    const auto a=m[0],b=m[1],c=m[2],d=m[3],e=m[4],f=m[5],g=m[6],h=m[7],i=m[8];
    const std::array<float,9> first{e,0.f-h,b,0.f-f,i,0.f-c,d,0.f-g,a};
    std::array<float,9> out{};out[0]=sh[0];
    for(unsigned row=0;row<3;++row){float value=first[row*3]*sh[1];
        for(unsigned col=1;col<3;++col)value=first[row*3+col]*sh[col+1]+value;out[row+1]=value;}
    constexpr float r=std::bit_cast<float>(0x410a9067u); // sqrt(15)*sqrt(5), x87→f32.
    constexpr float p=.06666667014360428f,q=.03333333507180214f,t=.13333334028720856f;
    const auto ad=d*a,eb=e*b,gd=g*d,gg=g*g,hh=h*h,ga=g*a,aa=a*a,bb=b*b,dd=d*d,ee=e*e;
    const auto ea=e*a,db=d*b,ge=g*e,fb=f*b,ec=e*c,ef=f*e,cb=c*b,fc=f*c,ii=i*i,ff=f*f,cc=c*c;
    const auto ri=r*i;
    const std::array<float,25> second{
        ea+db,(0.f-h*d)-ge,r*h*g*.2f,(0.f-h*a)-g*b,b*a-e*d,
        (0.f-ec)-fb,h*f+i*e,ri*h*-.2f,h*c+i*b,ef-cb,
        (fc*t-ad*p)*r-eb*r*p,(gd*r*p-ri*f*t)+h*e*r*p,(ii-gg*.5f)-hh*.5f,
        (ga*r*p-ri*c*t)+h*b*r*p,((cc*r*p-(ff*p+aa*q)*r)-bb*r*q)+(ee+dd)*r*q,
        (0.f-f*a)-d*c,i*d+g*f,ri*g*-.2f,i*a+g*c,f*d-c*a,
        ad-eb,h*e-gd,gg*r*.1f-hh*r*.1f,h*b-ga,((ee+aa)*.5f-bb*.5f)-dd*.5f};
    for(unsigned row=0;row<5;++row){float value=second[row*5]*sh[4];
        for(unsigned col=1;col<5;++col)value=second[row*5+col]*sh[col+4]+value;out[row+4]=value;}
    return out;
}
ShaderVector source_prt_scale_constants(){return {1,4,std::bit_cast<float>(0x461f6200u),std::bit_cast<float>(0x3b80a166u)};}
std::vector<ShaderVector> source_prt_project(const SourcePrtDefinition& d,unsigned lod,const SourcePrtSh&sh){
    if(d.order!=3 || !d.clusters || !d.pca_vectors || d.lods.empty() || lod>5)
        throw FormatError("unsupported source PRT coefficient dimensions");
    const auto& l=d.lods[std::min<std::size_t>(d.lods.size()-1,5-lod)];
    const auto basis_count=std::size_t(d.clusters)*(d.pca_vectors+1)*27;
    if(l.cluster_basis_offset>d.cluster_basis.size() || basis_count>d.cluster_basis.size()-l.cluster_basis_offset)
        throw FormatError("PRT coefficient basis exceeds authored bank");
    const auto stride=unsigned(d.pca_vectors)*3+4;
    if(stride%4)throw FormatError("unaligned source PRT constant bank");
    std::vector<ShaderVector> result(std::size_t(d.clusters)*stride/4);
    const auto store=[&](std::size_t at,float value){result[at/4][at%4]=value;};
    for(unsigned cluster=0;cluster<d.clusters;++cluster){
        const auto base=std::size_t(l.cluster_basis_offset)+std::size_t(cluster)*(d.pca_vectors+1)*27;
        for(unsigned channel=0;channel<3;++channel)
            store(std::size_t(cluster)*stride+channel,projection_dot(std::span(d.cluster_basis).subspan(base+channel*9,9),sh.coefficients[channel]));
        for(unsigned i=0;i<d.pca_vectors;++i)for(unsigned channel=0;channel<3;++channel)
            store(std::size_t(cluster)*stride+4+channel*d.pca_vectors+i,
                projection_dot(std::span(d.cluster_basis).subspan(base+27+std::size_t(i)*27+channel*9,9),sh.coefficients[channel]));
    }return result;
}
std::shared_ptr<const VertexProgramBank> source_prt_bank(const SourcePrtDefinition&d,unsigned lod,std::span<const SourcePrtSh> nodes){
    if(nodes.empty() || nodes.size()>2)throw FormatError("unsupported source PRT node palette count");
    if(!nodes.back().incident_valid)throw FormatError("PRT bank requires retained source incident for an empty SH bank");
    return source_prt_bank(d,lod,nodes,nodes.back().incident);
}
std::shared_ptr<const VertexProgramBank> source_prt_bank(const SourcePrtDefinition&d,unsigned lod,
    std::span<const SourcePrtSh> nodes,Vec3 incident){
    if(nodes.empty() || nodes.size()>2)throw FormatError("unsupported source PRT node palette count");
    auto result=std::make_shared<VertexProgramBank>();result->first_constant=68;result->prt_inputs=true;
    //c68.w is adjacent cache metadata and is not consumed by any admitted
    //PRT program.23CA0 likewise leaves each ambient padding word unwritten.
    result->constants.push_back({incident.x,incident.y,incident.z,0});
    result->constants.push_back(source_prt_scale_constants());
    for(const auto&node:nodes){auto values=source_prt_project(d,lod,node);
        result->constants.insert(result->constants.end(),values.begin(),values.end());}
    return result;
}
std::vector<SourcePrtSectionBank> source_prt_section_banks(const Cache&cache,const RenderModel&model,
    const SourcePrtDefinition&definition,unsigned lod,std::span<const unsigned> sections,
    std::span<const Transform> world_nodes,const SourcePrtLighting&lighting){
    std::vector<std::vector<SourcePrtSh>> selected;Vec3 incident{};bool incident_valid=false;
    for(const auto section:sections){
        const auto nodes=cache.source_prt_section_nodes(definition.render_model,section);
        std::vector<SourcePrtSh> bank;
        for(unsigned i=0;i<std::min<std::size_t>(nodes.size(),2);++i){
            const auto node=nodes[i];if(node>=world_nodes.size())throw FormatError("source PRT world node bank missing");
            auto sh=source_prt_node_lighting(model,node,world_nodes[node],lighting);
            if(sh.incident_valid){incident=sh.incident;incident_valid=true;}
            bank.push_back(std::move(sh));
        }selected.push_back(std::move(bank));
    }
    if(!incident_valid)throw FormatError("source PRT object incident requires retained caller state");
    std::vector<SourcePrtSectionBank> result;
    for(unsigned i=0;i<sections.size();++i)if(!selected[i].empty())
        result.push_back({sections[i],source_prt_bank(definition,lod,selected[i],incident)});
    return result;
}
std::optional<SourcePrtDefinition> Cache::load_source_prt_definition(std::uint32_t datum)const {
    const Bytes b(cache_bytes());const auto t=query_tag({.handle=TagHandle{datum}, .group=tag_groups::render_model, .require_address=true});
    if(!t)throw FormatError("source PRT render model missing");
    const auto root=metadata_offset(t->address,0x7c);const auto bank=TagAddressSpace(b, metadata_magic_).block(root+0x74, 0x58, 1000000);
    if(!bank.count)return {};
    if(bank.count!=1)throw FormatError("multiple source PRT banks require original selection");
    const auto p=bank.offset;SourcePrtDefinition result;result.render_model=datum;
    result.order=b.u16(p);result.clusters=b.u16(p+2);result.pca_vectors=b.u16(p+4);
    const auto basis=TagAddressSpace(b, metadata_magic_).block(p+0x1c, 4, 1000000);
    for(unsigned i=0;i<basis.count;++i)result.cluster_basis.push_back(b.f32(basis.offset+i*4));
    const auto lods=TagAddressSpace(b, metadata_magic_).block(p+0x14, 12, 1000000);
    for(unsigned i=0;i<lods.count;++i){const auto at=lods.offset+i*12;SourcePrtLod l;l.cluster_basis_offset=b.u32(at);
        const auto sections=TagAddressSpace(b, metadata_magic_).block(at+4, 8, 1000000);
        for(unsigned j=0;j<sections.count;++j)l.sections.push_back({b.u32(sections.offset+j*8),b.u32(sections.offset+j*8+4)});
        result.lods.push_back(std::move(l));}
    const auto raw=raw_data(b.u32(p+0x34),b.u32(p+0x38));const Bytes data(raw);
    data.require(8,32);std::copy(data.slice(8,32).begin(),data.slice(8,32).end(),result.stream_descriptor.begin());
    const auto resources=TagAddressSpace(b, metadata_magic_).block(p+0x44, 16, 1000000);bool found=false;
    const auto base=std::size_t(b.u32(p+0x3c))+8;
    for(unsigned i=0;i<resources.count;++i){const auto at=resources.offset+i*16;
        if(b.slice(at,1)[0]!=std::byte{2} || b.u16(at+4)!=0 || b.u16(at+6)!=0xffff)continue;
        if(found)throw FormatError("multiple source PRT streams");found=true;
        const auto source=base+std::size_t(b.u32(at+12));const auto size=b.u32(at+8);const auto stream=data.slice(source,size);result.stream.assign(stream.begin(),stream.end());}
    if(!found)throw FormatError("source PRT stream missing");
    return result;
}
SourcePrtLighting Cache::load_source_prt_lighting(std::size_t index)const {
    if(index>=bsps_.size())throw FormatError("PRT lighting BSP outside scenario");
    const auto& bsp=bsps_[index];const Bytes b(Bytes(cache_bytes()).slice(bsp.file_offset,bsp.size));
    SourcePrtLighting result;const auto light=b.u32(8);
    if(light==0 || light==0xffffffffu)return result;
    if(light<bsp.memory_address)throw FormatError("PRT lighting header precedes BSP");
    const auto groups=TagAddressSpace(b, bsp.memory_address).block(light-bsp.memory_address+0x80, 104, 1000000);
    if(!groups.count)return result;
    const auto group=groups.offset;
    result.structure_matches=b.u32(group+4)==b.u32(16+8) && b.u32(group+0x1c)!=0xffffffffu;
    const auto records=TagAddressSpace(b, bsp.memory_address).block(group+0x38, 220, 1000000);
    for(unsigned i=0;i<records.count;++i){const auto p=records.offset+i*220;SourcePrtShRecord r;
        r.position={b.f32(p),b.f32(p+4),b.f32(p+8)};
        for(unsigned channel=0;channel<3;++channel)for(unsigned j=0;j<9;++j)r.lighting.coefficients[channel][j]=b.f32(p+12+channel*36+j*4);
        r.lighting.incident={b.f32(p+0xa4),b.f32(p+0xa8),b.f32(p+0xac)};r.lighting.incident_valid=true;result.records.push_back(r);
    }return result;
}
}
