#include "halo2/render_interpolation.hpp"
#include "halo2/model_node_interpolation.hpp"
#include "halo2/math.hpp"
#include "halo2/source_model_pose.hpp"
#include "halo2/source_shadow.hpp"
#include <glm/gtc/quaternion.hpp>
#include <cstring>
#include <set>

namespace halo2 {
void validate_render_fraction(float f) {
    if(!std::isfinite(f) || f<0 || f>1)throw FormatError("Invalid render interpolation fraction");
}
Vec3 render_lerp(Vec3 a,Vec3 b,float f) {
    validate_render_fraction(f);
    if(f==0)return a;
    if(f==1)return b;
    return {std::lerp(a.x,b.x,f),std::lerp(a.y,b.y,f),std::lerp(a.z,b.z,f)};
}
float render_loop(float a,float b,float period,float f) {
    validate_render_fraction(f);
    if(!std::isfinite(period) || period<=0)throw FormatError("Invalid render interpolation period");
    if(f==0)return a;
    if(f==1 || a==b)return b;
    const float value=a+std::remainder(b-a,period)*f;
    return value-std::floor(value/period)*period;
}
float render_angle(float a,float b,float f) {
    validate_render_fraction(f);
    if(f==0)return a;
    if(f==1 || a==b)return b;
    return a+std::remainder(b-a,6.283185307179586f)*f;
}
Transform render_transform(const Transform &a,const Transform &b,float f) {
    validate_render_fraction(f);
    if(f==0)return a;
    if(f==1 || !std::memcmp(&a,&b,sizeof(a)))return b;
    const auto mirrored=[](const Transform &t) {
        return dot(cross(t.basis[0],t.basis[1]),t.basis[2])<0;
    };
    const bool mirror_a=mirrored(a),mirror_b=mirrored(b);
    const auto rotation=[](const Transform &t,bool mirror) {
        const float handedness=mirror?-1.f:1.f;
        return glm::normalize(glm::quat_cast(glm::mat3(
            glm::vec3(t.basis[0].x,t.basis[0].y,t.basis[0].z),
            glm::vec3(t.basis[1].x*handedness,t.basis[1].y*handedness,t.basis[1].z*handedness),
            glm::vec3(t.basis[2].x,t.basis[2].y,t.basis[2].z))));
    };
    Transform out=b;
    // Keep unchanged rotation words exact, including authored signed zeros.
    // A reflection is a discrete source flag, not a quaternion rotation.
    // Interpolate within the retained handedness and snap its flag transition.
    if(mirror_a==mirror_b && std::memcmp(a.basis.data(),b.basis.data(),sizeof(a.basis))) {
        const auto basis=glm::mat3_cast(glm::slerp(rotation(a,mirror_a),rotation(b,mirror_b),f));
        for(unsigned i=0;i<3;++i)out.basis[i]={basis[i].x,basis[i].y,basis[i].z};
        if(mirror_b)out.basis[1]=out.basis[1]*-1.f;
    }
    out.position=render_lerp(a.position,b.position,f);
    out.scale=std::lerp(a.scale,b.scale,f);
    return out;
}
std::vector<Transform> interpolate_model_nodes(const RenderModel &model,
    std::span<const Transform> before,std::span<const Transform> after,float f,
    std::span<const ModelScaleReset> resets) {
    validate_render_fraction(f);
    if(before.size()!=after.size() || after.size()!=model.bones.size())
        throw FormatError("Completed model interpolation changed its authored hierarchy");
    for(const auto &reset:resets)
        if(reset.node>=model.bones.size() || !std::isfinite(reset.visible_min) ||
           !std::isfinite(reset.visible_max) || !std::isfinite(reset.hidden) ||
           reset.hidden<0 || reset.hidden>=reset.visible_min || reset.visible_min>reset.visible_max)
            throw FormatError("Completed model interpolation received invalid scale reset metadata");
    if(f==0)return {before.begin(),before.end()};
    if(f==1 || !std::memcmp(before.data(),after.data(),after.size_bytes()))return {after.begin(),after.end()};
    const auto blend=[&](unsigned node,const Transform &a,const Transform &b) {
        auto shown=render_transform(a,b,f);
        // A terminal authored scale reset hides the geometry. Blending its
        // completed endpoints would sweep a one-shot blast backwards through
        // the camera. Only that channel steps; root motion and other nodes
        // retain their normal render interpolation.
        for(const auto &reset:resets)
            if(reset.node==node && model_scale_reset(reset,a.scale,b.scale))shown.scale=b.scale;
        return shown;
    };
    std::vector<Transform> result;result.reserve(after.size());
    for(unsigned n=0;n<after.size();++n) {
        const auto parent=model.bones[n].parent;
        if(parent<0)result.push_back(blend(n,before[n],after[n]));
        else {
            if(unsigned(parent)>=n)throw FormatError("Completed model has an unordered parent hierarchy");
            const auto a=compose_transform(inverse_transform(before[parent]),before[n]);
            const auto b=compose_transform(inverse_transform(after[parent]),after[n]);
            result.push_back(compose_transform(result[parent],blend(n,a,b)));
        }
    }
    return result;
}
void ModelNodeInterpolation::capture(std::uint64_t tick,std::span<const Transform> nodes,
    std::optional<Transform> reference_frame) {
    capture(tick,nodes,reference_frame,false);
}
void ModelNodeInterpolation::capture(std::uint64_t tick,std::span<const Transform> nodes,
    std::optional<Transform> reference_frame,bool render) {
    if(nodes.empty())throw FormatError("Completed model interpolation requires nodes");
    if(!initialized_ || nodes.size()!=current_.size() ||
       reference_frame.has_value()!=current_frame_.has_value() || tick<tick_ || (!render && tick-tick_>1)) {
        previous_.clear();previous_frame_.reset();paired_=false;
    } else if(tick!=tick_) {
        previous_=std::move(current_);previous_frame_=current_frame_;previous_tick_=tick_;paired_=true;
    }
    current_.assign(nodes.begin(),nodes.end());current_frame_=reference_frame;
    tick_=tick;initialized_=true;
}
void ModelNodeInterpolation::capture_render(std::uint64_t tick,std::span<const Transform> nodes,
    std::optional<Transform> frame) {
    capture(tick,nodes,frame,true);
}
std::vector<Transform> ModelNodeInterpolation::sample_render(const RenderModel &model,float f,
    std::optional<Transform> frame,std::span<const ModelScaleReset> resets)const {
    validate_render_fraction(f);
    // The presentation clock is one simulation tick behind its current
    // completed endpoint. A slow draw may span several ticks; using f alone
    // would replay that entire interval and then jump at the next endpoint.
    if(paired_ && f!=1)f=float(1.0-(1.0-double(f))/double(tick_-previous_tick_));
    return sample(model,f,frame,resets);
}
std::vector<Transform> ModelNodeInterpolation::sample(const RenderModel &model,float f,
    std::optional<Transform> frame,std::span<const ModelScaleReset> resets)const {
    validate_render_fraction(f);
    if(!initialized_ || current_.size()!=model.bones.size() ||
       frame.has_value()!=current_frame_.has_value())
        throw FormatError("Completed model interpolation lost its model/reference frame");
    const auto reframe=[](std::span<const Transform> nodes,const Transform &from,const Transform &to) {
        if(!std::memcmp(&from,&to,sizeof(Transform)))return std::vector<Transform>(nodes.begin(),nodes.end());
        auto result=std::vector<Transform>(nodes.begin(),nodes.end());
        const auto relative=compose_transform(to,inverse_transform(from));
        for(auto &node:result)node=compose_transform(relative,node);
        return result;
    };
    if(!paired_ || f==1)return frame?reframe(current_,*current_frame_,*frame):current_;
    if(f==0)return frame?reframe(previous_,*previous_frame_,*frame):previous_;
    if(!frame)return interpolate_model_nodes(model,previous_,current_,f,resets);
    // FP animation is relative to its camera. Never blend yesterday's camera
    // into today's mouse input, nor apply interpolated player movement twice.
    const auto a=reframe(previous_,*previous_frame_,Transform{});
    const auto b=reframe(current_,*current_frame_,Transform{});
    auto result=interpolate_model_nodes(model,a,b,f,resets);
    for(auto &node:result)node=compose_transform(*frame,node);
    return result;
}
namespace {
bool same_world_attachment(const SourceWorldPose &a,const SourceWorldPose &b) {
    if(a.attachment.has_value()!=b.attachment.has_value())return false;
    if(!a.attachment)return true;
    return a.attachment->parent==b.attachment->parent &&
        a.attachment->parent_node==b.attachment->parent_node;
}
void validate_source_world_poses(std::span<const SourceWorldPose> poses) {
    for(unsigned n=0;n<poses.size();++n) {
        if(poses[n].object==0xffffffffu || poses[n].nodes.empty())
            throw FormatError("WORLD interpolation requires a live full object and current hierarchy");
        for(unsigned before=0;before<n;++before)if(poses[before].object==poses[n].object)
            throw FormatError("WORLD interpolation has duplicate full object ownership");
        if(const auto &binding=poses[n].attachment) {
            const bool attached=binding->parent!=0xffffffffu;
            if(attached!=binding->parent_frame.has_value() || binding->parent==poses[n].object ||
                (attached && (!(binding->parent>>16u) || (binding->parent&65535u)>=2048u)))
                throw FormatError("WORLD interpolation has an invalid native parent binding");
            if(binding->parent_frame) {
                const auto &frame=*binding->parent_frame;
                for(float value:{frame.scale,frame.position.x,frame.position.y,frame.position.z,
                    frame.basis[0].x,frame.basis[0].y,frame.basis[0].z,
                    frame.basis[1].x,frame.basis[1].y,frame.basis[1].z,
                    frame.basis[2].x,frame.basis[2].y,frame.basis[2].z})
                    if(!std::isfinite(value))throw FormatError("WORLD interpolation has a nonfinite native parent frame");
            }
        }
    }
}
}
void SourceWorldInterpolation::reset(std::uint64_t tick,std::span<const SourceWorldPose> poses) {
    validate_source_world_poses(poses);
    auto next=std::vector<SourceWorldPose>(poses.begin(),poses.end());
    current_=std::move(next);previous_.clear();tick_=tick;previous_tick_=tick;
    initialized_=true;paired_=false;native_pair_=false;
}
void SourceWorldInterpolation::capture(std::uint64_t tick,std::span<const SourceWorldPose> poses) {
    capture(tick,poses,{});
}
void SourceWorldInterpolation::capture(std::uint64_t tick,std::span<const SourceWorldPose> poses,
    std::span<const std::uint32_t> discontinuities) {
    if(!initialized_ || tick<=tick_ || tick-tick_!=1){reset(tick,poses);return;}
    validate_source_world_poses(poses);
    auto next=std::vector<SourceWorldPose>(poses.begin(),poses.end());
    previous_=std::move(current_);current_=std::move(next);previous_tick_=tick_;tick_=tick;paired_=true;native_pair_=false;
    std::erase_if(previous_,[&](const auto &pose) {
        return std::find(discontinuities.begin(),discontinuities.end(),pose.object)!=discontinuities.end();
    });
}
void SourceWorldInterpolation::capture_native_cut(const SourceNativeCutPermit &permit,
    std::span<const SourceWorldPose> poses,std::span<const std::uint32_t> discontinuities) {
    if(!initialized_ || tick_!=permit.before_ || permit.after_<=permit.before_ || permit.after_-permit.before_>120)
        throw FormatError("Native WORLD pair lost its actual completed tick owners");
    validate_source_world_poses(poses);
    auto next=std::vector<SourceWorldPose>(poses.begin(),poses.end());
    auto before=current_;
    std::erase_if(before,[&](const auto &old) {
        const auto now=std::find_if(next.begin(),next.end(),[&](const auto &pose){return pose.object==old.object;});
        return now==next.end() || now->nodes.size()!=old.nodes.size() || !same_world_attachment(old,*now) ||
            old.model!=now->model || std::find(discontinuities.begin(),discontinuities.end(),old.object)!=discontinuities.end();
    });
    previous_=std::move(before);current_=std::move(next);previous_tick_=permit.before_;tick_=permit.after_;
    paired_=native_pair_=true;presented_.clear();predicted_.clear();
}
namespace {
const Transform *presented_motion(const std::vector<std::pair<std::uint32_t,Transform>> &motions,std::uint32_t object) {
    const auto found=std::lower_bound(motions.begin(),motions.end(),object,
        [](const auto &entry,std::uint32_t key){return entry.first<key;});
    return found!=motions.end() && found->first==object?&found->second:nullptr;
}
std::vector<std::pair<std::uint32_t,Transform>> sorted_motions(std::span<const std::pair<std::uint32_t,Transform>> motions) {
    std::vector<std::pair<std::uint32_t,Transform>> result(motions.begin(),motions.end());
    std::sort(result.begin(),result.end(),[](const auto &a,const auto &b){return a.first<b.first;});
    for(std::size_t i=1;i<result.size();++i)if(result[i].first==result[i-1].first)
        throw FormatError("Presentation motion repeats a full object owner");
    for(const auto &[object,motion]:result)
        for(float f:{motion.scale,motion.position.x,motion.position.y,motion.position.z,
            motion.basis[0].x,motion.basis[0].y,motion.basis[0].z,motion.basis[1].x,motion.basis[1].y,
            motion.basis[1].z,motion.basis[2].x,motion.basis[2].y,motion.basis[2].z})
            if(!std::isfinite(f))throw FormatError("Non-finite presentation motion");
    return result;
}
}
void SourceWorldInterpolation::present(std::span<const std::pair<std::uint32_t,Transform>> motions) {
    presented_=sorted_motions(motions);
}
void SceneInterpolation::present(std::span<const std::pair<std::uint32_t,Transform>> motions) {
    presented_=sorted_motions(motions);
}
void SourceWorldInterpolation::predict(std::span<const SourceWorldPredictedRoot> roots) {
    std::vector<SourceWorldPredictedRoot> next(roots.begin(),roots.end());
    for(std::size_t i=0;i<next.size();++i) {
        if(next[i].object==0xffffffffu)throw FormatError("Predicted body has no full object owner");
        for(std::size_t j=0;j<i;++j)if(next[j].object==next[i].object)
            throw FormatError("Predicted body repeats a full object owner");
        for(const auto &origin:{next[i].previous,next[i].current,next[i].predicted})
            for(float value:{origin.x,origin.y,origin.z})
                if(!std::isfinite(value))throw FormatError("Non-finite predicted body origin");
    }
    predicted_=std::move(next);
}
std::optional<Vec3> SourceWorldInterpolation::predicted_offset(std::uint32_t object,float f,bool model_order)const {
    if(predicted_.empty())return std::nullopt;
    const auto pose=[](const std::vector<SourceWorldPose> &poses,std::uint32_t owner)->const SourceWorldPose* {
        const auto at=std::find_if(poses.begin(),poses.end(),[&](const auto &p){return p.object==owner;});
        return at==poses.end()?nullptr:&*at;
    };
    // The own predicted body this object is, or hangs from through the
    // current bank's attachment chain (held weapons, stuck projectiles).
    const SourceWorldPredictedRoot *root=nullptr;
    auto owner=object;
    for(std::size_t depth=0;!root;++depth) {
        for(const auto &candidate:predicted_)if(candidate.object==owner)root=&candidate;
        if(root)break;
        const auto *child=pose(current_,owner);
        // A parent cycle is longer than the bank; model sampling rejects it.
        if(depth>current_.size() || !child || !child->attachment || child->attachment->parent==0xffffffffu ||
           !pose(current_,child->attachment->parent))return std::nullopt;
        owner=child->attachment->parent;
    }
    // Dead-reckoned objects already carry their own rigid motion.
    if(presented_motion(presented_,object) || presented_motion(presented_,root->object))return std::nullopt;
    const auto *now=pose(current_,root->object);
    if(!now)return std::nullopt;
    // The authoritative origin the ordinary sample of that body shows: its
    // endpoints blend exactly where its WORLD nodes do. A legacy binding's
    // model-order sample keeps the current completed bank.
    const auto *old=pose(previous_,root->object);
    const bool paired=paired_ && old && old->nodes.size()==now->nodes.size() && same_world_attachment(*old,*now) &&
        (!model_order || now->attachment.has_value());
    const auto shown=paired?render_lerp(root->previous,root->current,f):root->current;
    return root->predicted-shown;
}
std::vector<Transform> SourceWorldInterpolation::sample(std::uint32_t object,float f)const {
    auto result=sample_ordinary(object,f);
    if(const auto offset=predicted_offset(object,f,false))
        for(auto &node:result)node.position=node.position+*offset;
    return result;
}
std::vector<Transform> SourceWorldInterpolation::sample(std::uint32_t object,const RenderModel &model,float f,
    std::optional<Transform> shown_parent)const {
    const bool framed=shown_parent.has_value();
    auto result=sample_ordinary(object,model,f,shown_parent);
    // A shown parent frame already carries its predicted body's displacement.
    if(!framed)if(const auto offset=predicted_offset(object,f,true))
        for(auto &node:result)node.position=node.position+*offset;
    return result;
}
std::vector<Transform> SourceWorldInterpolation::sample_ordinary(std::uint32_t object,float f)const {
    validate_render_fraction(f);
    const auto now=std::find_if(current_.begin(),current_.end(),[&](const auto &p){return p.object==object;});
    if(now==current_.end())throw FormatError("WORLD interpolation has no current full object owner");
    if(const auto *motion=presented_motion(presented_,object)) {
        auto result=now->nodes;
        for(auto &node:result)node=compose_transform(*motion,node);
        return result;
    }
    if(!paired_ || f==1)return now->nodes;
    const auto old=std::find_if(previous_.begin(),previous_.end(),[&](const auto &p){return p.object==object;});
    if(old==previous_.end() || old->nodes.size()!=now->nodes.size() || !same_world_attachment(*old,*now))return now->nodes;
    if(f==0)return old->nodes;
    auto result=now->nodes;
    for(unsigned n=0;n<result.size();++n)result[n]=render_transform(old->nodes[n],now->nodes[n],f);
    return result;
}
std::vector<Transform> SourceWorldInterpolation::sample_ordinary(std::uint32_t object,const RenderModel &model,float f,
    std::optional<Transform> shown_parent)const {
    validate_render_fraction(f);
    const auto now=std::find_if(current_.begin(),current_.end(),[&](const auto &pose){return pose.object==object;});
    if(now==current_.end())throw FormatError("Native WORLD interpolation has no current full object owner");
    if(now->nodes.size()!=model.bones.size())
        throw FormatError("Native WORLD interpolation differs from its admitted model hierarchy");
    // Dead-reckoned props already carry their newest completed WORLD bank
    // rigidly. Do not interpolate or apply another parent motion to it.
    if(presented_motion(presented_,object))return sample(object,f);
    // A legacy packet declares no native attachment relationship. Preserve
    // its completed output rather than manufacture a parent or local pose.
    if(!now->attachment)return now->nodes;
    const auto old=std::find_if(previous_.begin(),previous_.end(),[&](const auto &pose){return pose.object==object;});
    const bool paired=paired_ && old!=previous_.end() && old->nodes.size()==now->nodes.size() && same_world_attachment(*old,*now);
    ModelNodeInterpolation history;
    if(native_pair_) {
        if(paired)history.capture_render(previous_tick_,old->nodes,old->attachment->parent_frame);
        history.capture_render(tick_,now->nodes,now->attachment->parent_frame);
    }else {
        if(paired)history.capture(previous_tick_,old->nodes,old->attachment->parent_frame);
        history.capture(tick_,now->nodes,now->attachment->parent_frame);
    }
    if(now->attachment->parent_frame && !shown_parent) {
        if(std::any_of(current_.begin(),current_.end(),[&](const auto &pose) {
            return pose.object==now->attachment->parent;
        }))throw FormatError("Native carried WORLD sampling requires its actual shown parent hierarchy");
        shown_parent=paired?render_transform(*old->attachment->parent_frame,*now->attachment->parent_frame,f):
            *now->attachment->parent_frame;
    }
    if(!now->attachment->parent_frame && shown_parent)
        throw FormatError("Native unparented WORLD pose received an invented parent frame");
    return history.sample(model,f,shown_parent);
}
std::vector<Transform> SourceWorldInterpolation::sample(std::uint32_t object,float f,const ModelProvider &models)const {
    if(!models)throw FormatError("Native WORLD interpolation requires admitted model hierarchies");
    std::set<std::uint32_t> active;
    std::function<std::vector<Transform>(std::uint32_t)> resolve=[&](std::uint32_t owner) {
        if(!active.insert(owner).second)throw FormatError("Native WORLD interpolation found a parent cycle");
        const auto current=std::find_if(current_.begin(),current_.end(),[&](const auto &pose){return pose.object==owner;});
        if(current==current_.end())throw FormatError("Native WORLD interpolation lost a selected parent owner");
        const auto *model=models(owner);
        if(!model)throw FormatError("Native WORLD interpolation has no admitted owner model hierarchy");
        if(presented_motion(presented_,owner)) {
            auto result=sample(owner,*model,f);active.erase(owner);return result;
        }
        std::optional<Transform> parent_frame;
        if(current->attachment && current->attachment->parent!=0xffffffffu) {
            const auto parent=current->attachment->parent;
            if(std::any_of(current_.begin(),current_.end(),[&](const auto &pose){return pose.object==parent;})) {
                const auto nodes=resolve(parent);
                const auto node=current->attachment->parent_node;
                if(node>=nodes.size())throw FormatError("Native WORLD interpolation escaped its shown parent node bank");
                parent_frame=nodes[node];
            }
        }
        auto result=sample(owner,*model,f,parent_frame);active.erase(owner);return result;
    };
    return resolve(object);
}
void SourceWorldInterpolation::refresh(std::uint64_t tick,std::span<const SourceWorldPose> poses,
    std::span<const std::uint32_t> discontinuities) {
    if(!initialized_ || tick!=tick_)throw FormatError("WORLD refresh requires the current completed tick");
    validate_source_world_poses(poses);
    auto next=std::vector<SourceWorldPose>(poses.begin(),poses.end());
    std::erase_if(previous_,[&](const auto &pose) {
        return std::find(discontinuities.begin(),discontinuities.end(),pose.object)!=discontinuities.end() ||
            std::none_of(next.begin(),next.end(),[&](const auto &now){return now.object==pose.object;});
    });
    current_=std::move(next);
}
void SceneInterpolation::reset(std::uint64_t tick,std::span<const SceneObjects::Object> objects) {
    tick_=tick;initialized_=true;paired_=false;previous_.clear();
    current_.assign(objects.begin(),objects.end());
}
void SceneInterpolation::capture(std::uint64_t tick,std::span<const SceneObjects::Object> objects) {
    if(!initialized_ || tick<=tick_ || tick-tick_!=1){reset(tick,objects);return;}
    previous_.swap(current_);current_.assign(objects.begin(),objects.end());
    tick_=tick;paired_=true;
}
void SceneInterpolation::capture_native_cut(const SourceNativeCutPermit &permit,
    std::span<const SceneObjects::Object> objects) {
    if(!initialized_ || tick_!=permit.before_ || permit.after_<=permit.before_ || permit.after_-permit.before_>120)
        throw FormatError("Native scene pair lost its actual completed tick owners");
    auto next=std::vector<SceneObjects::Object>(objects.begin(),objects.end());
    auto before=current_;
    std::erase_if(before,[&](const auto &old) {
        const auto now=std::find_if(next.begin(),next.end(),[&](const auto &o){return o.handle==old.handle;});
        if(now==next.end() || old.placement!=now->placement || old.created_tick!=now->created_tick ||
            old.bsp!=now->bsp || old.attachment_parent!=now->attachment_parent ||
            old.inventory_attached!=now->inventory_attached || old.visible!=now->visible ||
            old.bsp_hidden!=now->bsp_hidden || old.script_hidden!=now->script_hidden ||
            old.source_model_regions.has_value()!=now->source_model_regions.has_value())return true;
        if(!old.source_model_regions)return false;
        return old.source_model_regions->selection.variant_index!=now->source_model_regions->selection.variant_index ||
            old.source_model_regions->selection.permutations!=now->source_model_regions->selection.permutations ||
            old.source_model_regions->selection.states!=now->source_model_regions->selection.states ||
            old.source_model_regions->selection.unresolved_rng_regions!=now->source_model_regions->selection.unresolved_rng_regions;
    });
    previous_=std::move(before);current_=std::move(next);tick_=permit.after_;paired_=true;presented_.clear();
}
void SceneInterpolation::refresh(std::uint64_t tick,std::span<const SceneObjects::Object> objects) {
    if(!initialized_ || tick!=tick_)throw FormatError("Scene refresh requires the current completed tick");
    auto next=std::vector<SceneObjects::Object>(objects.begin(),objects.end());
    std::erase_if(previous_,[&](const auto &old) {
        return std::none_of(next.begin(),next.end(),[&](const auto &now) {
            return old.handle==now.handle && old.placement==now.placement && old.created_tick==now.created_tick &&
                old.bsp==now.bsp && old.visible && now.visible && !old.bsp_hidden && !now.bsp_hidden;
        });
    });
    current_=std::move(next);
}
Transform scene_object_transform(const SceneObjects::Object &o) {
    if(o.solved_transform)return *o.solved_transform;
    const auto basis=authored_basis(o.rotation);
    return source_object_transform(o.position,basis.forward,basis.up,o.scale,false);
}
Transform SceneInterpolation::sample(const SceneObjects::Object &current,float f)const {
    validate_render_fraction(f);
    const auto now=scene_object_transform(current);
    if(current.handle!=0xffffffffu && current.visible && !current.bsp_hidden)
        if(const auto *motion=presented_motion(presented_,current.handle))return compose_transform(*motion,now);
    if(!paired_ || current.handle==0xffffffffu || !current.visible || current.bsp_hidden)return now;
    const auto old=std::find_if(previous_.begin(),previous_.end(),[&](const auto &o) {
        return o.handle==current.handle && o.placement==current.placement &&
            o.created_tick==current.created_tick && o.bsp==current.bsp && o.visible && !o.bsp_hidden;
    });
    return old==previous_.end()?now:render_transform(scene_object_transform(*old),now,f);
}
std::vector<Transform> SceneInterpolation::sample_nodes(const SceneObjects::Object &current,float f)const {
    validate_render_fraction(f);
    if(!current.solved_nodes)throw FormatError("Scene WORLD interpolation requires the current source hierarchy");
    const auto &now=*current.solved_nodes;
    if(current.handle!=0xffffffffu && current.visible && !current.bsp_hidden)
        if(const auto *motion=presented_motion(presented_,current.handle)) {
            auto result=now;
            for(auto &node:result)node=compose_transform(*motion,node);
            return result;
        }
    if(!paired_ || current.handle==0xffffffffu || !current.visible || current.bsp_hidden || f==1)return now;
    const auto old=std::find_if(previous_.begin(),previous_.end(),[&](const auto &o) {
        return o.handle==current.handle && o.placement==current.placement &&
            o.created_tick==current.created_tick && o.bsp==current.bsp && o.visible && !o.bsp_hidden;
    });
    if(old==previous_.end() || !old->solved_nodes || old->solved_nodes->size()!=now.size())return now;
    if(f==0)return *old->solved_nodes;
    auto result=now;
    for(unsigned n=0;n<result.size();++n)result[n]=render_transform(old->solved_nodes->at(n),now[n],f);
    return result;
}
}
