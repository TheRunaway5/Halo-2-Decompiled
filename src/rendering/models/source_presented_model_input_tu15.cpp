#include "halo2/source_presented_model_pose_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include <thread>

namespace halo2 {
namespace {
using Poses=std::map<std::uint32_t,SourcePresentedModelPoseTu15>;
struct Binding {const Poses *poses{};std::uint32_t frame{};std::set<std::uint32_t> built;};
thread_local std::map<const SourcePhysicsContactMemory *,const Binding *> bindings;
}
struct SourcePresentedModelInputBindingTu15::Impl {
    SourcePhysicsContactMemory &memory;Binding value;
    const std::thread::id executor=std::this_thread::get_id();bool registered{};
    Impl(SourcePhysicsContactMemory &m,const Poses &p,std::uint32_t frame,std::set<std::uint32_t> built)
        :memory(m),value{&p,frame,std::move(built)}{}
    ~Impl(){if(executor!=std::this_thread::get_id())std::terminate();if(registered)bindings.erase(&memory);}
};
SourcePresentedModelInputBindingTu15::SourcePresentedModelInputBindingTu15(SourcePhysicsContactMemory &memory,
    const Poses &poses,std::uint32_t frame,std::set<std::uint32_t> built)
    :impl_(std::make_unique<Impl>(memory,poses,frame,std::move(built))) {
    if(!bindings.emplace(&memory,&impl_->value).second)
        throw FormatError("Native presented WORLD inputs already have a frame owner");
    impl_->registered=true;
}
SourcePresentedModelInputBindingTu15::~SourcePresentedModelInputBindingTu15()=default;
bool source_has_presented_model_pose_tu15(const SourcePhysicsContactMemory &memory)noexcept {
    return bindings.contains(&memory);
}
void source_validate_presented_model_frame_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t object,std::uint32_t frame) {
    const auto it=bindings.find(&memory);
    if(it!=bindings.end() && it->second->frame==(frame&0x3fffffffu) && it->second->built.contains(object))
        throw FormatError("Presented WORLD inputs require a fresh native model frame before cached palette reuse");
}
SourcePresentedModelNodeUseTu15::SourcePresentedModelNodeUseTu15(SourcePhysicsContactMemory &m):memory_(m){}
SourcePresentedModelNodeUseTu15::~SourcePresentedModelNodeUseTu15() {
    if(before_.empty())return;
    try{memory_.write(address_,before_);}catch(...){std::terminate();}
}
void SourcePresentedModelNodeUseTu15::publish(std::uint32_t object,std::uint32_t model,
    std::uint32_t nodes,unsigned count,bool first_person) {
    const auto bound=bindings.find(&memory_);if(first_person || bound==bindings.end())return;
    const auto pose=bound->second->poses->find(object);if(pose==bound->second->poses->end())return;
    if(!before_.empty() || pose->second.render_model!=model || pose->second.nodes.size()!=count)
        throw FormatError("Native palette lost its presented full-salt WORLD input owner");
    const auto bytes=std::as_bytes(std::span(pose->second.nodes));
    before_.resize(bytes.size());memory_.read(nodes,before_);address_=nodes;
    memory_.write(nodes,bytes);
}
}
