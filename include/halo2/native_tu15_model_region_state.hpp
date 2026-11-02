#pragma once
#include <cstdint>

namespace halo2 {
struct NativeTu15ModelRegionStateComparison {
    std::int32_t desired,current;
};
class NativeTu15ModelRegionStateStorage {
public:
    virtual ~NativeTu15ModelRegionStateStorage()=default;
    virtual std::int32_t begin()=0;
    virtual void enter_regions()=0;
    virtual std::uint32_t requested_state()=0;
    virtual void retain_current_state()=0;
    virtual void use_requested_state()=0;
    virtual std::uint32_t property_bit()=0;
    virtual bool property_region_selected()=0;
    virtual void set_property_bit()=0;
    virtual void clear_property_bit()=0;
    virtual NativeTu15ModelRegionStateComparison state_comparison()=0;
    virtual std::uint8_t allow_lower_state()=0;
    virtual std::uint32_t region_filter()=0;
    virtual bool region_matches_filter()=0;
    virtual std::uint32_t model_reference()=0;
    virtual std::uint32_t variant_index()=0;
    virtual std::uint32_t mapping_region_index()=0;
    virtual void lookup_mapping()=0;
    virtual std::int32_t mapping_and_clear_candidates()=0;
    virtual void collect_candidates()=0;
    virtual std::uint32_t select_candidate()=0;
    virtual NativeTu15ModelRegionStateComparison write_permutations()=0;
    virtual std::uint32_t old_effect()=0;
    virtual void stop_effect()=0;
    virtual std::uint32_t new_effect_tag()=0;
    virtual void prepare_effect()=0;
    virtual void create_effect()=0;
    virtual void commit_state()=0;
    virtual void restore_model_index()=0;
    virtual bool advance_region()=0;
    virtual void finish(bool entered_regions)=0;
};

// Complete 1704C0 control work. Candidate collection/selection and effect
// operations consume their own live banks and genuine callee ABI. The count
// is reread after every region; force, filter and property decisions preserve
// the original short circuits. A selected record remains live across effects.
inline void native_tu15_model_region_state(NativeTu15ModelRegionStateStorage& storage) {
    const bool entered=storage.begin()>0;
    if(entered){
        storage.enter_regions();
        do {
            if(storage.requested_state()==0xffffffffu)storage.retain_current_state();
            else storage.use_requested_state();
            if(storage.property_bit()!=0xffffffffu){
                if(storage.property_region_selected())storage.set_property_bit();
                else storage.clear_property_bit();
            }
            const auto state=storage.state_comparison();
            if(state.desired<state.current && !storage.allow_lower_state())continue;
            if(storage.region_filter()!=0xffffffffu && !storage.region_matches_filter())continue;
            if(storage.model_reference()!=0xffffffffu && storage.variant_index()!=0xffffffffu &&
               storage.mapping_region_index()!=0xffffffffu)storage.lookup_mapping();
            if(storage.mapping_and_clear_candidates()!=-1)storage.collect_candidates();
            if(storage.select_candidate()!=0xffffffffu){
                const auto chosen=storage.write_permutations();
                if(chosen.current!=chosen.desired){
                    if(storage.old_effect()!=0xffffffffu)storage.stop_effect();
                    if(storage.new_effect_tag()!=0xffffffffu){storage.prepare_effect();storage.create_effect();}
                }
                storage.commit_state();
            }
            storage.restore_model_index();
        } while(storage.advance_region());
    }
    storage.finish(entered);
}
}
