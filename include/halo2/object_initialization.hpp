#pragma once
#include "halo2/content.hpp"
#include "halo2/model_selection.hpp"
#include <functional>
#include <map>
#include <span>

namespace halo2 {
struct SourceScenarioObjectIdentity {
    std::uint32_t unique_id=0xffffffffu;
    int type{},placement=-1,source=2,origin_bsp=-1;
};
struct SourceScenarioCreationKey {
    int type{},placement{};
    bool operator==(const SourceScenarioCreationKey&)const=default;
};
// OriginalD5A70 andD59F0. These are construction attempts, beforeBF0F0's
// name/resource/multiplayer factory admission and allocation result.
// D5060's factory gate, before resource/authority/allocation admission.
// Explicit game-options inputs: type2 means multiplayer; engines are retail
// values (CTF1, Slayer2). Specific SCEN masks replace the flag64 predicate.
bool source_scenario_factory_game_allowed(const ScenarioObject&,int game_type,int engine);
int source_scenario_creation_mode(const ScenarioObject&);
bool source_scenario_initial_creation_candidate(const ScenarioObject&);
std::vector<SourceScenarioCreationKey> source_scenario_initial_creation_attempts(
    std::span<const ScenarioObject>,bool suppress_existing_identifiers,
    std::span<const SourceScenarioObjectIdentity> existing={});
std::vector<SourceScenarioCreationKey> source_scenario_bsp_creation_attempts(
    std::span<const ScenarioObject>,int active_bsp,
    std::span<const SourceScenarioObjectIdentity> existing={});

struct SourceObjectConstructorMetadata {
    int type=-1;
    std::uint32_t animation=0xffffffffu,creation_effect=0xffffffffu;
    std::uint32_t attachment_count{},widget_count{},function_count{},change_color_count{};
    std::vector<std::uint32_t> variant_child_counts;
    // ColdSCEN B7B40/10A1B0 beforeB6BB0 registration. Other constructor
    // services cannot be silently assumed to consume zero primary RNG.
    bool has_verified_cold_scenery_rng_path()const;
};
enum class SourceObjectConstructorStage {before_model,after_model};
struct SourceGameRandomInitializationServices {
    // Other137CA0 initializers on either side of11BE20. Returnfalse if their
    // complete primary consumption is not supplied/proven for this context.
    std::function<bool(std::uint32_t&)> before_noise,after_noise;
};
struct SourceObjectConstructorRandomServices {
    // Originalgraph/type initialization precedesBA590; variants' child/effect
    // constructors follow it. Use the same primary block as HS/weapon code.
    std::function<bool(const Cache&,const ScenarioObject&,
        SourceObjectConstructorStage,std::uint32_t&)> apply;
};
struct SourceInitializedObjectModel {
    SourceScenarioCreationKey key;
    std::uint32_t handle=0xffffffffu,definition=0xffffffffu;
    std::int32_t created_tick{};
    SourceInitialModelSelection selection;
    bool seed_before_known{},constructor_consumption_known{};
    std::uint32_t seed_before{},seed_after{};
    std::uint64_t creation_sequence{};
};
// Owns the construction ledger, not a second random stream. The caller's
// actual primary RNG block remains shared with other original consumers.
class SourceObjectInitialization {
public:
    explicit SourceObjectInitialization(std::uint32_t& primary):primary_(&primary){}
    void initialize_game(std::uint32_t actual_launch_seed,
                         const SourceGameRandomInitializationServices&);
    const SourceInitializedObjectModel& create(const Cache&,const ScenarioObject&,
        std::uint32_t full_handle,std::int32_t created_tick,bool cold_scenery_phase,
        const SourceObjectConstructorRandomServices& services={});
    void destroy(std::uint32_t full_handle);
    const SourceInitializedObjectModel* find(std::uint32_t full_handle)const;
    const SourceInitializedObjectModel* find(SourceScenarioCreationKey)const;
    bool primary_known()const{return primary_known_;}
    void invalidate_primary(){primary_known_=false;}
private:
    std::uint32_t* primary_;
    bool primary_known_{};
    std::uint64_t sequence_{};
    std::map<std::uint32_t,SourceInitializedObjectModel> objects_;
};
// Original211CC0 always consumes24 primary draws, and leaves the auxiliary
// block unchanged. This does not execute the other137CA0 initializers.
void source_initialize_object_noise_random(std::uint32_t& primary);
}
