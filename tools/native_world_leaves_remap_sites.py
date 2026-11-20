"""The phantom/world composition's canonical-PC remap tables for the group H
harnesses' remap modes (tests/native_tu15_world_leaves_remap.hpp).

write() extracts the composition's own table (world_sites for the physics
world unit, vehicle_sites for the vehicle update unit) from
src/physics/physics_phantom_world_tu15_generated.cpp into remap_sites.inc;
run_modes() runs a harness in the three remap modes.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
PHANTOM = ROOT / 'src/physics/physics_phantom_world_tu15_generated.cpp'
HARNESS = ROOT / 'tests/native_tu15_world_leaves_remap.hpp'
SOURCES = [PHANTOM, HARNESS, Path(__file__).resolve()]
MODES = ('remap', 'remap-census', 'remap-verify')


def write(directory, table='world_sites'):
    found = re.search(r'constexpr std::array<Binding,(\d+)> ' + table + r'=\{\{\n(.*?)\}\};', PHANTOM.read_text(), re.S)
    if not found:
        raise ValueError(f'Composition table {table} not found')
    rows = re.findall(r'\{0x[0-9a-f]+u,0x[0-9a-f]+u\},', found[2])
    if len(rows) != int(found[1]):
        raise ValueError(f'Composition table {table} has {len(rows)} rows, not {found[1]}')
    (Path(directory) / 'remap_sites.inc').write_text('\n'.join(rows) + '\n')


def run_modes(run, binary, cases):
    """{mode: result} for the remap modes; each must compare cases."""
    results = {mode: run(binary, cases, mode) for mode in MODES}
    if any(result['compared'] == 0 for result in results.values()):
        raise ValueError(f'A remap mode compared nothing: {results}')
    return results
