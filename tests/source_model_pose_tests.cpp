#include "halo2/source_model_pose.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>

namespace {
using namespace halo2;
using Words=std::vector<std::uint32_t>;
void require(bool value,const char *message){if(!value)throw FormatError(message);}
Words evaluate(const Words &row) {
    unsigned at=0;const auto word=[&]{return row.at(at++);};
    const auto has_root=word(),count=word(),explicit_seed=word();
    require(has_root<=1 && count<=256 && explicit_seed<=1,"invalid source pose fixture header");
    Transform root;
    if(has_root) {
        std::array<std::uint32_t,13> data;for(auto &v:data)v=word();root=std::bit_cast<Transform>(data);
    }
    std::vector<BonePose> poses;std::vector<int> parents;std::vector<Transform> locals;Words output;
    for(unsigned i=0;i<count;++i) {
        parents.push_back(std::bit_cast<std::int32_t>(word()));BonePose pose;
        for(auto &q:pose.rotation)q=std::bit_cast<float>(word());
        pose.translation={std::bit_cast<float>(word()),std::bit_cast<float>(word()),std::bit_cast<float>(word())};
        pose.scale=std::bit_cast<float>(word());poses.push_back(pose);
        const auto transform=explicit_seed?source_model_local_transform(pose,std::bit_cast<float>(word())):
            source_model_local_transform(pose);locals.push_back(transform);
        output.push_back(std::bit_cast<std::uint32_t>(source_model_local_squared_norm(pose)));
        const auto data=std::bit_cast<std::array<std::uint32_t,13>>(transform);output.insert(output.end(),data.begin(),data.end());
    }
    require(at==row.size(),"trailing source pose fixture input");
    const auto world=source_model_world_nodes(locals,parents,has_root?&root:nullptr);
    if(!explicit_seed) {
        const auto direct=source_model_world_nodes(poses,parents,has_root?&root:nullptr);
        require(direct.size()==world.size(),"source local hierarchy wrapper lost a node");
        for(unsigned i=0;i<count;++i)
            require(std::bit_cast<std::array<std::uint32_t,13>>(direct[i])==
                std::bit_cast<std::array<std::uint32_t,13>>(world[i]),"source local hierarchy wrappers differ");
    }
    for(const auto &transform:world) {
        const auto data=std::bit_cast<std::array<std::uint32_t,13>>(transform);output.insert(output.end(),data.begin(),data.end());
    }
    return output;
}
void controls() {
    BonePose pose;pose.rotation={0,0,0,0};pose.translation={-0.f,1,-2};pose.scale=-.75f;
    auto local=source_model_local_transform(pose);
    require(local.scale==-.75f && std::bit_cast<std::uint32_t>(local.position.x)==0x80000000u &&
        local.basis[0].x==1 && local.basis[1].y==1 && local.basis[2].z==1,
        "source near-zero quaternion guard lost authored scale/translation");
    std::array<Transform,2> transforms;std::array<int,2> parents{-1,1};bool rejected=false;
    try{(void)source_model_world_nodes(transforms,parents);}catch(const FormatError&){rejected=true;}
    require(rejected,"source node-order hierarchy silently reordered its bank");
    pose.rotation[0]=std::numeric_limits<float>::infinity();rejected=false;
    try{(void)source_model_local_transform(pose);}catch(const FormatError&){rejected=true;}
    require(rejected,"source local helper admitted nonfinite quaternion");
}
void assets(const std::filesystem::path &path) {
    std::ifstream input(path);require(bool(input),"source raw bind asset fixture missing");
    std::map<std::string,std::unique_ptr<Cache>> maps;std::string line;unsigned models=0,nodes=0,words=0,world_words=0;
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        const auto separator=line.find('|');require(separator!=std::string::npos,"source raw bind WORLD bank missing");
        std::istringstream row(line.substr(0,separator)),expected_world(line.substr(separator+1));
        expected_world>>std::hex;std::string name;unsigned datum,count;row>>name>>std::hex>>datum>>count;
        require(bool(row),"source raw bind identity missing");
        if(!maps.contains(name)) {
            auto file=root/"assets/disc/maps"/(name+".map");
            if(!std::filesystem::exists(file))file=root/"assets/map-pack/maps"/(name+".map");
            maps.emplace(name,std::make_unique<Cache>(file));
        }
        const auto model=maps.at(name)->load_model(datum,false);
        require(model.bones.size()==count,"native raw bind node count differs");
        for(const auto &bone:model.bones) {
            unsigned node_name,parent;row>>node_name>>parent;
            require(bool(row) && bone.name==node_name && std::uint32_t(bone.parent)==parent,
                "native raw bind identity/parent differs");
            std::array<std::uint32_t,8> expected;
            for(auto &v:expected)require(bool(row>>v),"source raw bind fixture truncated");
            const auto &pose=bone.bind;
            const std::array<float,8> actual{pose.rotation[0],pose.rotation[1],pose.rotation[2],pose.rotation[3],
                pose.translation.x,pose.translation.y,pose.translation.z,pose.scale};
            for(unsigned i=0;i<expected.size();++i)if(std::bit_cast<std::uint32_t>(actual[i])!=expected[i])
                throw FormatError("native loader changed raw bind word in "+name+" model "+std::to_string(datum)+" node "+std::to_string(nodes)+" component "+std::to_string(i));
            ++nodes;words+=8;
        }
        std::string extra;require(!(row>>extra),"source raw bind fixture trailing data");
        const auto world=pose_model_nodes(model);require(world.size()==count,"native model pose WORLD bank count differs");
        for(unsigned node=0;node<count;++node)
            for(const auto value:std::bit_cast<std::array<std::uint32_t,13>>(world[node])) {
                unsigned expected;require(bool(expected_world>>expected),"source WORLD fixture truncated");
                if(value!=expected)throw FormatError("native WORLD node differs from source bind hierarchy in "+name+
                    " model "+std::to_string(datum)+" node "+std::to_string(node)+" component "+std::to_string(world_words%13));
                ++world_words;
            }
        require(!(expected_world>>extra),"source WORLD fixture trailing data");++models;
    }
    require(maps.size()==21 && models==403 && nodes==4119 && words==32952 && world_words==53547,
        "source raw bind all-map coverage changed");
    std::cout<<models<<" raw bind models, "<<nodes<<" nodes, "<<words<<" original16D940 copied words + "<<
        world_words<<" original WORLD pose words exact across21 maps\n";
}
}
int main(int argc,char **argv) {
    try {
        if(argc==3 && std::string_view(argv[1])=="--assets"){assets(argv[2]);return 0;}
        if(argc==2 && std::string_view(argv[1])=="--probe") {
            std::string line;while(std::getline(std::cin,line)) {
                std::istringstream input(line);Words row;unsigned value;while(input>>std::hex>>value)row.push_back(value);
                const auto out=evaluate(row);for(const auto v:out)std::cout<<std::hex<<v<<' ';std::cout<<'\n';
            }return 0;
        }
        require(argc==2,"source pose test requires its frozen fixture");controls();std::ifstream input(argv[1]);
        require(bool(input),"source pose fixture missing");std::string line;unsigned cases=0,nodes=0,words=0;
        while(std::getline(input,line)) {
            if(line.empty() || line[0]=='#')continue;
            const auto separator=line.find('|');require(separator!=std::string::npos,"source pose fixture output missing");
            std::istringstream in(line.substr(0,separator)),expected(line.substr(separator+1));Words row,golden;unsigned value;
            while(in>>std::hex>>value)row.push_back(value);while(expected>>std::hex>>value)golden.push_back(value);
            const auto actual=evaluate(row);require(actual.size()==golden.size(),"source pose output count differs");
            for(unsigned i=0;i<actual.size();++i)if(actual[i]!=golden[i])
                throw FormatError("source pose fixture differs at case "+std::to_string(cases)+" word "+std::to_string(i));
            ++cases;nodes+=row.at(1);words+=unsigned(actual.size());
        }
        require(cases>900 && nodes>6000,"source pose frozen coverage missing");
        std::cout<<cases<<" original local/hierarchy cases, "<<nodes<<" nodes, "<<words<<" words exact\n";
        return 0;
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
