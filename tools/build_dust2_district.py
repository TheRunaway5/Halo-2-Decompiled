#!/usr/bin/env python3
"""Construct Deserted from measured Dust II routes and an editable District plan.

No CS2 render meshes or render decimation are used for the architecture.
Floors are closed architectural profiles; masonry is built as solid city lots.
Run under tools/run_test.py with manifold3d 3.5.4 and shapely 2.2.0.
"""
import argparse
import collections
import json
import math
import os
from pathlib import Path
import shutil
import copy
import io
from PIL import Image

import manifold3d as mf
import numpy as np
from shapely import union_all, make_valid
from shapely.geometry import Polygon, Point, LineString, box
from shapely.geometry.polygon import orient

from build_dust2_mombasa import Glb, save_json, sha, write_ass, dds
from refine_dust2_environment import SceneWriter


def polygons(geometry):
    if geometry.is_empty:
        return []
    if geometry.geom_type == 'Polygon':
        return [geometry]
    if not hasattr(geometry,'geoms'):
        return []
    return [p for g in geometry.geoms for p in polygons(g)]


def section(polygon):
    polygon = orient(polygon, sign=1.)
    loops = [np.array(polygon.exterior.coords)[:-1]]
    loops.extend(np.array(r.coords)[:-1] for r in polygon.interiors)
    return mf.CrossSection(loops, mf.FillRule.NonZero)


def role(solid, value):
    assert solid.status() == mf.Error.NoError, solid.status()
    return solid.set_properties(1, lambda _p, _old: [float(value)])


def extrude(polygon, bottom, top, value):
    return role(section(polygon).extrude(top-bottom).translate([0, 0, bottom]), value)


