#!/usr/bin/env python3
"""Generate native pStencil actor and weapon bindings from the May 2003 caches."""

import argparse
from collections import defaultdict
import hashlib
import json
import mmap
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
INVENTORY = ROOT / "analysis/pstencil/2003-cache-inventory"
sys.path.insert(0, str(ROOT / "analysis/pstencil/2003-presentation"))
import decode_passes as schema

TOOL_SHA256 = "043fd958e12bbacb76aadc5c589dc3f2fc2586dfec80bbd4b6fd8d8b426b66d5"
PREFIXES = ("objects\\characters\\", "objects\\weapons\\")
# These rows already have independent source proofs and are compiled separately.
EXISTING = {
    "objects\\characters\\masterchief\\shaders\\masterchief",
    "objects\\characters\\masterchief\\shaders\\masterchief_visor",
    "objects\\characters\\masterchief\\fp\\shaders\\fp_arms",
    "objects\\weapons\\grenade\\frag_grenade\\shaders\\frag_metal",
    "objects\\weapons\\grenade\\frag_grenade\\shaders\\frag_plastic",
    "objects\\weapons\\grenade\\plasma_grenade\\shaders\\pg_body",
    *(f"objects\\characters\\elite\\shaders\\{part}"
      for part in ("arms", "head", "helmet", "legs", "teeth", "torso")),
}


def offset(definition, name):
    return next(field["offset"] for field in schema.schema(definition)["fields"]
                if field["name"] == name)


class ResidentCache:
    def __init__(self, data, metadata):
        self.data = data
        self.magic = metadata["pointer_bias"]
        self.tags = {int(tag["datum"], 16): tag for tag in metadata["tags"]}

    def word(self, at):
        if not 0 <= at <= len(self.data) - 4:
            raise ValueError("resident field outside cache")
        return struct.unpack_from("<I", self.data, at)[0]

    def block(self, at, definition):
        count = self.word(at)
        stride = schema.schema(definition)["size"]
        if not count:
            return range(0)
        start = self.word(at + 4) - self.magic
        if count > 8192 or not 0 <= start <= len(self.data) - count * stride:
            raise ValueError("resident block outside cache")
        return range(start, start + count * stride, stride)

    def selected_pass(self, layer):
        passes = self.block(layer + offset(0x811218, "passes*"), 0x811190)
        if not passes:
            return None
        if len(passes) != 1:
            raise ValueError("multiple selected passes")
        selected = passes[0]
        ref = self.tags.get(self.word(selected + offset(0x811190, "pass*") + 12))
        if not ref or ref["group"] != "spas" or not ref["payload_file_offset"]:
            raise ValueError("missing resident pass")
        definitions = self.block(int(ref["payload_file_offset"], 16) + 0x30, 0x811760)
        implementations = self.block(selected + offset(0x811190, "implementations*"), 0x8110f8)
        if len(definitions) != 1 or len(implementations) != 1:
            raise ValueError("multiple pass definitions or implementations")
        selector = definitions[0] + offset(0x811760, "hack pixel shader index*")
        return struct.unpack_from("<h", self.data, selector)[0], implementations[0], selector

    def response(self, model):
        layers = self.block(model + offset(0x8112d0, "runtime layers*"), 0x811218)
        passes = [self.selected_pass(layers[i]) if i < len(layers) else None
                  for i in (2, 16, 17)]
        if not any(passes):
            return None
        normal, utility, dependent = passes
        if ((normal and normal[0] != 1) or not utility or utility[0] not in (6, 9, 10)
                or not dependent or dependent[0] != 7):
            raise ValueError("different normal, utility or dependent program")
        colors = {}
        positions = {}
        for at in self.block(dependent[1] + offset(0x8110f8, "VS constants*"), 0x810dd8):
            bank = self.data[at + offset(0x810dd8, "register bank*")]
            register = self.data[at + offset(0x810dd8, "register index*")]
            if bank != 0 or register not in (10, 11):
                continue
            if register in colors or self.block(at + offset(0x810dd8, "overlays*"), 0x810cd0):
                raise ValueError("animated or duplicate facing/grazing color")
            position = at + offset(0x810dd8, "x*")
            colors[register] = self.data[position:position + 16].hex()
            positions[register] = position
        if set(colors) != {10, 11}:
            raise ValueError("missing explicit facing/grazing color")
        texture = self.selected_pass(layers[4]) if normal else None
        detail = None
        if texture and texture[0] == 3:
            stages = self.block(texture[1] + offset(0x8110f8, "textures*"), 0x811040)
            if len(stages) == 4:
                binding = stages[1]
                tag = self.tags.get(self.word(binding + 12))
                if tag and tag["group"] == "bitm":
                    detail = [tag["name"], binding]
        return {
            "normal": bool(normal), "utility": utility[0], "v10": colors[10], "v11": colors[11],
            "offsets": [utility[2], dependent[2], positions[10], positions[11]], "detail": detail,
        }


