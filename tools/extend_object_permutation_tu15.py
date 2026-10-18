#!/usr/bin/env python3
"""Add complete authenticated permutation owners, preserving retained bodies.

Generate object functions to a separate output first, then apply this checked
extension. It reuses the module's existing runtime helpers; every retained
shader-function body stays byte-identical. Reapplication verifies idempotence.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

NEW = {"bc440", "170030", "170080"}
RETAINED = {"10fa30", "10fa80", "10fa90", "10fb60", "10fb70", "10fc30", "10fd50"}


def digest(value):
    return hashlib.sha256(value).hexdigest()


def bodies(text):
    return {m[1]: text[m.start():text.index("\n}", m.start()) + 2]
            for m in re.finditer(r"static void run_([0-9a-f]+)\(WorldState &s\) \{", text)}


def extend(retained, fresh):
    old, generated = bodies(retained), bodies(fresh)
    if set(generated) != RETAINED | NEW or not RETAINED <= set(old) <= RETAINED | NEW:
        raise ValueError("Unexpected original object-function ownership")
    if set(old) & NEW:
        if set(old) != RETAINED | NEW or any(old[k] != generated[k] for k in NEW):
            raise ValueError("Partial or changed permutation extension")
        return retained
    result = retained
    anchor = "static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){"
    if result.count(anchor) != 1:
        raise ValueError("Missing original dispatcher")
    declarations = "".join(f"static void run_{k}(WorldState &);\n" for k in sorted(NEW))
    calls = "".join(f"if(target==0x{k}u){{run_{k}(s);return;}}" for k in sorted(NEW))
    result = result.replace(anchor, declarations + anchor + calls, 1)
    anchor = "} // namespace\nvoid source_run_object_functions_tu15"
    if result.count(anchor) != 1:
        raise ValueError("Missing original module boundary")
    result = result.replace(anchor, "\n".join(generated[k] for k in sorted(NEW)) + "\n" + anchor, 1)
    anchor = "case 0x10fa30u:run_10fa30(s);break;"
    if result.count(anchor) != 1:
        raise ValueError("Missing original entry switch")
    result = result.replace(anchor, "\n".join(f"case 0x{k}u:run_{k}(s);break;" for k in sorted(NEW)) + "\n" + anchor, 1)
    anchor = "bool source_object_functions_tu15_owns_entry(std::uint32_t p) {switch(p){"
    if result.count(anchor) != 1:
        raise ValueError("Missing original ownership switch")
    result = result.replace(anchor, anchor + "".join(f"case 0x{k}u:" for k in sorted(NEW)), 1)
    updated = bodies(result)
    if set(updated) != RETAINED | NEW or any(updated[k] != old[k] for k in RETAINED):
        raise ValueError("Retained object-function body changed")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "generated", "manifest", "output", "report"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    if digest(NEW_DEFAULT.read_bytes()) != NEW_PIN:
        raise ValueError("Expected authenticated English TU1.5")
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, ".text")
    if image.read(0x457374, 4).hex() != "d0c32a00":
        raise ValueError("Original evaluator identity differs")
    for site, target in ((0x2ac409, 0xbc440), (0xbc48f, 0x170030), (0xbc51d, 0x170080)):
        instruction = code.at(site)
        if instruction.mnemonic != "call" or instruction.operands[0].imm != target:
            raise ValueError("Original permutation call differs")
    before, fresh = args.source.read_text(), args.generated.read_text()
    manifest = json.loads(args.manifest.read_text())
    if manifest["later_original_sha256"] != NEW_PIN or manifest["generated_cpp_sha256"] != digest(fresh.encode()):
        raise ValueError("Generated module provenance differs")
    after = extend(before, fresh)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(after)
    report = dict(original_sha256=NEW_PIN, before_sha256=digest(before.encode()),
                  after_sha256=digest(after.encode()), generator_sha256=digest(Path(__file__).read_bytes()),
                  full_generation_sha256=digest(fresh.encode()), idempotent=before == after,
                  retained_body_sha256={k: digest(v.encode()) for k, v in bodies(before).items() if k in RETAINED},
                  added_body_sha256={k: digest(bodies(after)[k].encode()) for k in sorted(NEW)})
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS three complete permutation owners; seven retained bodies unchanged")


if __name__ == "__main__":
    main()
