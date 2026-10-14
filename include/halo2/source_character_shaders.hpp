#pragma once
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/object_light_query.hpp"
#include <memory>

namespace halo2 {
struct Unit;
// Authored owner inputs shared by ordinary and PRT unit materials.
class SourceCharacterShaderInputs {
public:
    SourceCharacterShaderInputs(const Cache &,std::uint32_t unit_definition);
    ShadowCasterSphere sphere(const Transform& node0,float object_scale)const;
    ShaderInput input(const Unit&,float seconds)const;
    ObjectLightQueryContext light_query_context()const{return lighting_;}
    ObjectLightQueryContext light_query_context(const Unit&)const;
    unsigned object_type()const{return unsigned(lighting_.object_type);}
private:
    ObjectLightQueryContext lighting_;
    ShadowCasterDefinition bounds_;
    std::vector<ObjectFunction> functions_;
    std::shared_ptr<const ShaderFunctionTables> tables_;
};
// Resident authored unit/creature passes, including vehicle bodies/turrets,
// independent of AI team or species.
// Geometry/pose/visibility and live object-light query chronology remain the
// caller's inputs. Shared images never share an actor's draw constants.
class SourceCharacterShaders : public SourceCharacterShaderInputs {
public:
    SourceCharacterShaders(const Cache&,RenderModel&,std::uint32_t unit_definition);
    void select(std::vector<Vertex>&,ShadowCasterSphere,const SourceModelShaderView&)const;
    void select(std::vector<Vertex>&,const SourceModelShaderCollector&,const SourceModelShaderView&)const;
private:
    std::unique_ptr<SourceDynamicModelShaders> resident_;
};
}
