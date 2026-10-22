#!/usr/bin/env python3
"""Retexture an actual VRF Dust II export and write Halo 2 ASS revision 2.

Inputs are user-owned CS2 geometry exported by Source 2 Viewer 20.1 and an
existing MCC Classic import. No geometry is reconstructed from screenshots.
Large intermediate files live in an owned temporary directory; only the
explicitly requested model package is published to --output.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import io
import json
import mmap
import os
from pathlib import Path
import re
import shutil
import struct
import tempfile

import numpy as np
from PIL import Image

from mcc_content import MccCache


MAP = "03b_newmombasa.map"
METERS_PER_ASS_UNIT = 0.03048  # H2 EK: 100 ASS units = 10 feet.
PALETTE = {
    "wall": "nm_concrete_wall", "floor": "nm_concrete_floor",
    "white": "nm_wall_white", "blue": "nm_metal_blue",
    "metal": "nm_metal_grey_panels", "stone": "nm_stone_red",
    "walkway": "nm_walkway", "stairs": "nm_stairs",
    "glass": "nm_horiz_glass", "concrete": "nm_concrete",
    "light": "nm_lightbox", "terrain": "park_concrete",
    "stucco": "base_stucco1", "brick": "base_brick1",
    "fence": "fence_chainlink_a", "door": "nm_door_interior",
    "trim": "nm_metal_grey_trim", "roof": "nm_building_slopedface",
    "plant": "tree_ancient_leaves", "bark": "tree_ancient",
}


def sha(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def save_json(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + "\n")


class Glb:
    def __init__(self, path):
        self.path = Path(path)
        self.file = self.path.open("rb")
        self.raw = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)
        magic, version, size, length, kind = struct.unpack_from("<5I", self.raw)
        if (magic, version, size, kind) != (0x46546C67, 2, len(self.raw), 0x4E4F534A):
            raise ValueError("Invalid GLB header")
        self.doc = json.loads(self.raw[20:20 + length])
        self.bin_at = 28 + length
        bin_size, bin_type = struct.unpack_from("<II", self.raw, 20 + length)
        if bin_type != 0x004E4942 or self.bin_at + bin_size != size:
            raise ValueError("Invalid GLB buffer extent")
        self.bin_size = self.doc["buffers"][0]["byteLength"]

    def array(self, index):
        a = self.doc["accessors"][index]
        if "sparse" in a or a.get("normalized"):
            raise ValueError("Unsupported sparse/normalized geometry accessor")
        view = self.doc["bufferViews"][a["bufferView"]]
        dtype = np.dtype({5121: "u1", 5123: "<u2", 5125: "<u4", 5126: "<f4"}[a["componentType"]])
        columns = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}[a["type"]]
        offset = self.bin_at + view.get("byteOffset", 0) + a.get("byteOffset", 0)
        stride = view.get("byteStride", dtype.itemsize * columns)
        end = offset + (a["count"] - 1) * stride + columns * dtype.itemsize
        if offset < self.bin_at or end > self.bin_at + self.bin_size:
            raise ValueError("Accessor exceeds binary buffer")
        return np.ndarray((a["count"], columns), dtype, self.raw, offset,
                          strides=(stride, dtype.itemsize))

    def nodes(self):
        def visit(index, parent):
            node = self.doc["nodes"][index]
            if "matrix" in node:
                local = np.asarray(node["matrix"], dtype=np.float64).reshape(4, 4).T
            else:
                x, y, z, w = node.get("rotation", [0, 0, 0, 1])
                local = np.eye(4)
                local[:3, :3] = [[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
                                    [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
                                    [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]]
                local[:3, :3] *= np.asarray(node.get("scale", [1, 1, 1]))
                local[:3, 3] = node.get("translation", [0, 0, 0])
            world = parent @ local
            if "mesh" in node:
                yield index, node, self.doc["meshes"][node["mesh"]], world
            for child in node.get("children", []):
                yield from visit(child, world)
        for root in self.doc["scenes"][self.doc.get("scene", 0)]["nodes"]:
            yield from visit(root, np.eye(4))


def dds(width, height, fmt, payload, additional_mips=0):
    # Only the authored top mip is decoded for PNG/TGA. The retained DDS keeps
    # the complete original compressed/uncompressed partition verbatim.
    compressed = fmt in (14, 15, 16)
    if compressed:
        code = {14: b"DXT1", 15: b"DXT3", 16: b"DXT5"}[fmt]
        pf = [4, int.from_bytes(code, "little"), 0, 0, 0, 0, 0]
        pitch = ((width+3)//4)*((height+3)//4)*(8 if fmt == 14 else 16)
    elif fmt in (10, 11):
        pf = [0x41 if fmt == 11 else 0x40, 0, 32, 0xff0000, 0xff00, 0xff,
              0xff000000 if fmt == 11 else 0]
        pitch = width*4
    else:
        raise ValueError(f"Unsupported palette bitmap format {fmt}")
    flags = 0x81007 if compressed else 0x100f
    if additional_mips:
        flags |= 0x20000
    header = [124, flags, height, width, pitch, 0, additional_mips+1]
    header += [0]*11 + [32] + pf + [0x401008 if additional_mips else 0x1000, 0, 0, 0, 0]
    return b"DDS " + struct.pack("<31I", *header) + payload


def export_palette(root, output):
    inventory_path = root / "bitmap-resources.json"
    inventory = json.loads(inventory_path.read_text())
    banks = {(b["map"], b["tag"]): b for b in inventory["banks"]}
    cache = MccCache(root / "halo2/h2_maps_win64_dx11" / MAP)
    result = {}
    for key, leaf in PALETTE.items():
        tags = [t for t in cache.tags.values() if t["group"] == "shad" and t["name"].split("\\")[-1] == leaf]
        if len(tags) != 1:
            raise ValueError(f"Missing or ambiguous New Mombasa shader {leaf}")
        tag = tags[0]
        props = cache.block(tag["offset"]+12, 80)
        if len(props) != 1:
            raise ValueError(f"Missing diffuse binding for {leaf}")
        bitmap, _ = cache.tag(cache.u32(props[0]+4), "bitm")
        frame = banks[MAP, bitmap["name"]]["frames"][0]
        if frame["type"] != 0 or any(frame["partitions"][1:]):
            raise ValueError("Unsupported split or non-2D palette texture")
        resource = inventory["resources"][frame["partitions"][0]]
        source = root / resource["path"]
        if sha(source) != resource["sha256"]:
            raise ValueError("MCC texture resource changed")
        width, height, depth = frame["dimensions"]
        if depth != 1:
            raise ValueError("Non-2D palette bitmap")
        blob = dds(width, height, frame["format"], source.read_bytes(), frame["additional_mips"])
        image = Image.open(io.BytesIO(blob)).convert("RGBA")
        # DXT5 alpha is often specular in Halo shaders. Use opaque base color
        # except for the explicit foliage/fence alpha-test families.
        if key not in ("plant", "fence"):
            image.putalpha(255)
        (output / "textures").mkdir(exist_ok=True)
        (output / "textures" / f"{key}.dds").write_bytes(blob)
        image.save(output / "textures" / f"{key}.png")
        image.save(output / "textures" / f"{key}.tga")
        result[key] = dict(shader=tag["name"], bitmap=bitmap["name"],
            datum=f"{bitmap['datum']:08x}", dimensions=frame["dimensions"],
            format=frame["format"], partition=frame["partitions"][0],
            source_sha256=resource["sha256"], png=f"textures/{key}.png",
            tga=f"textures/{key}.tga")
    cache.close()
    return result


def classify(material):
    name = material["name"].lower()
    vmat = material.get("extras", {}).get("vmat", {})
    path = vmat.get("Name", "").lower()
    shader = vmat.get("ShaderName", "").lower()
    if "/tools/" in path or "nodraw" in name:
        return None, "invisible Source compiler helper"
    if "overlay" in shader or "/decals/" in path or "/overlays/" in path or "effects" in shader:
        return None, "Source decal/effect plane; retained separately in helper ASS"
    rules = [
        ("plant", r"leav|frond|weeds|grass|sagebrush|sumac|agave|grape"),
        ("bark", r"bark|tree"), ("fence", r"chainlink|fence_wire"),
        ("glass", r"glass|windows|window_inset"),
        ("door", r"door|gate|trap_door"), ("stairs", r"stair|step"),
        ("walkway", r"sidewalk|curb|pavement|paver"),
        ("floor", r"asphalt|road|floor|groundstone"),
        ("terrain", r"sand|groundrough|groundsmooth|dirt|terrain"),
        ("roof", r"roof|shingle|ceiling"),
        ("light", r"light|lantern|lamp"),
        ("blue", r"blue|tarp|awning|cloth|umbrella|fabric"),
        ("stone", r"tile|terracotta|pottery|ornament|pattern|stone"),
        ("brick", r"brick|breeze_block"),
        ("trim", r"trim|bracket|frame|brace|wire|pipe|pole|antenna"),
        ("metal", r"metal|barrel|drum|crate|container|panel|truck|car_|vehicle|ac_unit|electric|satellite|bicycle|chair"),
        ("white", r"white|plaster_18|plaster_09"),
        ("stucco", r"plaster|stucco"),
        ("concrete", r"concrete|rock|rubble"),
    ]
    for key, pattern in rules:
        if re.search(pattern, name):
            return key, "authored surface-family replacement"
    return "wall", "authored default New Mombasa concrete replacement"


def write_glb(source, output, palette, mapping):
    doc = copy.deepcopy(source.doc)
    keys = list(palette)
    doc["materials"] = []
    doc["images"] = []
    doc["textures"] = []
    doc["samplers"] = [dict(magFilter=9729, minFilter=9987, wrapS=10497, wrapT=10497)]
    extra = bytearray()
    start = (source.bin_size+3)//4*4
    for i, key in enumerate(keys):
        blob = (output.parent / palette[key]["png"]).read_bytes()
        view = len(doc["bufferViews"])
        doc["bufferViews"].append(dict(buffer=0, byteOffset=start+len(extra), byteLength=len(blob)))
        extra.extend(blob)
        extra.extend(b"\0"*((-len(extra))%4))
        doc["images"].append(dict(bufferView=view, mimeType="image/png", name=key))
        doc["textures"].append(dict(sampler=0, source=i))
        mat = dict(name=palette[key]["shader"], pbrMetallicRoughness=dict(
            baseColorTexture=dict(index=i), metallicFactor=0, roughnessFactor=0.85),
            extras=dict(halo2_shader=palette[key]["shader"], mcc_bitmap=palette[key]["bitmap"]))
        if key in ("plant", "fence"):
            mat.update(alphaMode="MASK", alphaCutoff=0.5, doubleSided=True)
        doc["materials"].append(mat)
    hidden = 0
    for mesh in doc["meshes"]:
        visible = []
        for prim in mesh["primitives"]:
            row = mapping[prim["material"]]
            if row["palette"] is None:
                hidden += source.doc["accessors"][prim["indices"]]["count"]//3
                continue
            prim["material"] = keys.index(row["palette"])
            visible.append(prim)
        mesh["primitives"] = visible
    # Empty meshes are invalid in glTF. Keep their source nodes as transform
    # nodes and remap all nonempty mesh references without altering placements.
    remap = {}
    meshes = []
    for i, mesh in enumerate(doc["meshes"]):
        if mesh["primitives"]:
            remap[i] = len(meshes)
            meshes.append(mesh)
    doc["meshes"] = meshes
    for node in doc["nodes"]:
        if "mesh" in node:
            old = node.pop("mesh")
            if old in remap:
                node["mesh"] = remap[old]
    doc["buffers"] = [dict(byteLength=start+len(extra))]
    doc["asset"]["generator"] = "Dust II / New Mombasa; geometry exported by Source 2 Viewer 20.1"
    payload = json.dumps(doc, separators=(",", ":")).encode()
    payload += b" "*((-len(payload))%4)
    binary_size = start+len(extra)
    with output.open("wb") as stream:
        stream.write(struct.pack("<5I", 0x46546C67, 2, 28+len(payload)+binary_size, len(payload), 0x4E4F534A))
        stream.write(payload)
        stream.write(struct.pack("<II", binary_size, 0x004E4942))
        stream.write(source.raw[source.bin_at:source.bin_at+source.bin_size])
        stream.write(b"\0"*(start-source.bin_size))
        stream.write(extra)
    return hidden


def ass_shader_name(path):
    """Resolve exact kit shader paths through Classic Tool collection syntax."""
    collections = {
        'scenarios\\shaders\\solo\\earthcity\\new_mombassa': 'city_nm',
        'scenarios\\shaders\\solo\\earthcity': 'city',
        'scenarios\\shaders\\human\\urban\\metals': 'h_um',
        'scenarios\\multi\\dust2_mombasa\\shaders': 'dust2',
        'scenarios\\shaders\\multi\\zanzibar': 'zanzibar',
        'scenarios\\shaders\\multi\\turf': 'tu',
        'scenarios\\shaders\\nature\\trees': 'tre',
        'objects\\vehicles\\warthog\\shaders': 'hog',
    }
    directory, leaf = path.rsplit('\\', 1)
    return f'{collections[directory]} {leaf}'


def write_ass(source, output, palette, mapping=None, helpers=False, collision=False, shared_collision_materials=()):
    keys = list(palette)
    chunks = []
    total = 0
    for ni, node, mesh, world in source.nodes():
        for pi, prim in enumerate(mesh["primitives"]):
            if prim.get("mode", 4) != 4:
                raise ValueError("Only source triangle lists are supported")
            row = mapping[prim["material"]] if mapping is not None else None
            if row is not None and (row["palette"] is None) != helpers:
                continue
            count = source.doc["accessors"][prim["indices"]]["count"]
            if count % 3:
                raise ValueError("Incomplete triangle")
            for begin in range(0, count, 60000):
                chunks.append((ni, node, prim, world, begin, min(count, begin+60000), row))
            total += count//3
    # Revision 2 is the documented H2 level interchange revision. A chunk has
    # at most 60000 referenced vertices; no decimation, welding, or face loss.
    with output.open("w", buffering=1 << 20) as stream:
        stream.write(';### HEADER ###\n2\n"DUST2_MOMBASA"\n"1.0"\n"Eric"\n"LOCAL"\n')
        stream.write(f';### MATERIALS ###\n{len(keys)}\n')
        for key in keys:
            suffix = "@" if collision else "%" if key in shared_collision_materials else "!"
            if key in ("plant", "fence"):
                suffix += "%"
            stream.write(f'"{ass_shader_name(palette[key]["shader"])}{suffix}"\n""\n')
        stream.write(f';### OBJECTS ###\n{len(chunks)}\n')
        lower, upper = np.full(3, np.inf), np.full(3, -np.inf)
        for ci, (_, node, prim, world, begin, end, row) in enumerate(chunks):
            ids = source.array(prim["indices"])[begin:end, 0]
            used, inverse = np.unique(ids, return_inverse=True)
            attrs = prim["attributes"]
            position = source.array(attrs["POSITION"])[used].astype(np.float64)
            position = position @ world[:3, :3].T + world[:3, 3]
            # VRF glTF meters Y-up -> original Source axes -> ASS units Z-up.
            position = position[:, [2, 0, 1]] / METERS_PER_ASS_UNIT
            normal = source.array(attrs["NORMAL"])[used].astype(np.float64)
            normal = normal @ np.linalg.inv(world[:3, :3])
            normal = normal[:, [2, 0, 1]]
            normal /= np.maximum(np.linalg.norm(normal, axis=1, keepdims=True), 1e-30)
            uv = source.array(attrs["TEXCOORD_0"])[used].astype(np.float64)
            uv[:, 1] = 1-uv[:, 1]  # glTF and ASS use opposite texture origins.
            if not all(np.isfinite(a).all() for a in (position, normal, uv)):
                raise ValueError("Nonfinite source geometry")
            lower = np.minimum(lower, position.min(axis=0))
            upper = np.maximum(upper, position.max(axis=0))
            stream.write(f';OBJECT {ci}\n"MESH"\n""\n""\n{len(used)}\n')
            # Write incrementally rather than assembling the full text in RAM.
            for p, n, t in zip(position, normal, uv):
                # Classic Tool's vector reader requires tabs between components.
                # Spaces shift the following records even though the published
                # ASS specification describes whitespace as interchangeable.
                stream.write('%.9g\t%.9g\t%.9g\n%.9g\t%.9g\t%.9g\n0\n1\n%.9g\t%.9g\n' % (*p, *n, *t))
            triangles = inverse.reshape(-1, 3)
            if np.linalg.det(world[:3, :3]) < 0:
                triangles = triangles[:, [0, 2, 1]]
            material = keys.index(row["palette"]) if row and row["palette"] else keys.index("concrete")
            stream.write(f'{len(triangles)}\n')
            for a, b, c in triangles:
                stream.write(f'{material}\n{a}\n{b}\n{c}\n')
        stream.write(f';### INSTANCES ###\n{len(chunks)+1}\n')
        identity = '0\t0\t0\t1\n0\t0\t0\n1\n0\t0\t0\t1\n0\t0\t0\n1\n'
        stream.write('-1\n"Scene Root"\n-1\n-1\n0\n'+identity)
        for ci, (ni, node, _, _, _, _, _) in enumerate(chunks):
            name = re.sub(r'[^A-Za-z0-9_]', '_', node.get("name", "mesh"))[:70]
            stream.write(f'{ci}\n"%{name}_{ni}_{ci}"\n{ci+1}\n0\n0\n'+identity)
    return dict(objects=len(chunks), triangles=total,
                bounds_ass=[lower.tolist(), upper.tolist()] if chunks else None,
                max_vertices_per_object=60000)


def entity_records(path):
    result = []
    for block in re.split(r"====\d+====", path.read_text()):
        row = {}
        for line in block.splitlines():
            fields = line.strip().split(None, 1)
            if len(fields) == 2:
                value = fields[1]
                row[fields[0]] = value[1:-1] if value.startswith('"') and value.endswith('"') else value
        if row.get("classname") in ("info_player_terrorist", "info_player_counterterrorist", "func_bomb_target", "func_buyzone", "sky_camera"):
            if "origin" in row:
                position = [float(x) for x in row["origin"].strip("[] ").split(",")]
                row["halo_world_position"] = [x/120 for x in position]
                row["ass_position"] = [x/1.2 for x in position]
            result.append(row)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("render-glb", "collision-glb", "entities", "mcc-cache-root", "source-vpk", "output"):
        parser.add_argument("--"+name, required=True, type=Path)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError("Refusing to overwrite an existing model package")
    with tempfile.TemporaryDirectory(prefix="dust2-mombasa-", dir=os.environ.get("HALO2_TEST_SCRATCH")) as owned:
        stage = Path(owned) / "package"
        stage.mkdir()
        palette = export_palette(args.mcc_cache_root, stage)
        print(f'Exported {len(palette)} verified New Mombasa diffuse textures', flush=True)
        render = Glb(args.render_glb)
        mapping = [dict(source_index=i, source_name=m["name"],
                        source_path=m.get("extras", {}).get("vmat", {}).get("Name"),
                        palette=classify(m)[0], reason=classify(m)[1])
                   for i, m in enumerate(render.doc["materials"])]
        hidden = write_glb(render, stage / "dust2_mombasa.glb", palette, mapping)
        print('Wrote textured GLB', flush=True)
        structure = stage / "data/scenarios/multi/dust2_mombasa/structure"
        structure.mkdir(parents=True)
        visible = write_ass(render, structure / "dust2_mombasa.ass", palette, mapping)
        print(f'Wrote render ASS: {visible}', flush=True)
        helpers = write_ass(render, stage / "dust2_source_helpers.ass", palette, mapping, helpers=True)
        collision = Glb(args.collision_glb)
        physics = write_ass(collision, stage / "dust2_collision.ass", palette, collision=True)
        print(f'Wrote collision ASS: {physics}', flush=True)
        shutil.copyfile(args.collision_glb, stage / "dust2_collision.glb")
        save_json(stage / "source_entities.json", entity_records(args.entities))
        save_json(stage / "material_mapping.json", dict(palette=palette, mappings=mapping))
        report = dict(schema="H2_DUST2_MOMBASA_MODEL_V1", edition="installed Counter-Strike 2",
            sources=dict(vpk=str(args.source_vpk.resolve()), vpk_sha256=sha(args.source_vpk),
                render_glb_sha256=sha(args.render_glb), collision_glb_sha256=sha(args.collision_glb),
                mcc_cache_root=str(args.mcc_cache_root.resolve()),
                mcc_map_sha256=sha(args.mcc_cache_root / "halo2/h2_maps_win64_dx11" / MAP),
                generator_sha256=sha(__file__)),
            render=visible, helpers=helpers, collision=physics,
            source_render_triangles=visible["triangles"]+helpers["triangles"],
            preview_excluded_helper_triangles=hidden, palette_textures=len(palette),
            geometry_decimated=False, axes="original Source X/Y/Z, Z-up in ASS",
            scale="1 Source inch = 5/6 ASS units = 1/120 Halo world unit",
            source_topology_converted=True, halo2_bsp_compiled=False,
            halo2_runtime_tested=False, certified_one_to_one_runtime=False,
            limitations=["Material replacement is authored by source surface family; UV0 is retained.",
                "MCC shader tag dependencies are references, not loose editable shader tags.",
                "Helper/decal/effect surfaces are separated from visible geometry.",
                "Collision layers retain Source interaction metadata in the collision GLB; Halo collision semantics need authoring.",
                "No Halo portals, sealed BSP shell, lightmaps, gameplay placements or cache were compiled.",
                "CS2 particle systems, animated props, and separate 3D skybox are not reproduced.",
                "Full CS2 mesh density exceeds a practical unoptimized Halo 2 level budget."])
        save_json(stage / "provenance.json", report)
        save_json(stage / "files.json", {str(p.relative_to(stage)):dict(bytes=p.stat().st_size, sha256=sha(p))
                                        for p in stage.rglob("*") if p.is_file()})
        args.output.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(stage), str(args.output))
        print(json.dumps(report, indent=2), flush=True)


if __name__ == "__main__":
    main()
