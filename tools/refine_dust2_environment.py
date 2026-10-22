#!/usr/bin/env python3
"""Add an authored desert surround and a portable lighting rig to Dust II.

The original model buffer, mesh attributes, placements and collision remain
unchanged. New scenery is render-only and lies below/outside the protected
world bounds. Run through tools/run_test.py; publish only a reviewed package.
"""
from __future__ import annotations

import argparse
import copy
import io
import json
import math
import os
from pathlib import Path
import shutil
import struct
import tempfile

import numpy as np
from PIL import Image

from build_dust2_mombasa import Glb, MAP, METERS_PER_ASS_UNIT, dds, save_json, sha, write_ass


class SceneWriter:
    def __init__(self, source=None):
        self.source = source
        self.doc = copy.deepcopy(source.doc) if source else dict(asset=dict(version="2.0"),
            scenes=[dict(nodes=[])], scene=0, nodes=[], meshes=[], materials=[],
            accessors=[], bufferViews=[], buffers=[dict(byteLength=0)])
        self.prefix = source.bin_size if source else 0
        self.data = bytearray(b"\0"*((-self.prefix)%4))

    def blob(self, data):
        offset = self.prefix + len(self.data)
        view = len(self.doc["bufferViews"])
        self.doc["bufferViews"].append(dict(buffer=0, byteOffset=offset, byteLength=len(data)))
        self.data.extend(data)
        self.data.extend(b"\0"*((-len(self.data)-self.prefix)%4))
        return view

    def array(self, values, kind):
        values = np.asarray(values, dtype="<u4" if kind == "SCALAR" else "<f4")
        view = self.blob(values.tobytes())
        index = len(self.doc["accessors"])
        a = dict(bufferView=view, componentType=5125 if kind == "SCALAR" else 5126,
                 count=len(values), type=kind)
        if kind == "VEC3":
            a.update(min=values.min(axis=0).tolist(), max=values.max(axis=0).tolist())
        self.doc["accessors"].append(a)
        return index

    def texture(self, path):
        self.doc.setdefault("images", []).append(dict(bufferView=self.blob(path.read_bytes()),
                                                     mimeType="image/png", name=path.stem))
        self.doc.setdefault("samplers", [dict(magFilter=9729, minFilter=9987, wrapS=10497, wrapT=10497)])
        self.doc.setdefault("textures", []).append(dict(sampler=0, source=len(self.doc["images"])-1))
        return len(self.doc["textures"])-1

    def mesh(self, name, positions, triangles, material, uv_scale=12, extras=None):
        positions = np.asarray(positions, dtype=np.float64)
        triangles = np.asarray(triangles, dtype=np.uint32)
        normals = np.zeros_like(positions)
        face = np.cross(positions[triangles[:, 1]]-positions[triangles[:, 0]],
                        positions[triangles[:, 2]]-positions[triangles[:, 0]])
        for j in range(3):
            np.add.at(normals, triangles[:, j], face)
        normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-30)
        uv = positions[:, [0, 2]]/uv_scale
        if name.startswith("desert_mesa_"):
            # Give the cliff walls vertical UV area. A top-down projection
            # stretches the rock texture into long vertical streaks.
            original_triangles=triangles
            positions=positions[original_triangles.reshape(-1)]
            normals=normals[original_triangles.reshape(-1)]
            uv=np.empty((len(positions),2),dtype=np.float64)
            dominant=np.argmax(np.abs(face),axis=1)
            for i,axis in enumerate(dominant):
                axes=[2,1] if axis==0 else [0,2] if axis==1 else [0,1]
                uv[i*3:i*3+3]=positions[i*3:i*3+3][:,axes]/uv_scale
            triangles=np.arange(len(positions),dtype=np.uint32).reshape(-1,3)
        primitive = dict(attributes=dict(POSITION=self.array(positions, "VEC3"),
            NORMAL=self.array(normals, "VEC3"), TEXCOORD_0=self.array(uv, "VEC2")),
            indices=self.array(triangles.reshape(-1), "SCALAR"), material=material)
        index = len(self.doc["meshes"])
        self.doc["meshes"].append(dict(name=name, primitives=[primitive]))
        node = len(self.doc["nodes"])
        self.doc["nodes"].append(dict(name=name, mesh=index,
            extras=dict(role="nonplayable_desert_backdrop", collision=False, **(extras or {}))))
        self.doc["scenes"][self.doc.get("scene", 0)]["nodes"].append(node)
        return dict(name=name, triangles=len(triangles), vertices=len(positions),
                    bounds_meters=[positions.min(axis=0).tolist(), positions.max(axis=0).tolist()])

    def write(self, path, generator="Dust II desert environment and lighting authoring"):
        length = self.prefix + len(self.data)
        self.doc["buffers"] = [dict(byteLength=length)]
        self.doc["asset"]["generator"] = generator
        data = json.dumps(self.doc, separators=(",", ":")).encode()
        data += b" "*((-len(data))%4)
        with Path(path).open("wb") as f:
            f.write(struct.pack("<5I", 0x46546C67, 2, 28+len(data)+length, len(data), 0x4E4F534A))
            f.write(data)
            f.write(struct.pack("<II", length, 0x004E4942))
            if self.source:
                f.write(self.source.raw[self.source.bin_at:self.source.bin_at+self.prefix])
            f.write(self.data)


