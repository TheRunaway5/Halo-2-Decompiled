#include "halo2/world_effects.hpp"
#include "halo2/particle_resource_error.hpp"
#include "halo2/particle_material_validation.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {void require(bool value,const char* message){if(!value)throw FormatError(message);}}
int main(int argc,char** argv) {
    nlohmann::json report{{"passed",false},{"scope","Actual imported MCC world visual admission; no native simulation or framebuffer claim"}};
    const auto save=[&]{if(argc>2){std::ofstream output(argv[2]);output<<report.dump(2)<<'\n';}};
    try {
        require(argc==3,"Supply prepared MCC map and owned JSON report");
        if(!*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
        require(std::getenv("HALO2_TEST_SCRATCH"),"World resource fixture needs its scratch owner");
        const std::filesystem::path path=argv[1];Cache cache(path,path.parent_path());require(cache.is_mcc(),"World resource fixture requires imported MCC content");
        const auto* valid=cache.query_tag({.group=tag_groups::effect,.name="effects\\impact\\explosion_small\\plasma_grenade\\hard_terrain",.require_address=true});
        require(valid,"Actual valid world effect control is missing");
        constexpr auto absent=0x1234fff0u;require(!cache.query_tag({.handle=TagHandle{absent}}),"Missing full-handle negative control is resident");
        const auto* firing=cache.query_tag({.group=tag_groups::effect,.name="effects\\objects\\weapons\\rifle\\smg\\fire_bullet",.require_address=true});
        require(firing,"Actual authored stock SMG firing control is missing");
        const auto muzzle=cache.load_muzzle_effect(firing->datum);require(!muzzle.systems.empty(),"Stock SMG muzzle has no validated systems");
        for(const auto& system:muzzle.systems){require(bool(system.material),"Stock SMG muzzle lost its admitted material");(void)validate_particle_material(*system.material);}
        bool missing_muzzle=false;try{(void)cache.load_muzzle_effect(absent);}catch(const ParticleResourceError&){missing_muzzle=true;}
        require(missing_muzzle,"Missing muzzle full handle lost its typed content rejection");report["muzzle_resources_validated"]=true;
        Mesh mesh;CollisionWorld collision(std::vector<CollisionTriangle>{});std::uint32_t primary=0x12345678u,auxiliary=0x87654321u;
        report["stage"]="construct_world_resources";
        NativeWorldEffects effects(cache,mesh,collision,SourceEffectRandom{primary,auxiliary});
        report["stage"]="valid_effect_control";
        const WorldEffectFrame frame{{1,2,3},{0,0,1},{1,0,0}};effects.spawn(valid->datum,0x11110001u,0,frame);
        const auto initial=effects.diagnostics();const auto random=std::array{primary,auxiliary};
        require(initial.effects==1,"Valid authored world effect did not retain its normal admission");
        report["stage"]="missing_effect_admission";
        effects.spawn(absent,0x22220002u,0,frame);
        require(effects.diagnostics().effects==initial.effects,"Unavailable world visual changed the live effect bank");
        const auto diagnostics=effects.diagnostics().unavailable.size();
        require(diagnostics>initial.unavailable.size(),"Unavailable world visual was not diagnosed");
        effects.spawn(absent,0x22220002u,0,frame);
        require(effects.diagnostics().unavailable.size()==diagnostics,"Unavailable world visual repeated its diagnostic");
        const SourceEffectLocation location{0,Transform{}};
        effects.spawn_source(absent,0x22220002u,0,std::span(&location,1));
        NativeWorldEffects::ObjectMarkers markers{{"",{frame}}};effects.object_markers(0x22220002u,markers);
        effects.spawn_attached(absent,0x22220002u,0x22220002u,0);
        effects.request_weapon(absent,0x22220002u,0,{}, {},1,1,0x33330003u,true,&markers);
        effects.spawn_looping_attached(absent,0x22220002u,0,"",1,1);
        require(effects.diagnostics().effects==initial.effects && std::array{primary,auxiliary}==random,
            "Rejected source/attached visual mutated an existing effect or consumed RNG");
        unsigned partial{};
        for(const auto& tag:cache.tags()) {
            if(tag.group!="effe" || !tag.address)continue;
            bool complete=false;
            try{effects.preload_attached(tag.datum);complete=true;}
            catch(const ParticleResourceError& error) {
                if(!std::string_view(error.what()).starts_with("Attached effect graph could not preload every authored particle system"))continue;
            }
            if(complete)continue;
            const auto before=effects.diagnostics().effects;
            require(!effects.try_preload_attached(tag.datum),"Partial attached graph lost its stricter admission policy");
            effects.spawn(tag.datum,0x11110001u,0,frame);
            require(effects.diagnostics().effects==before+1,"Attached rejection suppressed valid raw partial presentation");
            partial=tag.datum;break;
        }
        require(partial,"Actual partial world-graph control is missing");report["raw_partial_graph_preserved_after_attached_rejection"]=true;
        bool strict=false;try{effects.preload_attached(absent);}catch(const ParticleResourceError&){strict=true;}
        require(strict,"Strict world resource preload lost its typed content rejection");
        bool native=false;try{effects.spawn_source(absent,0,0,{});}catch(const ParticleResourceError&){throw FormatError("Native source location invariant was classified as content");}
        catch(const FormatError&){native=true;}
        require(native,"Missing native source location owner was swallowed");
        SourceAttachedEffect bad_source;bad_source.definition=absent;
        native=false;try{effects.spawn_looping_attached(absent,0x22220002u,0,"",1,1,&bad_source,&markers);}
        catch(const ParticleResourceError&){throw FormatError("Native source identity invariant was classified as content");}
        catch(const FormatError&){native=true;}
        require(native,"Rejected content bypassed source attachment identity validation");
        for(unsigned n=0;n<70;++n)effects.spawn(0xabcdfff0u-n,0x22220002u,0,frame);
        unsigned bounded{};for(const auto& message:effects.diagnostics().unavailable)
            if(message.starts_with("World visual resource ") || message.starts_with("Additional unavailable world visual resources"))++bounded;
        require(bounded<=65,"Unavailable world resource diagnostics exceeded their bounded report bank");
        for(auto n=effects.diagnostics().effects;n<256;++n)effects.spawn(valid->datum,0x11110001u,0,frame);
        native=false;try{effects.spawn(valid->datum,0x11110001u,0,frame);}
        catch(const ParticleResourceError&){throw FormatError("Live world effect pool exhaustion was classified as content");}
        catch(const FormatError&){native=true;}
        require(native,"Live world effect pool exhaustion was swallowed");
        effects.spawn(absent,0x22220002u,0,frame);
        require(effects.diagnostics().effects==256 && std::array{primary,auxiliary}==random,
            "Rejected content touched the full live pool or consumed RNG");
        report["passed"]=true;report["valid_effects_preserved"]=initial.effects;report["content_diagnostic_once"]=true;
        report["rng_unchanged"]=true;report["native_invariants_strict"]=true;report["bounded_resource_diagnostics"]=bounded;save();
        std::cout<<"PASS actual world content quarantine, retained valid effect, unchanged RNG and strict native owners\n";return 0;
    }catch(const std::exception& error){report["error"]=error.what();save();std::cerr<<error.what()<<'\n';return 1;}
}
