#include "halo2/source_sandbox_special_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include "halo2/death_cycle_runtime_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/physics_phantom_world_tu15.hpp"
#include "halo2/source_damage_delivery_runtime.hpp"
#include "halo2/source_sound_cache_io_tu15.hpp"
#include "halo2/source_sound_cache_query_tu15.hpp"
#include "halo2/source_sound_scalar_tu15.hpp"
#include "halo2/source_sound_listener_selection_tu15.hpp"
#include "halo2/source_sound_gain_tu15.hpp"
#include "halo2/source_sound_random_scalar_tu15.hpp"
#include "halo2/source_sound_properties_tu15.hpp"
#include "halo2/source_sound_random_short_tu15.hpp"
#include "halo2/source_sound_range_selection_tu15.hpp"
#include "halo2/source_sound_permutation_tu15.hpp"
#include "halo2/source_sound_binding_tu15.hpp"
#include "halo2/source_sound_chunk_readiness_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/source_map_world_support_tu15.hpp"
#include "halo2/source_campaign_audio_tu15.hpp"
#include "halo2/source_runtime_resources.hpp"
#include "halo2/cache.hpp"
#include "physics_live_entries_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
using J=nlohmann::json;
using namespace halo2;
namespace {
struct ObserverDomainScope {
    bool &domain;
    bool previous;
    ObserverDomainScope(bool &value,bool next):domain(value),previous(value){domain=next;}
    ~ObserverDomainScope(){domain=previous;}
};
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string out;out.reserve(bytes.size()*2);
    for(auto byte:bytes){const auto value=std::to_integer<unsigned>(byte);out+=digits[value>>4];out+=digits[value&15];}
    return out;
}
struct Memory final:SourcePhysicsContactMemory {
    std::vector<std::byte> data;std::uint32_t cr3,cr0,cr4,instruction{};J writes=J::array();
    Memory(const char* path,const J& cpu):cr3(cpu["control_registers"]["CR3"]),cr0(cpu["control_registers"]["CR0"]),cr4(cpu["control_registers"]["CR4"]) {
        std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)throw std::runtime_error("Original RAM input missing");
        data.resize(std::size_t(file.tellg()));file.seekg(0);file.read(reinterpret_cast<char*>(data.data()),std::streamsize(data.size()));
        if(!file || !(cr0&0x80000000u) || cr4&0x20u)throw std::runtime_error("Original RAM/page mode invalid");
    }
    std::uint32_t word(std::uint32_t p)const {
        if(std::size_t(p)+4>data.size())throw std::runtime_error("Original page table outside retained RAM");
        std::uint32_t value;std::memcpy(&value,data.data()+p,4);return value;
    }
    std::pair<std::uint32_t,bool> physical(std::uint32_t address)const {
        const auto directory=word((cr3&0xfffff000u)+(address>>22)*4u);
        if(!(directory&1u))throw std::runtime_error("Original virtual PDE absent");
        std::uint32_t p;bool writable;
        if(directory&128u){if(!(cr4&16u))throw std::runtime_error("Original large page without PSE");p=(directory&0xffc00000u)+(address&0x3fffffu);writable=directory&2u;}
        else{const auto table=word((directory&0xfffff000u)+((address>>12)&1023u)*4u);if(!(table&1u))throw std::runtime_error("Original virtual PTE absent");p=(table&0xfffff000u)+(address&4095u);writable=directory&table&2u;}
        if(p>=data.size())throw std::runtime_error("Original virtual operand addresses non-RAM backing");
        return {p,writable};
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        while(!out.empty()){const auto [p,w]=physical(address);const auto n=std::min<std::size_t>(out.size(),4096u-(address&4095u));if(p+n>data.size())throw std::runtime_error("RAM read overrun");std::memcpy(out.data(),data.data()+p,n);address+=std::uint32_t(n);out=out.subspan(n);}
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        while(!in.empty()){const auto [p,w]=physical(address);if(!w&&(cr0&0x10000u))throw std::runtime_error("Original virtual operand is write protected");const auto n=std::min<std::size_t>(in.size(),4096u-(address&4095u));if(p+n>data.size())throw std::runtime_error("RAM write overrun");writes.push_back({{"instruction",instruction},{"address",address},{"before",hex(std::span(data).subspan(p,n))},{"written",hex(in.first(n))}});std::memcpy(data.data()+p,in.data(),n);address+=std::uint32_t(n);in=in.subspan(n);}
    }
};
}
int main(int argc,char** argv) {
    try {
        if(argc!=6 && argc!=8)throw std::runtime_error("Supply normalized CPU, original RAM, native report, native RAM output and actual entry [MAP SHARED]");
        const auto entry=std::uint32_t(std::stoul(argv[5],nullptr,0));
        if(entry!=0x168dedu && entry!=0x168ca1u && entry!=0x1030f0u)throw std::runtime_error("Original live entry not admitted");
        J initial;std::ifstream(argv[1])>>initial;Memory memory(argv[2],initial);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=initial["gp"];abi.xmm=initial["xmm"];abi.eflags=initial["eflags"];abi.fs_base=initial["fs_base"];abi.x87_status=initial["x87_status"];
        if(initial["x87_tag"]!=65535)throw std::runtime_error("Live original x87 stack requires a measured raw ABI owner");
        SourcePhysicsFactoryFloatingPoint fp{initial["x87_control"].get<std::uint16_t>(),initial["mxcsr"].get<std::uint32_t>()};abi.floating_point=fp;
        J instructions=J::array(),writes=J::array(),calls=J::array(),estimates=J::array();std::uint32_t last=0;
        bool damage_observer_domain=false,prefix_stop_reached=false;
        J canonical_damage_ranges=J::array();
        std::uint32_t prefix_stop_target=0;
        if(const auto text=std::getenv("HALO2_WEAPON_DIAGNOSTIC_STOP_TARGET")) {
            const auto requested=std::stoull(text,nullptr,0);
            if(!requested || requested>UINT32_MAX)throw std::runtime_error("Invalid declared weapon-prefix target");
            prefix_stop_target=std::uint32_t(requested);
        }
        std::unique_ptr<Cache> cache;
        std::unique_ptr<SourceRuntimeResources> resources;
        std::map<std::uint32_t,std::array<std::uint32_t,3>> resource_receipts;
        std::uint64_t next_receipt=0xf8000001u;
        J resource_reads=J::array();
        if(argc==8) {
            cache=std::make_unique<Cache>(argv[6],argv[7]);
            resources=std::make_unique<SourceRuntimeResources>(*cache);
        }
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){
            if(damage_observer_domain) {
                const auto index=instructions.size();
                if(canonical_damage_ranges.empty() || canonical_damage_ranges.back()["end"]!=index)
                    canonical_damage_ranges.push_back({{"begin",index},{"end",index}});
                canonical_damage_ranges.back()["end"]=index+1;
            }
            memory.instruction=last=pc;instructions.push_back(pc);
        };
        services.reciprocal_sqrt=[&](auto word){const auto value=source_host_x86_rsqrtss_estimate(word);estimates.push_back({{"instruction",last},{"input",word},{"output",value}});return value;};
        services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observe) {
            // External owners use actual TU1.5 PCs unless their adapter below
            // explicitly selects the authenticated canonical damage domain.
            ObserverDomainScope external_domain(damage_observer_domain,false);
            calls.push_back({{"instruction",call.instruction},{"target",call.target},{"tail",call.tail}});
            if(prefix_stop_target && call.target==prefix_stop_target) {
                prefix_stop_reached=true;
                throw std::runtime_error("Declared weapon-prefix stop before required callee");
            }
            if(resources && call.instruction==0x21f4bdu) {
                const SourceSoundCacheIoTu15Services io{
                    [&](auto encoded,auto size) {
                        auto bytes=resources->load(encoded,size);
                        resource_reads.push_back({{"encoded",encoded},{"bytes",size},{"actual_bytes",bytes.size()}});
                        return bytes;
                    },
                    [&](auto descriptor,auto destination,auto size) {
                        if(next_receipt>UINT32_MAX)throw std::runtime_error("Native resource receipt namespace exhausted");
                        const auto receipt=std::uint32_t(next_receipt++);
                        if(!resource_receipts.emplace(receipt,std::array{descriptor,destination,size}).second)
                            throw std::runtime_error("Native resource receipt identity reused");
                        return receipt;
                    }};
                if(source_try_sound_cache_io_tu15(bank,call,state,io))return;
            }
            if(source_sandbox_special_tu15_owns_entry(call.target)){source_run_sandbox_special_tu15(bank,call.target,state,fp,services,observe);return;}
            if(source_sound_cache_query_tu15_owns_entry(call.target)){source_run_sound_cache_query_tu15(bank,call.target,state,fp,services,observe);return;}
            if(call.target==0x127b20u){source_run_sound_scalar_tu15(bank,call.target,state,fp,services,observe);return;}
            if(call.target==0x12a810u || call.target==0x12a930u){source_run_sound_listener_selection_tu15(bank,call.target,state,fp,services,observe);return;}
            if(call.target==0x127cf0u){source_run_sound_gain_tu15(bank,call.target,state,fp,services,observe);return;}
            if(call.target==0x127d70u){source_run_sound_random_scalar_tu15(bank,call.target,state,fp,services,observe);return;}
            if(call.target==0x2290c0u){source_run_sound_properties_tu15(bank,call.target,state,fp,services,observe);return;}
            if(call.target==0x127e00u){source_run_sound_random_short_tu15(bank,call.target,state,fp,services,observe);return;}
            if(call.target==0x21f960u){source_run_sound_range_selection_tu15(bank,call.target,state,fp,services,observe);return;}
            if(call.target==0x21fa80u){source_run_sound_permutation_tu15(bank,call.target,state,fp,services,observe);return;}
            if(call.target==0x12b1c0u){source_run_sound_binding_tu15(bank,call.target,state,fp,services,observe);return;}
            if(call.target==0x128970u || call.target==0x1293f0u || call.target==0x21f3b0u){source_run_sound_chunk_readiness_tu15(bank,call.target,state,fp,services,observe);return;}
            // Match the live world's existing data-array handle lookup owner.
            if(call.target==0x16e940u){source_run_effect_particle_frame_tu15(bank,call.target,state,fp,services,observe);return;}
            if(source_campaign_audio_tu15_owns_entry(call.target)){source_run_campaign_audio_tu15(bank,call.target,state,fp,services,observe);return;}
            if(source_combat_action_tu15_owns_entry(call.target)){source_run_combat_action_tu15(bank,call.target,state,fp,services,observe);return;}
            if(source_death_cycle_runtime_tu15_owns_entry(call.target)){source_run_death_cycle_runtime_tu15(bank,call.target,state,fp,services,observe);return;}
            if(source_scene_device_constructor_tu15_owns_entry(call.target)){source_run_scene_device_constructor_tu15(bank,call.target,state,fp,services,observe);return;}
            if(source_map_world_support_tu15_owns_entry(call.target)){source_run_map_world_support_tu15(bank,call.target,state,fp,services,observe);return;}
            if(source_physics_live_support_tu15_owns_entry(call.target)){source_run_physics_live_support_tu15(bank,call.target,state,fp,services,observe);return;}
            for(const auto& entry:physics_live_entries_tu15::entries)if(entry.actual==call.target) {
                using Module=physics_live_entries_tu15::Module;
                if(entry.module==Module::Damage){ObserverDomainScope canonical_domain(damage_observer_domain,true);source_run_damage_delivery_runtime(bank,entry.canonical,state,SourcePhysicsContactImage::TitleUpdate15,fp,services,observe);return;}
                if(entry.module==Module::World){source_run_physics_world_runtime_selected_observations(bank,entry.canonical,state,SourcePhysicsContactImage::TitleUpdate15,fp,services,observe);return;}
                if(entry.module==Module::Phantom){source_run_physics_phantom_world_tu15_runtime(bank,entry.canonical,state,fp,services,observe);return;}
            }
            std::ostringstream message;message<<"Measured pose callee owner missing "<<std::hex<<call.instruction<<" -> "<<call.target;throw std::runtime_error(message.str());
        };
        std::string failure;bool returned=false;
        try {
            const SourcePhysicsContactRecordObserver observe=[&](const auto& store) {
                writes.push_back({{"instruction",store.instruction},{"address",store.address},{"before",hex(store.before)},{"written",hex(store.written)}});
            };
            if(entry==0x1030f0u)source_run_death_cycle_runtime_tu15(memory,entry,abi,fp,services,observe);
            else source_run_sandbox_special_tu15(memory,entry,abi,fp,services,observe);
            returned=true;
        }catch(const std::exception& error){failure=error.what();}
        J cpu={{"gp",abi.gp},{"xmm",abi.xmm},{"eflags",abi.eflags},{"fs_base",abi.fs_base},{"x87_status",abi.x87_status},{"x87_depth",abi.x87_depth},{"active_mmx",abi.active_mmx.has_value()}};
        if(abi.floating_point){cpu["x87_control"]=abi.floating_point->x87_control_word;cpu["mxcsr"]=abi.floating_point->mxcsr;}
        J report={{"returned",returned},{"error",failure},{"last_instruction",last},{"CPU",cpu},{"instructions",instructions},{"writes",writes},{"memory_writes",memory.writes},{"external_calls",calls},{"actual_host_CPU_estimates",estimates},{"supplied_gameplay_returns",J::array()}};
        report["actual_resource_reads"]=resource_reads;
        report["canonical_damage_instruction_ranges"]=canonical_damage_ranges;
        report["declared_prefix_stop_target"]=prefix_stop_target;
        report["declared_prefix_stop_reached"]=prefix_stop_reached;
        report["native_completed_resource_receipts"]=J::array();
        for(const auto& [id,fields]:resource_receipts)
            report["native_completed_resource_receipts"].push_back({{"identity",id},{"descriptor",fields[0]},{"destination",fields[1]},{"bytes",fields[2]}});
        report["synchronous_native_resource_policy"]=bool(resources);
        std::ofstream(argv[3])<<report.dump()<<'\n';
        std::ofstream ram(argv[4],std::ios::binary);ram.write(reinterpret_cast<const char*>(memory.data.data()),std::streamsize(memory.data.size()));if(!ram)throw std::runtime_error("Native RAM output failed");
        std::cout<<J{{"returned",returned},{"instructions",instructions.size()},{"writes",writes.size()},{"error",failure}}.dump()<<'\n';
        return returned?0:2;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
