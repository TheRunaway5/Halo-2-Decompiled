#!/usr/bin/env python3
"""Extract user-owned MCC CE Danger Canyon for Halo 2 Classic authoring.

Run under tools/run_test.py. Halo 1 layouts follow Reclaimer's Halo1 cache,
BSP reader and General-101's Halo-Tag-Interface layouts. Geometry is never
reconstructed from images. The CE teleporter shield is deliberately omitted;
the scenario authoring stage installs real Halo 2 teleporter assets instead.
"""
from __future__ import annotations

import argparse
import collections
import hashlib
import json
from pathlib import Path
import struct

import numpy as np


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + '\n')


def sha(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


class CeCache:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        if self.data[:8] != b'daeh\x0d\0\0\0':
            raise ValueError('Expected installed MCC Halo CE version-13 cache')
        if self.u32(8) != len(self.data):
            raise ValueError('Compressed/truncated CE caches are not supported')
        self.index = self.u32(16)
        self.magic = self.u32(self.index) - self.index - 40
        self.tags = {}
        for i in range(self.u32(self.index + 12)):
            at = self.index + 40 + i * 32
            handle, name, meta = self.unpack('<III', at + 12)
            p = name - self.magic
            self.require(p, 1)
            self.tags[handle] = dict(group=self.data[at:at+4][::-1].decode('ascii'),
                name=self.data[p:self.data.index(0, p)].decode('ascii'), offset=meta-self.magic)
        self.scenario = next(t['offset'] for t in self.tags.values() if t['group'] == 'scnr')
        rows = self.block(self.scenario + 1444, 32)
        if len(rows) != 1:
            raise ValueError('Danger Canyon must contain exactly one BSP')
        off, size, magic = self.unpack('<III', rows[0])
        self.require(off, size)
        self.bsp = off
        self.bsp_magic = magic - off
        self.root = off + 24

    def require(self, at, size):
        if at < 0 or size < 0 or at + size > len(self.data):
            raise ValueError(f'CE read outside cache: {at}/{size}')

    def unpack(self, fmt, at):
        self.require(at, struct.calcsize(fmt))
        return struct.unpack_from(fmt, self.data, at)

    def u32(self, at):
        return self.unpack('<I', at)[0]

    def block(self, at, stride, bsp=False):
        count, pointer = self.unpack('<II', at)
        if not count:
            return []
        if count > 262144:
            raise ValueError('Excessive CE block count')
        off = pointer - (self.bsp_magic if bsp else self.magic)
        self.require(off, count*stride)
        return [off+i*stride for i in range(count)]

    def tagref(self, at):
        return self.tags.get(self.u32(at+12))


def material(source):
    """Explicit authored mapping to existing Halo 2 shader collections."""
    leaf = source.rsplit('\\', 1)[-1]
    if 'teleporter_shield' in source:
        return None
    if leaf == 'dangercanyon_ground':
        return 'coag coag_ground'
    if leaf == 'dangercanyon_cliff_walls':
        return 'coag coag_cliffs_top'
    if leaf == 'player_clip':
        return 'f_im flat_generic'
    if 'red' in leaf:
        return 'coag coag_light_bands_red'
    if 'blue' in leaf:
        return 'coag coag_light_bands_blue'
    if 'white_light' in leaf or leaf == 'greenlight':
        return 'f_il light_10_white'
    if 'arrow_white' in leaf:
        return 'f_il light_shapes_ltblue'
    if 'metal_strips' in leaf:
        return 'f_im panels_thin_strips'
    if 'decorative' in leaf:
        return 'f_im panel_decorative_wide'
    if 'tubes' in leaf:
        return 'f_im panel_pipe'
    if 'floor' in leaf or leaf == 'flagstand':
        return 'f_if plate'
    if 'panels_ext' in leaf:
        return 'f_im panels_generic_ext'
    if 'panel' in leaf:
        return 'f_im panel_i'
    if 'emblem' in leaf:
        return 'f_im panel_decorative_circle'
    if 'flat' in leaf:
        return 'f_im flat_generic_graytech'
    raise ValueError(f'Unmapped source shader: {source}')


IDENTITY = '0\t0\t0\t1\n0\t0\t0\n1\n'*2


def extract(source, output):
    c = CeCache(source)
    output.mkdir(parents=True, exist_ok=False)
    materials, mapping, vertices, triangles = [], {}, [], []
    removed = []

    def mat(name, flags):
        key = name+flags
        if '!' in flags:
            key += ' lm:0.25'
        if key not in materials:
            materials.append(key)
        return materials.index(key)

    def add(v, faces, m):
        v = np.asarray(v, dtype=float)
        if not np.isfinite(v).all():
            raise ValueError('Nonfinite CE geometry')
        base = len(vertices)
        vertices.extend(v.tolist())
        triangles.extend((m, *(int(i)+base for i in face)) for face in faces)

    # CE rendered vertices are position/normal/binormal/tangent/UV, 56 bytes.
    indices = np.array([c.unpack('<3H', a) for a in c.block(c.root+248, 6, True)])
    render_count = 0
    for lightmap in c.block(c.root+260, 32, True):
        for at in c.block(lightmap+20, 256, True):
            tag = c.tagref(at)
            target = material(tag['name'])
            first, count = c.unpack('<II', at+20)
            n, offset = c.unpack('<II', at+180)
            vb = c.u32(c.bsp+8)+offset
            c.require(vb, n*56)
            raw = np.ndarray((n,14), dtype='<f4', buffer=c.data, offset=vb, strides=(56,4)).copy()
            faces = indices[first:first+count, ::-1]
            if faces.max(initial=0) >= n:
                raise ValueError('Source render triangle outside vertex buffer')
            if target is None:
                removed.append(dict(source=tag['name'], triangles=count,
                    bounds_world=[raw[:,:3].min(axis=0).tolist(),raw[:,:3].max(axis=0).tolist()],
                    reason='Replaced by Halo 2 scenario teleporter assets'))
                continue
            mapping[tag['name']] = target
            add(raw[:,[0,1,2,3,4,5,12,13]], faces, mat(target,'!)'))
            render_count += count

    # Original collision polygons include the sky seals and player clips.
    cb = c.block(c.root+176,96,True)[0]
    planes = [c.unpack('<4f', a) for a in c.block(cb+12,16,True)]
    edges = [c.unpack('<6I', a) for a in c.block(cb+72,24,True)]
    points = [c.unpack('<3f', a) for a in c.block(cb+84,16,True)]
    palette = [c.tagref(a)['name'] for a in c.block(c.root+164,20,True)]
    stats = collections.Counter()
    for si, at in enumerate(c.block(cb+60,12,True)):
        plane, first = c.unpack('<II', at)
        flags, _, shader = c.unpack('<Bbh', at+8)
        poly, visited, edge = [], set(), first
        while edge not in visited:
            visited.add(edge)
            start, end, forward, reverse, left, right = edges[edge]
            if left == si:
                poly.append(points[start]); edge = forward
            elif right == si:
                poly.append(points[end]); edge = reverse
            else:
                raise ValueError('Collision edge lost its source surface')
        if edge != first or len(poly) < 3:
            raise ValueError('Invalid CE collision polygon loop')
        normal = np.asarray(planes[plane & 0x7fffffff][:3]) * (-1 if plane >> 31 else 1)
        p = np.asarray(poly)
        # H1 BSP surfaces are convex polygons. Choose the fan origin with the
        # best worst triangle angle: an arbitrary origin can produce slivers
        # through nearly collinear boundary subdivisions. Every original
        # vertex, boundary segment and polygon remains intact.
        if len(p)>3:
            quality=[]
            for start in range(len(p)):
                q=np.roll(p,-start,axis=0)
                e=q[1:-1]-q[0];f=q[2:]-q[0]
                quality.append(np.min(np.linalg.norm(np.cross(e,f),axis=1)/
                    np.maximum(np.linalg.norm(e,axis=1)*np.linalg.norm(f,axis=1),1e-30)))
            p=np.roll(p,-int(np.argmax(quality)),axis=0)
        faces = np.array([(0,i,i+1) for i in range(1,len(p)-1)])
        cross = np.cross(p[faces[:,1]]-p[faces[:,0]],p[faces[:,2]]-p[faces[:,0]])
        if np.sum(cross@normal) < 0:
            faces = faces[:,::-1]
        if shader == -1:
            m = mat('+sky','')
            stats['sky_polygons'] += 1
        else:
            target = material(palette[shader]); mapping[palette[shader]] = target
            m = mat(target,'@' + ('%' if flags & 1 else ''))
        add(np.column_stack((p,np.tile(normal,(len(p),1)),np.zeros((len(p),2)))), faces, m)
        stats['collision_polygons'] += 1
        stats['collision_triangles'] += len(faces)
    if len(vertices) >= 65535 or len(triangles)*3 >= 65535:
        raise ValueError('Primary geometry exceeds Classic Tool section limits')

    ass = output/'data/scenarios/multi/dire_valley/structure/dire_valley.ass'
    ass.parent.mkdir(parents=True)
    with ass.open('w') as f:
        f.write('2\n"DIRE_VALLEY"\n"1.0"\n"Eric"\n"LOCAL"\n'+str(len(materials))+'\n')
        for name in materials:
            f.write(f'"{name}"\n""\n')
        # Keep the source render stream away from the collision plane chop:
        # H1's independently rounded render/collision streams differ by small
        # amounts. The complete original ground remains primary for finite
        # cluster/lightmapper bounds; other render faces are exact instances.
        primary, rendered = [], []
        for face in triangles:
            name=materials[face[0]]
            (rendered if '!' in name and not name.startswith('coag coag_ground') else primary).append(face)
        f.write(';### OBJECTS ###\n2\n')
        for batch in (primary,rendered):
            rows=np.asarray(batch)
            used,inverse=np.unique(rows[:,1:],return_inverse=True)
            f.write(f'"MESH"\n""\n""\n{len(used)}\n')
            for index in used:
                row=vertices[index]
                p,n,uv=np.array(row[:3])*100,row[3:6],row[6:8]
                f.write('%.9g\t%.9g\t%.9g\n%.9g\t%.9g\t%.9g\n0\n1\n%.9g\t%.9g\n'%(*p,*n,*uv))
            f.write(f'{len(batch)}\n')
            for m,(a,b,d) in zip(rows[:,0],inverse.reshape(-1,3)):
                f.write(f'{m}\n{a}\n{b}\n{d}\n')
        f.write(';### INSTANCES ###\n3\n-1\n"Scene Root"\n-1\n-1\n0\n'+IDENTITY+
                '0\n"level"\n1\n0\n0\n'+IDENTITY+
                '1\n"%source_render"\n2\n0\n0\n'+IDENTITY)
    np.savez_compressed(output/'source_geometry.npz', vertices=np.array(vertices),triangles=np.array(triangles))
    placements={}
    starts=[]
    for a in c.block(c.scenario+852,52):
        x,y,z,yaw,team,bsp,*types=c.unpack('<4f6h',a)
        starts.append(dict(position=[x,y,z],facing=yaw,team=team,bsp=bsp,types=types))
    placements['starts']=starts
    placements['netgame_flags']=[dict(zip(('x','y','z','facing','type','identifier'),c.unpack('<4f2h',a)))
        for a in c.block(c.scenario+888,148)]
    for name,field,stride,palfield in [('scenery',528,72,540),('vehicles',576,120,588)]:
        pal=[c.tagref(a) for a in c.block(c.scenario+palfield,48)]
        rows=[]
        for a in c.block(c.scenario+field,stride):
            index=c.unpack('<h',a)[0]
            if index<0:continue
            rows.append(dict(source=pal[index]['name'],position=c.unpack('<3f',a+8),rotation=c.unpack('<3f',a+20)))
        placements[name]=rows
    placements['equipment']=[]
    for a in c.block(c.scenario+900,144):
        placements['equipment'].append(dict(source=c.tagref(a+80)['name'],position=c.unpack('<3f',a+64),
            facing=c.unpack('<f',a+76)[0],types=c.unpack('<4h',a+4),spawn_seconds=c.unpack('<h',a+14)[0]))
    save(output/'source_placements.json',placements)
    save(output/'material_mapping.json',mapping)
    save(output/'provenance.json',dict(schema='H2_DIRE_VALLEY_CE_EXTRACTION_V1',name='Dire Valley',
        source=str(source),source_sha256=sha(source),source_edition='Installed MCC Halo CE Classic, build 01.03.43.0000',
        source_render_triangles=len(indices),render_triangles=render_count,removed_ce_teleporter_surfaces=removed,
        geometry_scale='One CE world unit = one Halo 2 world unit = 100 ASS units',
        bounds_world=[np.array(vertices)[:,:3].min(axis=0).tolist(),np.array(vertices)[:,:3].max(axis=0).tolist()],
        geometry=dict(vertices=len(vertices),triangles=len(triangles),**stats),ass_sha256=sha(ass),
        starts=len(starts),teleporter_flags=[v for v in placements['netgame_flags'] if v['type'] in (6,7)],
        runtime_verified=False))
    print(json.dumps(dict(output=str(output),triangles=len(triangles),**stats)))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    a=p.parse_args()
    extract(a.source,a.output)
