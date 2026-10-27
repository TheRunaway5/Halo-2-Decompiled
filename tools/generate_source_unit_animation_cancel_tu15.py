"""Lower the complete native cancellation callees reached by EB0A0."""
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
parser.add_argument('--output', type=Path, default=root / 'src/gameplay/units/source_unit_animation_cancel_tu15_generated.cpp')
parser.add_argument('--manifest', type=Path, default=root / 'tools/source_unit_animation_cancel_tu15_generated.json')
args = parser.parse_args()
assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
image = Xbe(NEW_DEFAULT)
code = _Code(image, '.text')
call = code.at(0xeb0f8)
assert call.bytes.hex() == 'e893b60200' and call.operands[0].imm == 0x116790
entries = {0x105420, 0x116790}
follow_on = code.at(0xeb137)
assert follow_on.bytes.hex() == 'e8e4a20100' and follow_on.operands[0].imm == 0x105420
functions = {entry: _trace(code, entry, entries) for entry in sorted(entries)}
assert sorted(functions[0x116790].code)[-1] == 0x11691c
assert sorted(functions[0x105420].code)[-1] == 0x105453
for function in functions.values():
    assert code.at(max(function.code)).mnemonic == 'ret' and not function.tables and not function.tail_jumps
sites = sorted(set().union(*(set(function.code) for function in functions.values())))
external = {code.at(p).operands[0].imm for p in sites if code.at(p).mnemonic == 'call'}
assert external == {0xbc380, 0x111d10, 0x111b10, 0x10a980}
# These complete native callees belong to the existing live-support owner.
# Keep their actual ABI/service dispatch instead of duplicating their bodies.
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
with tempfile.TemporaryDirectory(prefix='halo2-unit-animation-cancel-',
                                 dir=os.environ.get('HALO2_TEST_SCRATCH')) as temporary:
    temporary = Path(temporary)
    correspondence = temporary / 'tu15-bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2) + '\n')
    additions = temporary / 'entries.json'
    additions.write_text(json.dumps({'image_sha256': NEW_PIN, 'entries': sorted(entries)}) + '\n')
    generate(argparse.Namespace(output=args.output, manifest=args.manifest,
        correspondence=correspondence, additional_entries=additions), paired)
source = args.output.read_text().replace(
    'tools/generate_physics_live_support_tu15.py', 'tools/generate_source_unit_animation_cancel_tu15.py'
).replace('source_run_physics_live_support_tu15', 'source_run_unit_animation_cancel_tu15'
).replace('source_physics_live_support_tu15_owns_entry', 'source_unit_animation_cancel_tu15_owns_entry'
).replace('#include "halo2/physics_live_support_tu15.hpp"', '#include "halo2/source_unit_animation_cancel_tu15.hpp"')
args.output.write_text(source)
manifest = json.loads(args.manifest.read_text())
manifest.update({
    'generated_cpp_sha256': hashlib.sha256(source.encode()).hexdigest(),
    'original_sha256': NEW_PIN, 'later_original_sha256': NEW_PIN,
    'reached_native_call': {'site': '000eb0f8', 'target': '00116790',
                            'return': '000eb0fd', 'instruction_hex': call.bytes.hex()},
    'static_caller_closure': {'site': '000eb137', 'target': '00105420',
                              'return': '000eb13c', 'instruction_hex': follow_on.bytes.hex()},
    'existing_native_callees': [f'{p:08x}' for p in sorted(external)],
    'boundary': 'Complete English TU1.5 static native body; external owners retain the actual source ABI. No runtime opcode decoder or emulation.',
})
args.manifest.write_text(json.dumps(manifest, indent=2) + '\n')
print(json.dumps({'entries': [hex(entry) for entry in sorted(entries)], 'instructions': len(sites),
                  'existing_native_callees': manifest['existing_native_callees']}))
