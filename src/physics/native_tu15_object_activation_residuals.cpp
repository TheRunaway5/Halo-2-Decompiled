#include "halo2/native_tu15_object_activation_residuals.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#include "halo2/native_tu15_contact_agent_factory.hpp"
#include "halo2/native_tu15_centered_shape_bounds.hpp"
#include "halo2/native_tu15_compound_shape_bounds.hpp"
#include "halo2/native_tu15_sphere_shape_bounds.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
void require(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
}
namespace {
void with_activation_services(SourcePhysicsContactMemory&memory,unsigned depth,
 NativeTu15ObjectActivationScratch scratch,const NativeTu15ObjectActivationResidualCall&invoke,
 const SourcePhysicsContactRecordObserver&observer,NativeTu15ObjectActivationBindingActivity*activity,
 const NativeTu15ObjectActivationNativeServices*native,
 const std::function<void(const NativeTu15IslandActivationServices&)>&use){
    require(scratch.filter_result!=scratch.discrete_result,
        "Native TU1.5 object activation requires distinct caller-owned result bytes");
    require(scratch.policy_bounds&&!(scratch.policy_bounds&15u)&&scratch.shape_scratch_first&&
        !(scratch.shape_scratch_first&15u)&&scratch.shape_scratch_bytes>=64u&&
        !(scratch.shape_scratch_bytes&15u)&&std::uint64_t(scratch.shape_scratch_first)+scratch.shape_scratch_bytes<=0xffffffffu,
        "Native TU1.5 object activation requires declared aligned policy/shape DATA storage");
    const auto call=[&](std::uint32_t owner,std::span<const std::uint32_t>arguments,
      std::optional<std::uint32_t>receiver,unsigned pop,NativeTu15PhysicsFrameFp&state){
        require(bool(invoke),"Native TU1.5 object activation requires its actual remaining owner");
        if(activity){++activity->residual_calls;++activity->residual_owners[owner];}
        return invoke(owner,arguments,receiver,pop,state);
    };
    NativeTu15ArrayAllocatorMemory allocator_memory(memory);
    NativeTu15ArrayAllocatorServices heap;
    heap.allocate_owner=[&](const auto&request,auto&state){
        if(request.owner==0x233250u){
            if(activity)++activity->native_pool_allocations;
            NativeTu15PoolMemory pool(memory,request.receiver,observer);
            return native_tu15_pool_allocate(pool,request.bytes);
        }
        const std::array arguments{request.bytes,request.memory_class};
        return call(request.owner,arguments,request.receiver,8,state);
    };
    heap.release_owner=[&](const auto&request,auto&state){
        if(request.owner==0x233420u){
            NativeTu15PoolMemory pool(memory,request.receiver,observer);
            if(native_tu15_pool_release(pool,request.data,request.bytes)){
                if(activity)++activity->native_pool_releases;
                return;
            }
        }
        const std::array arguments{request.data,request.bytes,request.memory_class};
        call(request.owner,arguments,request.receiver,12,state);
    };
    heap.default_heap=[&](auto&state){return call(0x149930u,{}, {},0,state);};
    NativeTu15ArrayGrowthServices allocation;
    allocation.allocate=[&](const auto&request,auto&state){
        if(request.owner==0x149da0u)
            return native_tu15_route_array_allocation(allocator_memory,request.bytes,request.memory_class,state,heap);
        const std::array arguments{request.bytes,request.memory_class};
        return call(request.owner,arguments,request.receiver,8,state);
    };
    allocation.release=[&](const auto&request,auto&state){
        if(request.owner==0x149e00u){
            native_tu15_route_array_release(allocator_memory,request.data,request.bytes,request.memory_class,state,heap);
            return;
        }
        const std::array arguments{request.data,request.bytes,request.memory_class};
        call(request.owner,arguments,request.receiver,12,state);
    };
    NativeTu15BodyPolicyServices body_policy;body_policy.allocation=allocation;
    auto shape_cursor=scratch.shape_scratch_first+scratch.shape_scratch_bytes;
    std::function<void(NativeTu15BoundsShape,std::uint32_t,NativeTu15BoundsTransform,
        std::uint32_t,NativeTu15BoundsOutput,NativeTu15PhysicsFrameFp&)>shape_bounds;
    shape_bounds=[&](auto shape,std::uint32_t owner,auto transform,std::uint32_t margin,auto output,auto&state){
        const bool native=owner==0x306760u||owner==0x307e30u||owner==0x2e8d90u||owner==0x2e8120u||
            owner==0x306170u||owner==0x2e8870u||owner==0x2e9600u||owner==0x2e5e80u;
        if(native&&activity)++activity->native_policy_bounds_owners[owner];
        if(owner==0x306760u)native_tu15_convex_box_bounds(memory,shape,transform,margin,output,state,depth,observer);
        else if(owner==0x307e30u)native_tu15_centered_shape_bounds(memory,shape,transform,margin,output,state,depth,observer);
        else if(owner==0x2e8d90u)native_tu15_sphere_shape_bounds(memory,shape,transform,margin,output,state,depth,observer);
        else if(owner==0x2e8120u)native_tu15_endpoint_shape_bounds(memory,shape,transform,margin,output,state,depth,observer);
        else if(owner==0x306170u||owner==0x2e8870u){
            require(shape_cursor>=scratch.shape_scratch_first+64u,
                "Native TU1.5 activation bounds exhausted declared nested DATA storage");
            const auto previous=shape_cursor;shape_cursor-=64u;
            try{
                if(owner==0x306170u)native_tu15_compound_shape_bounds(memory,shape,transform,margin,output,
                    {shape_cursor},state,body_policy.bounds,observer);
                else native_tu15_transformed_shape_bounds(memory,shape,transform,[margin]{return margin;},
                    [output]{return output;},{shape_cursor},state,body_policy.bounds,observer);
            }catch(...){shape_cursor=previous;throw;}
            shape_cursor=previous;
        }else if(owner==0x2e9600u)native_tu15_forward_shape_bounds(memory,shape,transform,margin,output,state,body_policy.bounds);
        else if(owner==0x2e5e80u)native_tu15_motion_shape_bounds(memory,shape,transform,margin,output,state,body_policy.bounds,observer);
        else{
            if(activity)++activity->unconverted_policy_bounds_owners[owner];
            const std::array arguments{transform.identity,margin,output.identity};
            call(owner,arguments,shape.identity,12,state);
        }
    };
    body_policy.bounds.child_bounds=shape_bounds;
    body_policy.policy_kind=[&](auto policy,auto owner,auto&state){return call(owner.identity,{},policy,0,state);};
    body_policy.destroy_policy=[&](auto policy,auto owner,auto disposing,auto&state){
        const std::array arguments{disposing};call(owner.identity,arguments,policy,4,state);
    };
    body_policy.release_base_lock=[&](auto object,auto flags,auto&state){
        const std::array arguments{flags};call(0x2e12a0u,arguments,object,4,state);
    };
    NativeTu15ActivationAgentServices agents;
    agents.filter_pair=[&](auto receiver,auto owner,auto result,auto first,auto second,auto&state){
        if(owner.identity==0x2e9a00u)
            return native_tu15_activation_group_filter(memory,receiver,result,first,second,observer,
                activity?&activity->services:nullptr);
        const std::array arguments{result,first.identity,second.identity};
        return call(owner.identity,arguments,receiver,12,state);
    };
    agents.create_manager=[&](auto factory,auto owner,auto first,auto second,auto context,auto&state){
        if(owner.identity==0x3232e0u){
            if(activity)++activity->native_manager_factories;
            NativeTu15ActivationContactManagerServices services;services.allocation=allocation;
            services.destroy_body=[&](auto body,auto target,auto disposing,auto&fp){
                const std::array arguments{disposing};call(target.identity,arguments,body.identity,4,fp);
            };
            services.release_base_lock=[&](auto object,auto flags,auto&fp){
                const std::array arguments{flags};call(0x2e12a0u,arguments,object,4,fp);
            };
            return native_tu15_create_activation_contact_manager(memory,factory,first,second,state,
                services,observer,activity?&activity->managers:nullptr);
        }
        if(activity)++activity->unconverted_manager_factories;
        const std::array arguments{first.identity,second.identity,context.identity};
        return NativeTu15ActivationContactManager{call(owner.identity,arguments,factory,12,state)};
    };
    agents.classify_shape=[&](auto shape,auto owner,auto&state){
        std::optional<std::uint32_t>kind;
        if(owner.identity==0xa9520u)kind=native_tu15_contact_child_class_23();
        else if(owner.identity==0xab4b0u)kind=native_tu15_contact_child_class_24();
        else kind=native_tu15_activation_type(owner.identity);
        if(kind){if(activity)++activity->native_shape_type_getters;return *kind;}
        if(activity)++activity->unconverted_shape_type_getters;
        return call(owner.identity,{},shape.identity,0,state);
    };
    agents.create_agent=[&](auto owner,auto first,auto second,auto context,auto manager,auto&state){
        if(owner.identity==0x303c10u||owner.identity==0x3040d0u){
            if(activity)++activity->native_agent_factories;
            return native_tu15_create_contact_agent(memory,
                owner.identity==0x303c10u?NativeTu15ContactAgentFactory::owner_303c10:
                    NativeTu15ContactAgentFactory::owner_3040d0,
                NativeTu15ContactAgentFactoryArguments{[&]{return manager.identity;}},state,allocation,observer);
        }
        if(native&&native->create_agent){
            if(activity)++activity->native_bound_agent_factory_attempts;
            try{
                const auto result=native->create_agent(owner,first,second,context,manager,state,depth);
                if(activity){++activity->native_bound_agent_factory_calls;++activity->native_agent_factories;}
                return result;
            }catch(...){if(activity)++activity->native_bound_agent_factory_refusals;throw;}
        }
        if(activity)++activity->unconverted_agent_factories;
        const std::array arguments{first.identity,second.identity,context.identity,manager.identity};
        //28296F publishes the context in ECX before the cdecl call as well
        // as in its real argument packet. Retain that explicit source input.
        return NativeTu15ContactAgent{call(owner.identity,arguments,context.identity,0,state)};
    };
    agents.motion_kind=[&](auto motion,auto owner,auto&state){
        if(const auto kind=native_tu15_activation_type(owner.identity)){
            if(activity)++activity->native_motion_type_getters;return *kind;
        }
        if(activity)++activity->unconverted_motion_type_getters;
        return call(owner.identity,{},motion,0,state);
    };
    agents.test_discrete_pair=[&](auto dispatcher,auto owner,auto result,auto first,auto second,auto&state){
        if(owner.identity==0x281530u){
            NativeTu15ActivationDiscreteServices services;
            services.test_filter=[&](auto filter,auto target,auto a,auto b,auto&context){
                if(target.identity==0x281f10u){
                    NativeTu15ActivationPayloadFilterServices query;
                    query.motion_kind=[&](auto motion,auto owner,auto&fp){
                        if(activity)++activity->unconverted_motion_type_getters;
                        return call(owner.identity,{},motion,0,fp);
                    };
                    return native_tu15_activation_payload_filter(memory,a,b,context,query,
                        activity?&activity->payload_filter:nullptr);
                }
                const std::array arguments{a.identity,b.identity};
                return call(target.identity,arguments,filter,8,context);
            };
            return native_tu15_activation_discrete_pair(memory,dispatcher,result,first,second,state,
                services,observer,activity?&activity->services:nullptr);
        }
        const std::array arguments{result,first.identity,second.identity};
        return call(owner.identity,arguments,dispatcher,12,state);
    };
    agents.release_manager=[&](auto manager,auto owner,auto&state){
        call(owner.identity,{},manager.identity,0,state);
    };
    agents.body_policy_kind=[&](auto policy,auto owner,auto&state){
        if(const auto kind=native_tu15_activation_type(owner.identity)){
            if(activity)++activity->native_policy_type_getters;return *kind;
        }
        if(activity)++activity->unconverted_policy_type_getters;
        return call(owner.identity,{},policy,0,state);
    };
    agents.mark_body=[&](auto body,auto flags,auto&state){
        native_tu15_select_body_policy(memory,body,flags,{{scratch.policy_bounds}},state,depth,
            body_policy,observer,activity?&activity->body_policy:nullptr);
    };
    NativeTu15IslandActivationServices activation;
    activation.grow_array=[&](auto header,auto stride,auto&state){
        if(activity)++activity->native_array_growths;
        NativeTu15ArrayGrowthMemory data(memory,{header.identity},observer);
        native_tu15_grow_array(data,stride,state,allocation);
    };
    activation.island_activated=[&](auto listener,auto owner,auto island,auto&state){
        if(owner.identity==0x282870u){
            native_tu15_recreate_island_agents(memory,island,{scratch.filter_result,scratch.discrete_result},
                state,agents,observer,activity?&activity->agents:nullptr);
            return;
        }
        const std::array arguments{island.identity};call(owner.identity,arguments,listener.identity,4,state);
    };
    activation.body_activated=[&](auto listener,auto owner,auto body,auto&state){
        if(owner.identity==0x2816a0u){
            NativeTu15ActivationBodyServices services;
            services.publish_body_update=[&](auto datum,auto&context){
                // Reuse ordinary DATA storage only after policy/bounds work
                // has returned. Positive creation owners remain explicitly
                // unbound until their actual native closure is qualified.
                NativeTu15ConstraintRuntimeHandleServices lifetime;
                lifetime.publish_zero_tagged_value=[&](auto reference,auto context,auto scalar,auto&state){
                    native_tu15_publish_constraint_tag_value(memory,reference,context,scalar,state,
                        observer,activity?&activity->constraint_tag_values:nullptr);
                };
                lifetime.destroy_handle=[&](auto datum,auto&state){
                    NativeTu15ConstraintHandleDestructionServices destruction;
                    destruction.tagged_values.publish_zero_tagged_value=lifetime.publish_zero_tagged_value;
                    native_tu15_destroy_constraint_runtime_handle(memory,datum,state,destruction,
                        observer,activity?&activity->constraint_handle_destruction:nullptr);
                };
                NativeTu15ConstraintReferenceReleaseServices reference;
                reference.remove_runtime_constraint=[&](auto instance,auto datum,auto slots,auto&fp){
                    const auto word=[&](std::uint32_t p,unsigned width=4){
                        std::array<std::byte,4>bytes{};memory.read(p,std::span(bytes).first(width));
                        std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
                        return value;
                    };
                    if(scratch.shape_scratch_bytes<512u){
                        if(activity)++activity->unavailable_constraint_runtime_storage;
                        throw std::runtime_error("Native TU1.5 constraint runtime update requires512 caller-owned DATA bytes");
                    }
                    // Only the positively inventoried counter-only profile
                    // is currently admitted by this production binding.
                    // Other native parent branches remain explicit refusals
                    // until source-authenticated positive inputs qualify them.
                    const auto kind=word(instance+0xcu,1);
                    if(word(instance+0x11u,1)||kind!=1u||word(instance+0x30u)!=0xffffffffu||
                       word(instance+0x34u)!=0xffffffffu||word(instance+0x38u)!=0xffffffffu||word(instance+0x3cu)!=0xffffffffu){
                        if(activity)++activity->unqualified_constraint_runtime_profiles;
                        throw std::runtime_error("Native TU1.5 constraint runtime update requires its positive state/effect/frame qualification");
                    }
                    NativeTu15ConstraintRuntimeUpdateServices runtime;
                    runtime.handles=lifetime;
                    runtime.select_effects=[&](auto request,auto&state){
                        native_tu15_select_constraint_effects(memory,request,
                            {scratch.shape_scratch_first+416u,scratch.shape_scratch_first+432u,scratch.shape_scratch_first+440u},
                            state,observer,activity?&activity->constraint_effect_selector:nullptr);
                    };
                    native_tu15_update_constraint_runtime(memory,instance,datum,slots,
                        {scratch.shape_scratch_first+192u,{scratch.shape_scratch_first+416u,scratch.shape_scratch_first+464u}},
                        fp,depth,runtime,observer,activity?&activity->constraint_runtime_update:nullptr);
                };
                NativeTu15ModelConstraintServices constraints;
                constraints.find_existing=[&](auto component,auto request,auto flags,auto&fp){
                    return native_tu15_find_model_constraint(memory,component,request,flags,fp,
                        activity?&activity->constraint_lookup:nullptr);
                };
                constraints.allow_creation=[&](auto component,auto slot,auto extent,auto&fp){
                    NativeTu15ModelConstraintCapacityServices capacity;
                    capacity.remove_constraint=[&](auto constraint,auto&state){
                        // Remaining child-link/22FEA0 lifetime owners count
                        // and throw. No admitted native parent replays an old owner.
                        NativeTu15ModelConstraintRemovalServices removal;
                        removal.reference=reference;
                        removal.release_runtime_handle=[&](auto handle,auto deleting,auto&fp){
                            native_tu15_release_constraint_runtime_handle(memory,handle,deleting,fp,
                                lifetime,observer,activity?&activity->constraint_runtime_handles:nullptr);
                        };
                        native_tu15_remove_model_constraint(memory,constraint,state,depth,
                            removal,observer,activity?&activity->constraint_removal:nullptr);
                    };
                    return native_tu15_admit_model_constraint(memory,component,slot,extent,fp,
                        capacity,observer,activity?&activity->constraint_capacity:nullptr);
                };
                constraints.create=[&](auto request,auto flags,auto&fp){
                    if(scratch.shape_scratch_bytes<192u){
                        if(activity)++activity->unavailable_constraint_creation_storage;
                        throw std::runtime_error("Native TU1.5 constraint creation requires192 caller-owned request/frame DATA bytes");
                    }
                    NativeTu15ModelConstraintCreationServices creation;
                    creation.reference=reference;
                    return native_tu15_create_model_constraint(memory,request,flags,
                        {scratch.shape_scratch_first+64u,scratch.shape_scratch_first+128u},fp,depth,
                        creation,observer,activity?&activity->constraint_creation:nullptr);
                };
                native_tu15_update_body_constraints(memory,datum,{{scratch.shape_scratch_first}},
                    context,constraints,observer,activity?&activity->body_constraints:nullptr);
            };
            native_tu15_body_activation_listener(memory,body,state,services,observer,
                activity?&activity->services:nullptr);
            return;
        }
        const std::array arguments{body.identity};call(owner.identity,arguments,listener.identity,4,state);
    };
    activation.destroy_body=[&](auto body,auto owner,auto disposing,auto&state){
        const std::array arguments{disposing};call(owner.identity,arguments,body.identity,4,state);
    };
    use(activation);
}
} // namespace
void native_tu15_set_object_velocities_with_residuals(SourcePhysicsContactMemory&memory,
 std::uint32_t object,const Vec3*linear,const Vec3*angular,NativeTu15PhysicsFrameFp&fp,
 unsigned depth,NativeTu15ObjectActivationScratch scratch,
 const NativeTu15ObjectActivationResidualCall&invoke,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ObjectActivationBindingActivity*activity){
    if(activity)++activity->setters;
    with_activation_services(memory,depth,scratch,invoke,observer,activity,nullptr,[&](const auto&activation){
        NativeTu15ObjectActivationContext context{fp,activation,observer,activity?&activity->activation:nullptr,depth};
        native_tu15_set_object_velocities(memory,object,linear,angular,context);
    });
}
void native_tu15_activate_island_with_residuals(SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland island,NativeTu15PhysicsFrameFp&fp,
 unsigned depth,NativeTu15ObjectActivationScratch scratch,
 const NativeTu15ObjectActivationResidualCall&invoke,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ObjectActivationBindingActivity*activity){
    with_activation_services(memory,depth,scratch,invoke,observer,activity,nullptr,[&](const auto&activation){
        native_tu15_activate_physics_island(memory,world,island,fp,activation,observer,
            activity?&activity->activation:nullptr);
    });
}
void native_tu15_activate_island_with_native_services(SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland island,NativeTu15PhysicsFrameFp&fp,
 unsigned depth,NativeTu15ObjectActivationScratch scratch,
 const NativeTu15ObjectActivationNativeServices&native,
 const NativeTu15ObjectActivationResidualCall&invoke,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ObjectActivationBindingActivity*activity){
    with_activation_services(memory,depth,scratch,invoke,observer,activity,&native,[&](const auto&activation){
        native_tu15_activate_physics_island(memory,world,island,fp,activation,observer,
            activity?&activity->activation:nullptr);
    });
}
void native_tu15_activate_body_with_native_services(SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsBody body,NativeTu15PhysicsFrameFp&fp,unsigned depth,
 NativeTu15ObjectActivationScratch scratch,const NativeTu15ObjectActivationNativeServices&native,
 const NativeTu15ObjectActivationResidualCall&invoke,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ObjectActivationBindingActivity*activity){
    with_activation_services(memory,depth,scratch,invoke,observer,activity,&native,[&](const auto&activation){
        native_tu15_activate_physics_body(memory,body,fp,activation,observer,
            activity?&activity->activation:nullptr);
    });
}
} // namespace halo2
