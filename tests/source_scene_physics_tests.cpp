#include "halo2/source_scene_physics.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/native_object_lifetimes.hpp"
#include "halo2/math.hpp"
#include "halo2/source_world_pose_wire.hpp"
#include <bit>
#include <cstring>
#include <iostream>
#include <string_view>

using namespace halo2;
namespace {
void check(bool v,const char *message){if(!v)throw FormatError(message);}
constexpr std::uint32_t pool=0x10000,headers=0x11000,child=0x18000,parent=0x19000;
constexpr std::uint32_t child_id=0x13570000,parent_id=0x24680001;
void put(SourceRuntimeMemory &m,std::uint32_t p,std::uint32_t v,unsigned n=4) {
    auto b=m.mutable_view(p,n);for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(i*8));
}
void put(SourceRuntimeMemory &m,std::uint32_t p,Vec3 v) {
    for(auto x:{v.x,v.y,v.z}){put(m,p,std::bit_cast<std::uint32_t>(x));p+=4;}
}
struct Fixture {
    SourceRuntimeMemory memory;
    Fixture(){
        memory.mount(pool,std::vector<std::byte>(0xa000),SourceRuntimeMemory::Access::ReadWrite,"Explicit transform input fixture, not a simulated world");
        put(memory,pool+0x24,12);put(memory,pool+0x38,2);put(memory,pool+0x44,headers);
        put(memory,pool+0x20,0x800);put(memory,pool+0x29,1,1);put(memory,pool+0x48,0x17000);
        put(memory,0x17000,3);put(memory,pool+0x3c,2);
        for(unsigned i=0;i<2;++i){
            put(memory,headers+i*12,i?parent_id>>16:child_id>>16,2);
            put(memory,headers+i*12+6,0x300,2);put(memory,headers+i*12+8,i?parent:child);
            put(memory,headers+i*12+3,11,1);put(memory,i?parent:child,0x10000u+i);
        }
        put(memory,child+0x14,0xffffffffu);put(memory,parent+0x14,0xffffffffu);
        put(memory,child+0x70,Vec3{1,0,0});put(memory,child+0x7c,Vec3{0,0,1});
        put(memory,child+0xa0,std::bit_cast<std::uint32_t>(1.f));
        put(memory,parent+0x70,Vec3{1,0,0});put(memory,parent+0x7c,Vec3{0,0,1});
        put(memory,parent+0xa0,std::bit_cast<std::uint32_t>(1.f));
        put(memory,parent+0x114,0x68,2);put(memory,parent+0x116,0x200,2);
    }
    void configure(Vec3 position,Vec3 forward,Vec3 up,float scale,bool mirrored,
        bool attached,bool parent_mirrored,const Transform &node) {
        put(memory,child+0x64,position);put(memory,child+0x70,forward);put(memory,child+0x7c,up);
        put(memory,child+0xa0,std::bit_cast<std::uint32_t>(scale));put(memory,child+4,mirrored?0x400:0);
        put(memory,child+0x14,attached?parent_id:0xffffffffu);put(memory,child+0x18,1,1);
        put(memory,parent+4,parent_mirrored?0x400:0);
        memory.write(parent+0x234,std::as_bytes(std::span(&node,1)));
    }
    Transform read()const {
        const auto native=memory.native_address(pool,0x4c);
        check(native.value>0xffffffffu,"Scene handoff truncated its live pool address");
        return source_scene_physics_transform(memory,native,child_id);
    }
};
template<class F> void rejects(F f,const char *message){bool failed=false;try{f();}catch(const FormatError&){failed=true;}check(failed,message);}
void body_history_checks() {
    SourceWorldInterpolation history;SourceWorldPose first{child_id,{Transform{},Transform{}}};
    history.reset(100,std::span(&first,1));
    auto second=first;second.nodes[0].position={4,8,12};second.nodes[1].position={8,16,24};
    history.capture(101,std::span(&second,1));
    for(unsigned frame=0;frame<=8;++frame) {
        const float fraction=float(frame)/8;
        const auto nodes=history.sample(child_id,fraction);
        check(nodes[0].position.x==4*fraction && nodes[1].position.x==8*fraction,
            "Source body WORLD nodes did not advance at render fractions");
    }
    const auto a=history.sample(child_id,0),b=history.sample(child_id,1);
    check(!std::memcmp(a.data(),first.nodes.data(),a.size()*sizeof(Transform)) &&
        !std::memcmp(b.data(),second.nodes.data(),b.size()*sizeof(Transform)),"Source body endpoint words changed");
    std::array<SourceWorldPose,2> duplicate{first,first};
    rejects([&]{history.capture(102,duplicate);},"Duplicate source WORLD owners were accepted");
    check(history.sample(child_id,.5f)[1].position.x==4,"Failed WORLD capture partially replaced its completed bank");
    auto fresh=second;fresh.object+=0x10000u;fresh.nodes[0].position.x=100;
    history.capture(102,std::span(&fresh,1));
    check(history.sample(fresh.object,0)[0].position.x==100,"Source body WORLD poses crossed a reused object index");
    rejects([&]{history.sample(child_id,1);},"Retired full source body remained available to rendering");
    fresh.nodes.resize(1);fresh.nodes[0].position.x=200;history.capture(103,std::span(&fresh,1));
    check(history.sample(fresh.object,0).front().position.x==200,"Source body WORLD poses crossed a hierarchy replacement");
    fresh.nodes[0].position.x=300;history.capture(105,std::span(&fresh,1));
    check(history.sample(fresh.object,0).front().position.x==300,"Source body WORLD poses interpolated across missing ticks");
    fresh.nodes[0].position.x=400;history.capture(1,std::span(&fresh,1));
    check(history.sample(fresh.object,0).front().position.x==400,"Source body WORLD poses interpolated across a restored clock");
    history.capture(2,{});rejects([&]{history.sample(fresh.object,1);},"Deleted body pose was retained after capture");
}
void body_wire_checks() {
    SourceWorldPose pose{child_id,{Transform{},Transform{}},{{-0.f,2,3},1.25f}};
    pose.nodes[0].scale=-0.f;pose.nodes[0].basis[1].y=-1;pose.nodes[1].position={4,-0.f,8};
    const std::array<SourceWorldPoseOwner,2> owners{{{child_id,true},{parent_id,false}}};
    const auto bytes=encode_source_world_poses(true,std::span(&pose,1),owners);
    const auto decoded=decode_source_world_poses(bytes,owners);
    check(decoded.authoritative && decoded.consumed==bytes.size() && decoded.poses.size()==1 &&
        !std::memcmp(decoded.poses[0].nodes.data(),pose.nodes.data(),pose.nodes.size()*sizeof(Transform)) &&
        !std::memcmp(&decoded.poses[0].sphere,&pose.sphere,sizeof(pose.sphere)),"Native transport changed original WORLD words or sphere");
    for(unsigned n=0;n<bytes.size();++n)rejects([&]{decode_source_world_poses(std::span(bytes).first(n),owners);},
        "Truncated source WORLD bank was accepted");
    const auto empty=encode_source_world_poses(false,{},owners);
    check(!decode_source_world_poses(empty,owners).authoritative,"Non-source host claimed source WORLD authority");
    rejects([&]{encode_source_world_poses(false,std::span(&pose,1),owners);},"Source nodes were accepted without their authority");
    rejects([&]{encode_source_world_poses(true,{},owners);},"Living source body was silently omitted");
    std::array<SourceWorldPose,2> duplicate{pose,pose};
    rejects([&]{encode_source_world_poses(true,duplicate,owners);},"Duplicate source WORLD body was transmitted");
    auto bad=pose;bad.object+=0x10000;
    rejects([&]{encode_source_world_poses(true,std::span(&bad,1),owners);},"WORLD transport crossed a source salt");
    bad=pose;bad.nodes[0].position.x=std::numeric_limits<float>::infinity();
    rejects([&]{encode_source_world_poses(true,std::span(&bad,1),owners);},"Non-finite source WORLD node was transmitted");
    auto malformed=bytes;malformed[1]=std::byte(1);malformed[2]=std::byte(8);
    rejects([&]{decode_source_world_poses(malformed,owners);},"Unbounded WORLD body count was accepted");
    malformed=bytes;malformed[7]=std::byte(0xff);malformed[8]=std::byte(0xff);
    rejects([&]{decode_source_world_poses(malformed,owners);},"Unbounded WORLD hierarchy was accepted");
    std::vector<SourceWorldPose> full;std::vector<SourceWorldPoseOwner> roster;
    for(unsigned n=0;n<64;++n) {
        auto p=pose;p.object=child_id+n;p.nodes.resize(40);full.push_back(p);roster.push_back({p.object,true});
    }
    const auto large=encode_source_world_poses(true,full,roster);
    check(large.size()>65535,"Retained body fixture did not exceed the old snapshot bound");
    const auto received=decode_source_world_poses(large,roster);
    check(received.poses.size()==64 && received.poses.back().nodes.size()==40 && received.consumed==large.size(),
        "WORLD bank was truncated to the previous native snapshot bound");
}
void checks(){
    body_history_checks();body_wire_checks();
    Fixture fixture;Transform node;node.position={10,20,30};node.scale=2;
    fixture.configure({1,2,3},{1,0,0},{0,0,1},1,false,false,false,node);
    const auto before=std::vector<std::byte>(fixture.memory.view(pool,0xa000).begin(),fixture.memory.view(pool,0xa000).end());
    const auto root=fixture.read();check(root.position.x==1 && root.position.y==2 && root.position.z==3,"Solved source position was lost");
    check(!std::memcmp(before.data(),fixture.memory.view(pool,0xa000).data(),before.size()),"Presentation wrote source physics state");
    check(source_scene_physics_nodes(fixture.memory,pool,child_id).empty(),"Empty original WORLD bank became a supplied bind pose");
    const auto parent_nodes=source_scene_physics_nodes(fixture.memory,pool,parent_id);
    check(parent_nodes.size()==2 && !std::memcmp(&parent_nodes[1],&node,sizeof(node)),"WORLD hierarchy decode changed current source words");
    fixture.configure({1,2,3},{1,0,0},{0,0,1},1,true,true,true,node);
    const auto attached=fixture.read();check(attached.position.x==12 && attached.position.y==16 && attached.position.z==36,"Actual selected parent WORLD node was lost");
    rejects([&]{source_scene_physics_transform(fixture.memory,pool,child_id+0x10000);},"Stale object salt was accepted");
    put(fixture.memory,child+0x14,parent_id+0x10000);
    rejects([&]{fixture.read();},"Stale parent salt was accepted");
    put(fixture.memory,child+0x14,parent_id);put(fixture.memory,child+0x18,2,1);
    rejects([&]{fixture.read();},"Missing parent node became a placement fallback");
    SceneObjects::Object object;object.handle=child_id;object.position={999,999,999};object.rotation={.4f,.2f,.1f};
    object.solved_transform=root;SceneInterpolation history;history.reset(0,std::span(&object,1));
    auto moved=root;moved.position.x+=4;object.solved_transform=moved;history.capture(1,std::span(&object,1));
    const auto a=history.sample(object,0),b=history.sample(object,1),half=history.sample(object,.5f);
    check(!std::memcmp(&a,&root,sizeof(root)) && !std::memcmp(&b,&moved,sizeof(moved)),"Source interpolation changed endpoint words");
    check(half.position.x==3 && !std::memcmp(half.basis.data(),root.basis.data(),sizeof(root.basis)),"Solved scene root was replaced with stale Euler metadata");
    object.solved_nodes=std::vector<Transform>{root,root};history.reset(2,std::span(&object,1));
    object.solved_nodes->at(1)=moved;history.capture(3,std::span(&object,1));
    const auto old_nodes=history.sample_nodes(object,0),new_nodes=history.sample_nodes(object,1),half_nodes=history.sample_nodes(object,.5f);
    check(!std::memcmp(&old_nodes[1],&root,sizeof(root)) && !std::memcmp(&new_nodes[1],&moved,sizeof(moved)),"WORLD interpolation changed endpoint words");
    check(half_nodes[1].position.x==3 && half_nodes[0].position.x==1,"Child WORLD node did not interpolate independently of its root");
    auto next_generation=object;next_generation.handle+=0x10000u;
    const auto fresh_nodes=history.sample_nodes(next_generation,0);
    check(!std::memcmp(&fresh_nodes[1],&moved,sizeof(moved)),"WORLD interpolation crossed source lifetime salt");
    next_generation=object;next_generation.solved_nodes->resize(1);
    check(history.sample_nodes(next_generation,0).size()==1,"WORLD interpolation crossed a hierarchy change");
    auto mirror_from=root,mirror_to=root;
    mirror_from.basis[1]={0,-1,0};mirror_to.basis={Vec3{0,1,0},Vec3{1,0,0},Vec3{0,0,1}};
    const auto mirror_half=render_transform(mirror_from,mirror_to,.5f);
    check(dot(cross(mirror_half.basis[0],mirror_half.basis[1]),mirror_half.basis[2])<-.999f,
        "WORLD interpolation discarded the source handedness flag");
    Fixture batch;batch.configure({1,2,3},{0,1,0},{0,0,1},2,false,false,false,node);
    LevelContent content;content.objects.resize(2);
    for(unsigned i=0;i<2;++i){auto &o=content.objects[i];o.type=11;o.placement=i;o.definition=0x10000+i;o.name=-1;o.source=-1;o.rotation={.7f,.3f,.4f};o.bounding_offset={1,0,0};}
    NativeObjectLifetimes lifetimes(batch.memory,pool);
    SceneObjects scene(content,0,{}, {}, &lifetimes,{
        [](const ScenarioObject &o){return o.placement?parent_id:child_id;},[](auto){}});
    publish_source_scene_physics(scene,batch.memory,pool);
    check(scene.get(child_id).position.x==1 && scene.get(child_id).rotation.x==.7f,"Publication lost source position or changed placement metadata");
    const auto center=scene.center(child_id);check(center.x==1 && center.y==4 && center.z==3,"Scene bounds use stale Euler orientation");
    const auto prior=*scene.get(child_id).solved_transform;
    put(batch.memory,child+0x64,Vec3{99,99,99});put(batch.memory,headers+12+6,0x20,2);
    rejects([&]{publish_source_scene_physics(scene,batch.memory,pool);},"Incomplete source body was published");
    check(!std::memcmp(&prior,&*scene.get(child_id).solved_transform,sizeof(prior)),"Failed batch partially published solved transforms");
    rejects([&]{scene.observe_source_retirement(child_id);},"Live source generation was retired by presentation");
    put(batch.memory,headers+12+6,0x300,2);
    // A source-owned deletion/reallocation changes the salt before the next
    // publication. The presentation must retire only the old generation.
    put(batch.memory,headers,0x1358,2);
    const auto retired_bank=std::vector<std::byte>(batch.memory.view(pool,0xa000).begin(),batch.memory.view(pool,0xa000).end());
    const auto prior_events=lifetimes.events().size();
    publish_source_scene_physics(scene,batch.memory,pool);
    rejects([&]{scene.get(child_id);},"Source-retired scene generation remained visible");
    check(scene.get(parent_id).handle==parent_id && lifetimes.events().size()==prior_events+1 &&
        lifetimes.events().back().kind==NativeObjectLifetimeEventKind::release,
        "Source retirement lost a live peer or omitted its lifetime observation");
    check(!std::memcmp(retired_bank.data(),batch.memory.view(pool,0xa000).data(),retired_bank.size()),
        "Source retirement observation invoked another deletion or changed the new generation");
    rejects([&]{scene.observe_source_retirement(child_id);},"Source generation was retired twice");
    std::cout<<"PASS source root/parent/salt ownership, read-only bank and solved render interpolation\n";
}
}
int main(int argc,char **argv){try{
    if(argc==2 && std::string_view(argv[1])=="--probe"){
        for(;;){
            std::array<std::uint32_t,26> input{};if(!(std::cin>>std::hex>>input[0]))break;
            for(unsigned i=1;i<input.size();++i)check(bool(std::cin>>std::hex>>input[i]),"Truncated source transform probe");
            const auto f=[&](unsigned i){return std::bit_cast<float>(input[i]);};
            const auto v=[&](unsigned i){return Vec3{f(i),f(i+1),f(i+2)};};
            Transform parent_node;std::memcpy(&parent_node,input.data()+13,sizeof(parent_node));
            Fixture fixture;fixture.configure(v(0),v(3),v(6),f(9),input[10],input[11],input[12],parent_node);
            for(auto word:std::bit_cast<std::array<std::uint32_t,13>>(fixture.read()))std::cout<<std::hex<<word<<' ';
            std::cout<<'\n';
        }
        return 0;
    }
    check(argc==1,"Unknown source scene physics arguments");checks();return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
