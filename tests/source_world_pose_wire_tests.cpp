#include "halo2/source_world_pose_wire.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void need(bool value,const char *why){if(!value)throw FormatError(why);}
bool same(const Transform &a,const Transform &b){return std::memcmp(&a,&b,sizeof(a))==0;}
}
int main(int argc,char **argv)try {
    const std::array owners{SourceWorldPoseOwner{0x10001u,true},SourceWorldPoseOwner{0x20002u,true}};
    SourceWorldPose parent{owners[0].object,std::vector<Transform>(160),{{1,2,3},4}};
    parent.attachment=SourceWorldPoseAttachment{0xffffffffu,0xc7u,{}};
    auto &frame=parent.nodes[129];frame.position={-0.f,3.25f,-9.f};frame.basis[1].x=.125f;
    SourceWorldPose child{owners[1].object,{Transform{}},{{2,3,4},5}};
    child.attachment=SourceWorldPoseAttachment{parent.object,129u,frame};
    std::vector<SourceWorldPose> poses{parent,child};
    poses[0].model=SourceWorldPoseModel{0x30003u,{0,255,2,0}};
    poses[1].model=SourceWorldPoseModel{0x40004u,{1}};
    const auto encode=[&](const auto &p,SourceWorldPoseWireFormat format){return encode_source_world_poses(true,p,owners,format);};
    const auto legacy=encode(poses,SourceWorldPoseWireFormat::legacy);
    auto unavailable=poses;for(auto &pose:unavailable)pose.attachment.reset();
    need(legacy==encode(unavailable,SourceWorldPoseWireFormat::legacy),"Legacy WORLD bytes changed with new attachment metadata");
    const auto old=decode_source_world_poses(legacy,owners);
    need(old.consumed==legacy.size() && !old.poses[0].attachment && !old.poses[1].attachment,
        "Legacy WORLD bank fabricated known native linkage");
    const auto bytes=encode(poses,SourceWorldPoseWireFormat::parent_frame);
    const auto decoded=decode_source_world_poses(bytes,owners,SourceWorldPoseWireFormat::parent_frame);
    need(!decoded.poses[0].model && !decoded.poses[1].model,
        "Existing WORLD matrix format fabricated model region metadata");
    auto with_models=decoded;
    const auto models=encode_source_world_pose_models(poses);
    need(decode_source_world_pose_models(models,with_models.poses)==models.size() &&
        with_models.poses[0].model==poses[0].model && with_models.poses[1].model==poses[1].model,
        "Native WORLD snapshot dropped actual biped/held-weapon model region choices");
    need(encode_source_world_pose_models(with_models.poses)==models,
        "Native WORLD model region metadata changed source bytes");
    need(decoded.consumed==bytes.size() && decoded.poses.size()==2,"New WORLD bank changed its complete extent");
    need(decoded.poses[0].attachment->parent==0xffffffffu && decoded.poses[0].attachment->parent_node==0xc7u &&
        !decoded.poses[0].attachment->parent_frame,"Known NONE lost its actual unused node byte");
    const auto &binding=*decoded.poses[1].attachment;
    need(binding.parent==parent.object && binding.parent_node==129u && binding.parent_frame &&
        same(*binding.parent_frame,frame),"Native full parent/node or exact frame words changed in transport");
    need(std::bit_cast<std::uint32_t>(binding.parent_frame->position.x)==0x80000000u,
        "Lossless native parent frame normalized a source float word");
    need(bytes==encode(decoded.poses,SourceWorldPoseWireFormat::parent_frame),"New WORLD bank did not roundtrip exact bytes");
    unsigned rejected{};
    const auto reject=[&](auto action){bool failed{};try{action();}catch(const FormatError&){failed=true;}
        need(failed,"Malformed WORLD linkage or format was accepted");++rejected;};
    reject([&]{decode_source_world_poses(bytes,owners);});
    reject([&]{decode_source_world_poses(legacy,owners,SourceWorldPoseWireFormat::parent_frame);});
    auto bad_bytes=bytes;bad_bytes[4]=std::byte{2};
    reject([&]{decode_source_world_poses(bad_bytes,owners,SourceWorldPoseWireFormat::parent_frame);});
    for(const auto length:{0u,4u,7u,unsigned(bytes.size()-1)})
        reject([&]{decode_source_world_poses(std::span(bytes).first(length),owners,SourceWorldPoseWireFormat::parent_frame);});
    for(unsigned invalid=0;invalid<9;++invalid) {
        auto bad=poses;
        if(invalid==0)bad[1].attachment.reset();
        if(invalid==1)bad[1].attachment->parent_frame.reset();
        if(invalid==2)bad[0].attachment->parent_frame=Transform{};
        if(invalid==3)bad[1].attachment->parent=0x20002u;
        if(invalid==4)bad[1].attachment->parent=0x00000001u;
        if(invalid==5)bad[1].attachment->parent_node=200u;
        if(invalid==6)bad[1].attachment->parent_frame->position.z=std::numeric_limits<float>::infinity();
        if(invalid==7)bad[0].attachment=SourceWorldPoseAttachment{child.object,0u,child.nodes[0]};
        if(invalid==8)bad[1].object=parent.object;
        reject([&]{encode(bad,SourceWorldPoseWireFormat::parent_frame);});
    }
    for(unsigned invalid=0;invalid<7;++invalid) {
        auto bad=models;
        if(invalid==0)bad.erase(bad.begin()+1,bad.end());
        if(invalid==1)bad.pop_back();
        if(invalid==2)bad[0]=std::byte{3}; // More models than completed poses.
        if(invalid==3)bad[2]=std::byte{7}; // Unpublished salted owner.
        if(invalid==4)std::fill_n(bad.begin()+6,4,std::byte{255});
        if(invalid==5)bad[10]=std::byte{17};
        if(invalid==6)std::copy_n(bad.begin()+2,4,bad.begin()+15); // Repeated full owner.
        reject([&]{auto target=decoded.poses;(void)decode_source_world_pose_models(bad,target);});
    }
    for(unsigned invalid=0;invalid<3;++invalid) {
        auto bad=poses;
        if(invalid==0)bad[0].model->definition=0xffffffffu;
        if(invalid==1)bad[0].model->permutations.resize(17);
        if(invalid==2)bad[1].object=bad[0].object;
        reject([&]{(void)encode_source_world_pose_models(bad);});
    }
    // Parent/node identity is a full uint8 source field; no guessed signed
    // 128-node cap may replace the actual published hierarchy bound.
    auto large=poses;large[0].nodes.resize(300);large[1].attachment->parent_node=255u;
    large[1].attachment->parent_frame=large[0].nodes[255];
    const auto large_bytes=encode(large,SourceWorldPoseWireFormat::parent_frame);
    need(decode_source_world_poses(large_bytes,owners,SourceWorldPoseWireFormat::parent_frame).poses[1].attachment->parent_node==255,
        "Source unsigned parent node was narrowed independently of the hierarchy");
    if(argc>1)std::ofstream(argv[1])<<"{\"passed\":true,\"legacy_bytes_unchanged\":true,\"legacy_unknown\":true,\"known_none_raw_node\":true,\"exact_frame_words\":true,\"unsigned_node_255\":true,\"malformed_cases\":"<<rejected<<"}\n";
    std::cout<<"PASS source WORLD parent-frame wire, explicit legacy and malformed inputs\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
