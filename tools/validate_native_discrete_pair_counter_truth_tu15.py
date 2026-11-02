#!/usr/bin/env python3
"""Compile exact native discrete Getter, both JSON paths and positive gate."""
import inspect
import validate_native_pair_creation_queue_counter_truth_tu15 as parent


def once(text, old, new):
    assert text.count(old) == 1, old[:120]
    return text.replace(old, new)


def main():
    source = inspect.getsource(parent.main)
    source = source.replace('pair_creation_queues', 'pair_discrete_query')
    source = source.replace('HALO2_REQUIRE_NATIVE_PAIR_CREATION_QUEUES', 'HALO2_REQUIRE_NATIVE_PAIR_DISCRETE_QUERY')
    source = source.replace('HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP', 'HALO2_REQUIRE_NATIVE_PAIR_ISLAND_WAKE')
    source = source.replace('queue_counter_truth', 'discrete_counter_truth')
    source = once(source,
        'NativeTu15PhysicsFrameExecutionStats result;\n',
        'NativeTu15PhysicsFrameExecutionStats result;auto&world_integration=impl.native_physics_integrations;\n')
    source = once(source,
        'NativeTu15PhysicsFrameExecutionStats good;good.native_pair_merge_queue_attempts=7;good.native_pair_merge_queue_calls=7;good.native_pair_defer_queue_attempts=11;good.native_pair_defer_queue_calls=11;check(test_gate(good));',
        'NativeTu15PhysicsFrameExecutionStats good;good.native_pair_discrete_attempts=7;good.native_pair_discrete_calls=7;good.native_pair_discrete_payload_filters=7;check(test_gate(good));')
    source = once(source,
        "['native_pair_merge_queue_calls=0','native_pair_defer_queue_calls=0','native_pair_merge_queue_attempts=8','native_pair_defer_queue_attempts=12','native_pair_merge_queue_refusals=1','native_pair_defer_queue_refusals=1','residual_owner_calls[0x317200u]=1','residual_owner_calls[0x320da0u]=1']",
        "['native_pair_discrete_calls=0','native_pair_discrete_attempts=8','native_pair_discrete_refusals=1','native_pair_discrete_unbound_filters=1','native_pair_discrete_unbound_motion_types=1','native_pair_discrete_payload_filters=0','residual_owner_calls[0x281530u]=1','native_pair_discrete_attempts=0']")
    source = source.replace('Exact6getter', 'Exact7getter')
    fields = dict(attempts='attempts', calls='completed', refusals='refusals',
                  unbound_filters='unbound_filters', unbound_motion_types='unbound_motion_types',
                  payload_filters='payload.calls', motion_queries='payload.motion_queries')
    namespace = dict(parent.__dict__)
    namespace['FIELDS'] = {'native_pair_discrete_' + name: member for name, member in fields.items()}
    exec(source, namespace)
    return namespace['main']()


if __name__ == '__main__':
    raise SystemExit(main())
