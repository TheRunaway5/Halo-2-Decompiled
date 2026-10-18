"""Compile the complete original TU1.5 light constant builder, with byte evidence."""
import argparse
import json
import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--directory", type=Path, required=True)
parser.add_argument("--output", type=Path, default=ROOT / "src/pstencil/native_light_constants_tu15_generated.cpp")
args = parser.parse_args()
args.directory.mkdir(parents=True, exist_ok=True)
assert sha(NEW_DEFAULT) == NEW_PIN
image = Xbe(NEW_DEFAULT)
code = _Code(image, ".text")
entries=(0x49790,)
instructions,addresses,routines,body_sites={},{},[],{}
for entry in entries:
    body=_trace(code,entry,set(entries));assert body.code and not body.tables
    body_sites[f"{entry:08x}"]=sorted(body.code)
    routines.append(dict(entry=entry,tables=[],byte_tables=[]))
    for pc in sorted(body.code):
        ins=code.at(pc);assert image.read(pc,ins.size)==bytes(ins.bytes)
        instructions[f"{pc:08x}"]=pc;addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
        for op in ins.operands:
            if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
            elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
paired = dict(image="English TU1.5", original_sha256=NEW_PIN,
    entries={f"{entry:08x}":entry for entry in entries}, body_sites=body_sites,
    instruction_map=instructions, address_map={f"{p:08x}": q for p, q in addresses.items()},
    routines=routines)
correspondence = args.directory / "bodies.json"
correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + "\n")
manifest = args.directory / "generation.json"
generate(SimpleNamespace(output=args.output, manifest=manifest, correspondence=correspondence,
    additional_entries=Path(__file__)), paired)
source = args.output.read_text().replace("tools/generate_physics_live_support_tu15.py",
    "tools/pstencil/generate_native_light_constants_tu15.py").replace(
    '"physics_world_runtime_internal.hpp"', '"../physics_world_runtime_internal.hpp"').replace(
    "halo2/physics_live_support_tu15.hpp", "halo2/pstencil/native_light_constants_tu15.hpp").replace(
    "source_run_physics_live_support_tu15", "pstencil_run_native_light_constants_tu15").replace(
    "source_physics_live_support_tu15_owns_entry", "pstencil_native_light_constants_tu15_owns_entry")
args.output.write_text(source)
report = json.loads(manifest.read_text())
report.update(generated_cpp_sha256=sha(args.output), module_generator_sha256=sha(Path(__file__)),
    source_caller="4A1D0 -> 49790", complete_instructions=len(instructions),
    scope="Whole original sphere/frustum light constant builder. Actual native shape/frame and completed source fade inputs; no supplied uniform output.")
manifest.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
print(json.dumps(dict(complete_instructions=len(instructions), generated_sha256=sha(args.output))))
