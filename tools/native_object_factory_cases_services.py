"""Oracle cases for factory track "services": the script host's evaluator tails
over the unitactions track's vehicle exit cores.

register(v) receives tools/validate_native_campaign_script_builtins_tu15.py as
a module (see tools/native_object_factory_cases_nodes.py for the protocol).

The HS builtins vehicle_unload (2ADE60 -> 11D990), unit_exit_vehicle (2ADC30 ->
11D670) and ai_vehicle_exit (2AFE00 -> 27EAB0 with EBX NONE) are authenticated
by their descriptors and entered after their argument evaluation, like every
builtin kernel. The harness runs the services track's script_* evaluators: the
host's own code (src/campaign/scripting/native_campaign_script_host_tu15.cpp
registers them), including ai_vehicle_exit's production 27CAA0/27CB30 walk
over squad, encounter group, actor and starting location references.

Fixtures are the unitactions track's (borrowed read only through
tools/native_object_factory_track_fixtures.py). That track's refused callees
are the stops: the original's run ends at their entry (stop_before_entries)
and their subtrees stay out of these evaluators' closures.
"""
from native_object_factory_track_fixtures import borrow

NONE = 0xffffffff
# HS name: (TU1.5 argument types, result kind, cases).
HOST = {'vehicle_unload': ([50, 12], 'short', 400),
        'unit_exit_vehicle': ([51, 7], 'void', 300),
        'ai_vehicle_exit': ([19], 'void', 500)}


def register(v):
    ua, ua_cases = borrow(v, 'unitactions')
    table = ua.action_table(v)
    stops = ua.REFUSED | ({c for row in table for c in row[1:] if c} - ua.NATIVE_CALLBACKS)
    opaque = sorted(stops | ua.UNREACHED)
    builders = ua_cases.TRACK_ARGUMENTS
    for name, (types, kind, count) in HOST.items():
        v.KERNELS[name] = (types, kind)
        v.TRACK_COUNTS[name] = count

    def reference(world, rng, squad):
        """An AI reference: the builder's squad, or any tag (as the
        validator's other AI builtins draw them)."""
        tag = rng.choice([0, 0, 0, 1, 1, 2, 2, 3])
        if tag == 0:
            value = squad if rng.random() < .7 else rng.randrange(v.SQUADS)
        elif tag == 1:
            value = (1 << 30) | rng.randrange(v.AI_GROUPS)
        elif tag == 2:
            value = (2 << 30) | rng.randrange(v.ACTORS) | (rng.getrandbits(14) << 16 if rng.random() < .3 else 0)
        else:
            value = (3 << 30) | (rng.choice([0, 1, 2, 3, 4, 5, 6, 0x3fff]) << 16) | rng.choice([0, 1, 2, 3, 0xffff, 0x8000])
        return NONE if rng.random() < .04 else value

    def host_case(name, world, rng):
        if name == 'vehicle_unload':
            mapping, vehicle = builders['ua_vehicle_unload'](name, world, rng)
            return [vehicle, mapping]
        if name == 'unit_exit_vehicle':
            return builders['ua_exit_vehicle'](name, world, rng)
        squad, _ = builders['ua_ai_vehicle_exit'](name, world, rng)
        return [reference(world, rng, squad)]

    original_arguments = v.arguments

    def arguments(name, world, rng):
        if name in HOST:
            return host_case(name, world, rng), []
        return original_arguments(name, world, rng)
    v.arguments = arguments

    original_metadata = v.metadata

    def metadata(image, evaluators):
        # The evaluators' closures exclude the refused subtrees (the
        # validator reads closure_stops from ROUTINES; these are builtins,
        # not routines, so the entries exist only while metadata runs).
        added = [name for name in HOST if name in evaluators and name not in v.ROUTINES]
        for name in added:
            v.ROUTINES[name] = dict(closure_stops=opaque)
        try:
            return original_metadata(image, evaluators)
        finally:
            for name in added:
                del v.ROUTINES[name]
    v.metadata = metadata

    original_cases = v.cases

    def cases(seed, evaluators, tails):
        rows = original_cases(seed, evaluators, tails)
        for row in rows:
            if row['kernel'] in HOST:
                row['stop_before_entries'] = sorted(set(row['stop_before_entries']) | stops)
        return rows
    v.cases = cases