def texture(root, package, leaf, name):
    inventory = json.loads((root/"bitmap-resources.json").read_text())
    bank = next(b for b in inventory["banks"] if b["map"] == MAP and b["tag"].split("\\")[-1] == leaf)
    frame = bank["frames"][0]
    assert frame["type"] == 0 and not any(frame["partitions"][1:])
    resource = inventory["resources"][frame["partitions"][0]]
    source = root/resource["path"]
    assert sha(source) == resource["sha256"]
    blob = dds(*frame["dimensions"][:2], frame["format"], source.read_bytes(), frame["additional_mips"])
    path = package/"textures"/name
    path.with_suffix(".dds").write_bytes(blob)
    image = Image.open(io.BytesIO(blob)).convert("RGB")
    image.save(path.with_suffix(".png"))
    image.save(path.with_suffix(".tga"))
    return dict(map=MAP, bitmap=bank["tag"], source_sha256=resource["sha256"],
                png=str(path.with_suffix(".png").relative_to(package)))


def landscape(bounds):
    lower, upper = np.asarray(bounds)*METERS_PER_ASS_UNIT
    # ASS X/Y/Z -> glTF X/Y/Z. The full collision AABB is protected.
    lower, upper = lower[[1, 2, 0]], upper[[1, 2, 0]]
    center = (lower+upper)/2
    half = (upper-lower)/2
    half[[0, 2]] += 18
    floor = float(lower[1]-4)
    segments, rings = 256, 72
    angle = np.arange(segments)*2*np.pi/segments
    cx, cz = np.cos(angle), np.sin(angle)
    inner = 1/np.maximum(np.abs(cx)/half[0], np.abs(cz)/half[2])
    positions = [[center[0], floor, center[2]]]
    for r in range(rings+1):
        distance = (r/rings)**1.65*1400
        radius = inner + distance
        ramp = 1-np.exp(-distance/70)
        x, z = center[0]+cx*radius, center[2]+cz*radius
        # Broad wind-aligned dunes plus smaller ripples; deterministic authored
        # scenery, deliberately independent of the source's playable topology.
        dunes = 12+11*np.sin(x/95+z/190)+6*np.sin(x/37-z/83)+3*np.sin(z/28+x/140)
        horizon = 38*(1-np.exp(-distance/500))
        y = floor+ramp*(dunes+horizon)
        positions.extend(np.column_stack((x, y, z)).tolist())
    triangles = []
    for s in range(segments):
        triangles.append([0, 1+(s+1)%segments, 1+s])
    for r in range(rings):
        a = 1+r*segments
        b = a+segments
        for s in range(segments):
            t = (s+1)%segments
            triangles.extend([[a+s, a+t, b+t], [a+s, b+t, b+s]])
    # Distant closed rock mesas break the silhouette beyond the dunes.
    mesas = []
    for k in range(15):
        theta = 2*np.pi*(k+0.21)/15
        distance = 620+145*np.sin(k*2.13)
        x, z = center[0]+np.cos(theta)*distance, center[2]+np.sin(theta)*distance
        radius = 65+20*np.cos(k*1.9)
        height = 55+28*(1+np.sin(k*1.71))
        p, t = [], []
        for ring, (scale, h) in enumerate([(1.4, floor-3), (1.0, 24), (.72, height), (.48, height+8)]):
            for s in range(48):
                a = s*2*np.pi/48
                rough = 1+.1*np.sin(7*a+k)+.045*np.cos(13*a-k)
                p.append([x+np.cos(a)*radius*scale*rough, h+2*np.sin(5*a+k),
                          z+np.sin(a)*radius*scale*rough])
        for ring in range(3):
            for s in range(48):
                a=ring*48+s; b=ring*48+(s+1)%48; c=a+48; d=b+48
                t.extend([[a,d,b],[a,c,d]])
        p.append([x,height+8,z])
        for s in range(48):t.append([192,144+(s+1)%48,144+s])
        mesas.append((f"desert_mesa_{k:02d}",p,t))
    return positions, triangles, mesas, dict(protected_collision_bounds_meters=[lower.tolist(),upper.tolist()],
        terrain_floor_meters=floor, clearance_meters=18, radial_extent_meters=1400)


