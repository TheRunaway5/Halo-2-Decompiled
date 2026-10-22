#include "halo2/source_campaign_ai_object_decisions_tu15.hpp"
#include "halo2/source_campaign_ai_object_callbacks_tu15.hpp"
#include "halo2/source_campaign_birth_query_variants_tu15.hpp"
#include "halo2/source_campaign_ai_actor_behavior_tu15.hpp"
#include "halo2/source_damage_delivery_runtime.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/native_tu15_physics_branch_data.hpp"
#include "halo2/source_final_startup_storage_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace halo2 {
void source_run_campaign_ai_bsp_cache_test_slice_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,const SourcePhysicsContactRecordObserver &);
void source_run_campaign_camera_effect_request_test_slice_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,const SourcePhysicsContactRecordObserver &);
void source_run_campaign_geometry_permutation_test_slice_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,const SourcePhysicsContactRecordObserver &);
void mount_campaign_existing_projection_test_slice_tu15(SourceRuntimeMemory &);
void source_run_campaign_behavior_iterator_test_slice_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,const SourcePhysicsContactRecordObserver &);
}

namespace {
using namespace halo2;
struct PrefixEnd {};
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> unhex(const std::string& text) {
    check(text.size()%2==0,"Odd decision reference bytes");std::vector<std::byte> result;
    for(unsigned i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(auto b:bytes){auto v=std::to_integer<unsigned>(b);result+=digits[v>>4];result+=digits[v&15];}
    return result;
}
void run_case(const nlohmann::json& fixture,const nlohmann::json& input) {
    SourceRuntimeMemory memory;
    for(const auto& bank:fixture.at("native_component_banks"))memory.mount(bank.at(0),
        std::vector<std::byte>(bank.at(1).get<unsigned>(),std::byte(input.at("poison").get<unsigned>())),
        SourceRuntimeMemory::Access::ReadWrite,"Declared original decision caller/pool/tag input");
    for(const auto& operand:fixture.at("immutable_operands"))memory.mount(operand.at(0),unhex(operand.at(1)),
        SourceRuntimeMemory::Access::ReadOnly,"Original immutable decision operand");
    for(const auto& operand:fixture.value("authored_operands",nlohmann::json::array()))
        memory.write(operand.at(0),unhex(operand.at(1)));
    for(const auto& patch:input.at("patches"))memory.write(patch.at(0),unhex(patch.at(1)));
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=input.at("gp").get<decltype(abi.gp)>();
    abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();abi.eflags=0x202;
    abi.x87_status=input.value("initial_x87_status",std::uint16_t{});
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    std::vector<unsigned> trace;nlohmann::json stores=nlohmann::json::array();
    SourcePhysicsWorldRuntimeServices services{};bool stopped=false;
    services.instruction=[&](unsigned pc){
        if(!input.at("prefix_end").is_null()&&pc==input.at("prefix_end").get<unsigned>())throw PrefixEnd{};
        if(trace.empty()||trace.back()!=pc)trace.push_back(pc);};
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observe){
        if(fixture.value("admit_original_behavior_iterator",false)&&call.target==0x278dd0u) {
            source_run_campaign_behavior_iterator_test_slice_tu15(bank,state,
                state.floating_point.value_or(*abi.floating_point),services,observe);return;
        }
        throw std::runtime_error("Decision supplied a dependency owner");};
    auto observer=[&](const auto& store){
        stores.push_back(nlohmann::json::array({store.instruction,store.address,hex(store.before),hex(store.written)}));};
    try{
        if(fixture.value("admit_original_behavior_iterator",false)&&input.at("entry")==0x278dd0u)
            source_run_campaign_behavior_iterator_test_slice_tu15(memory,abi,*abi.floating_point,services,observer);
        else if(fixture.value("executor",std::string{})=="geometry_permutation")
            source_run_campaign_geometry_permutation_test_slice_tu15(memory,input.at("entry"),abi,
                *abi.floating_point,services,observer);
        else if(fixture.value("executor",std::string{})=="damage")
            source_run_damage_delivery_runtime_selected_observations(memory,input.at("canonical_entry"),abi,
                SourcePhysicsContactImage::TitleUpdate15,*abi.floating_point,services,observer);
        else if(fixture.value("executor",std::string{})=="camera_effect_request")
            source_run_campaign_camera_effect_request_test_slice_tu15(memory,abi,*abi.floating_point,services,observer);
        else if(fixture.value("executor",std::string{})=="bsp_cache")
            source_run_campaign_ai_bsp_cache_test_slice_tu15(memory,abi,*abi.floating_point,services,observer);
        else if(fixture.value("executor",std::string{})=="frame")
            source_run_ai_frame_tu15(memory,input.at("entry"),abi,*abi.floating_point,services,observer);
        else if(source_campaign_ai_actor_behavior_tu15_owns_entry(input.at("entry")))
            source_run_campaign_ai_actor_behavior_tu15(memory,input.at("entry"),abi,*abi.floating_point,services,observer);
        else if(source_campaign_birth_query_variants_tu15_owns_entry(input.at("entry")))
            source_run_campaign_birth_query_variants_tu15(memory,input.at("entry"),abi,*abi.floating_point,services,observer);
        else if(source_campaign_ai_object_callbacks_tu15_owns_entry(input.at("entry")))
            source_run_campaign_ai_object_callbacks_tu15(memory,input.at("entry"),abi,*abi.floating_point,services,observer);
        else source_run_campaign_ai_object_decisions_tu15(memory,input.at("entry"),abi,*abi.floating_point,services,observer);
    }
    catch(const PrefixEnd&){stopped=true;}
    catch(const std::exception& error){throw std::runtime_error(std::string(error.what())+
        " entry="+std::to_string(input.at("entry").get<unsigned>())+
        " pc="+std::to_string(trace.empty()?0u:trace.back())+" spec="+input.dump().substr(0,220));}
    check(stopped==!input.at("prefix_end").is_null(),"Decision whole/HS-prefix boundary differs");
    auto require=[&](bool value,const char* what){if(!value)throw std::runtime_error(std::string(what)+
        " entry="+std::to_string(input.at("entry").get<unsigned>())+" spec="+input.dump().substr(0,220));};
    require(abi.gp==input.at("final_gp").get<decltype(abi.gp)>(),"Decision complete GPR state differs");
    require(abi.xmm==input.at("final_xmm").get<decltype(abi.xmm)>(),"Decision complete SIMD state differs");
    require((abi.eflags&~0x10u)==(input.at("final_flags").get<unsigned>()&~0x10u),"Decision modeled flags differ");
    require(trace==input.at("trace").get<std::vector<unsigned>>(),"Decision instruction order differs");
    require(stores==input.at("stores"),"Decision ordered writes or previous bytes differ");
    require(abi.x87_depth==input.value("x87_depth",0u),"Decision x87 depth differs");
    if(input.contains("final_x87_status"))
        require(abi.x87_status==input.at("final_x87_status").get<std::uint16_t>(),"Original x87 status capture/clear differs");
    if(input.contains("final_mxcsr"))
        require(abi.floating_point&&abi.floating_point->mxcsr==input.at("final_mxcsr").get<unsigned>(),"Original camera request MXCSR differs");
    for(const auto& output:input.at("outputs")) {
        const auto expected=unhex(output.at(1));
        require(hex(memory.view(output.at(0),expected.size()))==hex(expected),"Decision source publication, RNG or preserved padding differs");
    }
    const auto saved=memory.checkpoint();
    for(const auto& output:input.at("outputs"))memory.write(output.at(0),
        std::vector<std::byte>(unhex(output.at(1)).size(),std::byte{0x42}));
    memory.restore(saved);
    for(const auto& output:input.at("outputs")) {
        const auto expected=unhex(output.at(1));
        require(hex(memory.view(output.at(0),expected.size()))==hex(expected),"Decision checkpoint lost produced state");
    }
}
void mounted_operands() {
    SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
    for(const auto& [address,bits]:std::array<std::pair<unsigned,const char*>,5>{{
        {0x4500d0,"00006040"},{0x46a0d0,"0000803f"},{0x46a0e8,"00000000"},
        {0x46a110,"00002041"},{0x46a6d8,"00005841"}}})
        check(hex(memory.view(address,4))==bits&&memory.is_read_only(address,4),"Mounted original decision operand differs");
    for(const auto& [address,size]:std::array<std::pair<unsigned,unsigned>,6>{{
        {0x50208c,24},{0x52be54,4},{0x52be58,1},{0x50eee4,8},{0x502200,64},{0x502240,2}}})
        check(std::ranges::all_of(memory.view(address,size),[](auto b){return b==std::byte{0};})&&
            !memory.is_read_only(address,size),"AI lease declaration differs from original loader-zero mutable storage");
    bool hole=false;try{(void)memory.view(0x52be59,1);}catch(const std::out_of_range&){hole=true;}
    check(hole,"AI lease latch borrowed adjacent unowned bytes");
}
std::array<std::byte,4> bytes(unsigned word) {
    return {std::byte(word),std::byte(word>>8),std::byte(word>>16),std::byte(word>>24)};
}
void run_leases(const nlohmann::json& fixture) {
    for(const auto& sequence:fixture.at("sequences")) {
        SourceRuntimeMemory memory;
        for(const auto& bank:fixture.at("banks"))memory.mount(bank.at(0),std::vector<std::byte>(bank.at(1).get<unsigned>()),
            SourceRuntimeMemory::Access::ReadWrite,"Declared original AI workspace caller/storage");
        for(const auto& row:sequence.at("initial"))memory.write(row.at(0),unhex(row.at(1)));
        std::vector<unsigned> leases;
        for(const auto& input:sequence.at("cases")) {
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp=input.at("gp").get<decltype(abi.gp)>();
            if(input.at("operation")=="release")abi.gp[2]=leases.at(input.at("lease_index"));
            abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();abi.eflags=0x202;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            memory.write(fixture.at("stack"),bytes(fixture.at("return_address")));
            std::vector<unsigned> trace;nlohmann::json stores=nlohmann::json::array();bool stopped=false;
            SourcePhysicsWorldRuntimeServices services{};
            services.instruction=[&](unsigned pc){
                if(!input.at("prefix_end").is_null()&&pc==input.at("prefix_end").get<unsigned>())throw PrefixEnd{};
                trace.push_back(pc);};
            services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Workspace supplied a lock service");};
            try{source_run_ai_frame_tu15(memory,input.at("entry"),abi,*abi.floating_point,services,[&](const auto& store){
                stores.push_back(nlohmann::json::array({store.instruction,store.address,hex(store.before),hex(store.written)}));});}
            catch(const PrefixEnd&){stopped=true;}
            check(stopped==!input.at("prefix_end").is_null(),"Workspace original lock boundary differs");
            check(abi.gp==input.at("final_gp").get<decltype(abi.gp)>(),"Workspace complete GPR/caller state differs");
            check(abi.xmm==input.at("final_xmm").get<decltype(abi.xmm)>(),"Workspace SIMD state differs");
            check((abi.eflags&~0x10u)==(input.at("final_flags").get<unsigned>()&~0x10u),"Workspace modeled flags differ");
            check(trace==input.at("trace").get<std::vector<unsigned>>(),"Workspace instruction lifecycle differs");
            check(stores==input.at("stores"),"Workspace ordered stores or prior padding differ");
            if(input.at("operation")=="acquire")leases.push_back(abi.gp[0]);
            for(const auto& output:input.at("outputs")) {
                const auto expected=unhex(output.at(1));
                check(hex(memory.view(output.at(0),expected.size()))==hex(expected),"Workspace publication/reset state differs");
            }
            const auto saved=memory.checkpoint();
            for(const auto& output:input.at("outputs"))memory.write(output.at(0),
                std::vector<std::byte>(unhex(output.at(1)).size(),std::byte{0x42}));
            memory.restore(saved);
            for(const auto& output:input.at("outputs")) {
                const auto expected=unhex(output.at(1));
                check(hex(memory.view(output.at(0),expected.size()))==hex(expected),"Checkpoint lost workspace occupancy, pointers or padding");
            }
        }
    }
}
void child_link_storage(const nlohmann::json& fixture) {
    const auto& fields=fixture.at("authored_heads");
    check(fields.size()==120,"Original child head family differs");
    SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
    for(const auto& field:fields) {
        const auto address=field.at("address").get<unsigned>(),row=field.at("row").get<unsigned>();
        check(address==row+12&&field.at("bytes")==4&&hex(memory.view(address,4))=="00000000",
            "Authored child head lacks exact loader-zero storage");
        memory.validate_mutable_view(address,4);
        for(const auto& [at,bits]:std::array<std::pair<unsigned,std::string>,2>{{
            {row,field.at("header_hex")},{row+8,field.at("timer_hex")}}}) {
            const auto expected=unhex(bits);
            for(unsigned n=0;n<expected.size();++n)
                check(memory.view(at+n,1)[0]==expected[n]&&memory.is_read_only(at+n,1),
                    "Mutable child head replaced authored header/timer metadata");
        }
        memory.write(address,bytes(address));
    }
    const auto saved=memory.checkpoint();
    for(const auto& field:fields)memory.write(field.at("address"),bytes(0));
    memory.restore(saved);
    for(const auto& field:fields)check(hex(memory.view(field.at("address"),4))==hex(bytes(field.at("address"))),
        "Checkpoint lost produced child command heads");
    for(unsigned at:{0x47be0eu,0x47be0fu,0x47be18u,0x47be19u,0x47be1au,0x47be1bu}) {
        bool missing=false;try{(void)memory.view(at,1);}catch(const std::out_of_range&){missing=true;}
        check(missing,"Child command head borrowed unread row padding or tail");
    }
    const auto first=fields.front().at("address").get<unsigned>(),last=fields.back().at("address").get<unsigned>();
    auto allocation=std::make_shared<std::array<std::byte,4>>(bytes(0x12345678));
    auto backing=std::make_shared<SourceRuntimeMemoryBacking>(allocation,std::span<std::byte>(*allocation));
    SourceRuntimeMemory shared;
    shared.mount(first,backing,SourceRuntimeMemory::Access::ReadWrite,"Existing original child head backing");
    shared.mount(last+1,std::array{std::byte{0xf1}},SourceRuntimeMemory::Access::ReadWrite,"Existing partial head");
    const auto native=shared.native_address(first,4),partial=shared.native_address(last+1,1);
    mount_native_tu15_final_startup_storage_data(shared);
    check(shared.has_backing(first,backing)&&shared.native_address(first,4)==native&&
        hex(shared.view(first,4))=="78563412"&&shared.native_address(last+1,1)==partial&&
        shared.view(last+1,1)[0]==std::byte{0xf1},"Child head publication replaced a prior mutable value/backing");
    for(unsigned n:{0u,2u,3u})check(shared.view(last+n,1)[0]==std::byte{0},"Missing partial head bytes differ");
    shared.write(first,bytes(0x87654321));
    check(*allocation==bytes(0x87654321),"Original child head lost its retained native backing");
    SourceRuntimeMemory rejected;
    rejected.mount(last+3,std::array{std::byte{0}},SourceRuntimeMemory::Access::ReadOnly,"Conflicting head access");
    bool conflict=false;
    try{mount_native_tu15_final_startup_storage_data(rejected);}catch(const std::invalid_argument&){conflict=true;}
    check(conflict,"Read-only original child head was accepted");
    for(const auto& field:fields)for(unsigned n=0;n<4;++n) {
        const auto at=field.at("address").get<unsigned>()+n;
        if(at==last+3)continue;
        bool missing=false;try{(void)rejected.view(at,1);}catch(const std::out_of_range&){missing=true;}
        check(missing,"Child head admission mutated topology before checking every existing owner");
    }
}
}
int main(int argc,char** argv)try {
    check(argc==33,"Expected original decision, lease, callback, operand, query, selector, type, actor-state, bucket, birth, complete query, behavior, script-context, Marine command, HS registry, shared property, actor scalar, BSP-cache, difficulty, camera-request, hardware-status, damage-operand, reaction, property-owner, creation, decision-workspace, geometry-permutation, child-command link, path-distance and descriptor27 references");
    nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        fixture.at("instruction_patches").empty()&&fixture.at("supplied_dependency_returns").empty(),"Decision original authority differs");
    nlohmann::json prop_owner;std::ifstream(argv[26])>>prop_owner;
    check(prop_owner.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
        prop_owner.at("instruction_patches").empty()&&prop_owner.at("supplied_dependency_returns").empty(),
        "Original property-owner lookup authority differs");
    nlohmann::json creation;std::ifstream(argv[27])>>creation;
    check(creation.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
        creation.at("instruction_patches").empty()&&creation.at("supplied_dependency_returns").empty(),
        "Original property-creation authority differs");
    for(const auto& input:creation.at("cases"))run_case(creation,input);
    SourceRuntimeMemory creation_memory;mount_native_tu15_final_startup_storage_data(creation_memory);
    for(const auto& field:creation.at("cold")) {
        const unsigned address=field.at(0),size=field.at(1);const auto data=creation_memory.view(address,size);
        check(std::ranges::all_of(data,[](auto b){return b==std::byte{0};}),"Property creation lacks original loader-zero storage");
        creation_memory.validate_mutable_view(address,size);
        creation_memory.write(address,std::vector<std::byte>(size,std::byte{0x5a}));
    }
    const auto saved_creation=creation_memory.checkpoint();
    for(const auto& field:creation.at("cold"))creation_memory.write(field.at(0),std::vector<std::byte>(field.at(1).get<unsigned>(),std::byte{0x42}));
    creation_memory.restore(saved_creation);
    for(const auto& field:creation.at("cold"))check(std::ranges::all_of(creation_memory.view(field.at(0),field.at(1)),
        [](auto b){return b==std::byte{0x5a};}),"Checkpoint lost property creation epoch/latch/stamps");
    for(unsigned address:{0x4eabc9u,0x4eabcau,0x4eabcbu}) {
        bool missing=false;try{(void)creation_memory.view(address,1);}catch(const std::out_of_range&){missing=true;}
        check(missing,"Property creation borrowed unowned alignment bytes");
    }
    SourceRuntimeMemory shared_creation;
    const std::array<std::byte,1> prior_latch{std::byte{0x7f}};
    shared_creation.mount(0x4eabc8,prior_latch,SourceRuntimeMemory::Access::ReadWrite,"Prior original creation latch");
    shared_creation.mount(0x4eabcc,bytes(0xffffffff),SourceRuntimeMemory::Access::ReadWrite,"Prior original creation epoch");
    shared_creation.mount(0x4ecbcc,bytes(0x12345678),SourceRuntimeMemory::Access::ReadWrite,"Prior final object stamp");
    const auto latch_native=shared_creation.native_address(0x4eabc8,1),epoch_native=shared_creation.native_address(0x4eabcc,4),
        stamp_native=shared_creation.native_address(0x4ecbcc,4);
    mount_native_tu15_final_startup_storage_data(shared_creation);
    check(hex(shared_creation.view(0x4eabc8,1))=="7f"&&hex(shared_creation.view(0x4eabcc,4))=="ffffffff"&&
        hex(shared_creation.view(0x4ecbcc,4))=="78563412"&&shared_creation.native_address(0x4eabc8,1).value==latch_native.value&&
        shared_creation.native_address(0x4eabcc,4).value==epoch_native.value&&shared_creation.native_address(0x4ecbcc,4).value==stamp_native.value,
        "Property creation replaced existing mutable values/backing lifetimes");
    SourceRuntimeMemory readonly_creation;readonly_creation.mount(0x4ebbd0,bytes(0),SourceRuntimeMemory::Access::ReadOnly,"Invalid read-only object stamp");
    bool creation_rejected=false;try{mount_native_tu15_final_startup_storage_data(readonly_creation);}catch(const std::invalid_argument&){creation_rejected=true;}
    check(creation_rejected,"Property creation silently accepted read-only stamp storage");
    bool creation_missing=false;try{(void)readonly_creation.view(0x4eabcc,1);}catch(const std::out_of_range&){creation_missing=true;}
    check(creation_missing,"Property creation admitted missing fields before validating prior owners");
    for(const auto& input:prop_owner.at("cases"))run_case(prop_owner,input);
    mounted_operands();
    nlohmann::json damage_operands;std::ifstream(argv[24])>>damage_operands;
    check(damage_operands.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
        damage_operands.at("instruction_patches").empty()&&damage_operands.at("supplied_dependency_returns").empty(),
        "Original damage operand authority differs");
    SourceRuntimeMemory damage_memory;mount_native_tu15_final_startup_storage_data(damage_memory);
    for(const auto& field:damage_operands.at("immutable_operands")) {
        const auto expected=unhex(field.at(1));std::vector<std::byte> actual(expected.size());
        damage_memory.read(field.at(0),actual);check(actual==expected,"Original fixed damage operand differs");
        for(unsigned i=0;i<actual.size();++i)check(damage_memory.is_read_only(field.at(0).get<unsigned>()+i,1),
            "Original fixed damage operand became writable");
    }
    nlohmann::json reaction;std::ifstream(argv[25])>>reaction;
    check(reaction.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
        reaction.at("instruction_patches").empty()&&reaction.at("supplied_dependency_returns").empty(),
        "Original damage reaction authority differs");
    // Actual11266F reads row1 of44AA00. Keep the specific missing backing
    // regression before checking the rest of the complete table family.
    std::array<std::byte,4> requested_reaction;damage_memory.read(0x44aa04,requested_reaction);
    for(const auto& field:reaction.at("immutable_operands")) {
        const auto expected=unhex(field.at(1));std::vector<std::byte> actual(expected.size());damage_memory.read(field.at(0),actual);
        check(actual==expected,"Original reaction table/operand differs");
        for(unsigned i=0;i<actual.size();++i)check(damage_memory.is_read_only(field.at(0).get<unsigned>()+i,1),
            "Original reaction table became writable");
    }
    for(const auto& field:reaction.at("authored_operands")) {
        const auto expected=unhex(field.at(1));check(hex(damage_memory.view(field.at(0),expected.size()))==hex(expected),
            "Authored reaction animation names differ");damage_memory.validate_mutable_view(field.at(0),expected.size());
        const auto saved=damage_memory.checkpoint();damage_memory.write(field.at(0),std::vector<std::byte>(expected.size(),std::byte{0x42}));
        damage_memory.restore(saved);check(hex(damage_memory.view(field.at(0),expected.size()))==hex(expected),
            "Reaction animation names lost checkpoint state");
        const unsigned address=field.at(0);SourceRuntimeMemory shared;
        shared.mount(address+8,std::span(expected).subspan(8,4),SourceRuntimeMemory::Access::ReadOnly,"Prior camera/death animation-name word");
        shared.mount(address+12,std::span(expected).subspan(12,4),SourceRuntimeMemory::Access::ReadWrite,"Prior writable animation-name word");
        const auto prior_readonly=shared.native_address(address+8,4),prior_writable=shared.native_address(address+12,4);
        mount_native_tu15_final_startup_storage_data(shared);std::vector<std::byte> actual(expected.size());shared.read(address,actual);
        check(actual==expected&&shared.is_read_only(address+8,4)&&!shared.is_read_only(address+12,4)&&!shared.is_read_only(address,8)&&
            shared.native_address(address+8,4).value==prior_readonly.value&&shared.native_address(address+12,4).value==prior_writable.value,
            "Reaction names replaced an existing owner/access/lifetime");
        SourceRuntimeMemory conflicting;conflicting.mount(address+8,bytes(0xdeadbeef),SourceRuntimeMemory::Access::ReadWrite,"Conflicting reaction name");
        bool rejected=false;try{mount_native_tu15_final_startup_storage_data(conflicting);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"Reaction names silently replaced conflicting metadata");
        bool missing=false;try{(void)conflicting.view(address,1);}catch(const std::out_of_range&){missing=true;}
        check(missing,"Reaction name storage admitted before checking prior owners");
    }
    for(const auto& input:reaction.at("cases"))run_case(reaction,input);
    for(const auto& input:fixture.at("cases"))run_case(fixture,input);
    nlohmann::json lease;std::ifstream(argv[2])>>lease;
    check(lease.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&lease.at("instruction_patches").empty()&&
        lease.at("supplied_dependency_returns").empty(),"Workspace original authority differs");
    run_leases(lease);
    nlohmann::json callbacks;std::ifstream(argv[3])>>callbacks;
    check(callbacks.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&callbacks.at("instruction_patches").empty()&&
        callbacks.at("supplied_dependency_returns").empty(),"Object callbacks original authority differs");
    SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
    for(const auto& field:callbacks.at("immutable_operands")) {
        const auto expected=unhex(field.at(1));
        check(hex(memory.view(field.at(0),expected.size()))==hex(expected)&&memory.is_read_only(field.at(0),expected.size()),
            "Original object callback mask/identity/terminator data or access differs");
    }
    for(const auto& input:callbacks.at("cases"))run_case(callbacks,input);
    nlohmann::json bodies;std::ifstream(argv[4])>>bodies;
    check(bodies.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&bodies.at("instruction_patches").empty()&&
        bodies.at("supplied_dependency_returns").empty(),"Callback bodies original authority differs");
    for(const auto& field:bodies.at("immutable_operands")) {
        const auto expected=unhex(field.at(1));std::vector<std::byte> actual(expected.size());
        const unsigned address=field.at(0);memory.read(address,actual);
        check(actual==expected,"Original callback body operand/table bytes differ");
        for(unsigned i=0;i<actual.size();++i)check(memory.is_read_only(address+i,1),"Callback body operand became writable");
    }
    for(const auto& field:bodies.at("context_storage")) {
        const unsigned address=field.at(0),size=field.at(1);
        check(std::ranges::all_of(memory.view(address,size),[](auto b){return b==std::byte{0};})&&
            !memory.is_read_only(address,size),"Original callback context lacks cold mutable storage");
        memory.write(address,std::vector<std::byte>(size,std::byte{0xa5}));
    }
    const auto saved_context=memory.checkpoint();
    for(const auto& field:bodies.at("context_storage"))memory.write(field.at(0),std::vector<std::byte>(field.at(1).get<unsigned>()));
    memory.restore(saved_context);
    for(const auto& field:bodies.at("context_storage"))check(std::ranges::all_of(memory.view(field.at(0),field.at(1)),
        [](auto b){return b==std::byte{0xa5};}),"Checkpoint lost original callback context bytes");
    for(unsigned address:{0x52d876u,0x52d877u}) {
        bool hole=false;try{(void)memory.view(address,1);}catch(const std::out_of_range&){hole=true;}
        check(hole,"Callback context storage borrowed its unowned two-byte gap");
    }
    for(const auto& input:bodies.at("cases"))run_case(bodies,input);
    nlohmann::json numeric;std::ifstream(argv[5])>>numeric;
    check(numeric.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&numeric.at("instruction_patches").empty()&&
        numeric.at("supplied_dependency_returns").empty(),"Numeric dependency original authority differs");
    for(const auto& field:numeric.at("immutable_operands")) {
        const unsigned address=field.at(0);const auto expected=unhex(field.at(1));
        std::vector<std::byte> actual(expected.size());memory.read(address,actual);
        check(actual==expected,"Existing numeric dependency operand differs");
        for(unsigned i=0;i<actual.size();++i)check(memory.is_read_only(address+i,1),"Numeric dependency operand became writable");
    }
    nlohmann::json query;std::ifstream(argv[6])>>query;
    check(query.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&query.at("instruction_patches").empty()&&
        query.at("supplied_dependency_returns").empty(),"Registered identity query original authority differs");
    for(const auto& input:query.at("cases"))run_case(query,input);
    nlohmann::json selector;std::ifstream(argv[7])>>selector;
    check(selector.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&selector.at("instruction_patches").empty()&&
        selector.at("supplied_dependency_returns").empty(),"Property selector original authority differs");
    nlohmann::json types;std::ifstream(argv[8])>>types;
    check(types.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&types.at("instruction_patches").empty()&&
        types.at("supplied_dependency_returns").empty(),"Property selector type dependency original authority differs");
    // The unchanged original cases execute2673A0 for several property kinds.
    // Reuse their already-authenticated type fields, including overlapping
    // bytes, instead of supplying a lookup result or changing the reference.
    std::map<unsigned,std::byte> selector_inputs;
    auto merge_input=[&](unsigned address,const std::vector<std::byte>& data) {
        for(unsigned i=0;i<data.size();++i) {
            const auto [it,inserted]=selector_inputs.emplace(address+i,data[i]);
            check(inserted||it->second==data[i],"Original selector/type input owners disagree");
        }
    };
    for(const auto& field:selector.at("immutable_operands"))merge_input(field.at(0),unhex(field.at(1)));
    for(const auto& row:types.at("rows"))merge_input(row.at("address"),unhex(row.at("data_hex")));
    selector["immutable_operands"]=nlohmann::json::array();
    for(const auto& [address,value]:selector_inputs)
        selector["immutable_operands"].push_back(nlohmann::json::array({address,hex(std::span(&value,1))}));
    for(const auto& field:selector.at("immutable_operands")) {
        const unsigned address=field.at(0);const auto expected=unhex(field.at(1));
        std::vector<std::byte> actual(expected.size());memory.read(address,actual);
        check(actual==expected,"Original property selector matrix/switch/operand differs");
        for(unsigned i=0;i<actual.size();++i)check(memory.is_read_only(address+i,1),"Property selector input became writable");
    }
    for(unsigned address:{0x4568dfu,0x4568f8u}) {
        bool hole=false;try{(void)memory.view(address,1);}catch(const std::out_of_range&){hole=true;}
        check(hole,"Property selector borrowed adjacent unowned matrix cells");
    }
    for(const auto& input:selector.at("cases"))run_case(selector,input);
    nlohmann::json actor_state;std::ifstream(argv[9])>>actor_state;
    check(actor_state.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&actor_state.at("instruction_patches").empty()&&
        actor_state.at("supplied_dependency_returns").empty(),"Actor-state update original authority differs");
    for(const auto& input:actor_state.at("cases"))run_case(actor_state,input);
    nlohmann::json bucket;std::ifstream(argv[10])>>bucket;
    check(bucket.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&bucket.at("instruction_patches").empty()&&
        bucket.at("supplied_dependency_returns").empty(),"Registered bucket query original authority differs");
    for(const auto& input:bucket.at("cases"))run_case(bucket,input);
    nlohmann::json birth;std::ifstream(argv[11])>>birth;
    check(birth.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&birth.at("instruction_patches").empty()&&
        birth.at("supplied_dependency_returns").empty(),"Birth-query variants original authority differs");
    for(const auto& input:birth.at("cases"))run_case(birth,input);
    nlohmann::json update;std::ifstream(argv[12])>>update;
    check(update.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&update.at("instruction_patches").empty()&&
        update.at("supplied_dependency_returns").empty(),"Update-query variants original authority differs");
    for(const auto& input:update.at("cases"))run_case(update,input);
    nlohmann::json remaining;std::ifstream(argv[13])>>remaining;
    check(remaining.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&remaining.at("instruction_patches").empty()&&
        remaining.at("supplied_dependency_returns").empty(),"Remaining query variants original authority differs");
    for(const auto& input:remaining.at("cases"))run_case(remaining,input);
    nlohmann::json behavior;std::ifstream(argv[14])>>behavior;
    check(behavior.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&behavior.at("instruction_patches").empty()&&
        behavior.at("supplied_dependency_returns").empty(),"Actor behavior original authority differs");
    mount_native_tu15_physics_branch_data(memory);
    for(const auto& field:behavior.at("immutable_operands")) {
        const auto expected=unhex(field.at(1));std::vector<std::byte> actual(expected.size());
        memory.read(field.at(0),actual);check(actual==expected,"Mounted actor behavior operand differs");
        for(unsigned i=0;i<actual.size();++i)check(memory.is_read_only(field.at(0).get<unsigned>()+i,1),"Actor behavior operand became writable");
    }
    for(const auto& input:behavior.at("cases"))run_case(behavior,input);
    nlohmann::json context;std::ifstream(argv[15])>>context;
    check(context.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&context.at("instruction_patches").empty()&&
        context.at("supplied_dependency_returns").empty(),"Actor script-context authority differs");
    for(const auto& input:context.at("cases"))run_case(context,input);
    nlohmann::json marine;std::ifstream(argv[16])>>marine;
    check(marine.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&marine.at("instruction_patches").empty()&&
        marine.at("supplied_dependency_returns").empty(),"Marine command authority differs");
    for(const auto& field:marine.at("immutable_operands")) {
        const auto expected=unhex(field.at(1));std::vector<std::byte> actual(expected.size());
        memory.read(field.at(0),actual);check(actual==expected,"Mounted Marine command operand differs");
        for(unsigned i=0;i<actual.size();++i)check(memory.is_read_only(field.at(0).get<unsigned>()+i,1),"Marine command operand became writable");
    }
    for(const auto& input:marine.at("cases"))run_case(marine,input);
    nlohmann::json registry;std::ifstream(argv[17])>>registry;
    check(registry.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&registry.at("instruction_patches").empty()&&
        registry.at("supplied_dependency_returns").empty(),"Original HS registry authority differs");
    for(const auto& field:registry.at("immutable_operands")) {
        const auto expected=unhex(field.at(1));std::vector<std::byte> actual(expected.size());
        memory.read(field.at(0),actual);check(actual==expected,"Original HS definition/usage differs");
        for(unsigned i=0;i<actual.size();++i)check(memory.is_read_only(field.at(0).get<unsigned>()+i,1),"Original HS definition became writable");
    }
    for(const auto& field:registry.at("authored_operands")) {
        const auto expected=unhex(field.at(1));std::vector<std::byte> actual(expected.size());
        memory.read(field.at(0),actual);check(actual==expected,"Original HS authored registry differs");
        memory.validate_mutable_view(field.at(0),actual.size());const auto saved=memory.checkpoint();
        memory.write(field.at(0),bytes(0));memory.restore(saved);memory.read(field.at(0),actual);
        check(actual==expected,"Original HS registry checkpoint differs");
        const auto address=field.at(0).get<unsigned>();SourceRuntimeMemory shared;
        shared.mount(address+4,std::span(expected).subspan(4,4),SourceRuntimeMemory::Access::ReadOnly,"Prior readonly HS slot");
        shared.mount(address+8,std::span(expected).subspan(8,4),SourceRuntimeMemory::Access::ReadWrite,"Prior mutable HS slot");
        mount_native_tu15_final_startup_storage_data(shared);shared.read(address,actual);
        check(actual==expected&&shared.is_read_only(address+4,4)&&!shared.is_read_only(address+8,4)&&
            !shared.is_read_only(address,4),"Original HS registry lost existing owners or missing authored bytes");
        SourceRuntimeMemory conflicting;conflicting.mount(address+4,bytes(0xdeadbeef),SourceRuntimeMemory::Access::ReadWrite,"Conflicting HS slot");
        bool rejected=false;try{mount_native_tu15_final_startup_storage_data(conflicting);}
        catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"Original HS registry silently replaced conflicting metadata");
        bool missing=false;try{(void)conflicting.view(address,1);}catch(const std::out_of_range&){missing=true;}
        check(missing,"Original HS registry partially admitted before validating prior owners");
    }
    for(const auto& input:registry.at("cases"))run_case(registry,input);
    nlohmann::json shared_prop;std::ifstream(argv[18])>>shared_prop;
    check(shared_prop.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&shared_prop.at("instruction_patches").empty()&&
        shared_prop.at("supplied_dependency_returns").empty(),"Shared property helper authority differs");
    for(const auto& input:shared_prop.at("cases"))run_case(shared_prop,input);
    nlohmann::json scalars;std::ifstream(argv[19])>>scalars;
    check(scalars.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&scalars.at("instruction_patches").empty()&&
        scalars.at("supplied_dependency_returns").empty(),"Actor scalar authority differs");
    for(const auto& field:scalars.at("immutable_operands")) {
        const auto expected=unhex(field.at(1));std::vector<std::byte> actual(expected.size());
        memory.read(field.at(0),actual);check(actual==expected,"Actor scalar operand differs");
        for(unsigned i=0;i<actual.size();++i)check(memory.is_read_only(field.at(0).get<unsigned>()+i,1),"Actor scalar operand became writable");
    }
    for(const auto& input:scalars.at("cases"))run_case(scalars,input);
    nlohmann::json bsp_cache;std::ifstream(argv[20])>>bsp_cache;
    check(bsp_cache.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&bsp_cache.at("instruction_patches").empty()&&
        bsp_cache.at("supplied_dependency_returns").empty(),"BSP audibility cache authority differs");
    for(const auto& input:bsp_cache.at("cases"))run_case(bsp_cache,input);
    nlohmann::json difficulty;std::ifstream(argv[21])>>difficulty;
    check(difficulty.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&difficulty.at("instruction_patches").empty()&&
        difficulty.at("supplied_dependency_returns").empty(),"Original difficulty selection authority differs");
    for(const auto& field:difficulty.at("immutable_operands")) {
        const auto expected=unhex(field.at(1));std::vector<std::byte> actual(expected.size());
        memory.read(field.at(0),actual);check(actual==expected,"Original difficulty operand differs");
        for(unsigned i=0;i<actual.size();++i)check(memory.is_read_only(field.at(0).get<unsigned>()+i,1),"Original difficulty operand became writable");
    }
    for(const auto& input:difficulty.at("cases"))run_case(difficulty,input);
    nlohmann::json camera;std::ifstream(argv[22])>>camera;
    check(camera.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&camera.at("instruction_patches").empty()&&
        camera.at("supplied_dependency_returns").empty(),"Original camera request authority differs");
    for(const auto& field:camera.at("authored_operands")) {
        const auto expected=unhex(field.at(1));
        check(hex(memory.view(field.at(0),expected.size()))==hex(expected),"Original camera type table differs");
        memory.validate_mutable_view(field.at(0),expected.size());const auto saved=memory.checkpoint();
        memory.write(field.at(0),std::vector<std::byte>(expected.size(),std::byte{0x42}));memory.restore(saved);
        check(hex(memory.view(field.at(0),expected.size()))==hex(expected),"Original camera type table checkpoint differs");
    }
    for(unsigned address:{0x47525au,0x47525bu}) {
        bool hole=false;try{(void)memory.view(address,1);}catch(const std::out_of_range&){hole=true;}
        check(hole,"Camera type table borrowed following padding or director names");
    }
    nlohmann::json camera_hardware;std::ifstream(argv[23])>>camera_hardware;
    check(camera_hardware.at("original_xbe_sha256")==camera.at("original_xbe_sha256")&&
        camera_hardware.at("component_reference_sha256")=="56a02f8477d4386a25b0edf57c0e54fe9a91796602b1612bca4c706a607139c5"&&
        camera_hardware.at("oracle_backend")=="original_i386_hardware_single_step"&&
        camera_hardware.at("instruction_patches").empty()&&camera_hardware.at("supplied_dependency_returns").empty()&&
        camera_hardware.at("cases").size()==camera.at("cases").size(),"Original camera hardware reference differs");
    for(unsigned i=0;i<camera.at("cases").size();++i) {
        auto input=camera.at("cases").at(i);const auto& status=camera_hardware.at("cases").at(i);
        check(status.at("index")==i,"Camera hardware case ordering differs");
        check(((input.at("final_mxcsr").get<unsigned>()^status.at("final_mxcsr").get<unsigned>())&~0x3fu)==0,
            "Camera emulator/hardware control bits differ");
        // The unchanged whole original routine was separately single-stepped
        // on i386. Keep its full sticky status; Unicorn omits those updates.
        input["final_mxcsr"]=status.at("final_mxcsr");run_case(camera,input);
    }
    nlohmann::json workspace;std::ifstream(argv[28])>>workspace;
    check(workspace.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
        workspace.at("instruction_patches").empty()&&workspace.at("supplied_dependency_returns").empty(),
        "Decision-workspace original authority differs");
    for(const auto& input:workspace.at("cases"))run_case(workspace,input);
    nlohmann::json geometry;std::ifstream(argv[29])>>geometry;
    check(geometry.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
        geometry.at("instruction_patches").empty()&&geometry.at("supplied_dependency_returns").empty(),
        "Geometry permutation original authority differs");
    mount_campaign_existing_projection_test_slice_tu15(memory);
    for(const auto& operand:geometry.at("immutable_operands")) {
        const auto expected=unhex(operand.at(1));const auto address=operand.at(0).get<unsigned>();
        for(unsigned n=0;n<expected.size();++n)
            check(memory.view(address+n,1)[0]==expected[n]&&memory.is_read_only(address+n,1),
                "Original projection/inverse table bytes or access differ");
    }
    for(const auto& input:geometry.at("cases"))run_case(geometry,input);
    nlohmann::json links;std::ifstream(argv[30])>>links;
    check(links.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
        links.at("instruction_patches").empty()&&links.at("supplied_dependency_returns").empty(),
        "Child command link original authority differs");
    child_link_storage(links);
    for(const auto& input:links.at("cases"))run_case(links,input);
    nlohmann::json distance;std::ifstream(argv[31])>>distance;
    check(distance.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
        distance.at("instruction_patches").empty()&&distance.at("supplied_dependency_returns").empty(),
        "Path distance original authority differs");
    for(const auto& input:distance.at("cases"))run_case(distance,input);
    nlohmann::json behavior27;std::ifstream(argv[32])>>behavior27;
    check(behavior27.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
        behavior27.at("instruction_patches").empty()&&behavior27.at("supplied_dependency_returns").empty(),
        "Descriptor27 original authority differs");
    for(const auto& input:behavior27.at("cases"))run_case(behavior27,input);
    std::cout<<"PASS "<<fixture.at("cases").size()<<" original object-decision cases, GP/SIMD/flags, exact stores/RNG, "
        "real nested character/random owners and checkpoint restoration,22 lease cases and "<<callbacks.at("cases").size()<<
        " object callback prefixes, "<<bodies.at("cases").size()<<" callback body/prefix cases and "<<query.at("cases").size()<<
        " registered identity query cases and "<<selector.at("cases").size()<<
        " whole property selectors and "<<actor_state.at("cases").size()<<
        " whole actor-state updates and "<<bucket.at("cases").size()<<
        " whole bucket queries and "<<birth.at("cases").size()<<
        " birth-query whole/prefix cases and "<<update.at("cases").size()<<
        " update-query whole/prefix cases and "<<remaining.at("cases").size()<<
        " remaining query whole/prefix cases and "<<behavior.at("cases").size()<<
        " actor behavior/scalar cases and "<<context.at("cases").size()<<
        " actor script-context cases and "<<marine.at("cases").size()<<
        " Marine command cases and "<<registry.at("cases").size()<<
        " HS registry cases and "<<shared_prop.at("cases").size()<<
        " shared property cases and "<<scalars.at("cases").size()<<
        " actor scalar cases and "<<bsp_cache.at("cases").size()<<
        " whole BSP-cache cases and "<<difficulty.at("cases").size()<<
        " whole difficulty cases and "<<camera.at("cases").size()<<
        " whole camera request cases and "<<reaction.at("cases").size()<<
        " original reaction whole/prefix cases and "<<prop_owner.at("cases").size()<<
        " whole property-owner lookup cases and "<<creation.at("cases").size()<<
        " original property-creation cases and "<<workspace.at("cases").size()<<
        " decision-workspace whole/prefix cases and "<<geometry.at("cases").size()<<
        " whole geometry-permutation cases and "<<links.at("cases").size()<<
        " whole child-command link cases and "<<distance.at("cases").size()<<
        " path-distance whole/prefix cases and "<<behavior27.at("cases").size()<<
        " whole descriptor27/character/iterator cases; external dependencies remain required\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
