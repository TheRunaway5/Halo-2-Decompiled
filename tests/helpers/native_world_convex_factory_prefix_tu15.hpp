#pragma once
#include "halo2/native_tu15_activation_contact_manager.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
// Included after the residual model's Memory/model/constants declarations.
// This is a positive World binding PREFIX. It stops at the actual source
// record publication; it supplies no outcome for the unconverted processor.
inline int native_world_convex_factory_prefix(const std::filesystem::path&reference,bool gsk=false,bool wake_prefix=false){
    using J=nlohmann::json;
    auto read_file=[](const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);check(bool(f),"Missing owned convex reference input");std::vector<char>c((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>b(c.size());for(unsigned n=0;n<c.size();++n)b[n]=std::byte(std::uint8_t(c[n]));return b;};
    std::ifstream mi(reference/"capture-native-before.json");const auto capture=J::parse(mi);
    check(capture.at("schema")=="native-tu15-pair-consumer-typed-before-v1","Actual typed02 source reference required");
    std::map<std::uint32_t,std::vector<std::byte>>before;
    for(const auto&r:capture.at("regions"))before.emplace(r.at("address"),read_file(reference/r.at("before_file").get<std::string>()));
    const auto source_word=[&](std::uint32_t p){auto i=before.upper_bound(p);check(i!=before.begin(),"Missing true reference word");--i;check(std::uint64_t(p-i->first)+4u<=i->second.size(),"Reference word crosses unbacked extent");std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(i->second[p-i->first+n])<<(n*8u);return v;};
    auto m=model();
    // Exact heap and authored asset BEFORE inputs only. Source control/TLS
    // ranges60000000..6FFFFFFF are never imported into this native model.
    for(const auto&[p,b]:before)if(p>=0x08000000u&&!(p>=0x60000000u&&p<0x70000000u))m.banks.emplace(p,b);
    std::ifstream ri(reference/"selected-readonly.json");const auto readonly=J::parse(ri);
    for(const auto&r:readonly.at("regions")){const auto text=r.at("hex").get<std::string>();std::vector<std::byte>b;for(unsigned n=0;n<text.size();n+=2)b.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));m.write(r.at("address"),b);}
    check(source_word(0x486064u)==2u,"Reference must contain the actual mode2 pool");
    m.put(0x486064u,2);m.put(0x48606cu,source_word(0x48606cu));
    m.put(world+0x5e50u,0x149da0u);m.put(world+0x5e54u,0x149e00u);
    m.put(world+0x24u,0);m.put(world+0x10cu,0);m.put(world+0xa0u,0);m.put(world+0x118u,0);
    m.put(world+0xf4u,0);m.put(world+0x100u,0);m.put(world+0xcu,1);
    m.put(island0+0x6cu,0);m.put(island0+0x78u,0); // Declared fresh pair, valid empty lists.
    m.put(0x48c974u,world+0x5600u);m.put(0x48c978u,world+0x5800u);
    auto state=initial();state.x87_depth=0;state.x87_status=0x4020u;state.floating_point=SourcePhysicsFactoryFloatingPoint{0x27fu,0x1fa0u};
    NativeTu15PhysicsFrameFp fp{0x27fu,0x4020u,0x1fa0u};NativeTu15WorldIntegrationStats stats;std::uint64_t residuals{},ticks{};
    std::vector<std::uint32_t>calls;std::uint32_t published_agent{},published_manager{},published_record{};
    struct Published{};
    const auto invoke=[&](std::uint32_t target,SourcePhysicsContactRuntimeAbi&a,const SourcePhysicsContactRecordObserver&){
        calls.push_back(target);const auto sp=a.gp[4];unsigned pop{};
        if(target==shapeBounds){
            const auto output=m.get(sp+12u);for(unsigned n=0;n<4;++n){m.put(output+n*4u,0xbf800000u);m.put(output+16u+n*4u,0x3f800000u);}pop=12;
        }else if(target==dispatcherSweep){
            const auto first=m.get(sp+16u);m.put(m.get(first),body0+0x1cu);m.put(m.get(first)+4u,body1+0x1cu);m.put(first+4u,1);
            // Authored real02 class23 shapes and their actual transforms,
            // attached to declared model bodies at the current sweep boundary.
            const std::array<std::uint32_t,2>actual_payloads=gsk?std::array<std::uint32_t,2>{0x1007bd6cu,0x1007d32cu}:std::array<std::uint32_t,2>{0x1007abecu,0x1007de2cu};
            for(unsigned n=0;n<2;++n){const auto body=n?body1:body0,payload=body+0xcu;
                m.put(payload,source_word(actual_payloads[n]));m.put(payload+8u,source_word(actual_payloads[n]+8u));m.put(payload+0x20u,body);
                m.put(body+6u,3,2);m.put(body+8u,world);m.put(body+0x44u,island0);m.put(body+0x48u,0,1);
                m.put(body+0x88u,world+0xdc00u+n*0x20u);m.put(body+0x8cu,0);m.put(body+0x90u,0x80000008u);
            }
            if(wake_prefix){
                // Declared valid inactive-island DATA; no live flags or rows are injected.
                m.put(world+0x10u,3u);m.put(world+0x14u,world+0x950u);m.put(world+0x18u,1u);
                m.put(world+0x1cu,1u);m.put(world+0x950u,island1);
                m.put(body1+0x44u,island1);m.put(island1+0x24u,0u);m.put(island1+0x29u,0u,1);
                m.put(island1+0x3cu,world+0xdb00u);m.put(world+0xdb00u,body1);m.put(island1+0x40u,1u);
                m.put(island1+0x74u,world+0xdb20u);m.put(island1+0x78u,0u);m.put(island1+0x7cu,0x80000004u);
            }
            constexpr auto context=world+0xe800u,dispatch=world+0xe900u,factory=world+0xea00u,table=world+0xeb00u;
            m.put(world+0xccu,context);m.put(context,dispatch);m.put(dispatch+0xcu,factory);m.put(factory,table);m.put(table+4u,0x3232e0u);m.put(factory+8u,world+0x140u);
            const auto kind=gsk?8u:23u;
            m.put(dispatch+((kind<<5u)+kind)*4u+0x18cu,gsk?0x2f83b0u:0x2e4eb0u);pop=20;
        }else if(target==0x3232e0u){
            throw std::runtime_error("Old World manager factory transport must not execute");
        }else throw std::runtime_error("Unexpected positive World prefix residual="+std::to_string(target));
        a.gp[4]+=4u+pop;
    };
    bool reached=false;
    try{native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889u,0x41f00000u},fp,state,0x60300000u,{stack,0x10000u},invoke,
        [&](const SourcePhysicsContactRecordStore&e){if(wake_prefix&&e.instruction==0x320dd9u){check(e.address==island1+0x78u&&m.get(island1+0x78u)==1u,"Native wake deferred queue publication differs");throw Published{};}if(!wake_prefix&&e.instruction==0x3210deu){published_record=e.address-8u;published_agent=m.get(e.address);reached=true;throw Published{};}},
        &stats,&residuals,[&]{return ++ticks;});}
    catch(const Published&){}
    if(wake_prefix){
        check(stats.pair_island_wake.attempts==1&&stats.pair_island_wake.completed==1&&
          stats.pair_island_wake.refusals==0&&stats.world_activation.activation.island_calls==1&&
          stats.world_activation.residual_calls==0&&stats.native_world_activation_calls==1&&
          stats.native_world_activation_refusals==0,"World native pair wake completion/refusal differs");
        check(m.get(island1+0x29u,1)==1&&m.get(world+0xcu)==2&&m.get(world+0x18u)==0&&
          m.get(island1+0x78u)==1,"World native wake/deferred pair DATA differs");
        check(std::find(calls.begin(),calls.end(),0x316d40u)==calls.end(),"World pair wake invoked old316D40");
        check(std::find(calls.begin(),calls.end(),0x320da0u)==calls.end(),"World wake invoked old deferred-pair owner");
        std::cout<<J{{"passed",true},{"native_pair_island_wakes",1},{"deferred_pair_requested",true},
          {"scope","Declared valid inactive-island World PREFIX at native320DD9 queue publication; no supplied result or live wake claim"}}.dump()<<'\n';return 0;
    }
    if(published_agent)published_manager=m.get(published_agent+8u);
    check(stats.pair_manager_factory.attempts==1&&stats.pair_manager_factory.completed==1&&stats.pair_manager_factory.refusals==0&&stats.pair_manager_factory.managers.registrations==2&&stats.pair_manager_factory.body_destructor_refusals==0&&stats.pair_manager_factory.base_lock_refusals==0,"Native World manager factory activity/lifetime differs");
    check(reached&&published_agent&&published_manager,"World did not publish a completed native convex agent");
    check(m.get(published_agent)==(gsk?0x41cd08u:0x41c370u)&&m.get(published_agent+4u,2)==(gsk?240u:256u)&&m.get(published_agent+8u)==published_manager&&(gsk||m.get(published_agent+0xf0u)==0),
        "Native World prefix published incomplete agent DATA");
    check(m.get(published_record)==body0+0x1cu&&m.get(published_record+4u)==body1+0x1cu,"Native World prefix lost current source handles");
    check(stats.native_pair_creation_attempts==1&&stats.native_pair_creation_calls==0&&stats.native_pair_creation_refusals==1&&
        stats.native_convex_agent_attempts==1&&stats.native_convex_agent_calls==1&&stats.native_convex_agent_refusals==0&&stats.native_convex_agent_pool_allocations==1&&
        stats.native_convex_agent_class_queries==1&&stats.native_convex_agent_centers==2&&stats.native_convex_agent_supports==2&&stats.native_convex_agent_encodings==2,
        "Native World prefix skipped source parent/factory or misreported observer-stop lifetime");
    check(stats.native_gsk_agent_calls==(gsk?1u:0u)&&stats.native_gsk_agent_refusals==0,"Native World prefix did not admit the selected GSK binding");
    for(auto target:{0x320ef0u,0x2e4eb0u,0x2f83b0u,0x2f8300u,0x3232e0u})check(std::find(calls.begin(),calls.end(),target)==calls.end(),"Native prefix invoked an old parent/factory");
    std::cout<<J{{"passed",true},{"scope","Positive World PREFIX at3210DE, before unconverted agent processor; no full positive World physics claim"},{"native_factory_completions",1},{"gsk_factory",gsk},{"factory_owner",gsk?0x2f83b0u:0x2e4eb0u},{"published_agent",published_agent},{"published_manager",published_manager},{"source_body_handles",J::array({body0+0x1cu,body1+0x1cu})},{"authored_shape_payload_sources",gsk?J::array({0x1007bd6cu,0x1007d32cu}):J::array({0x1007abecu,0x1007de2cu})},{"manager_factory_service_fully_native_no_residual",true},{"source_pool_and_shape_before_only",true},{"source_GP_SSE_or_after_supplied",false},{"remaining_process_outcome_supplied",false},{"explicit_readonly_observer_stop",true}}.dump()<<'\n';return 0;
}
