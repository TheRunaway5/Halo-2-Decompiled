"""Oracle cases for factory track "nodes" (template).

register(v) receives tools/validate_native_campaign_script_builtins_tu15.py as
a module. Add, per owner:
  v.KERNELS[name] = ([argument type], result kind)   kinds: void boolean short long
  v.ROUTINES[name] = dict(entry=..., regs={'eax': i}, stack=[j, ...])
      (registers and stack words take the i-th/j-th argument word)
  v.TRACK_ARGUMENTS[argument type] = fn(name, world, rng) -> argument words
  v.TRACK_COUNTS[name] = number of cases
Fixtures write the declared world banks: world.data/data2/data3/ai, the
optional world.factory (FACTORY) and world.track (TRACK, 0x10000) banks, and
world.extra[address] = bytearray for original .data/.bss blocks.
"""
import struct


def register(v):
    v.KERNELS['object_has_animation'] = (['nodes_object'], 'boolean')
    v.ROUTINES['object_has_animation'] = dict(entry=0xc15d0, regs={'eax': 0})
    v.KERNELS['object_node_matrix'] = (['nodes_matrix'], 'long')
    v.ROUTINES['object_node_matrix'] = dict(entry=0xbac00, regs={'eax': 1}, stack=[0])

    def object_case(name, world, rng):
        n = rng.randrange(v.ROWS - 1)
        struct.pack_into('<h', world.data, world.body(n) + 0x12a - v.DATA, rng.choice([-1, -1, 0, 0x40, -2]))
        return [world.handles[n]]

    def matrix_case(name, world, rng):
        n = rng.randrange(v.ROWS - 1)
        struct.pack_into('<h', world.data, world.body(n) + 0x116 - v.DATA, rng.choice([0, 0x200, -0x10, 0x7ff0]))
        node = rng.choice([0, 1, 5, 0xffff, 0x8000, 0x12345])
        return [node, world.handles[n]]

    v.TRACK_ARGUMENTS['nodes_object'] = object_case
    v.TRACK_ARGUMENTS['nodes_matrix'] = matrix_case
    v.TRACK_COUNTS['object_has_animation'] = 100
    v.TRACK_COUNTS['object_node_matrix'] = 100