def build(plan, reference, retail, source, output):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert output.resolve().is_relative_to(scratch) and not output.exists()
    output.mkdir()
    config = json.loads(plan.read_text())
    data = json.loads(reference.read_text())
    grid = config['coordinate_grid']
    floor_offset = config['floor_reference_offset']
    clearance = config['route_edge_clearance']
    wreck = json.loads((retail/'replacement_collision.json').read_text())
    car_low, car_high = np.array(wreck['source_car_bounds'])
    area_rows = []
    for area in data['areas']:
        p = np.round(np.array(area['corners_halo_world'])/grid)*grid
        p[:, 2] += floor_offset
        if (car_low[:2] <= p[:, :2].mean(axis=0)).all() and (p[:, :2].mean(axis=0) <= car_high[:2]).all() and p[:, 2].mean() > car_low[2]+.12:
            continue
        polygon = make_valid(Polygon(p[:, :2]))
        if polygon.is_empty or polygon.area < 1e-7:
            continue
        a, b, c = np.linalg.lstsq(np.column_stack([p[:, :2], np.ones(len(p))]), p[:, 2], rcond=None)[0]
        area_rows.append(dict(id=area['id'], polygon=polygon, plane=(a, b, c), z=float(p[:, 2].mean())))
    walk = make_valid(union_all([r['polygon'] for r in area_rows]).buffer(clearance, join_style='mitre'))
    walk = make_valid(walk.simplify(config['outline_tolerance'], preserve_topology=True))
    walk = union_all([walk,box(*car_low[:2],*car_high[:2]).buffer(.08,join_style='mitre')])
    for corridor in config.get('clear_passages', []):
        walk = union_all([walk, LineString(corridor['points']).buffer(
            corridor['width']/2, cap_style='square', join_style='mitre')])
    street_zone = union_all([box(*r) for r in config.get('street_bounds', [])])
    street = walk.intersection(street_zone)
    street_config = config.get('streets', {})
    sidewalk_width = street_config.get('sidewalk_width', .4)
    curb_width = street_config.get('curb_width', .1)
    curb_height = street_config.get('curb_height', .05)
    sidewalk = walk.difference(walk.buffer(-sidewalk_width+curb_width, join_style='mitre')).intersection(street)
    curb = walk.buffer(-sidewalk_width+curb_width, join_style='mitre').difference(
        walk.buffer(-sidewalk_width, join_style='mitre')).intersection(street)
    # Keep intersections and narrow doorways at road level. The beveled curb
    # profile also admits crossing anywhere along the street edge.
    cuts = union_all([LineString(r['points']).buffer(r.get('cut_width', .35),
        cap_style='square', join_style='mitre') for r in config.get('clear_passages', [])])
    sidewalk = sidewalk.difference(cuts)
    curb = curb.difference(cuts)
    road = street.difference(union_all([sidewalk, curb]))
    print('Measured floor profiles', len(area_rows), 'walk area', walk.area, flush=True)

    # The navigation is a measured route/height reference, not the render
    # tessellation. Authored planar grades and a separate Short A deck preserve
    # the CT underpass without terrain interpolation through elevated routes.
    centers = np.array([np.array(r['polygon'].centroid.coords[0]) for r in area_rows])
    heights = np.array([r['z'] for r in area_rows])
    def ground_z(xy, street_surface=False):
        result = np.zeros(len(xy))
        for grade in config['ground_sections']:
            x0, y0, x1, y1 = grade['bounds']
            a, b, c = grade['plane']
            inside = (xy[:, 0] >= x0) & (xy[:, 0] <= x1) & (xy[:, 1] >= y0) & (xy[:, 1] <= y1)
            result[inside] = a*xy[inside, 0]+b*xy[inside, 1]+c
        under = (xy[:, 0] > 2.05) & (xy[:, 0] < 4.3) & (xy[:, 1] > 17.) & (xy[:, 1] < 21.3)
        result[under] = -1.025
        if street_surface:
            from shapely import points, contains, distance
            pts=points(xy)
            result[contains(sidewalk,pts)] += curb_height
            inside=contains(curb,pts)
            result[inside] += curb_height*np.clip((sidewalk_width-distance(walk.boundary,pts[inside]))/curb_width,0,1)
        return result

    # Each authored grade has an exactly planar top. Region boundaries become
    # deliberate retaining faces, never interpolated triangular terrain humps.
    # Solid columns extend to the foundation and union into a closed volume.
    floor_parts = []
    # Cover every city lot, including tiny discarded masonry slivers. Closed
    # individual slabs alone did not guarantee ground beneath those gaps.
    remaining=box(*config['city_bounds'])
    floor_rows=[]
    def add_floor(shape, plane, name):
        a,b,c=plane
        for mask,value,lift in [(sidewalk,9,curb_height),(curb,10,None),(road,8,0.)]:
            part=shape.intersection(mask)
            shape=shape.difference(mask)
            add_profile(part,value,lift)
        add_profile(shape,0,0.)

    def add_profile(shape,value,lift):
        from shapely import points, distance
        for polygon in polygons(shape):
            if polygon.area < 1e-9:continue
            slab=section(polygon).extrude(1.)
            bottom=config['foundation_z']
            def warp(p):
                a,b,c=current_plane
                elevation=a*p[:,0]+b*p[:,1]+c
                if lift is None:
                    elevation+=curb_height*np.clip((sidewalk_width-distance(walk.boundary,points(p[:,:2])))/curb_width,0,1)
                else:elevation+=lift
                return np.column_stack((p[:,:2],bottom+p[:,2]*(elevation-bottom)))
            floor_parts.append(role(slab.warp_batch(warp),value))
            floor_rows.append(dict(name=current_name,area=polygon.area,plane=current_plane,material_role=value))
    for grade in reversed(config['ground_sections']):
        region=box(*grade['bounds'])
        shape=remaining.intersection(region)
        remaining=remaining.difference(region)
        current_plane=grade['plane'];current_name=grade['name']
        add_floor(shape,current_plane,current_name)
    current_plane=[0,0,0];current_name='street datum'
    add_floor(remaining,current_plane,current_name)
    bridge = box(2.19, 16.99, 5.15, 22.65).intersection(walk.buffer(.06))
    for polygon in polygons(bridge):
        floor_parts.append(extrude(polygon, .59, .80, 0))
    floor = mf.Manifold.batch_boolean(floor_parts, mf.OpType.Add).set_tolerance(.00001).simplify(.00001)
    print('Continuous new ground', floor.num_tri(), 'triangles', flush=True)

    # New masonry footprints occupy the measured blocks between routes. Lots
    # provide intentional storey/roof changes instead of imported faceted art.
    bounds = config['city_bounds']
    city = box(*bounds).difference(walk)
    city_blocks = polygons(city)
    lots, lot_rows = [], []
    width = config['lot_width']
    lot_index = 0
    for ix, x in enumerate(np.arange(bounds[0], bounds[2], width)):
        for iy, y in enumerate(np.arange(bounds[1], bounds[3], width)):
            for shape in polygons(city.intersection(box(x, y, min(x+width, bounds[2]), min(y+width, bounds[3])))):
                if shape.area < .035:
                    continue
                center = np.array(shape.centroid.coords[0])
                nearby = np.argsort(np.linalg.norm(centers-center, axis=1))[:5]
                ground = float(np.median(heights[nearby]))
                # Tiny isolated obstructions become supply crates/utility
                # housings. Full city blocks receive two-to-four storeys.
                small = any(b.area < 2.0 and b.covers(shape.representative_point()) for b in city_blocks)
                top = ground + (.65 if small else config['building_height']+config['building_height_variation'][(ix*3+iy)%5])
                value = 5 if small else 1+(ix+iy)%3
                shape = make_valid(shape.buffer(config['facade_overlap'], join_style='mitre'))
                for polygon in polygons(shape):
                    building = extrude(polygon, config['foundation_z'], top, value)
                    if not small:
                        parapet = polygon.difference(polygon.buffer(-config['roof_parapet_width'], join_style='mitre'))
                        for ring in polygons(parapet):
                            building += extrude(ring, top-.015, top+config['roof_parapet_height'], 4)
                    lots.append(building)
                    lot_rows.append(dict(id=lot_index, footprint=list(polygon.exterior.coords), ground=ground,
                        top=top, material_role=value, small_cover=small))
                    lot_index += 1
    print('New closed masonry lots', len(lot_rows), flush=True)

    # Concrete tunnel shells are modelled as roofs meeting the new walls.
    # Their bottom faces, rather than a sky card, form the interior ceilings.
    roofs = []
    for rectangle, low, high in [((-17.6,0,-11,5.7),1.25,1.43),
            ((-17,5.7,-10.5,10.8),1.45,1.63),
            ((-17,10.8,-15.7,13.3),1.45,1.63),
            ((-13.2,9.4,-10.5,13.1),1.45,1.63),
            ((-9,10.8,-4.2,13.1),.30,.48)]:
        for shape in polygons(walk.intersection(box(*rectangle)).buffer(.06, join_style='mitre')):
            roofs.append(extrude(shape, low, high, 4))
    # A continuous sloping stairwell ceiling follows the descent. The old flat
    # low-tunnel roof started above the ramp before a standing player fitted.
    for shape in polygons(walk.intersection(box(-10.5,9.4,-9,13.1)).buffer(.06,join_style='mitre')):
        roof=section(shape).extrude(.18).warp_batch(lambda p:np.column_stack((p[:,:2],
            p[:,2]+1.45-1.15*np.clip((p[:,0]+10.5)/1.5,0,1))))
        roofs.append(role(roof,4))

    # Recessed framed windows and masonry plinths follow the actual exposed
    # facade edges. All are closed volumes embedded into the wall by 3 cm.
    frames, pockets, facade_rows = [], [], []
    frame = role(mf.Manifold.cube([.46,.08,.48]),5)
    aperture = role(mf.Manifold.cube([.38,.1,.40]).translate([.04,.03,.04]),5)
    inset = role(mf.Manifold.cube([.38,.025,.40]).translate([.04,.015,.04]),6)
    window = ((frame-aperture)+inset).translate([0,-.08,0])
    pocket = mf.Manifold.cube([.38,.13,.40]).translate([.04,-.1,.04])
    for lot in lot_rows:
        if lot['small_cover']:
            continue
        polygon = orient(Polygon(lot['footprint']), sign=1.)
        vertices = np.array(polygon.exterior.coords)
        for a,b in zip(vertices[:-1],vertices[1:]):
            tangent=b-a; length=np.linalg.norm(tangent)
            if length < .7:
                continue
            tangent/=length; outward=np.array([tangent[1],-tangent[0]])
            mid=(a+b)/2
            if not walk.covers(Point(mid+outward*.10)):
                continue
            ground=float(ground_z(mid[None,:])[0])
            if length > 1.2:
                plinth = role(mf.Manifold.cube([length,.07,.17]),4)
                origin=np.r_[a-outward*.07,ground-.03]
                transform=np.array([[tangent[0],outward[0],0,origin[0]],
                    [tangent[1],outward[1],0,origin[1]],[0,0,1,origin[2]]])
                frames.append(plinth.transform(transform))
            count=int((length-.25)/config['window_spacing'])
            for j in range(count):
                start=a+tangent*((j+.5)*length/max(count,1)-.23)
                for level in (1.15,2.18):
                    if ground+level+.55 > lot['top']-.2:
                        continue
                    transform=np.array([[tangent[0],outward[0],0,start[0]],
                        [tangent[1],outward[1],0,start[1]],[0,0,1,ground+level]])
                    frames.append(window.transform(transform))
                    pockets.append(pocket.transform(transform))
                    facade_rows.append(dict(position=[*start,ground+level], outward=outward.tolist()))
    print('Closed facade windows',len(facade_rows),flush=True)
    covers=[]
    for row in config['cover_boxes']:
        position=np.array(row['position']); size=np.array(row['size'])
        surface=float(ground_z(position[None,:2])[0])
        assert -.075 <= position[2]-surface <= .005,('Unsupported cover',row,surface)
        position[2] += float(ground_z(position[None,:2],True)[0])-surface
        # Supply housings are new, low polygon geometry. Their metal
        # lids and feet are solid, so there are no coplanar decorative cards.
        base=role(mf.Manifold.cube(size.tolist()).translate(position-np.r_[size[:2]/2,0]),5)
        lid_size=size+np.array([.035,.035,.02])
        lid=role(mf.Manifold.cube(lid_size.tolist()).translate(position-np.r_[lid_size[:2]/2,0]+[0,0,size[2]-.04]),4)
        covers.append(base+lid)
    fixtures=[]
    for x,y,z in [(-15,1.5,1.25),(-13.2,3.8,1.25),(-14.3,8.0,1.45),
            (-11.8,10,1.45),(-8,11.8,.30),(-5.3,11.8,.30),(3.1,18.4,.59),(3.1,20.1,.59)]:
        shell=role(mf.Manifold.cube([.58,.2,.06]).translate([x-.29,y-.1,z-.025]),5)
        pane=role(mf.Manifold.cube([.48,.12,.03]).translate([x-.24,y-.06,z-.04]),7)
        fixtures.append(shell+pane)
    # Adjacent lots overlap at masonry joints. Boolean assembly removes the
    # interior faces; the lot plan remains editable and no QEM pass runs.
    masonry = mf.Manifold.batch_boolean(lots, mf.OpType.Add)
    if pockets:
        masonry -= mf.Manifold.batch_boolean(pockets,mf.OpType.Add)
    architecture = mf.Manifold.batch_boolean([floor, masonry, *roofs, *frames, *covers, *fixtures],
        mf.OpType.Add).set_tolerance(config.get('architecture_tolerance',.0001)).simplify(config.get('architecture_tolerance',.0001))
    assert architecture.status() == mf.Error.NoError, architecture.status()
    print('Closed city blockout', architecture.num_tri(), 'triangles', flush=True)

    palette = json.loads((retail/'material_mapping.json').read_text())['palette']
    palette['floor'] = json.loads((source/'material_mapping.json').read_text())['palette']['floor']
    shutil.copytree(retail/'textures', output/'textures')
    for suffix in ('png', 'tga', 'dds'):
        shutil.copyfile(source/f'textures/floor.{suffix}', output/f'textures/floor.{suffix}')
    imported=Path(json.loads((source/'provenance.json').read_text())['sources']['mcc_cache_root'])
    inventory=json.loads((imported/'bitmap-resources.json').read_text())
    road_name='scenarios\\bitmaps\\solo\\earthcity\\base_road1'
    road_bank=next(b for b in inventory['banks'] if b['tag']==road_name)
    road_frame=road_bank['frames'][0];resource=inventory['resources'][road_frame['partitions'][0]]
    raw=imported/resource['path'];assert sha(raw)==resource['sha256']
    blob=dds(*road_frame['dimensions'][:2],road_frame['format'],raw.read_bytes(),road_frame['additional_mips'])
    (output/'textures/road.dds').write_bytes(blob)
    road_image=Image.open(io.BytesIO(blob)).convert('RGBA');road_image.putalpha(255)
    road_image.save(output/'textures/road.png');road_image.save(output/'textures/road.tga')
    palette['road']=dict(shader='scenarios\\shaders\\solo\\earthcity\\base_paver',bitmap=road_name,
        map=road_bank['map'],source_sha256=resource['sha256'],png='textures/road.png',tga='textures/road.tga')
    sand_name='scenarios\\bitmaps\\detail_maps\\nature\\ground\\sand'
    sand_bank=next(b for b in inventory['banks'] if b['tag']==sand_name)
    frame=sand_bank['frames'][0]; resource=inventory['resources'][frame['partitions'][0]]
    raw=imported/resource['path']; assert sha(raw)==resource['sha256']
    blob=dds(*frame['dimensions'][:2],frame['format'],raw.read_bytes(),frame['additional_mips'])
    (output/'textures/desert_sand.dds').write_bytes(blob)
    sand_image=Image.open(io.BytesIO(blob)).convert('RGBA')
    sand_image.save(output/'textures/desert_sand.png');sand_image.save(output/'textures/desert_sand.tga')
    palette['desert_sand'].update(shader='scenarios\\multi\\dust2_mombasa\\shaders\\desert_sand',
        bitmap=sand_name,map=sand_bank['map'],source_sha256=resource['sha256'])
    writer = SceneWriter()
    material_ids = {}
    for key, row in palette.items():
        material_ids[key] = len(writer.doc['materials'])
        tex = writer.texture(output/row['png'])
        material = dict(name=row['shader'], pbrMetallicRoughness=dict(baseColorTexture=dict(index=tex),
            metallicFactor=0, roughnessFactor=.9), extras=dict(halo2_shader=row['shader']))
        if key in ('plant', 'fence') or 'frond' in row['shader']:
            material.update(alphaMode='MASK', alphaCutoff=.5, doubleSided=True)
        writer.doc['materials'].append(material)

    def emit(name, p, t, key, normals=None, uv=None):
        if not len(t):
            return
        if normals is None:
            p = p[t.reshape(-1)]
            t = np.arange(len(p), dtype='<u4').reshape(-1, 3)
            n = np.cross(p[t[:, 1]]-p[t[:, 0]], p[t[:, 2]]-p[t[:, 0]])
            length = np.linalg.norm(n, axis=1)
            assert (length > 1e-12).all(), (name, p[t[length <= 1e-12]].tolist())
            normals = np.repeat(n/length[:, None], 3, axis=0)
        if uv is None:
            axes = np.argmax(abs(normals), axis=1)
            uv = np.empty((len(p), 2))
            repeat = street_config.get('texture_repeat_meters',3.) if key in ('road','walkway') else config['floor_texture_repeat_meters'] if key == 'floor' else config['wall_texture_repeat_meters']
            for axis, plane in enumerate(([1, 2], [0, 2], [0, 1])):
                scale=np.array([repeat,9.]) if key in ('wall','white','stucco') and axis!=2 else repeat
                uv[axes == axis] = p[axes == axis][:, plane]*3.048/scale
        # Tool's chart adjacency follows render indices. Isolated triangle
        # corners create separate lightmap charts even on a single flat wall.
        # Share identical position/normal/UV tuples; hard edges and UV seams
        # deliberately retain separate vertices.
        attributes=np.column_stack((p,normals,uv)).astype('<f4')
        _,first,inverse=np.unique(attributes,axis=0,return_index=True,return_inverse=True)
        p,normals,uv=p[first],normals[first],uv[first]
        t=inverse[t].astype('<u4')
        primitive = dict(attributes=dict(POSITION=writer.array(p[:, [1, 2, 0]]*3.048, 'VEC3'),
            NORMAL=writer.array(normals[:, [1, 2, 0]], 'VEC3'), TEXCOORD_0=writer.array(uv, 'VEC2')),
            indices=writer.array(t.reshape(-1), 'SCALAR'), material=material_ids[key])
        writer.doc['meshes'].append(dict(name=name, primitives=[primitive]))
        writer.doc['nodes'].append(dict(name=name, mesh=len(writer.doc['meshes'])-1))
        writer.doc['scenes'][0]['nodes'].append(len(writer.doc['nodes'])-1)

    mesh = architecture.to_mesh64()
    properties = np.array(mesh.vert_properties)
    p, t = properties[:, :3], np.array(mesh.tri_verts, dtype='<u4')
    roles = np.rint(properties[t, 3].mean(axis=1)).astype(int)
    before_serialization=len(t)
    # Material identities belong to faces during serialization repair. Keeping
    # them as vertex attributes would prevent removal of collinear seam verts.
    # Round-trip the exact glTF float positions through Manifold, preserving the
    # face IDs while it retriangulates zero-area boundary slivers.
    for attempt in range(4):
        q=np.round((p*3.048).astype('<f4').astype(float),5)/3.048
        if config.get('serialization_grid'):
            q=np.round(q/config['serialization_grid'])*config['serialization_grid']
        serialized=mf.Mesh64(q,np.asarray(t,dtype=np.uint64),face_id=np.asarray(roles,dtype=np.uint64))
        serialized.merge()
        repaired=mf.Manifold(serialized)
        assert repaired.status()==mf.Error.NoError,repaired.status()
        mesh=repaired.to_mesh64()
        p=np.array(mesh.vert_properties)[:,:3]
        t=np.array(mesh.tri_verts,dtype='<u4')
        roles=np.array(mesh.face_id,dtype=int)
        q=np.round((p*3.048).astype('<f4').astype(float),5)/3.048
        area=np.linalg.norm(np.cross(q[t[:,1]]-q[t[:,0]],q[t[:,2]]-q[t[:,0]]),axis=1)
        if (area>1e-12).all():
            p=q
            break
    assert (area>1e-12).all(),'Serialized architecture has degenerate triangles'
    collapsed=before_serialization-len(t)
    roles_to_keys = {0:'floor', 1:'wall', 2:'white', 3:'stucco', 4:'concrete', 5:'metal',6:'glass',7:'light',8:'road',9:'walkway',10:'concrete'}
    for value, key in roles_to_keys.items():
        emit('architecture_facade_pane' if key=='glass' else 'architecture_emitter' if key=='light' else 'architecture_'+key, p, t[roles == value], key)
    # Keep the retail prop assets, not the rejected city's render geometry.
    prop_source = Glb(retail/'dust2_mombasa.glb')
    old_keys = list(json.loads((retail/'material_mapping.json').read_text())['palette'])
    old_placements=json.loads((retail/'redesign.json').read_text())['placements']
    old_palms=[r for r in old_placements if r['kind']=='palm']
    placements=[]
    for i,position in enumerate(config['palm_positions']):
        assert abs(float(ground_z(np.array(position)[None,:2])[0])-position[2])<.005,('Unsupported palm',position)
        position=position.copy();position[2]=float(ground_z(np.array(position)[None,:2],True)[0])
        placements.append(dict(old_palms[i],position=position))
    hog=copy.deepcopy(next(r for r in old_placements if r['kind']=='warthog'))
    old_hog=np.array(hog['position']);hog['position'][2]=0
    placements.append(hog)
    retained = []
    for _, node, mesh, world in prop_source.nodes():
        name = node.get('name', '')
        if not name.startswith(('palmtree_', 'warthog_destroyed_', 'district_desert_')):
            continue
        palm_index=int(name.split('_')[-2]) if name.startswith('palmtree_') else None
        if palm_index is not None and palm_index>=len(config['palm_positions']):
            continue
        retained.append(name)
        for prim in mesh['primitives']:
            q = prop_source.array(prim['attributes']['POSITION']).astype(float)
            q = (q @ world[:3, :3].T+world[:3, 3])[:, [2, 0, 1]]/3.048
            if palm_index is not None:
                q+=np.array(placements[palm_index]['position'])-np.array(old_palms[palm_index]['position'])
            if name.startswith('warthog_destroyed_'):
                q+=np.array(hog['position'])-old_hog
            n = prop_source.array(prim['attributes']['NORMAL']).astype(float)[:, [2, 0, 1]]
            f = prop_source.array(prim['indices'])[:, 0].reshape(-1, 3)
            emit(name, q, f, old_keys[prim['material']], n, prop_source.array(prim['attributes']['TEXCOORD_0']))
    writer.write(output/'dust2_mombasa.glb', generator='Deserted — new closed architectural construction')
    destination = output/'data/scenarios/multi/dust2_mombasa/structure'
    destination.mkdir(parents=True)
    rows = [dict(palette=k) for k in palette]
    geometry = write_ass(Glb(output/'dust2_mombasa.glb'), destination/'dust2_mombasa.ass', palette, rows,
        shared_collision_materials=set(roles_to_keys.values()))
    geometry['architecture_shares_render_collision_and_lightmap_surfaces']=True
    backdrop=SceneWriter(Glb(output/'dust2_mombasa.glb'))
    backdrop.doc['scenes'][0]['nodes']=[i for i,n in enumerate(backdrop.doc['nodes']) if n['name'].startswith('district_desert_')]
    backdrop.write(output/'desert_backdrop.glb')
    write_ass(Glb(output/'desert_backdrop.glb'), destination/'desert_backdrop.ass', palette, rows)
    for leaf in ('source_entities.json', 'lighting.json'):
        shutil.copyfile(retail/leaf, output/leaf)
    legacy=json.loads((retail/'provenance.json').read_text())
    save_json(output/'provenance.json',dict(schema='H2_DESERTED_AUTHORED_MODEL_V1',name=config['name'],
        sources=dict(mcc_cache_root=legacy['sources']['mcc_cache_root'],
            measured_layout_sha256=sha(reference),architecture_plan_sha256=sha(plan)),
        render=dict(geometry,bounds_ass=legacy['render']['bounds_ass'],
            bounds_role='Preserved enclosing compiler sky-shell bounds from the reference package'),
        architecture_rebuilt=True,reused_cs2_render_triangles=0,axes=legacy['axes'],scale=legacy['scale'],
        certified_one_to_one_runtime=False,
        reference_provenance=dict(role='Historical layout/material reference only; not the current render or collision geometry',data=legacy)))
    save_json(output/'replacement_collision.json',dict(schema='H2_DESERTED_CAR_FOOTPRINT_REFERENCE_V1',
        role='Measured old car footprint only; current collision is described in collision-authoring.json',
        source_car_bounds=wreck['source_car_bounds'],reference_sha256=sha(retail/'replacement_collision.json')))
    save_json(output/'material_mapping.json', dict(palette=palette, materials=rows))
    save_json(output/'authored-lots.json', lot_rows)
    save_json(output/'authored-ground.json',floor_rows)
    shutil.copyfile(plan, output/'architecture-plan.json')
    save_json(output/'authored-facades.json',facade_rows)
    save_json(output/'redesign.json', dict(schema='H2_DUST2_NEW_DISTRICT_ARCHITECTURE_V1', plan_sha256=sha(plan),
        name=config['name'], measured_layout_sha256=sha(reference), geometry=geometry, architecture_triangles=len(t),
        reused_cs2_render_triangles=0, retained_retail_and_desert_meshes=retained,
        placements=placements,
        floor_profiles=len(area_rows), masonry_lots=len(lot_rows), coincident_edge_triangles_collapsed=collapsed, runtime_verified=False,
        state='architectural blockout; detail, collision and lighting review pending'))
    # Preserve the new solid as the collision authoring reference. The final
    # collision writer also adds the actual Warthog hull and palm trunks.
    np.savez(output/'architecture-solid.npz', positions=p, triangles=t, roles=roles)
    print('Saved new architecture', output, flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('plan','reference','retail','source','output'):
        parser.add_argument('--'+name, required=True, type=Path)
    build(**vars(parser.parse_args()))
