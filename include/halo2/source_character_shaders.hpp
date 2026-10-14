#pragma once
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/object_light_query.hpp"
#include <memory>

namespace halo2 {
struct Unit;
// Resident authored unit/creature passes, including vehicle bodies/turrets,
// independent of AI team or species.
// Geometry/pose/visibility and live object-light query chronology remain the
// caller's inputs. Shared images never share an actor's draw constants.
class SourceCharacterShaders {
public:
    SourceCharacterShaders(const Cache&,RenderModel&,std::uint32_t unit_definition);
    ShadowCasterSphere sphere(const Transform& node0,float object_scale)const;
    void select(std::vector<Vertex>&,ShadowCasterSphere,const SourceModelShaderView&)const;
    ShaderInput input(const Unit&,float seconds)const;
    ObjectLightQueryContext light_query_context()const{return lighting_;}
    ObjectLightQueryContext light_query_context(const Unit&)const;
    unsigned object_type()const{return unsigned(lighting_.object_type);}
private:
    ObjectLightQueryContext lighting_;
    ShadowCasterDefinition bounds_;
    std::vector<ObjectFunction> functions_;
    std::shared_ptr<const ShaderFunctionTables> tables_;
    std::unique_ptr<SourceDynamicModelShaders> resident_;
};
}
