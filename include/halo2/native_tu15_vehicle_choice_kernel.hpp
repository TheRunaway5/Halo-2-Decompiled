#pragma once
#include "halo2/native_tu15_node_resource_kernel.hpp"
#include <bit>

namespace halo2 {
enum class NativeTu15VehicleChoiceFamily {light,heavy,turret,flying};
struct NativeTu15VehicleChoiceLayout {
    std::uint32_t entry,dispatch,table,none,finish,random,draw,ret;
    unsigned count,fixed_count,random_range;
    std::array<std::uint32_t,5> fixed;
    std::array<unsigned,5> reference;
    bool early_choice;
};
constexpr NativeTu15VehicleChoiceLayout native_tu15_vehicle_choice_layout(NativeTu15VehicleChoiceFamily family) {
    using F=NativeTu15VehicleChoiceFamily;
    switch(family) {
    case F::light:return {0x15cb60,0x15cb9b,0x15cc50,0x15cc45,0x15cc48,0x15cc0b,0x15cc10,0x15cc3f,
        7,5,5,{0x15cba2,0x15cbb7,0x15cbcc,0x15cbe1,0x15cbf6},{0,0,1,6,5},false};
    case F::heavy:return {0x15cc70,0x15cca7,0x15cd1c,0x15cd14,0x15cd17,0x15ccd8,0x15cce0,0x15cd0e,
        4,2,2,{0x15ccae,0x15ccc3},{2,3},true};
    case F::turret:return {0x15cdf0,0x15ce2b,0x15ced0,0x15cec5,0x15cec8,0x15ce86,0x15ce90,0x15cebf,
        6,4,4,{0x15ce32,0x15ce47,0x15ce5c,0x15ce71},{7,9,8,10},false};
    case F::flying:return {0x15cd30,0x15cd67,0x15cddc,0x15cdd4,0x15cdd7,0x15cd98,0x15cda0,0x15cdce,
        4,2,2,{0x15cd6e,0x15cd83},{4,4},true};
    }
    throw std::runtime_error("Unknown TU1.5 vehicle-choice family");
}
enum class NativeTu15VehicleChoicePhase {
    save_object,context,owner,rows,definition,runtime,choice,save_output,output,selected,table,
    references,symbol_sink,reference,restore_output,publish_symbol,restore_object,fixed_return,
    no_choice,finish,save_random,random_argument,seed_owner,seed,update_seed,reselect,retry,random_return
};
struct NativeTu15VehicleChoice {
    NativeTu15VehicleChoiceFamily family;
    NativeTu15VehicleChoiceLayout layout;
    std::uint32_t object{},context{},owner{},rows{},definition{},runtime{},choice{},output{},selected{},target{},
        references{},symbol_sink{},value{},symbol{},seed_owner{},seed{},next_seed{},random_choice{};
    bool none{};
};
// Select an authored runtime reference. The signed byte uses the source's
// decremented selector namespace: explicit NONE and out-of-range choices have
// different return values. Random choices consume the real wrapping LCG seed
// and retry the same selector until its actual reference is present. Caller
// argument views stay lazy because recursive frames can alias those words.
template<class Memory,class Owners,class Progress>void native_tu15_vehicle_choice(
    const Memory&memory,NativeTu15VehicleChoiceFamily family,std::uint32_t object,Owners&owners,Progress progress) {
    using P=NativeTu15VehicleChoicePhase;using F=NativeTu15VehicleChoiceFamily;
    NativeTu15VehicleChoice data{family,native_tu15_vehicle_choice_layout(family)};data.object=object;
    const auto&p=data.layout;
    const auto acquire=[&](P phase,std::uint32_t address,unsigned width=4){progress(phase,data);return native_node_resource_value(memory,address,width);};
    const auto choice=[&]{progress(P::choice,data);data.choice=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(owners.choice()))));};
    progress(P::save_object,data);
    data.context=acquire(P::context,0x4ecc1cu);
    data.owner=acquire(P::owner,data.context+0x16cu);
    data.rows=acquire(P::rows,0x4f0414u);
    data.definition=acquire(P::definition,data.rows+(data.owner&0xffffu)*16u+8u);
    if(p.early_choice)choice();
    data.runtime=acquire(P::runtime,data.definition+12u);
    if(!p.early_choice)choice();
    progress(P::save_output,data);progress(P::output,data);data.output=owners.output();
    data.selected=data.choice-1u;progress(P::selected,data);
    if(data.selected>=p.count){progress(P::finish,data);return;}
    data.target=acquire(P::table,p.table+data.selected*4u);
    const auto expected=data.selected<p.fixed_count?p.fixed[data.selected]:data.selected==p.fixed_count?p.random:p.none;
    if(data.target!=expected)throw std::runtime_error("Vehicle selector lost its authenticated jump-table owner");
    if(data.selected<p.fixed_count) {
        data.references=acquire(P::references,data.runtime+0x74u);
        progress(P::symbol_sink,data);data.symbol_sink=owners.symbol_sink();
        data.value=acquire(P::reference,data.references+p.reference[data.selected]*8u+4u);
        progress(P::restore_output,data);
        data.symbol=family==F::heavy?0x0b00078bu:family==F::light&&data.selected==1u?0x05000694u:0u;
        progress(P::publish_symbol,data);owners.publish_symbol(data);
        progress(P::restore_object,data);progress(P::fixed_return,data);return;
    }
    if(data.selected==p.fixed_count) {
        progress(P::save_random,data);progress(P::random_argument,data);owners.random_argument();
        do {
            data.seed_owner=acquire(P::seed_owner,0x4f3cd8u);
            data.seed=acquire(P::seed,data.seed_owner);
            data.next_seed=data.seed*0x19660du+0x3c6ef35fu;
            progress(P::update_seed,data);owners.publish_seed(data);
            data.random_choice=(((data.next_seed>>16u)*p.random_range)>>16u)+1u;
            progress(P::reselect,data);owners.reselect(data);
            progress(P::retry,data);
        }while(owners.result()==0xffffffffu);
        progress(P::random_return,data);return;
    }
    data.none=true;progress(P::no_choice,data);progress(P::finish,data);
}
} // namespace halo2
