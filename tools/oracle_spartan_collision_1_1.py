#!/usr/bin/env python3
"""Untouched original Spartan posed collision BSP queries, with explicit poses.

Executes original model/permutation lookup, inverse node transform, 3D/2D BSP
and surface traversal. No native collision implementation supplies expectations.
World broadphase/location, animation-pose production and live unit spawn are
outside the query slice; input poses/permutations/segments are recorded fixtures.
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import (
    ROOT, PIN, CACHE_PIN, Trace, Cache, Xbe, _Code, _trace, TAGS, UNIT, UI,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_XMM0,
    UC_X86_REG_XMM1,
)

CONTEXT, RESULT, ORIGIN, DELTA, INPUT_MATRIX, OUTPUT_MATRIX = (
    0x10F4000, 0x10F5000, 0x10F6000, 0x10F6100, 0x10F6200, 0x10F6300)
POSE_OFFSET = 0x1000
PERM_OFFSET = 0xD00


def unpack_floats(raw):
    return list(struct.unpack('<'+'f'*(len(raw)//4), raw))


class CollisionTrace(Trace):
    def __init__(self):
        super().__init__(1)
        del self.services[0x1E20C0]
        self.query_events = []
        self.query_returns = []
        self.query_active = False
        c = Cache(ROOT/'assets/disc/maps/lockout.map')
        assert hashlib.sha256(c.data).hexdigest() == CACHE_PIN
        self.cache_magic=c.magic
        for datum, tag in c.tags.items():
            self.g.mem_write(TAGS+(datum&0xFFFF)*16,
                            tag['group'].encode()[::-1])
        spartan = next(t for t in c.tags.values() if t['group'] == 'bipd' and
                       t['name'] == 'objects\\characters\\masterchief\\masterchief_mp')
        model = c.tags[c.u(spartan['offset']+0x38)]
        coll = c.tags[c.u(model['offset']+0xC)]
        self.w(UNIT, int(spartan['datum'], 16))
        self.w(UNIT+0xC, -1); self.w(UNIT+0x10, -1)
        self.sh(UNIT+0x116, POSE_OFFSET)
        regions = c.block(model['offset']+0x70, 0x10)
        self.region_count = len(regions)
        self.sh(UNIT+0x118, self.region_count*10)
        self.sh(UNIT+0x11A, PERM_OFFSET)
        self.g.mem_write(UNIT+PERM_OFFSET, bytes(self.region_count*10))
        self.metadata = {'spartan': spartan, 'model': model, 'collision': coll,
                         'model_regions': [], 'nodes': [], 'bsps': []}
        string = lambda sid: c.text(c.u(368)+c.u(c.u(364)+(sid&0xFFFF)*4))
        for i,at in enumerate(regions):
            self.metadata['model_regions'].append({
                'index': i, 'name': string(c.u(at)), 'file_offset': at,
                'collision_region_index': c.data[at+4],
                'permutations': [{'name': string(c.u(p)), 'file_offset': p,
                                  'collision_permutation_index': c.data[p+5]}
                                 for p in c.block(at+8, 8)]})
        self.default_poses = []
        identity = struct.pack('<13f', 1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.)
        for i,at in enumerate(c.block(model['offset']+0x78, 0x5C)):
            inverse = bytes(c.data[at+0x28:at+0x5C])
            self.g.mem_write(INPUT_MATRIX, inverse)
            self.active = 'authored_inverse_matrix_input'
            self.run(0x142D00, regs=((UC_X86_REG_ECX, INPUT_MATRIX),
                                    (UC_X86_REG_EAX, OUTPUT_MATRIX)))
            pose = bytes(self.g.mem_read(OUTPUT_MATRIX, 0x34))
            self.default_poses.append(pose)
            self.metadata['nodes'].append({
                'index': i, 'name': string(c.u(at)), 'file_offset': at,
                'authored_default_inverse_hex': inverse.hex(),
                'original142d00_inverse_result_hex': pose.hex()})
            self.g.mem_write(UNIT+POSE_OFFSET+i*0x34, identity)
        for ri,region in enumerate(c.block(coll['offset']+0x1C, 12)):
            for pi,perm in enumerate(c.block(region+4, 20)):
                for bi,bsp in enumerate(c.block(perm+4, 0x44)):
                    vertices = c.block(bsp+0x3C, 0x10)
                    points = [unpack_floats(bytes(c.data[v:v+12])) for v in vertices]
                    self.metadata['bsps'].append({
                        'region_index': ri, 'region_name': string(c.u(region)),
                        'permutation_index': pi, 'bsp_index': bi,
                        'file_offset': bsp, 'node_index': c.h(bsp),
                        'vertex_count': len(vertices),
                        'authored_vertex_average_fixture': [
                            sum(p[k] for p in points)/len(points) for k in range(3)],
                        'blocks': [{'relative_offset': f'{off:02x}', 'stride': stride,
                                    'count': c.u(bsp+off), 'file_offset': c.u(bsp+off+4)-c.magic}
                                   for off,stride in ((4,8),(12,16),(20,4),(28,4),
                                                      (36,16),(44,8),(52,12),(60,16))],
                        'surfaces': [{'index': i, 'file_offset': at,
                                      'raw_hex': bytes(c.data[at:at+8]).hex(),
                                      'material_entry': c.h(at+6)}
                                     for i,at in enumerate(c.block(bsp+0x2C,8))]})
        self.node_count = len(self.default_poses)
        self.model_materials = c.block(model['offset']+0x58, 0x14)
        self.metadata['model_materials'] = [
            {'index': i, 'name': string(c.u(at)), 'file_offset': at,
             'damage_section': c.h(at+6), 'global_material': c.h(at+0x10),
             'raw_hex': bytes(c.data[at:at+0x14]).hex()}
            for i,at in enumerate(self.model_materials)]
        c.data.close(); c.stream.close()
        self.active = 'normal_projectile_mask_conversion'
        self.run(0x16A560, regs=((UC_X86_REG_ECX,0x2480000F),))
        self.normal_flags = self.g.reg_read(UC_X86_REG_EAX)
        assert self.normal_flags == 9

    def hook(self, guest, address, size, user):
        if getattr(self, 'query_active', False):
            sp = guest.reg_read(UC_X86_REG_ESP)
            if address == 0x1E26C3:
                self.query_events.append({'instruction':'001e26c3',
                    'edge_pointer':f'{guest.reg_read(UC_X86_REG_ESI):08x}',
                    'cross_bits':f'{guest.reg_read(UC_X86_REG_XMM1)&0xFFFFFFFF:08x}'})
            # Recursion can share a return address. Only the newest matching
            # frame has returned; do not label still-active outer frames.
            for ret,row in reversed(self.query_returns[:]):
                if ret == address:
                    row['return_eax'] = f'{guest.reg_read(UC_X86_REG_EAX):08x}'
                    row['return_xmm0_low'] = f'{guest.reg_read(UC_X86_REG_XMM0)&0xFFFFFFFF:08x}'
                    if row['entry'] == '001e20c0':
                        descriptor = int(row['stack'][0],16)
                        output = self.u(descriptor+0x18)
                        row['lower_output_hex'] = bytes(guest.mem_read(output,0x420)).hex()
                    if row['entry'] == '001f3500':
                        row['context_hex'] = bytes(guest.mem_read(
                            int(row['registers']['esi'],16),20)).hex()
                    self.query_returns.remove((ret,row))
                    break
            if address in (0x1F3500, 0x1F3900, 0x1F3590, 0x1F3660,
                           0x142D00, 0x1E20C0, 0x1E23F0, 0x29C7D0,
                           0x1E25C0, 0xDA0E0, 0x16A5C0):
                row = {'entry': f'{address:08x}', 'registers': self.registers(),
                       'stack': [f'{self.u(sp+4+i*4):08x}' for i in range(5)]}
                if address == 0x1E20C0:
                    descriptor = self.u(sp+4)
                    row['descriptor_hex'] = bytes(guest.mem_read(descriptor, 40)).hex()
                    row['local_origin'] = self.vec(self.u(descriptor+0x10))
                    row['local_delta'] = self.vec(self.u(descriptor+0x14))
                    row['current_fraction_bits'] = f'{self.u(self.u(descriptor+0x18)):08x}'
                if address == 0x29C7D0:
                    row['projected_point'] = {'hex': bytes(guest.mem_read(guest.reg_read(UC_X86_REG_EDX),8)).hex()}
                self.query_events.append(row)
                self.query_returns.append((self.u(sp),row))
        super().hook(guest,address,size,user)

    def poses(self, kind='identity', shift=(0.,0.,0.), scale=1., rotate=False):
        identity = struct.pack('<13f', 1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.)
        result = []
        for i, default in enumerate(self.default_poses):
            values = unpack_floats(default if kind == 'authored_inverse' else identity)
            if rotate:
                for at in (1,4,7):
                    values[at],values[at+1] = -values[at+1],values[at]
                values[10],values[11] = -values[11],values[10]
            values[0] *= scale
            values[10:] = [values[10+k]*scale+shift[k] for k in range(3)]
            raw = struct.pack('<13f',*values)
            self.g.mem_write(UNIT+POSE_OFFSET+i*0x34, raw)
            result.append(raw.hex())
        return result

    def query(self, origin, delta, permutations=None, flags=None, upper=True,
              previous_fraction=1.):
        permutations = [0]*self.region_count if permutations is None else permutations
        assert len(permutations) == self.region_count
        self.g.mem_write(UNIT+PERM_OFFSET, bytes(v&0xFF for v in permutations))
        self.g.mem_write(ORIGIN,struct.pack('<3f',*origin))
        self.g.mem_write(DELTA,struct.pack('<3f',*delta))
        self.g.mem_write(RESULT,bytes(0x440))
        if upper:self.f(RESULT+4,previous_fraction)
        self.query_events = []; self.query_returns = []
        self.query_active = True; self.active = 'original_collision_query'
        event_start = len(self.events)
        if upper:
            self.run(0x16AF90,(UI,1,0x2480000F,self.normal_flags if flags is None else flags,
                              ORIGIN,DELTA,0xFFFFFFFF,0xFFFFFFFF,RESULT))
            raw = bytes(self.g.mem_read(RESULT,0x5C))
            decoded = {'kind': struct.unpack_from('<I',raw,0)[0],
                       'fraction_bits': f'{struct.unpack_from("<I",raw,4)[0]:08x}',
                       'fraction': struct.unpack_from('<f',raw,4)[0],
                       'normal': self.vec(RESULT+0x28),
                       'plane_distance_bits': f'{self.u(RESULT+0x34):08x}',
                       'object': f'{self.u(RESULT+0x40):08x}',
                       'region': struct.unpack_from('<h',raw,0x44)[0],
                       'node': struct.unpack_from('<h',raw,0x46)[0],
                       'packed_bsp': f'{self.u(RESULT+0x48):08x}',
                       'surface': self.u(RESULT+0x50),
                       'model_material_entry': struct.unpack_from('<h',raw,0x5A)[0],
                       'global_material': struct.unpack_from('<h',raw,0x24)[0]}
        else:
            self.run(0x1F3500,(UI,),((UC_X86_REG_ESI,CONTEXT),))
            self.run(0x1F3900,(CONTEXT,self.normal_flags if flags is None else flags,
                              ORIGIN,DELTA,RESULT))
            raw = bytes(self.g.mem_read(RESULT,0x24))
            decoded = {'packed_bsp': f'{self.u(RESULT):08x}',
                       'node': self.u(RESULT+4,2), 'region': self.u(RESULT+6,2),
                       'fraction_bits': f'{self.u(RESULT+8):08x}',
                       'fraction': self.fv(RESULT+8),
                       'plane_pointer': f'{self.u(RESULT+0xC):08x}',
                       'surface': self.u(RESULT+0x14),
                       'model_material_entry': self.u(RESULT+0x1E,2)}
        self.query_active = False
        returned = self.g.reg_read(UC_X86_REG_EAX)&0xFF
        services = [e for e in self.events[event_start:] if 'service' in e]
        assert not services, services
        return {'origin': self.vec(ORIGIN), 'delta': self.vec(DELTA),
                'caller_previous_fraction_bits': f'{float_bits(previous_fraction):08x}' if upper else None,
                'permutation_bytes': [v&0xFF for v in permutations],
                'lower_flags': self.normal_flags if flags is None else flags,
                'upper16af90': upper, 'returned_hit': bool(returned),
                'raw_result_hex': raw.hex(), 'decoded': decoded,
                'original_events': self.query_events,
                'external_services_executed': services}

    def edge_query(self,bsp,surface,axis,orientation,point):
        self.g.mem_write(DELTA,struct.pack('<2f',*point))
        self.query_events=[];self.query_returns=[]
        self.query_active=True;self.active='original_surface_edge_primitive'
        event_start=len(self.events)
        self.run(0x1E25C0,(0,surface,axis,orientation,DELTA),
                 ((UC_X86_REG_EAX,0),(UC_X86_REG_EDX,bsp+self.cache_magic+4)))
        self.query_active=False
        returned=self.g.reg_read(UC_X86_REG_EAX)&0xFF
        services=[e for e in self.events[event_start:] if 'service' in e]
        assert not services,services
        return {'complete_bsp_file_offset':bsp,'surface':surface,
                'projection_axis':axis,'projection_orientation':orientation,
                'point_bits':[f'{v:08x}' for v in struct.unpack('<2I',self.g.mem_read(DELTA,8))],
                'point_values':list(struct.unpack('<2f',self.g.mem_read(DELTA,8))),
                'breakable_bitset_pointer':'00000000','breakable_count':0,
                'returned_contains':bool(returned),'original_events':self.query_events,
                'external_services_executed':services}


def transform_fixture(matrix_hex, point):
    m = unpack_floats(bytes.fromhex(matrix_hex))
    return [m[0]*(m[1+k]*point[0]+m[4+k]*point[1]+m[7+k]*point[2])+m[10+k]
            for k in range(3)]


def float_bits(value):
    return struct.unpack('<I',struct.pack('<f',value))[0]


def bits_float(value):
    return struct.unpack('<f',struct.pack('<I',value))[0]


def generate_suite(directory):
    directory = directory.resolve(); directory.mkdir(parents=True,exist_ok=True)
    trace = CollisionTrace(); poses = []; cases = []
    for kind,shift,scale,rotate in [('identity',(0.,0.,0.),1.,False),
                                    ('authored_inverse',(0.,0.,0.),1.,False),
                                    ('authored_inverse',(10.,20.,30.),1.,True),
                                    ('authored_inverse',(2.,-3.,4.),2.,False)]:
        matrices = trace.poses(kind,shift,scale,rotate)
        pose_name = f'{kind}-shift{shift}-scale{scale}-rotate{rotate}'
        pose_index = len(poses)
        poses.append({'name': pose_name, 'matrices_hex': matrices,
                      'kind': kind, 'shift': shift, 'scale': scale, 'rotate90z': rotate})
        for bsp in trace.metadata['bsps']:
            center = transform_fixture(matrices[bsp['node_index']],
                                       bsp['authored_vertex_average_fixture'])
            for axis in range(3):
                origin = center[:]; origin[axis] -= 2.*scale
                delta = [0.,0.,0.]; delta[axis] = 4.*scale
                permutations = [-1]*trace.region_count
                permutations[bsp['region_index']] = 0
                result = trace.query(origin,delta,permutations)
                name = f'pose{pose_index}-region{bsp["region_index"]}-bsp{bsp["bsp_index"]}-axis{axis}'
                path = directory/f'{name}.json'
                result.update({'pose_index': pose_index,
                               'fixture_selected_bsp': bsp['file_offset'],
                               'oracle_tool_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()})
                path.write_text(json.dumps(result,indent=2)+'\n')
                cases.append({'name': name, 'pose_index': pose_index,
                              'report': str(path.relative_to(ROOT)),
                              'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                              'hit': result['returned_hit'], 'decoded': result['decoded']})
        # All active regions, all inactive, invalid active permutation and
        # alternate flags are distinct recorded input boundaries.
        for name, perms, flags in [('all_active',[0]*4,9),
                                    ('all_inactive',[-1]*4,9),
                                    ('invalid_permutation',[1]*4,9),
                                    ('flags0',[0]*4,0),('flags1',[0]*4,1),
                                    ('flags2',[0]*4,2),('flags3',[0]*4,3),
                                    ('flags256',[0]*4,0x100)]:
            origin = [shift[0]-2*scale,shift[1],shift[2]]
            result = trace.query(origin,(4.*scale,0.,0.),perms,flags)
            path = directory/f'pose{pose_index}-{name}.json'
            result['pose_index'] = pose_index
            path.write_text(json.dumps(result,indent=2)+'\n')
            cases.append({'name': f'pose{pose_index}-{name}', 'pose_index': pose_index,
                          'report': str(path.relative_to(ROOT)),
                          'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                          'hit': result['returned_hit'], 'decoded': result['decoded']})
    # Separate finite, explicit boundary inputs. Their original instruction
    # outputs are observations; no inferred geometric answer supplies expected.
    matrices=trace.poses('identity')
    pose_index=len(poses)
    poses.append({'name':'identity-boundaries','matrices_hex':matrices,
                  'kind':'identity','shift':(0.,0.,0.),'scale':1.,'rotate90z':False})
    canonical=trace.query((-2.,0.,0.),(4.,0.,0.))
    fraction_bits=int(canonical['decoded']['fraction_bits'],16)
    endpoint_delta=bits_float(float_bits(4.*canonical['decoded']['fraction']))
    boundary_inputs=[]
    for value in (fraction_bits-1,fraction_bits,fraction_bits+1,float_bits(0.),float_bits(1.)):
        boundary_inputs.append((f'previous-fraction-{value:08x}',(-2.,0.,0.),
                                (4.,0.,0.),[0]*4,9,True,bits_float(value)))
    delta_bits=float_bits(endpoint_delta)
    for offset in (-2,-1,0,1,2):
        for upper in (False,True):
            boundary_inputs.append((f'endpoint-delta-ulp{offset}-upper{upper}',
                                    (-2.,0.,0.),(bits_float(delta_bits+offset),0.,0.),
                                    [0]*4,9,upper,1.))
    for origin,delta,label in [((0.,0.,0.),(4.,0.,0.),'start-inside-positive'),
                               ((0.,0.,0.),(-4.,0.,0.),'start-inside-negative'),
                               ((-2.,0.,0.),(0.,0.,0.),'zero-delta'),
                               ((-2.,0.,0.),(-4.,0.,0.),'away'),
                               ((2.,0.,0.),(-4.,0.,0.),'reverse'),
                               ((-2.,2.,0.),(4.,0.,0.),'off-body'),
                               ((0.,0.,0.),(0.,0.,0.),'zero-delta-inside')]:
        for flags in (0,1,2,3,9):
            boundary_inputs.append((f'{label}-flags{flags}',origin,delta,[0]*4,flags,True,1.))
    for name,origin,delta,permutations,flags,upper,previous in boundary_inputs:
        result=trace.query(origin,delta,permutations,flags,upper,previous)
        result.update({'pose_index':pose_index,'boundary_fixture':True,
                       'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()})
        path=directory/f'{name}.json'
        path.write_text(json.dumps(result,indent=2)+'\n')
        cases.append({'name':name,'pose_index':pose_index,
                      'report':str(path.relative_to(ROOT)),
                      'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
                      'hit':result['returned_hit'],'decoded':result['decoded']})
    # Actual BSPs with explicitly arranged node poses produce an exact tie.
    # Every other node is displaced by 100 world units; node0 remains identity
    # and node18 receives a recorded finite translation. No metadata is changed.
    for shift_bits in (0xBD304393,0xBD304394,0xBD304395):
        matrices=trace.poses('identity')
        for i in range(trace.node_count):
            if i not in (0,18):trace.f(UNIT+POSE_OFFSET+i*0x34+0x28,100.)
        trace.f(UNIT+POSE_OFFSET+18*0x34+0x28,bits_float(shift_bits))
        matrices=[bytes(trace.g.mem_read(UNIT+POSE_OFFSET+i*0x34,0x34)).hex()
                  for i in range(trace.node_count)]
        pose_index=len(poses)
        poses.append({'name':f'explicit-tie-node18-translation-{shift_bits:08x}',
                      'matrices_hex':matrices,'kind':'explicit_arranged_nodes',
                      'node0':'identity','node18_translation_x_bits':f'{shift_bits:08x}',
                      'other_nodes_translation_x':100.})
        for name,permutations in [('arms',[0,-1,-1,-1]),
                                  ('body',[-1,0,-1,-1]),
                                  ('both',[0,0,-1,-1])]:
            for upper in (False,True):
                result=trace.query((-2.,0.,0.),(4.,0.,0.),permutations,9,upper)
                result.update({'pose_index':pose_index,'boundary_fixture':True,
                     'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()})
                case_name=f'tie-{shift_bits:08x}-{name}-upper{upper}'
                path=directory/f'{case_name}.json';path.write_text(json.dumps(result,indent=2)+'\n')
                cases.append({'name':case_name,'pose_index':pose_index,
                              'report':str(path.relative_to(ROOT)),
                              'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
                              'hit':result['returned_hit'],'decoded':result['decoded']})
    report = {'xbe_sha256': PIN, 'cache_sha256': CACHE_PIN,
              'oracle_tool_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'authored_metadata': trace.metadata, 'pose_inputs': poses, 'cases': cases,
              'case_count': len(cases), 'hit_count': sum(row['hit'] for row in cases),
              'normalization_epsilon': {'va': '0046504c', 'bits': f'{trace.u(0x46504C):08x}',
                                         'value': trace.fv(0x46504C)},
              'initial_lower_fraction': {'va': '00452044', 'bits': f'{trace.u(0x452044):08x}',
                                         'value': trace.fv(0x452044)},
              'projection_axis_table':{'va':'004471e4','raw_hex':bytes(trace.g.mem_read(0x4471E4,36)).hex(),
                   'sha256':hashlib.sha256(bytes(trace.g.mem_read(0x4471E4,36))).hexdigest(),
                   'entries':[list(struct.unpack('<3h',trace.g.mem_read(0x4471E4+i*6,6))) for i in range(6)]},
              'scope': 'Actual authored Spartan collision/model metadata and unchanged original posed BSP/surface traversal; explicitly supplied node transforms, active permutations and rays. No lower intersection service or native expected result. Full animation pose, broadphase/world-location and live spawn remain outside scope.'}
    (directory/'suite.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'cases':len(cases),'hits':report['hit_count']}))


def generate_edge_suite(path):
    trace=CollisionTrace();c=Cache(ROOT/'assets/disc/maps/lockout.map')
    u16=lambda at:struct.unpack_from('<H',c.data,at)[0]
    table=[struct.unpack('<3h',trace.g.mem_read(0x4471E4+i*6,6)) for i in range(6)]
    cases=[]
    for b in trace.metadata['bsps']:
        at=b['file_offset'];edges=c.block(at+0x34,12);vertices=c.block(at+0x3C,16)
        for si,surface in enumerate(c.block(at+0x2C,8)):
            plane=c.block(at+0xC,16)[u16(surface)&0x7FFF]
            normal=[c.f(plane+k*4) for k in range(3)]
            az,ay,ax=abs(normal[2]),abs(normal[1]),abs(normal[0])
            axis=2 if az>=ay and az>=ax else 1 if ay>=ax else 0
            orientation=int((normal[axis]>0.)!=bool(u16(surface)&0x8000))
            projection=table[axis*2+orientation][:2]
            first=u16(surface+2);ei=first;polygon=[]
            while True:
                edge=edges[ei];right=u16(edge+10)==si
                vi=u16(edge+2*int(right))
                polygon.append([c.f(vertices[vi]+k*4) for k in projection])
                ei=u16(edge+4+2*int(right))
                if ei==first:break
                assert len(polygon)<=len(edges)
            center=[sum(v[k] for v in polygon)/len(polygon) for k in range(2)]
            inputs=[('centroid',center),('far',(center[0]+10.,center[1]+10.))]
            for vi,v in enumerate(polygon):
                inputs.append((f'vertex{vi}',v))
                w=polygon[(vi+1)%len(polygon)]
                inputs.append((f'edge{vi}-midpoint',[(v[k]+w[k])*.5 for k in range(2)]))
            # All inputs are explicit fixtures, including neighboring float
            # points around the first edge midpoint; no native expectation.
            midpoint=inputs[3][1]
            for k in range(2):
                value=float_bits(midpoint[k])
                if value not in (0,0x80000000):
                    for step in (-1,1):
                        point=list(midpoint);point[k]=bits_float(value+step)
                        inputs.append((f'first-midpoint-axis{k}-bitsstep{step}',point))
            for name,point in inputs:
                result=trace.edge_query(at,si,axis,orientation,point)
                result.update({'name':f'region{b["region_index"]}-bsp{b["bsp_index"]}-surface{si}-{name}',
                               'region_index':b['region_index'],'bsp_index':b['bsp_index'],
                               'node_index':b['node_index'],'surface_raw_hex':bytes(c.data[surface:surface+8]).hex(),
                               'projected_edge_vertices_fixture':polygon})
                cases.append(result)
    c.data.close();c.stream.close()
    report={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
            'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'entry':'001e25c0','case_count':len(cases),
            'accepted_count':sum(c['returned_contains'] for c in cases),
            'cases':cases,
            'scope':'Unchanged1E25C0 on actual Spartan surfaces/edge/vertex records with explicitly supplied projection axis/orientation/2D points, no breakable bitset. Primitive branch evidence; stock original full query did not reach the class2-to-class2 edge-check branch.'}
    path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'edge_cases':len(cases),'accepted':report['accepted_count']}))


def dump_instructions(directory):
    directory = directory.resolve(); directory.mkdir(parents=True,exist_ok=True)
    image = Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==PIN
    code = _Code(image,'.text')
    starts = {int(r['address'],16) for r in csv.DictReader(
        (ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
    entries = (0x16A560,0x16B240,0x16AF90,0x1F3500,0x1F3590,0x1F3660,
               0x1F3900,0x142D00,0x1E20C0,0x1E23F0,0x29C7D0,
               0x1E25C0,0xDA0E0,0x16A5C0,0x30BD0)
    rows=[]
    for address in entries:
        f=_trace(code,address,starts|set(entries));instructions=[code.at(a) for a in sorted(f.code)]
        path=directory/f'{address:08x}.asm'
        path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions))
        rows.append({'start':f'{address:08x}','end_exclusive':f'{f.end:08x}',
                     'instruction_count':len(instructions),
                     'reachable_instruction_bytes_sha256':hashlib.sha256(b''.join(i.bytes for i in instructions)).hexdigest(),
                     'listing':str(path.relative_to(ROOT)), 'listing_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
                     'direct_calls':[{'site':f'{a:08x}','target':f'{b:08x}'} for a,b,k in f.sites if k=='call']})
    (directory/'inventory.json').write_text(json.dumps({'xbe_sha256':PIN,
        'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'routines':rows},indent=2)+'\n')
    print(json.dumps({'routines':len(rows)}))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--suite',type=Path)
    p.add_argument('--instruction-inventory',type=Path)
    p.add_argument('--edge-suite',type=Path)
    p.add_argument('--origin',type=float,nargs=3,default=(-2.,0.,0.))
    p.add_argument('--delta',type=float,nargs=3,default=(4.,0.,0.))
    p.add_argument('--pose',choices=('identity','authored_inverse'),default='identity')
    p.add_argument('--permutations',type=int,nargs=4,default=(0,0,0,0))
    p.add_argument('--flags',type=lambda v:int(v,0))
    p.add_argument('--lower-only',action='store_true')
    p.add_argument('--previous-fraction',type=float,default=1.)
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/map-pack/spartan-collision-1.1-oracle.json')
    args=p.parse_args()
    if args.suite is not None:generate_suite(args.suite);return
    if args.edge_suite is not None:generate_edge_suite(args.edge_suite);return
    if args.instruction_inventory is not None:dump_instructions(args.instruction_inventory);return
    trace=CollisionTrace();matrices=trace.poses(args.pose)
    result=trace.query(args.origin,args.delta,list(args.permutations),args.flags,
                       not args.lower_only,args.previous_fraction)
    result.update({'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
                   'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                   'pose_matrices_hex':matrices,'authored_metadata':trace.metadata})
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'hit':result['returned_hit'],'result':result['decoded']}))


if __name__=='__main__':main()