def extract():
    if hashlib.sha256(schema.TOOL.read_bytes()).hexdigest() != TOOL_SHA256:
        raise ValueError("2003 tool schema differs")
    rows = defaultdict(list)
    cache_hashes = {}
    maps = json.loads((INVENTORY / "summary.json").read_text())["maps"]
    for entry in maps:
        metadata = json.loads((INVENTORY / entry["report"]).read_text())
        selected = [tag for tag in metadata["tags"] if tag["group"] == "shad"
                    and tag["name"].startswith(PREFIXES) and tag["payload_file_offset"]]
        if not selected:
            continue
        with Path(metadata["path"]).open("rb") as stream, mmap.mmap(
                stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
            digest = hashlib.sha256(data).hexdigest()
            if digest != entry["sha256"] or digest != metadata["sha256"]:
                raise ValueError(f"2003 cache differs: {metadata['path']}")
            cache_hashes[entry["name"]] = digest
            cache = ResidentCache(data, metadata)
            for tag in selected:
                address = int(tag["payload_file_offset"], 16)
                models = []
                for model in cache.block(address + 0x30, 0x8112d0):
                    try:
                        result = cache.response(model)
                    except ValueError as error:
                        result = {"error": str(error)}
                    models.append(result)
                rows[tag["name"]].append([entry["name"], address, models])
    admitted = []
    unsupported = {}
    for name, sources in sorted(rows.items()):
        models = [model for _, _, models in sources for model in models]
        responses = [model for model in models if model and "utility" in model]
        identities = {(r["utility"], r["v10"], r["v11"]) for r in responses}
        if (len(responses) != len(models) or len(identities) != 1
                or not any(r["normal"] for r in responses)):
            unsupported[name] = {
                "source_records": len(sources),
                "reasons": sorted({r["error"] for r in models if r and "error" in r}),
                "has_normal_response": any(r["normal"] for r in responses),
                "response_identities": len(identities),
            }
            continue
        utility, v10, v11 = identities.pop()
        neutral_detail = all(
            model["detail"] and model["detail"][0] == "shaders\\default_bitmaps\\bitmaps\\default_detail"
            for model in responses if model["normal"])
        admitted.append({"name": name, "utility": utility, "v10": v10, "v11": v11,
                         "neutral_detail": neutral_detail, "existing": name in EXISTING, "sources": sources})
    return {"tool_sha256": TOOL_SHA256, "caches": cache_hashes, "materials": admitted,
            "unsupported": unsupported}


def generate(report):
    materials = [row for row in report["materials"] if not row["existing"]]

    def vector(bits):
        return "{" + ", ".join(f"std::bit_cast<float>(0x{word:08x}u)"
                               for word in struct.unpack("<4I", bytes.fromhex(bits))) + "}"

    lines = [
        "// Generated by tools/pstencil/extract_actor_light_materials_2003.py.",
        f"constexpr std::array<ReferenceLightMaterial, {len(materials)}> actor_light_materials{{{{",
    ]
    for row in materials:
        lines.extend((
            f"    {{{json.dumps(row['name'])},",
            f"     {vector(row['v10'])},",
            f"     {vector(row['v11'])}, {row['utility']}u, {str(row['neutral_detail']).lower()}}},",
        ))
    return "\n".join(lines + ["}};", ""])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--include", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    report = extract()
    evidence = json.dumps(report, separators=(",", ":")) + "\n"
    for path, contents in ((args.include, generate(report)), (args.evidence, evidence)):
        if args.check:
            if path.read_text() != contents:
                raise ValueError(f"stale generated reference: {path}")
        else:
            path.write_text(contents)
    print(f"{len(report['materials'])} unanimous resident responses across "
          f"{len(report['caches'])} caches, {len(report['unsupported'])} other material paths")


if __name__ == "__main__":
    main()
