#!/usr/bin/env python3
"""Generate metadata field maps from retained, pinned Assembly plugins.

The generated runtime has no XML/repository dependency. Offsets come only from
named descriptors, never from inspecting values that look like addresses.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

PIN = "fd14ef3c46135e9422966c6a64cd926fb0bd23fb"
# phmo/coll/jmad have normalizer-owned maps. BSP resource allocation ownership
# remains the delivery planner's responsibility; this visits metadata fields.
GROUPS = """<fx> MGS2 PRTM adlg ant! bipd bitm bloc bsdt char clwd colo cont crea ctrl deca DECR devi dobc effe egor eqip flck fog foot fpch garb gldf goof hlmt hmt hudg itmc jpt! lens lifi ligh lsnd ltmp mach matg mcsr mdlg mode mpdt mulg nhdt obje phys pmov pphy proj prt3 sbsp scen scnr shad sily skin sky sncl snd! snde snmx sfx+ spas spk! ssce sslt stem styl tdtl trak trg* udlg ugh! unic vehc vehi vrtx weap weat wgit wgtz wigl""".split()
# Group names with significant trailing spaces cannot use whitespace parsing.
GROUPS = sorted(set(GROUPS) - {"fog", "hmt", "sky"} | {"fog ", "hmt ", "sky "})

def read(repo, path):
    result = subprocess.run(["git", "-C", str(repo), "show", f"{PIN}:{path}"], capture_output=True)
    return result.stdout if result.returncode == 0 else None

def generate(repo):
    nodes, fields, schemas, sources = [], [], [], []
    # Check the authority for the documented plugin fallback rather than
    # silently guessing a donor layout when a plugin is absent.
    engines_path = "src/Blamite/Formats/Engines.xml"
    engines = read(repo, engines_path)
    if engines is None:
        raise ValueError("Pinned MCC engine definition unavailable")
    engine = next((e for e in ET.fromstring(engines).iter("engine")
                   if e.get("name") == "Halo 2 MCC"), None)
    if engine is None or engine.findtext("plugins") != "Halo2MCC" or engine.findtext("fallbackPlugins") != "Halo2":
        raise ValueError("Pinned MCC plugin fallback differs")
    sources.append({"path": engines_path, "sha256": hashlib.sha256(engines).hexdigest()})
    def node(element, group, path, flavor):
        index = len(nodes)
        nodes.append(None)
        children = []
        for e in element:
            kind = e.tag.lower()
            if "offset" not in e.attrib:
                continue
            offset = int(e.attrib["offset"], 0)
            name = e.get("name", "")
            if kind == "tagblock":
                stride = int(e.attrib["elementSize"], 0)
                # Exact output strides of the existing unit normalizer,
                # established by original TU1.5 1CAF40 and 20D810 consumers.
                shape = flavor == "Mcc" and not path and (
                    group == "bipd" and offset in (0x284, 0x28c, 0x294) or
                    group == "crea" and offset in (0xf4, 0xfc, 0x104) or
                    group == "vehi" and offset == 0x2f8)
                if shape:
                    stride = 0x2a0 if group == "vehi" else (0x50 if offset in (0x28c, 0xfc) else 0x80)
                    child = node(ET.Element("normalized_shape"), group, path + (offset,), flavor)
                elif flavor == "Mcc" and group == "sbsp" and path == (0x138,) and offset == 0xb0:
                    # The existing in-place BSP normalizer retains a single
                    # 0xa0 reserved bank but writes the original 0x70 record.
                    # Its MOPP descriptor moves from +98/+9c to +64/+68.
                    stride = 0xa0
                    normalized = ET.Element("normalized_bsp_physics")
                    ET.SubElement(normalized, "dataref", name="Normalized MOPP Code Data", offset="0x64")
                    child = node(normalized, group, path + (offset,), flavor)
                else:
                    child = node(e, group, path + (offset,), flavor)
                children.append((offset, stride, child, "Block", name))
            elif kind == "dataref":
                children.append((offset, 1, 0, "Data", name))
            elif kind == "uint64" and ("pointer" in name.lower() or name == "User Data"):
                children.append((offset, 0, 0, "Runtime64", name))
            elif kind in ("uint32", "int32") and "pointer" in name.lower():
                if group == "matg" and not path and name in ("String Reference Pointer", "String Data Pointer"):
                    children.append((offset, 8 if name == "String Reference Pointer" else 1, 0,
                                     "LocaleRows" if name == "String Reference Pointer" else "LocaleData", name))
                elif "pad" not in name.lower():
                    children.append((offset, 0, 0, "Runtime32", name))
            elif kind in ("uint32", "int32") and name.endswith(" Block"):
                # Paired hidden words are not a declared element layout.
                # Refuse a nonempty unknown owner until its stride/fields have
                # a specific source-backed schema.
                children.append((offset, 0, 0, "Unknown32", name))
            elif kind in ("uint32", "int32", "uint64") and "address" in name.lower():
                if not (group == "scnr" and (
                    path == (0x210,) and offset == 8 and name == "Structure BSP Address" or
                    path == (0x238,) and offset == 0xc and name == "String Address")):
                    raise ValueError(f"Unclassified explicit address field: {group} {path} {e.attrib}")
        first = len(fields)
        fields.extend(children)
        extent = max((o + (8 if k in ("Block", "Data", "Runtime64") else
                               16 if k == "LocaleRows" else
                               12 if k == "LocaleData" else 4)
                      for o, _, _, k, _ in children), default=0)
        nodes[index] = (first, len(children), extent)
        return index
    for flavor, game in (("Mcc", "Halo2MCC"),):
        for group in GROUPS:
            # Plugin filenames trim the significant fourcc padding.
            filename = group.rstrip().replace("*", "_").replace("<", "_").replace(">", "_") + ".xml"
            path = f"src/Assembly/Plugins/{game}/{filename}"
            raw = read(repo, path)
            if raw is None and flavor == "Mcc":
                path = path.replace("Halo2MCC", "Halo2")
                raw = read(repo, path)
            if raw is None:
                continue
            root = ET.fromstring(raw)
            start = node(root, group, (), flavor)
            schemas.append((group, flavor, start))
            source = {"path": path, "sha256": hashlib.sha256(raw).hexdigest()}
            if source not in sources:
                sources.append(source)
    out = ["// Generated by tools/generate_mcc_ordinary_pointer_layouts.py; do not edit.",
           f"// Assembly commit {PIN}; source hashes in tools/mcc-ordinary-pointer-layouts.json.",
           f"constexpr std::array<OrdinaryNode,{len(nodes)}> ordinary_nodes{{{{"]
    out += [f"    {{{a},{b},0x{c:x}}}," for a, b, c in nodes]
    out += ["}};", f"constexpr std::array<OrdinaryField,{len(fields)}> ordinary_fields{{{{"]
    out += [f"    {{0x{o:x},0x{s:x},{n},OrdinaryKind::{k},{json.dumps(label)}}}," for o, s, n, k, label in fields]
    out += ["}};", f"constexpr std::array<OrdinarySchema,{len(schemas)}> ordinary_schemas{{{{"]
    out += [f"    {{{json.dumps(g)},MccOrdinaryPointerFlavor::{f},{n}}}," for g, f, n in schemas]
    out += ["}};", ""]
    manifest = {"schema": "H2_PINNED_ORDINARY_POINTER_LAYOUTS_V1", "assembly_commit": PIN,
                "fallback": "Halo2MCC inherits Halo2 plugin paths in the pinned Engines.xml",
                "normalized_unit_shapes": "bipd/crea sphere 0x80, pill 0x50; vehi phantom 0x2a0; src/physics/mcc_physics.cpp",
                "normalized_bsp_physics": "sbsp root138/c8 row+B0 reserves A0; normalized MOPP dataref64/68; src/physics/mcc_physics.cpp",
                "relative_fields_preserved": ["scnr Script Expressions +0xc is ScriptNode::string_offset, src/assets/content.cpp and src/campaign/scripting/scripts.cpp"],
                "resource_addresses_delegated": ["scnr Structure BSPs +8 is bound by canonical BSP resource delivery"],
                "sources": sources, "schema_count": len(schemas), "node_count": len(nodes), "field_count": len(fields)}
    return "\n".join(out), json.dumps(manifest, indent=2) + "\n"

def main():
    p = argparse.ArgumentParser()
    root = Path(__file__).resolve().parents[1]
    p.add_argument("--assembly", type=Path, default=root / "tools/reference/assembly")
    p.add_argument("--check", action="store_true")
    args = p.parse_args()
    layout, manifest = generate(args.assembly)
    for path, value in ((root / "src/assets/mcc/mcc_ordinary_pointer_layout.inc", layout),
                        (root / "tools/mcc-ordinary-pointer-layouts.json", manifest)):
        if args.check:
            if not path.exists() or path.read_text() != value:
                raise SystemExit(f"Generated ordinary pointer source differs: {path}")
        else:
            path.write_text(value)

if __name__ == "__main__":
    main()