def add_environment(source, writer, terrain_materials, bounds):
    p,t,mesas,profile=landscape(bounds)
    records=[writer.mesh("desert_dunes_surround",p,t,terrain_materials[0],uv_scale=9)]
    records.extend(writer.mesh(n,p,t,terrain_materials[1],uv_scale=18) for n,p,t in mesas)
    return records,profile


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package",required=True,type=Path)
    parser.add_argument("--output",required=True,type=Path)
    args=parser.parse_args()
    if args.output.exists():raise ValueError("Output package must be new")
    original=json.loads((args.package/"provenance.json").read_text())
    with tempfile.TemporaryDirectory(prefix="dust2-environment-",dir=os.environ.get("HALO2_TEST_SCRATCH")) as owned:
        stage=Path(owned)/"package"
        shutil.copytree(args.package,stage)
        source=Glb(args.package/"dust2_mombasa.glb")
        root=Path(original["sources"]["mcc_cache_root"])
        textures={"sand":texture(root,stage,"ec_ocean_floor","desert_sand"),
                  "rock":texture(root,stage,"sedimentary_rock_a","desert_rock")}
        palette={"sand":dict(shader="scenarios\\shaders\\solo\\earthcity\\ec_ocean_floor"),
            "rock":dict(shader="scenarios\\multi\\dust2_mombasa\\shaders\\desert_sedimentary")}
        complete,backdrop=SceneWriter(source),SceneWriter()
        removed_tints=0
        for mesh in complete.doc["meshes"]:
            for primitive in mesh["primitives"]:
                # CS2 shader vertex tint/blend channels have no equivalent in
                # this authored New Mombasa material pass. Preserve the raw
                # reference bytes, but do not bind them as glTF base-color tint.
                if primitive["attributes"].pop("COLOR_0",None) is not None:
                    removed_tints+=1
        material_ids=[]
        for key,tint in [("sand",[1,.94,.82,1]),("rock",[1,.85,.65,1])]:
            for writer in (complete,backdrop):
                tex=writer.texture(stage/textures[key]["png"])
                writer.doc["materials"].append(dict(name="desert_"+key,
                    pbrMetallicRoughness=dict(baseColorTexture=dict(index=tex),baseColorFactor=tint,
                        metallicFactor=0,roughnessFactor=.94),
                    extras=dict(halo2_shader=palette[key]["shader"],mcc_bitmap=textures[key]["bitmap"])))
            material_ids.append(len(complete.doc["materials"])-1)
        records,profile=add_environment(source,complete,material_ids,original["collision"]["bounds_ass"])
        add_environment(source,backdrop,[0,1],original["collision"]["bounds_ass"])
        # Keep the source sun direction/color in portable viewers. The
        # editable Cycles scene supplies actual procedural sky GI.
        lights=complete.doc.setdefault("extensions",{}).setdefault("KHR_lights_punctual",{}).setdefault("lights",[])
        original_sun=copy.deepcopy(lights[0])
        lights[0].update(name="Dust II reference sun",intensity=1707.5)
        lighting=dict(schema="H2_DUST2_AUTHORED_LIGHTING_V1",source_sun=original_sun,
            sun_direction_node=next(n for n in source.doc["nodes"] if "KHR_lights_punctual" in n.get("extensions",{})),
            blender=dict(sun_energy=2.5,sun_angle_degrees=.8,sky_strength=.35,
                sky_model="Nishita",air_density=1.0,dust_density=2.0,diffuse_bounces=6,total_bounces=8,
                emissive_fixture_strength=.8,view_transform="AgX",exposure=-.3),
            halo2_lightmaps_baked=False,
            note="Authored desert daylight, sun shadows and indirect sky bounce; not original Halo 2 framebuffer parity.")
        complete.write(stage/"dust2_mombasa.glb")
        backdrop.write(stage/"desert_backdrop.glb")
        write_ass(Glb(stage/"desert_backdrop.glb"),
            stage/"data/scenarios/multi/dust2_mombasa/structure/desert_backdrop.ass",
            palette,mapping=[dict(palette="sand"),dict(palette="rock")])
        new=Glb(stage/"dust2_mombasa.glb")
        assert source.raw[source.bin_at:source.bin_at+source.bin_size]==new.raw[new.bin_at:new.bin_at+source.bin_size]
        expected_meshes=copy.deepcopy(source.doc["meshes"])
        for mesh in expected_meshes:
            for primitive in mesh["primitives"]:primitive["attributes"].pop("COLOR_0",None)
        assert expected_meshes==new.doc["meshes"][:len(source.doc["meshes"])]
        assert source.doc["nodes"]==new.doc["nodes"][:len(source.doc["nodes"])]
        base_palette=json.loads((args.package/"material_mapping.json").read_text())["palette"]
        full_palette=dict(base_palette,desert_sand=palette["sand"],desert_rock=palette["rock"])
        full_mapping=[dict(palette=key) for key in full_palette]
        combined_ass=write_ass(new,stage/"data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass",
                               full_palette,mapping=full_mapping)
        save_json(stage/"lighting.json",lighting)
        save_json(stage/"desert_environment.json",dict(schema="H2_DUST2_DESERT_ENVIRONMENT_V1",
            generator_sha256=sha(__file__),textures=textures,geometry=records,profile=profile,
            source_model_sha256=sha(args.package/"dust2_mombasa.glb"),
            source_geometry_buffer_unchanged=True,source_nodes_and_geometry_attributes_unchanged=True,
            inherited_cs2_appearance_tints_unbound=removed_tints,
            original_collision_unchanged=sha(stage/"dust2_collision.ass")==sha(args.package/"dust2_collision.ass"),
            background_has_collision=False,authored_surround=True))
        original["environment_revision"]=2
        original["original_render"]=original["render"]
        original["render"]=combined_ass
        original["authored_desert_backdrop_triangles"]=sum(r["triangles"] for r in records)
        original["lighting_scene_authored"]=True
        save_json(stage/"provenance.json",original)
        shutil.move(str(stage),str(args.output))
        print(json.dumps(dict(output=str(args.output),backdrop_triangles=original["authored_desert_backdrop_triangles"],
            source_buffer_unchanged=True,source_collision_unchanged=True),indent=2),flush=True)


if __name__=="__main__":main()
