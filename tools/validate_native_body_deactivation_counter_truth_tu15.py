#!/usr/bin/env python3
"""Compile exact native body-sleep Getter/JSON/gate transport and negatives."""
import inspect
import validate_native_pair_creation_queue_counter_truth_tu15 as parent
from prepare_native_discrete_pair_world_binding_tu15 import once
from prepare_native_body_deactivation_world_binding_tu15 import FIELDS


def main():
    source = inspect.getsource(parent.main)
    source = source.replace('pair_creation_queues', 'body_deactivation')
    source = source.replace('HALO2_REQUIRE_NATIVE_PAIR_CREATION_QUEUES', 'HALO2_REQUIRE_NATIVE_BODY_DEACTIVATION')
    source = source.replace('HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP', 'HALO2_REQUIRE_NATIVE_PAIR_CREATION_QUEUES')
    source = source.replace('queue_counter_truth', 'body_deactivation_counter_truth')
    source = once(source, 'NativeTu15PhysicsFrameExecutionStats result;\n',
                  'NativeTu15PhysicsFrameExecutionStats result;auto&world_integration=impl.native_physics_integrations;\n')
    source = once(source,
        'NativeTu15PhysicsFrameExecutionStats good;good.native_pair_merge_queue_attempts=7;good.native_pair_merge_queue_calls=7;good.native_pair_defer_queue_attempts=11;good.native_pair_defer_queue_calls=11;check(test_gate(good));',
        'NativeTu15PhysicsFrameExecutionStats good;good.native_body_deactivation_attempts=7;good.native_body_deactivation_calls=7;good.native_body_deactivation_property_queries=7;check(test_gate(good));')
    source = once(source,
        "['native_pair_merge_queue_calls=0','native_pair_defer_queue_calls=0','native_pair_merge_queue_attempts=8','native_pair_defer_queue_attempts=12','native_pair_merge_queue_refusals=1','native_pair_defer_queue_refusals=1','residual_owner_calls[0x317200u]=1','residual_owner_calls[0x320da0u]=1']",
        "['native_body_deactivation_calls=0','native_body_deactivation_attempts=8','native_body_deactivation_refusals=1','native_body_deactivation_unbound_services=1','native_body_deactivation_property_queries=0','residual_owner_calls[0x2815f0u]=1','native_body_deactivation_attempts=0','native_body_deactivation_calls=8']")
    source = source.replace('Exact6getter', 'Exact8getter')
    namespace = dict(parent.__dict__)
    namespace['FIELDS'] = {'native_body_deactivation_' + name: member for name, member in FIELDS.items()}
    exec(source, namespace)
    return namespace['main']()


if __name__ == '__main__':
    raise SystemExit(main())
