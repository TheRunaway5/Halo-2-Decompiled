#include "halo2/source_model_palette.hpp"
#include "halo2/source_model_pose.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

namespace {
using namespace halo2;
using Words=std::vector<std::uint32_t>;
void require(bool value,const char *message){if(!value)throw FormatError(message);}
Transform transform(std::istream &row) {
    std::array<std::uint32_t,13> words;
    for(auto &word:words)require(bool(row>>word),"truncated source root transform");
    return std::bit_cast<Transform>(words);
}
BonePose local(std::istream &row) {
    std::array<std::uint32_t,8> words;
    for(auto &word:words)require(bool(row>>word),"truncated original local pose");
    return {{std::bit_cast<float>(words[4]),std::bit_cast<float>(words[5]),std::bit_cast<float>(words[6])},
        {std::bit_cast<float>(words[0]),std::bit_cast<float>(words[1]),std::bit_cast<float>(words[2]),std::bit_cast<float>(words[3])},
        std::bit_cast<float>(words[7])};
}
Words node_words(std::span<const Transform> bank) {
    Words words;for(const auto &node:bank){const auto row=std::bit_cast<std::array<std::uint32_t,13>>(node);words.insert(words.end(),row.begin(),row.end());}return words;
}
void same(const Words &actual,const Words &expected,const std::string &identity) {
    require(actual.size()==expected.size(),"source root output size differs");
    for(unsigned i=0;i<actual.size();++i)if(actual[i]!=expected[i])
        throw FormatError(identity+" differs at word "+std::to_string(i)+" actual="+std::to_string(actual[i])+" expected="+std::to_string(expected[i]));
}
Vec3 original_palette_vertex(const Vertex &vertex,const SkinBinding &binding,
                             std::span<const std::array<float,12>> palette) {
    int last=3;while(last>=0 && binding.weights[unsigned(last)]==0)--last;
    if(last<0)return vertex.position;
    std::array<float,12> matrix{};
    for(int influence=last;influence>=0;--influence)
        for(unsigned component=0;component<12;++component) {
            const volatile float product=palette[binding.bones[unsigned(influence)]][component]*binding.weights[unsigned(influence)];
            matrix[component]=influence==last?product:matrix[component]+product;
        }
    const auto axis=[&](unsigned axis){const auto r=axis*4;
        const volatile float x=matrix[r]*vertex.position.x,y=matrix[r+1]*vertex.position.y,z=matrix[r+2]*vertex.position.z;
        return ((x+y)+z)+matrix[r+3];};
    return {axis(0),axis(1),axis(2)};
}
Words graph_case(const Words &input) {
    unsigned at=0;const auto next=[&]{return input.at(at++);};
    const auto count=next(),camera_control=next();require(count>0 && count<=64,"source FP graph count outside ordinary bank");
    const Vec3 offset{std::bit_cast<float>(next()),std::bit_cast<float>(next()),std::bit_cast<float>(next())};
    std::array<std::uint32_t,13> camera_words;for(auto &v:camera_words)v=next();
    const auto camera=std::bit_cast<Transform>(camera_words);
    RenderModel model;AnimationClip clip;clip.frame_count=1;clip.channels.resize(count);clip.frames.resize(count);
    std::vector<BonePose> locals;
    for(unsigned i=0;i<count;++i) {
        ModelBone bone;bone.name=i==camera_control?0x0e0000e0u:0x10000000u+i;bone.parent=std::bit_cast<std::int32_t>(next());
        for(auto &v:bone.bind.rotation)v=std::bit_cast<float>(next());
        bone.bind.translation={std::bit_cast<float>(next()),std::bit_cast<float>(next()),std::bit_cast<float>(next())};bone.bind.scale=std::bit_cast<float>(next());
        bone.inverse={1,0,0,0,1,0,0,0,1,0,0,0};model.bones.push_back(bone);locals.push_back(bone.bind);
        clip.nodes.push_back(bone.name);clip.parents.push_back(bone.parent);clip.components.push_back(0);
        for(const auto position:std::array<Vec3,4>{{{0,0,0},{1,0,0},{0,1,0},{0,0,1}}}) {
            Vertex vertex;vertex.position=position;model.mesh.vertices.push_back(vertex);
            SkinBinding skin;skin.bones[0]=std::uint16_t(i);model.skin.push_back(skin);
        }
    }
    require(at==input.size(),"trailing source FP graph input");
    Transform root;root.position=offset;
    const auto pre=pose_model_locals_nodes_world(model,root,locals);
    auto output=node_words(pre);
    same(node_words(pose_model_nodes_world_frame(model,root,&clip,0,&model)),output,"explicit graph root API");
    Transform adjustment;
    if(camera_control!=0xffffffffu) {
        require(camera_control<count,"camera-control node outside explicit graph");
        adjustment=pre[camera_control];adjustment.position=adjustment.position-offset;
        adjustment=inverse_transform(adjustment);
    }
    const auto final_root=compose_transform(camera,adjustment);
    std::vector<Transform> post;for(const auto &node:pre)post.push_back(compose_transform(final_root,node));
    const auto post_words=node_words(post);output.insert(output.end(),post_words.begin(),post_words.end());
    std::vector<Transform> inverse(count);const auto palette=source_model_skin_palette(post,inverse);
    for(const auto &matrix:palette){const auto words=std::bit_cast<std::array<std::uint32_t,12>>(matrix);output.insert(output.end(),words.begin(),words.end());}
    const auto pose=pose_first_person_models_world_frame(model,model,clip,0,offset,camera,FirstPersonIkDefinition{},0);
    for(const auto *vertices:{&pose.weapon,&pose.arms})for(const auto &vertex:*vertices) {
        const auto words=std::bit_cast<std::array<std::uint32_t,3>>(vertex.position);output.insert(output.end(),words.begin(),words.end());
    }
    return output;
}
void graph_cases(const std::filesystem::path &path) {
    std::ifstream input(path);require(bool(input),"ordinary FP root fixture missing");std::string line;unsigned cases=0,nodes=0,words=0;
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;const auto separator=line.find('|');require(separator!=std::string::npos,"ordinary FP graph output missing");
        std::istringstream in(line.substr(0,separator)),out(line.substr(separator+1));Words row,expected;unsigned word;
        while(in>>std::hex>>word)row.push_back(word);while(out>>std::hex>>word)expected.push_back(word);
        same(graph_case(row),expected,"ordinary FP root case "+std::to_string(cases));++cases;nodes+=row[0];words+=unsigned(expected.size());
    }
    require(cases>=200 && nodes>=1000,"ordinary FP root fixture coverage missing");
    std::cout<<cases<<" untouched ordinary FP root/camera cases, "<<nodes<<" nodes, "<<words<<" words exact\n";
}
void asset_cases(const std::filesystem::path &path) {
    std::ifstream input(path);require(bool(input),"source root assets missing");
    const auto directory=std::filesystem::path(HALO2_SOURCE_DIR);
    std::map<std::uint32_t,RenderModel> models;std::unique_ptr<Cache> cache;std::string current,line;
    unsigned cases=0,nodes=0,palette_words=0,vertices=0,post_bank_owners=0,post_bank_words=0;
    std::map<unsigned,unsigned> types;std::set<std::string> maps;
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        const auto separator=line.find('|');require(separator!=std::string::npos,"source root WORLD output missing");
        std::istringstream row(line.substr(0,separator)),golden(line.substr(separator+1));
        std::string label,name;unsigned type,placement,datum,count;row>>label>>name>>std::hex>>type>>placement>>datum>>count;
        require(bool(row) && label=="C","invalid original cold root fixture");
        if(name!=current) {
            auto file=directory/"assets/disc/maps"/(name+".map");if(!std::filesystem::exists(file))file=directory/"assets/map-pack/maps"/(name+".map");
            cache=std::make_unique<Cache>(file);models.clear();current=name;
        }
        if(!models.contains(datum))models.emplace(datum,cache->load_model(datum,false));
        auto &model=models.at(datum);require(count==model.bones.size(),"source root model node count differs");
        const auto root=transform(row);std::vector<BonePose> locals;
        for(unsigned i=0;i<count;++i)locals.push_back(local(row));
        std::string extra;require(!(row>>extra),"source root input has trailing words");
        golden>>std::hex;std::vector<Transform> expected_nodes;
        for(unsigned i=0;i<count;++i)expected_nodes.push_back(transform(golden));
        std::vector<std::array<float,12>> expected_palette(count);
        for(auto &matrix:expected_palette)for(auto &value:matrix){unsigned word;require(bool(golden>>word),"source1376C0 palette truncated");value=std::bit_cast<float>(word);}
        require(!(golden>>extra),"source root output has trailing words");
        const auto identity=name+":"+std::to_string(type)+":"+std::to_string(placement);
        const auto expected=node_words(expected_nodes);
        const auto actual=pose_model_locals_nodes_world(model,root,locals);
        same(node_words(actual),expected,identity+" local WORLD");
        auto old_bank=pose_model_locals_nodes(model,locals);for(auto &node:old_bank)node=compose_transform(root,node);
        const auto old_words=node_words(old_bank);unsigned differences=0;
        for(unsigned i=0;i<old_words.size();++i)differences+=old_words[i]!=expected[i];
        post_bank_owners+=differences!=0;post_bank_words+=differences;
        for(unsigned i=0;i<count;++i)model.bones[i].bind=locals[i];
        same(node_words(pose_model_nodes_world(model,root)),expected,identity+" bind WORLD");
        same(node_words(pose_model_nodes_world_frame(model,root,nullptr,0)),expected,identity+" authored-frame WORLD");
        std::vector<Transform> inverse;
        for(const auto &bone:model.bones){Transform node;node.scale=bone.inverse_scale;for(unsigned c=0;c<3;++c)node.basis[c]={bone.inverse[c*3],bone.inverse[c*3+1],bone.inverse[c*3+2]};node.position={bone.inverse[9],bone.inverse[10],bone.inverse[11]};inverse.push_back(node);}
        const auto palette=source_model_skin_palette(actual,inverse);
        for(unsigned i=0;i<count;++i)require(std::bit_cast<std::array<std::uint32_t,12>>(palette[i])==std::bit_cast<std::array<std::uint32_t,12>>(expected_palette[i]),"original rooted1376C0 palette differs");
        const auto posed=pose_model_locals_world(model,root,locals),bind=pose_model_world(model,root),frame=pose_model_world_frame(model,root,nullptr,0);
        require(posed.size()==model.mesh.vertices.size() && bind.size()==posed.size() && frame.size()==posed.size(),"rooted geometry count differs");
        for(unsigned i=0;i<posed.size();++i) {
            const auto expected_position=original_palette_vertex(model.mesh.vertices[i],model.skin[i],expected_palette);
            const auto bits=std::bit_cast<std::array<std::uint32_t,3>>(expected_position);
            require(std::bit_cast<std::array<std::uint32_t,3>>(posed[i].position)==bits && std::bit_cast<std::array<std::uint32_t,3>>(bind[i].position)==bits && std::bit_cast<std::array<std::uint32_t,3>>(frame[i].position)==bits,"rooted geometry did not consume original WORLD/inverse palette");
            require(posed[i].material==model.mesh.vertices[i].material && posed[i].source_section==model.mesh.vertices[i].source_section && posed[i].source_part==model.mesh.vertices[i].source_part && std::bit_cast<std::array<std::uint32_t,2>>(posed[i].uv)==std::bit_cast<std::array<std::uint32_t,2>>(model.mesh.vertices[i].uv),"rooted pose changed authored part/material/UV identity");
        }
        ++cases;++types[type];maps.insert(name);nodes+=count;palette_words+=count*12;vertices+=unsigned(posed.size());
    }
    require(cases==741 && types[6]==278 && types[7]==48 && types[11]==415 && maps.size()==21,"original cold root owner coverage changed");
    require(post_bank_owners>0 && post_bank_words>0,"root-order negative control lost its source-sensitive descendants");
    std::cout<<cases<<" actual cold constructor roots, "<<nodes<<" original WORLD nodes and "<<palette_words<<" original1376C0 words exact; "<<vertices<<" native vertices consume recorded palettes across21 maps\n";
    std::cout<<"Former post-hierarchy root order differs in "<<post_bank_owners<<" owners / "<<post_bank_words<<" WORLD words\n";
}
}
int main(int argc,char **argv){try{
    if(argc==2 && std::string_view(argv[1])=="--probe") {
        std::string line;while(std::getline(std::cin,line)){std::istringstream row(line);Words words;unsigned word;while(row>>std::hex>>word)words.push_back(word);for(const auto v:graph_case(words))std::cout<<std::hex<<v<<' ';std::cout<<'\n';}return 0;
    }
    if(argc==3 && std::string_view(argv[1])=="--graph"){graph_cases(argv[2]);return 0;}
    require(argc==2,"source root test requires frozen assets");asset_cases(argv[1]);return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
