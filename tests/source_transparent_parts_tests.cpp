#include "halo2/source_transparent_parts.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace {
using namespace halo2;using Words=std::vector<std::uint32_t>;
void require(bool b,const char *s){if(!b)throw FormatError(s);}
Words evaluate(const Words &row) {
    std::size_t at=0;const auto next=[&]{return row.at(at++);};const auto real=[&]{return std::bit_cast<float>(next());};
    const auto vector=[&]{return Vec3{real(),real(),real()};};const auto mode=next();
    SourceTransparentPart p;p.shader=next();p.lod_scale=real();p.center=vector();
    const bool plane=next();std::array<float,4> bank;for(auto &x:bank)x=real();if(plane)p.plane=bank;
    SourceTransparentPartGeometry result;
    if(mode==0)result=source_transparent_cluster_geometry(p);
    else if(mode==1) {
        SourceTransparentInstance instance;instance.transform.scale=real();
        for(auto &column:instance.transform.basis)column=vector();instance.transform.position=vector();instance.center=vector();
        result=source_transparent_instance_geometry(p,instance);
    }else if(mode==2) {
        const auto rigid=std::int16_t(next());const auto flags=std::uint16_t(next());const auto mapped=next(),bias=next(),count=next();
        std::vector<Vec3> nodes;for(unsigned i=0;i<count;++i)nodes.push_back(vector());
        result=source_transparent_model_geometry(p,rigid,flags,nodes,
            mapped==0xffffffffu?std::nullopt:std::optional<std::int16_t>(std::int16_t(mapped)),std::uint8_t(bias));
    }else throw FormatError("unknown source transparent part mode");
    require(at==row.size(),"source transparent part trailing input");
    Words out{result.shader,std::bit_cast<std::uint32_t>(result.center.x),std::bit_cast<std::uint32_t>(result.center.y),
        std::bit_cast<std::uint32_t>(result.center.z),std::bit_cast<std::uint32_t>(result.depth_bias),std::uint32_t(result.plane.has_value())};
    for(auto x:result.plane.value_or(std::array<float,4>{}))out.push_back(std::bit_cast<std::uint32_t>(x));
    out.push_back(std::bit_cast<std::uint32_t>(result.lod_scale));return out;
}
Words part_words(const SourceTransparentPart &p) {
    Words out(p.authored_words.begin(),p.authored_words.end());out.push_back(p.shader);
    out.push_back(p.shader_priority_flags);out.push_back(p.shader_command_flags);
    out.push_back(std::uint32_t(p.template_lod_layers.size()));out.insert(out.end(),p.template_lod_layers.begin(),p.template_lod_layers.end());
    out.push_back(p.plane.has_value());for(auto v:p.plane.value_or(std::array<float,4>{}))out.push_back(std::bit_cast<std::uint32_t>(v));
    require(p.type==(p.authored_words[0]&65535) && p.flags==(p.authored_words[0]>>16) &&
        std::uint16_t(p.material)==(p.authored_words[1]&65535) && p.index_first==(p.authored_words[1]>>16) &&
        p.index_count==(p.authored_words[2]&65535) && p.subpart_first==(p.authored_words[2]>>16) &&
        p.subpart_count==(p.authored_words[3]&65535),"decoded part identity differs from authored words");return out;
}
void assets(const std::string &path) {
    std::ifstream input(path);require(bool(input),"source transparent assets fixture missing");
    std::string line,last;std::unique_ptr<Cache> cache;SourceTransparentBspParts bsp;
    unsigned maps=0,parts=0,instances=0,planes=0,vertices=0,owners=0;std::map<std::pair<std::uint32_t,unsigned>,SourceTransparentPartSection> sections;
    while(std::getline(input,line)){if(line.empty())continue;std::istringstream stream(line);std::string name;unsigned kind,a,b;stream>>name>>kind>>a>>b;
        Words expected;std::uint32_t word;while(stream>>word)expected.push_back(word);
        if(name!=last){auto file=std::filesystem::path("assets/disc/maps")/(name+".map");if(!std::filesystem::exists(file))file=std::filesystem::path("assets/map-pack/maps")/(name+".map");
            cache=std::make_unique<Cache>(file);bsp=cache->load_source_transparent_bsp_parts(0);last=name;sections.clear();++maps;
            const auto mesh=cache->load_bsp(0,false);
            for(const auto &v:mesh.vertices){
                if(v.bsp_cluster>=0){require(v.source_section==unsigned(v.bsp_cluster),"BSP cluster source section identity changed");
                    (void)bsp.clusters.at(v.source_section).parts.at(v.source_part);}
                else{require(v.bsp_cluster<=-2,"BSP vertex lacks authored cluster/instance identity");
                    const auto &instance=bsp.instances.at(std::size_t(-2-v.bsp_cluster));require(instance.definition==v.source_section,"BSP instance definition identity changed");
                    (void)bsp.instance_definitions.at(v.source_section).parts.at(v.source_part);}++vertices;}}
        Words actual;
        if(kind<2) {const auto &section=(kind==0?bsp.clusters:bsp.instance_definitions).at(a);actual=part_words(section.parts.at(b));++parts;}
        else if(kind==2) {
            const auto datum=a,section=b;const auto key=std::pair{datum,section};
            if(!sections.contains(key)){const std::array<unsigned,2> duplicate{section,section};const auto model=cache->load_source_transparent_model_parts(datum,duplicate);
                require(model.sections.size()==2 && model.sections[0].section==section && model.sections[1].section==section,
                    "source transparent collector order/duplicates lost");
                require(model.sections[0].parts.size()==model.sections[1].parts.size(),"source transparent duplicate section changed parts");
                require(cache->load_source_transparent_model_parts(datum,{}).sections.empty(),"empty source collector invented geometry");sections.emplace(key,model.sections[0]);}
            const unsigned ordinal=expected.at(0);expected.erase(expected.begin());actual=part_words(sections.at(key).parts.at(ordinal));++parts;
        }else if(kind==3) {
            const auto &i=bsp.instances.at(a);actual={i.definition,std::bit_cast<std::uint32_t>(i.transform.scale)};
            for(auto v:i.transform.basis)for(auto x:{v.x,v.y,v.z})actual.push_back(std::bit_cast<std::uint32_t>(x));
            for(auto v:{i.transform.position,i.center})for(auto x:{v.x,v.y,v.z})actual.push_back(std::bit_cast<std::uint32_t>(x));actual.push_back(std::bit_cast<std::uint32_t>(i.radius));++instances;
        }else if(kind==4){const auto &p=bsp.planes.at(a);actual={p.geometry,p.part_and_instance};for(auto x:p.plane)actual.push_back(std::bit_cast<std::uint32_t>(x));++planes;}
        else if(kind==5){const auto orders=cache->load_source_transparent_model_sort_orders(a,std::bit_cast<std::int32_t>(b));actual.push_back(unsigned(orders.size()));
            for(auto order:orders)actual.push_back(std::uint16_t(order));}
        else if(kind==6){for(auto x:bsp.clusters.at(a).cluster_bounds)actual.push_back(std::bit_cast<std::uint32_t>(x));}
        else if(kind==7){const auto definition=cache->load_initial_model_definition(a);SourceInitialModelSelection selection;
            selection.variant_index=source_initial_model_variant(definition,b?b:definition.default_variant);selection.permutations.assign(expected.begin(),expected.end());
            selection.states.resize(selection.permutations.size());const auto collector=source_transparent_model_collector(definition,selection,5);
            std::vector<unsigned> selected;for(const auto &c:collector)selected.push_back(c.section);const auto bank=cache->load_source_transparent_model_parts(definition.render_model,selected);
            const auto model=cache->load_model(a,false,b,selection,5);require(model.vertex_origins.size()==model.mesh.vertices.size(),"model source vertex links lost");
            for(unsigned i=0;i<model.mesh.vertices.size();++i){const auto &v=model.mesh.vertices[i];const auto &origin=model.vertex_origins[i];
                require(v.source_section==origin.section && v.source_part==origin.part,"model expanded part/section identity changed");
                const auto section=std::find_if(bank.sections.begin(),bank.sections.end(),[&](const auto &s){return s.section==origin.section;});
                require(section!=bank.sections.end() && origin.part<section->parts.size(),"model vertex source identity outside actual selected bank");++vertices;}
            ++owners;actual=expected;}
        else throw FormatError("unknown source transparent asset fixture");
        if(actual!=expected){for(unsigned i=0;i<std::min(actual.size(),expected.size());++i)if(actual[i]!=expected[i])
            std::cerr<<"word"<<i<<" actual"<<actual[i]<<" expected"<<expected[i]<<'\n';
            throw FormatError("source transparent authored metadata differs at "+name+" kind"+std::to_string(kind)+" a"+std::to_string(a)+" b"+std::to_string(b));}
    }
    require(maps==21 && parts==6584 && planes==837,"source transparent census incomplete");
    require(owners==741,"source transparent automatic model coverage incomplete");
    std::cout<<"21 maps, "<<parts<<" authored parts, "<<instances<<" instances, "<<planes<<" sort planes, "<<owners<<" models, "<<vertices<<" emitted identities exact\n";
}
void boundaries() {
    SourceTransparentPart p;p.center={99,88,77};p.plane=std::array<float,4>{1,2,3,4};
    const std::array<Vec3,2> nodes{Vec3{1,2,3},Vec3{4,5,6}};bool rejected=false;
    try{(void)source_transparent_model_geometry(p,1,4,nodes,std::nullopt,0);}catch(const FormatError &){rejected=true;}
    require(rejected,"missing actual context node remap silently approximated");
    const auto output=source_transparent_model_geometry(p,-1,4,nodes,std::nullopt,0);
    require(!output.plane && output.center.x==1,"model NONE rigid node did not use actual node0/no plane");
    rejected=false;try{(void)source_transparent_model_geometry(p,1,0,nodes,std::nullopt,16);}catch(const FormatError &){rejected=true;}
    require(rejected,"invalid bias nibble silently admitted");
    SourceInitialModelDefinition definition;
    definition.render_sections={{{-1,-1,-1,-1,-1,-1}},{{7,7,7,7,7,7}},{{7,7,7,7,7,7}}};
    SourceInitialModelSelection selection;selection.permutations={0,0,0};
    const auto collector=source_transparent_model_collector(definition,selection,5);
    require(collector.size()==2 && collector[0].region==1 && collector[1].region==2 &&
        collector[0].section==7 && collector[1].section==7,
        "transparent collector compressed region ordinals or removed duplicate sections");
    const std::array<std::int16_t,3> orders{1,5,31};
    require(source_transparent_model_bias_nibble(orders,1,1)==5 &&
        source_transparent_model_bias_nibble(orders,2,1)==15 &&
        source_transparent_model_bias_nibble({},999,2)==0,
        "transparent bias bank did not preserve region nibble/model-count reset");
    const auto negative=source_transparent_model_geometry(p,-1,0,nodes,std::nullopt,1);
    require(std::bit_cast<std::uint32_t>(negative.depth_bias)==
        std::bit_cast<std::uint32_t>(-4.f*std::bit_cast<float>(0x3a83126fu)),
        "transparent model bias changed source offset arithmetic");
}
}
int main(int argc,char **argv)try {
    if(argc==2 && std::string(argv[1])=="--probe") {std::string line;while(std::getline(std::cin,line)) {std::istringstream in(line);Words row;std::uint32_t word;while(in>>word)row.push_back(word);
        const auto out=evaluate(row);for(unsigned i=0;i<out.size();++i)std::cout<<(i?" ":"")<<out[i];std::cout<<'\n';}return 0;}
    if(argc==3 && std::string(argv[1])=="--assets"){assets(argv[2]);return 0;}
    require(argc==2,"source transparent test requires fixture");std::ifstream input(argv[1]);require(bool(input),"source transparent fixture missing");
    std::string line;unsigned cases=0;while(std::getline(input,line)){if(line.empty())continue;std::istringstream in(line);Words row;std::uint32_t word;while(in>>word)row.push_back(word);
        require(row.size()>11,"source transparent fixture truncated");Words expected(row.end()-11,row.end());row.resize(row.size()-11);
        require(evaluate(row)==expected,"source transparent45CE0 instruction mismatch");++cases;}
    require(cases>100,"source transparent fixture incomplete");boundaries();std::cout<<cases<<" source transparent geometry fixtures exact\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
