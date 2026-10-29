"""Lower the complete TU1.5 Banshee evasion event producer B3350."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile

sys.dont_write_bytecode = True
root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'tools'))
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=root / 'src/gameplay/vehicles/source_vehicle_evasion_tu15_generated.cpp')
parser.add_argument('--manifest', type=Path, default=root / 'tools/source_vehicle_evasion_tu15_generated.json')
args = parser.parse_args()
assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
image = Xbe(NEW_DEFAULT)
code = _Code(image, '.text')
call = code.at(0xf6de1)
assert call.bytes.hex() == 'e86ac5fbff' and call.operands[0].imm == 0xb3350
entries = {0xb3350}
functions = {entry: _trace(code, entry, entries) for entry in sorted(entries)}
assert sorted(functions[0xb3350].code)[-1] == 0xb33ad
for function in functions.values():
    assert code.at(max(function.code)).mnemonic == 'ret' and not function.tables and not function.tail_jumps
sites = sorted(set().union(*(set(function.code) for function in functions.values())))
external = {code.at(p).operands[0].imm for p in sites if code.at(p).mnemonic == 'call'}
assert external == {0xb18b0, 0xb1900, 0xb8050}
# These complete native callees already own player admission and simulation
# event delivery. Preserve their full source ABI rather than supplying results.
support = (root / 'src/physics/physics_live_support_tu15_generated.cpp').read_text()
for target in external:
    assert f'static void run_{target:x}(WorldState &s) {{' in support
    assert f'case 0x{target:x}u:run_{target:x}(s);break;' in support
paired = {
    'image': 'English TU1.5', 'original_sha256': NEW_PIN,
    'entries': {f'{entry:08x}': entry for entry in sorted(entries)},
    'instruction_map': {f'{p:08x}': p for p in sites},
    'address_map': {f'{entry:08x}': entry for entry in sorted(entries)},
    'body_sites': {f'{entry:08x}': sorted(function.code) for entry, function in functions.items()},
    'routines': [{'entry': entry, 'tables': [], 'byte_tables': []} for entry in sorted(entries)],
}
with tempfile.TemporaryDirectory(prefix='halo2-vehicle-evasion-',
                                 dir=os.environ.get('HALO2_TEST_SCRATCH')) as temporary:
    temporary = Path(temporary)
    correspondence = temporary / 'tu15-bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2) + '\n')
    additions = temporary / 'entries.json'
    additions.write_text(json.dumps({'image_sha256': NEW_PIN, 'entries': sorted(entries)}) + '\n')
    generate(argparse.Namespace(output=args.output, manifest=args.manifest,
        correspondence=correspondence, additional_entries=additions), paired)
source = args.output.read_text().replace(
    'tools/generate_physics_live_support_tu15.py', 'tools/generate_source_vehicle_evasion_tu15.py'
).replace('source_run_physics_live_support_tu15', 'source_run_vehicle_evasion_tu15'
).replace('source_physics_live_support_tu15_owns_entry', 'source_vehicle_evasion_tu15_owns_entry'
).replace('#include "halo2/physics_live_support_tu15.hpp"', '#include "halo2/source_vehicle_evasion_tu15.hpp"')
args.output.write_text(source)
manifest = json.loads(args.manifest.read_text())
manifest.update({
    'generated_cpp_sha256': hashlib.sha256(source.encode()).hexdigest(),
    'original_sha256': NEW_PIN, 'later_original_sha256': NEW_PIN,
    'reached_native_call': {'site': '000f6de1', 'target': '000b3350',
                            'return': '000f6de6', 'instruction_hex': call.bytes.hex()},
    'existing_native_callees': [f'{p:08x}' for p in sorted(external)],
    'boundary': 'Complete English TU1.5 evasion event producer; admission and event delivery retain their original native ABI. No fabricated event or physics result, runtime opcode decoder or emulation.',
})
args.manifest.write_text(json.dumps(manifest, indent=2) + '\n')
print(json.dumps({'entries': [hex(entry) for entry in sorted(entries)], 'instructions': len(sites),
                  'existing_native_callees': manifest['existing_native_callees']}))
