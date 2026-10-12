#include "halo2/content.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/model.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

namespace {
using namespace halo2;
void require(bool condition,const char *message) {if(!condition)throw FormatError(message);}
Transform read_transform(std::istream &in) {
    std::array<float,13> f{};for(auto &v:f)in>>v;
    require(bool(in),"truncated IK transform fixture");
    return std::bit_cast<Transform>(f);
}
void write_transform(const Transform &t) {
    for(float v:std::bit_cast<std::array<float,13>>(t))std::cout<<' '<<v;
}
void probe() {
    std::cout<<std::setprecision(10);unsigned kind;
    while(std::cin>>kind) {
        if(kind==0) {
            auto root=read_transform(std::cin),mid=read_transform(std::cin),end=read_transform(std::cin),target=read_transform(std::cin);
            source_two_bone_ik(root,mid,end,target);write_transform(root);write_transform(mid);write_transform(end);
        } else if(kind==1) {
            Vec3 p;std::array<float,4> q;std::cin>>p.x>>p.y>>p.z;for(auto &v:q)std::cin>>v;
            write_transform(source_marker_transform(p,q));
        } else if(kind==2) {
            int value,count,curve,flags,active,tick_rate,advance;
            std::cin>>value>>count>>curve>>flags>>active>>tick_rate>>advance;
            FirstPersonAimBlend blend{std::int8_t(value),std::int8_t(count),std::int8_t(curve),std::uint8_t(flags)};
            source_aim_blend_select(blend,bool(active),unsigned(tick_rate));
            bool changed=false;for(int i=0;i<advance;++i)changed|=source_aim_blend_advance(blend);
            std::cout<<int(blend.value)<<' '<<int(blend.count)<<' '<<int(blend.curve)<<' '<<int(blend.flags)<<' '<<source_aim_blend_weight(blend)<<' '<<int(changed);
        } else if(kind==3) {
            unsigned n,m;std::cin>>n>>m;require(n<=64 && m<=64,"IK fixture node limit");
            std::vector<int> parents(n);for(auto &p:parents)std::cin>>p;
            std::vector<Transform> arms(n),weapon(m);for(auto &t:arms)t=read_transform(std::cin);for(auto &t:weapon)t=read_transform(std::cin);
            FirstPersonIkDefinition definition;FirstPersonIkConstraint constraint;
            std::cin>>constraint.arms_node>>constraint.weapon_node;
            constraint.arms_local=read_transform(std::cin);constraint.weapon_local=read_transform(std::cin);
            float weight;std::cin>>weight;definition.constraints.push_back(constraint);
            apply_first_person_ik(definition,parents,arms,weapon,weight);for(const auto &t:arms)write_transform(t);
        } else if(kind==4) {
            FirstPersonAimEligibility p;unsigned a,b,dual;
            std::cin>>p.base_state>>p.secondary_action>>a>>b>>dual;
            p.base_animation_flags=std::uint16_t(a);p.secondary_animation_flags=std::uint16_t(b);p.dual_wield=bool(dual);
            std::cout<<int(source_weapon_aim_eligible(p));
        } else if(kind==5) {
            static Cache cache(std::filesystem::path(__FILE__).parent_path().parent_path()/"assets/disc/maps/cyclotron.map");
            std::uint32_t graph,mode,weapon_class,type,action;unsigned overlay;
            std::cin>>graph>>mode>>weapon_class>>type>>action>>overlay;
            try {
                const auto clip=cache.load_unit_weapon_action(graph,mode,weapon_class,type,action,bool(overlay));
                const auto separator=clip.name.find_last_of(':');const auto name=clip.name.substr(0,separator);
                const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){return t.group=="jmad" && t.name==name;});
                require(tag!=cache.tags().end(),"unit action resolved graph missing");
                std::cout<<tag->datum<<' '<<std::stoul(clip.name.substr(separator+1))<<' '<<clip.authored_flags<<' '<<clip.frame_count;
            } catch(const FormatError &) {std::cout<<"4294967295 4294967295 4294967295 4294967295";}
        } else throw FormatError("unknown IK fixture kind");
        require(bool(std::cin),"truncated IK fixture");std::cout<<'\n';
    }
}
void synthetic() {
    std::ifstream weight_classes(std::filesystem::path(__FILE__).parent_path()/"fixtures/retail_skin_weight_classes.tsv");
    require(bool(weight_classes),"original compiled skin class fixture missing");
    std::string weight_line;unsigned weight_cases=0;
    while(std::getline(weight_classes,weight_line)) {
        if(weight_line.empty() || weight_line[0]=='#')continue;
        std::istringstream in(weight_line);unsigned count;std::array<std::uint8_t,4> raw;std::array<std::uint32_t,4> expected;
        in>>count;for(auto &v:raw) {unsigned byte;in>>byte;require(byte<=255,"invalid source skin byte");v=std::uint8_t(byte);}
        for(auto &v:expected)in>>std::hex>>v;
        require(bool(in),"truncated original compiled skin fixture");
        const auto actual=source_model_skin_weights(raw,count);
        for(unsigned j=0;j<4;++j)require(std::bit_cast<std::uint32_t>(actual[j])==expected[j],"skin weights differ from original one/two/three/four influence instructions");
        ++weight_cases;
    }
    require(weight_cases==36,"original compiled skin class coverage changed");
    // A signed first-weight complement must reach matrix blending. This bounded
    // source-class4 input covers behavior absent from all supplied map meshes.
    RenderModel signed_model;signed_model.bones.resize(4);signed_model.skin.resize(1);signed_model.mesh.vertices.resize(1);
    for(unsigned i=0;i<4;++i) {
        auto &bone=signed_model.bones[i];bone.name=i+1;bone.bind.translation={float(i),0,0};bone.inverse={1,0,0,0,1,0,0,0,1,0,0,0};
        signed_model.skin[0].bones[i]=std::uint16_t(i);
    }
    signed_model.skin[0].weights=source_model_skin_weights(std::array<std::uint8_t,4>{1,250,200,100},4);
    signed_model.mesh.vertices[0].position={0,0,0};signed_model.mesh.vertices[0].normal={1,0,0};
    const auto signed_pose=pose_model(signed_model);
    require(std::abs(signed_pose[0].position.x-3.72549033165f)<1e-6f && std::abs(signed_pose[0].normal.x-1.f)<2e-7f,"signed original skin complement was discarded");
    FirstPersonAimBlend blend;
    require(source_aim_blend_weight(blend)==0,"new unit aiming blend is not zero");
    source_aim_blend_select(blend,true,30);
    require(blend.count==7 && blend.value==1 && blend.curve==1 && blend.flags==1,"original 30Hz aiming activation");
    for(unsigned i=0;i<6;++i)source_aim_blend_advance(blend);
    require(source_aim_blend_weight(blend)==1 && blend.flags==3,"original aiming counter completion");
    source_aim_blend_select(blend,false,30);
    require(blend.count==1 && blend.value==0 && blend.flags==0,"original aiming fade selection");
    require(source_aim_blend_advance(blend) && source_aim_blend_weight(blend)==0,"original aiming fade completion");
    require(source_weapon_aim_eligible({}),"idle unit aiming rejected");
    require(!source_weapon_aim_eligible({0xffffffffu,0xffffffffu,8}),"authored aiming denial ignored");
    require(!source_weapon_aim_eligible({0xffffffffu,0xffffffffu,0,0,true}),"dual weapon IK admitted");
    Transform root,middle,end,target;middle.position={.2f,0,0};end.position={.4f,0,0};target.position={.3f,.1f,0};
    require(source_two_bone_ik(root,middle,end,target),"valid source IK chain rejected");
    require(std::abs(end.position.x-.3f)<1e-7f && std::abs(end.position.y-.1f)<1e-7f,"IK hand did not reach target");
    Transform zero;auto before=zero;require(!source_two_bone_ik(zero,zero,zero,zero),"degenerate source IK chain admitted");
    require(std::bit_cast<std::array<float,13>>(zero)==std::bit_cast<std::array<float,13>>(before),"degenerate IK mutated nodes");
}
void retail(const std::filesystem::path &path) {
    Cache cache(path);const auto content=cache.load_content(0,true);unsigned constraints=0;
    for(const char *name:{"smg","battle_rifle"}) {
        const auto w=std::find_if(content.weapons.begin(),content.weapons.end(),[&](const auto &entry){return entry.second.name.ends_with(std::string("\\")+name);});
        require(w!=content.weapons.end(),"retail FP weapon missing");
        const auto bank=cache.load_first_person_ik(w->second.first_person_animation,w->second.first_person_model,content.first_person_hands,0x06000086u);
        if(std::string(name)=="battle_rifle") {
            require(bank.constraints.empty(),"retail BR empty constraint block changed");continue;
        }
        require(bank.constraints.size()==1,"retail SMG marker constraint missing");
        const auto &c=bank.constraints.front();require(c.arms_marker==0x09000100u && c.weapon_marker==0x09000100u && c.arms_node==5 && c.weapon_node==0,"retail left-hand attachment changed");
        const auto identifiers=cache.load_weapon_animation_identifiers(w->first);
        require(identifiers[0]==0x05000720u && identifiers[1]==0x03000dccu,"retail SMG class/type changed");
        const auto unit_idle=cache.load_named_animation(0xea4e08c9u,0x110016f3u);
        require(unit_idle.authored_flags==0,"captured unit idle authored flag changed");
        const auto resolved_idle=cache.load_unit_weapon_action(0xea4e08c9u,0x06000086u,identifiers[0],identifiers[1],0x0400000cu);
        require(resolved_idle.name.ends_with(":185"),"SMG unit idle fallback differs from original");
        const auto unit_fire=cache.load_unit_weapon_action(0xea4e08c9u,0x06000086u,identifiers[0],identifiers[1],0x06000006u,true);
        const auto unit_reload=cache.load_unit_weapon_action(0xea4e08c9u,0x06000086u,identifiers[0],identifiers[1],0x08000002u,true);
        require(unit_fire.name.ends_with(":172") && unit_reload.name.ends_with(":215") && !unit_fire.authored_flags && !unit_reload.authored_flags,"SMG authored unit overlay changed");
        const auto weapon=cache.load_model(w->second.first_person_model,false),arms=cache.load_model(content.first_person_hands,false);
        // These local inputs come from the original captured vertex buffers,
        // c170..172 and post-VS UV outputs, independently of Cache::load_model.
        std::ifstream packed_fixture(std::filesystem::path(__FILE__).parent_path()/"fixtures/retail_fp_packed.tsv");
        require(bool(packed_fixture),"original packed FP vertex fixture missing");
        std::map<std::array<unsigned,3>,std::array<std::uint32_t,14>> packed_inputs;
        std::string packed_line;
        while(std::getline(packed_fixture,packed_line)) {
            if(packed_line.empty() || packed_line[0]=='#')continue;
            std::istringstream in(packed_line);std::array<unsigned,3> key;std::array<std::uint32_t,14> values;
            for(auto &v:key)in>>v;
            for(auto &v:values)in>>std::hex>>v;
            require(bool(in) && packed_inputs.emplace(key,values).second,"invalid original packed FP vertex fixture");
        }
        require(packed_inputs.size()==3295,"original packed FP coverage changed");
        struct SourceSkin {unsigned count;std::array<unsigned,4> bones;std::array<float,4> weights;};
        std::map<std::array<unsigned,3>,SourceSkin> source_skin;
        std::ifstream weight_fixture(std::filesystem::path(__FILE__).parent_path()/"fixtures/retail_fp_skin_weights.tsv");
        require(bool(weight_fixture),"original source FP weight fixture missing");
        while(std::getline(weight_fixture,packed_line)) {
            if(packed_line.empty() || packed_line[0]=='#')continue;
            std::istringstream in(packed_line);std::array<unsigned,3> key;SourceSkin skin;
            for(auto &v:key)in>>v;in>>skin.count;for(auto &v:skin.bones)in>>v;
            for(auto &v:skin.weights) {std::uint32_t word;in>>std::hex>>word;v=std::bit_cast<float>(word);}
            require(bool(in) && skin.count>=1 && skin.count<=4 && source_skin.emplace(key,skin).second,"invalid original FP weight fixture");
        }
        require(source_skin.size()==packed_inputs.size(),"original source weight coverage changed");
        std::map<std::array<unsigned,3>,std::array<float,12>> source_basis;
        std::ifstream basis_fixture(std::filesystem::path(__FILE__).parent_path()/"fixtures/retail_fp_world_basis.tsv");
        require(bool(basis_fixture),"original GPU hand basis fixture missing");
        while(std::getline(basis_fixture,packed_line)) {
            if(packed_line.empty() || packed_line[0]=='#')continue;
            std::istringstream in(packed_line);std::array<unsigned,3> key;std::array<float,12> values;
            for(auto &v:key)in>>v;
            for(auto &v:values) {std::uint32_t word;in>>std::hex>>word;v=std::bit_cast<float>(word);}
            require(bool(in) && source_basis.emplace(key,values).second,"invalid original GPU hand basis fixture");
        }
        require(source_basis.size()==1530,"original GPU hand basis coverage changed");
        unsigned packed_checked=0,negative_normal_components=0;
        for(unsigned kind=0;kind<2;++kind) {
            const auto &model=kind?arms:weapon;
            require(model.vertex_origins.size()==model.mesh.vertices.size(),"FP source vertex origins missing");
            for(unsigned i=0;i<model.mesh.vertices.size();++i) {
                const auto origin=model.vertex_origins[i];const auto found=packed_inputs.find({kind,origin.section,origin.vertex});
                require(found!=packed_inputs.end(),"FP model vertex lacks original captured input");
                const auto &v=model.mesh.vertices[i];const std::array<float,14> native{v.position.x,v.position.y,v.position.z,v.uv.x,v.uv.y,
                    v.normal.x,v.normal.y,v.normal.z,v.tangent.x,v.tangent.y,v.tangent.z,v.binormal.x,v.binormal.y,v.binormal.z};
                for(unsigned axis=0;axis<native.size();++axis) {
                    require(std::bit_cast<std::uint32_t>(native[axis])==found->second[axis],"FP packed position/UV/basis differs from original GPU input");
                    if(axis>=5 && native[axis]<-1.f)++negative_normal_components;
                }
                const auto &source=source_skin.at({kind,origin.section,origin.vertex});
                for(unsigned j=0;j<4;++j) {
                    require(std::bit_cast<std::uint32_t>(model.skin[i].weights[j])==std::bit_cast<std::uint32_t>(source.weights[j]),"FP effective weight differs from original compiled VP");
                    if(j<source.count)require(model.skin[i].bones[j]==source.bones[j],"FP bone differs from original captured indices/node map");
                }
                ++packed_checked;
            }
        }
        require(packed_checked==10344 && negative_normal_components>100,"insufficient original FP packed endpoint coverage");
        std::cout<<"original FP packed inputs: "<<packed_checked<<" expanded vertices bit exact, "<<negative_normal_components<<" packed normal components below -1\n";
        const auto clip=cache.load_action(w->second.first_person_animation,"any","any","any","idle");
        const auto camera=camera_transform({-5.106619358062744f,-5.505378723144531f,.6299997568130493f},
            {.9562700986862183f,-.2794550657272339f,.0863271951675415f},
            {-.08286145329475403f,.02421497367322445f,.9962668418884277f});
        const auto world=pose_first_person_models_world(weapon,arms,clip,32.f/30.f,w->second.first_person_offset,camera,bank,1.f);
        std::array<std::vector<std::array<float,12>>,2> matrices{std::vector<std::array<float,12>>(weapon.bones.size()),std::vector<std::array<float,12>>(arms.bones.size())};
        std::array<std::vector<bool>,2> seen{std::vector<bool>(weapon.bones.size()),std::vector<bool>(arms.bones.size())};
        std::ifstream fixture(std::filesystem::path(__FILE__).parent_path()/"fixtures/retail_fp_skin.tsv");
        require(bool(fixture),"original captured FP skin bank fixture missing");
        std::string line;
        while(std::getline(fixture,line)) {
            if(line.empty() || line[0]=='#')continue;
            std::istringstream input(line);unsigned kind,node;input>>kind>>node;
            require(kind<2 && node<matrices[kind].size(),"invalid original FP skin bank fixture");
            for(auto &f:matrices[kind][node])input>>f;
            require(bool(input),"truncated original FP skin bank fixture");seen[kind][node]=true;
        }
        unsigned checked=0,basis_checked=0;float worst=0,basis_worst=0;
        for(unsigned kind=0;kind<2;++kind) {
            const auto &model=kind?arms:weapon;const auto &vertices=kind?world.arms:world.weapon;
            require(vertices.size()==model.mesh.vertices.size(),"world FP skinning changed vertex expansion");
            for(unsigned i=0;i<vertices.size();++i) {
                const auto origin=model.vertex_origins[i];const auto key=std::array<unsigned,3>{kind,origin.section,origin.vertex};
                const auto &source=source_skin.at(key);const auto &words=packed_inputs.at(key);
                const Vec3 p{std::bit_cast<float>(words[0]),std::bit_cast<float>(words[1]),std::bit_cast<float>(words[2])};
                std::array<float,12> blended{};bool covered=true;
                for(int j=int(source.count)-1;j>=0;--j) {
                    const auto bone=source.bones[unsigned(j)];
                    if(bone>=seen[kind].size() || !seen[kind][bone]){covered=false;break;}
                    const auto &m=matrices[kind][bone];const auto weight=source.weights[unsigned(j)];
                    for(unsigned axis=0;axis<12;++axis) {
                        const volatile float product=m[axis]*weight;
                        blended[axis]=j==int(source.count)-1?product:blended[axis]+product;
                    }
                }
                if(!covered)continue;
                std::array<float,3> coordinates;
                for(unsigned axis=0;axis<3;++axis) {
                    const volatile float x=blended[axis*4]*p.x,y=blended[axis*4+1]*p.y,z=blended[axis*4+2]*p.z;
                    coordinates[axis]=((x+y)+z)+blended[axis*4+3];
                }
                const Vec3 expected{coordinates[0],coordinates[1],coordinates[2]};
                const auto d=vertices[i].position-expected;worst=std::max({worst,std::abs(d.x),std::abs(d.y),std::abs(d.z)});++checked;
                if(kind) {
                    const auto &v=vertices[i];const std::array<float,12> actual{v.position.x,v.position.y,v.position.z,v.normal.x,v.normal.y,v.normal.z,v.tangent.x,v.tangent.y,v.tangent.z,v.binormal.x,v.binormal.y,v.binormal.z};
                    const auto &original=source_basis.at(key);
                    for(unsigned axis=3;axis<12;++axis)basis_worst=std::max(basis_worst,std::abs(actual[axis]-original[axis]));
                    ++basis_checked;
                }
            }
        }
        require(checked>1000,"insufficient original captured FP skin coverage");
        require(worst<.000006f,"production FP vertices disagree with original GPU skin bank");
        require(basis_checked==4980 && basis_worst<.000006f,"production hand basis disagrees with actual original GPU output");
        std::cout<<"original FP packed inputs/weights/skin bank: "<<checked<<" vertices, largest position error "<<worst<<"; actual GPU hand basis "<<basis_checked<<" vertices, largest error "<<basis_worst<<'\n';
        constraints+=unsigned(bank.constraints.size());
    }
    std::cout<<"retail first-person IK: "<<constraints<<" authored attachments\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        if(argc>1 && std::string(argv[1])=="--probe") {probe();return 0;}
        synthetic();if(argc>1)retail(argv[1]);
        std::cout<<"first-person marker IK and aiming counter passed\n";
    } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
