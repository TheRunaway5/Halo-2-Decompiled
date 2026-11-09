#pragma once
#include "halo2/native_tu15_pair_deletion_native_scope.hpp"
#include "halo2/native_tu15_agent_cleanup_children.hpp"
#include "halo2/native_tu15_gameplay_phantom_membership.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#include <string>

// Declared pair/allocator DATA, not a recorded collision or an original-XBE
// comparison. Memory/check come from the World residual model. Cleanup,
// scalar destruction, routing and pool release execute their real native
// bodies. FFFF points and type0 phantom roots retain their genuine skip paths.
static inline void native_world_pair_deletion_cleanup_model() {
    constexpr std::uint32_t data=0x2400000u,pool=0x2600000u,formal=0x2800000u;
    constexpr auto handler=data+0x100u,pair=data+0x200u,records=data+0x300u;
    constexpr auto first=data+0x400u,second=data+0x440u,manager=data+0x600u;
    constexpr auto agent_table=data+0x700u,sphere_table=data+0x740u,point_table=data+0x780u;
    constexpr auto manager_table=data+0x7c0u,allocator=data+0x900u,allocator_table=data+0x940u;
    constexpr auto pool_table=data+0x980u,shape=data+0xa00u,shape_table=data+0xa40u;
    constexpr auto self_root=data+0xb00u,other_root=data+0xb40u;
    for(const auto owner:{0x2f0da0u,0x2f9960u,0x2fbb10u,0x2f7790u,0x2f3100u})
    for(unsigned failure=0;failure<4;++failure) {
        Memory m;m.banks[data].resize(0x10000u);m.banks[pool].resize(0x100000u);
        m.banks[0x400000u].resize(0x160000u);m.banks[formal].assign(64u,std::byte{0xa5});
        m.put(pool,pool_table);m.put(pool_table+20u,0x233420u);
        m.put(pool+0x68u,64u);m.put(pool+0x18u,pool+0x4340u);
        m.put(pool+0x1cu,pool+0x6300u);m.put(pool+0x20u,pool+0x4340u);
        m.put(pool+0xf0340u,1u);m.put(pool+0x2d0u,1u);
        m.put(0x486064u,1u);m.put(0x486068u,pool);m.put(0x48606cu,pool);
        m.put(0x48c938u,allocator);m.put(allocator,allocator_table);
        m.put(allocator_table+20u,0x149e00u);
        std::vector<std::uint32_t>sites,cleaned,released;
        const SourcePhysicsContactRecordObserver observe=[&](const auto&event){sites.push_back(event.instruction);};
        NativeTu15PoolMemory storage(m,pool,observe);
        const auto make=[&](std::uint32_t table) {
            const auto result=native_tu15_pool_allocate(storage,64u);
            check(result!=0u,"Pair deletion model exhausted its declared native pool");
            m.put(result,table);m.put(result+4u,64u,2);m.put(result+8u,manager);return result;
        };
        m.put(agent_table,failure==2u&&owner!=0x2f0da0u?0xdead0000u:
            owner==0x2f3100u?0x2f35f0u:0x2fc540u);
        m.put(agent_table+20u,failure==1u?0xdead0010u:owner);
        m.put(sphere_table,0x2fc540u);m.put(sphere_table+20u,failure==2u?0xdead0014u:0x2f9960u);
        m.put(point_table,0x2fc540u);m.put(point_table+20u,0x2fbb10u);
        const auto agent=make(agent_table);
        std::uint32_t primary{},secondary{};
        if(owner==0x2f0da0u) {
            primary=make(sphere_table);secondary=make(point_table);
            m.put(primary+12u,0xffffu,2);m.put(secondary+12u,0xffffu,2);
            m.put(agent+12u,primary);m.put(agent+16u,secondary);
        }else if(owner==0x2f3100u) {
            primary=make(sphere_table);m.put(primary+12u,0xffffu,2);
            m.put(agent+12u,data+0xc00u);m.put(agent+16u,1u);
            m.put(agent+20u,0x80000000u);m.put(data+0xc08u,primary);
        }else if(owner==0x2f7790u) {
            m.put(agent+12u,self_root);m.put(agent+16u,other_root);
            m.put(agent+20u,shape);m.put(agent+28u,22u);m.put(agent+32u,23u);
            m.put(shape,shape_table);m.put(shape_table+44u,0x1f3550u);
        }else m.put(agent+12u,0xffffu,2);
        const auto allocated=owner==0x2f0da0u?3u:owner==0x2f3100u?2u:1u;
        const auto in_use=m.get(pool+0x2e8u),free_blocks=m.get(pool+0xf0388u);
        m.put(manager,manager_table);m.put(manager_table,0x2e4d60u);
        m.put(manager_table+16u,failure==3u?0xdead0020u:0x323220u);
        m.put(first+4u,1u);m.put(second+4u,2u);m.put(pair,first);m.put(pair+4u,second);
        m.put(records,first);m.put(records+4u,second);m.put(records+8u,agent);
        m.put(handler+0x80u,records);m.put(handler+0x84u,records+16u);m.put(handler+0x88u,records);
        NativeTu15PairDeletionNativeScope scope;NativeTu15PairDeletionNativeScopeActivity scope_activity;
        NativeTu15ArrayAllocatorMemory router(m);NativeTu15ArrayAllocatorServices heap;
        unsigned old_transport{},leaves{},manager_disposals{};
        const auto unknown=[&](std::uint32_t target) {
            scope.reject_unconverted(target);++old_transport;
            throw std::runtime_error("Pair deletion model reached an old transport");
        };
        heap.release_owner=[&](const auto&request,auto&) {
            if(request.owner!=0x233420u)unknown(request.owner);
            NativeTu15PoolMemory current(m,request.receiver,observe);
            check(native_tu15_pool_release(current,request.data,request.bytes),
                "Pair deletion native pool release refused owned storage");
            released.push_back(request.data);
        };
        NativeTu15AgentCleanupChildrenServices children;
        children.allocation.release=[&](const auto&request,auto&fp) {
            if(request.owner!=0x149e00u)unknown(request.owner);
            native_tu15_route_array_release(router,request.data,request.bytes,request.memory_class,fp,heap);
        };
        children.agents.dispose=[&](auto object,auto target,auto flags,auto&fp) {
            if(target.identity==0x2e4d60u){++manager_disposals;return;} // Actual RET4.
            if(target.identity==0x2f35f0u){
                (void)native_tu15_destroy_contact_agent(m,NativeTu15AgentDestructor::owner_2f35f0,
                    {object},flags,fp,children,observe);return;
            }
            if(target.identity!=0x2fc540u)unknown(target.identity);
            (void)native_tu15_destroy_contact_agent(m,NativeTu15AgentDestructor::owner_2fc540,
                {object},flags,fp,children,observe);
        };
        NativeTu15PhantomCallbackCleanupServices phantom;phantom.agents=children.agents;
        phantom.leave=[&](auto selected,auto target,auto self,auto other,auto&fp) {
            if(target.identity!=0x1f3550u)unknown(target.identity);
            ++leaves;native_tu15_leave_gameplay_phantom(m,selected.identity,self.identity,other.identity,
                fp,children.allocation,observe);
        };
        children.agents.cleanup_child=[&](auto selected,auto target,auto&fp) {
            cleaned.push_back(target.identity);
            if(target.identity==0x2f0da0u)native_tu15_cleanup_bv_contact_agent(m,selected,fp,children,observe);
            else if(target.identity==0x2f9960u)native_tu15_cleanup_sphere_contact_agent(m,selected,fp,children,observe);
            else if(target.identity==0x2fbb10u)native_tu15_cleanup_single_point_contact_agent(m,selected,fp,children);
            else if(target.identity==0x2f7790u)native_tu15_cleanup_phantom_callback_agent(m,selected,fp,phantom);
            else if(target.identity==0x2f3100u)native_tu15_cleanup_contact_agent(m,
                NativeTu15AgentCleanupKind::records_2f3100,selected,fp,children.agents);
            else unknown(target.identity);
        };
        const NativeTu15PairDeletionBindingServices services{
            children.agents.cleanup_child,[&](auto selected,auto target,auto&fp) {
                if(target.identity!=0x323220u)unknown(target.identity);
                native_tu15_dispose_contact_reference(m,selected.identity,fp,children.agents);
            }};
        NativeTu15PhysicsFrameFp fp{0x27fu,0u,0x1f80u};NativeTu15PairDeletionBindingActivity activity;
        std::string refusal;
        try{native_tu15_delete_bound_pair_in_native_scope(m,handler,pair,{formal},fp,services,
            scope,observe,&activity,&scope_activity);}catch(const std::runtime_error&error){refusal=error.what();}
        if(!failure&&!refusal.empty())throw std::runtime_error("Native pair cleanup binding refused: "+refusal);
        const auto expected_releases=(!failure||failure==3u)?allocated:0u;
        check(released.size()==expected_releases&&m.get(pool+0x2e8u)==in_use-expected_releases&&
            m.get(pool+0xf0388u)==free_blocks+expected_releases&&old_transport==0u&&!scope.active(),
            "Pair deletion lost actual pool lifetime or escaped its native scope");
        if(!failure) {
            check(refusal.empty()&&activity.completed==1u&&activity.refusals==0u&&
                activity.agent_cleanups==1u&&activity.manager_releases==1u&&activity.unqualified_owners==0u&&
                m.get(handler+0x80u)==records+16u&&manager_disposals==1u&&scope_activity.refused==0u&&
                leaves==unsigned(owner==0x2f7790u),"Pair deletion skipped its real cleanup/release cursor phases");
        }else{
            check(!refusal.empty()&&activity.completed==0u&&activity.refusals==1u&&
                m.get(handler+0x80u)==records&&manager_disposals==0u&&
                activity.unqualified_owners==unsigned(failure!=2u)&&
                scope_activity.refused==unsigned(failure==2u),
                "Pair deletion did not refuse the current unknown owner before transport");
        }
        if(expected_releases) {
            check(released.back()==agent&&m.get(pool+0x24u)==agent,
                "Pair deletion did not return the outer agent to its native free list");
            if(owner==0x2f0da0u)check(released==std::vector<std::uint32_t>{primary,secondary,agent}&&
                m.get(agent+16u)==0u&&std::count(sites.begin(),sites.end(),0x2f0db7u)==1,
                "Pair deletion changed BV child order or secondary-reference clear");
            if(owner==0x2f3100u)check(released==std::vector<std::uint32_t>{primary,agent}&&
                std::count(sites.begin(),sites.end(),0x2f359du)==1,
                "Pair deletion skipped native collection destruction or its child lifetime");
        }
        for(auto byte:m.banks.at(formal))check(byte==std::byte{0xa5},
            "Pair deletion changed unrelated caller-owned formal DATA");
    }
}
