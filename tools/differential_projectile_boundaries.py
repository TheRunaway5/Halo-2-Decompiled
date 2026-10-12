#!/usr/bin/env python3
"""Compare original vector normalization and clear stock-SMG range boundaries.

Flight comparison uses supplied original state and clear world inputs; it does
not validate collision/event/cleanup services or complete callback integration.
"""
import argparse
import ctypes as c
import hashlib
import json
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[1]
class Vector(c.Structure):
    _fields_=[(axis,c.c_float) for axis in ('x','y','z')]
class Flight(c.Structure):
    _fields_=[('velocity',Vector),('position',Vector),('displacement',Vector),
              ('traveled',c.c_float),('range_exhausted',c.c_bool)]
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def value(bits): return struct.unpack('<f',bytes.fromhex(bits)[::-1])[0]
def bits(v): return f'{struct.unpack("<I",struct.pack("<f",v))[0]:08x}'
def vector(words):return Vector(*(value(v) for v in words))
def words(v):return [bits(v.x),bits(v.y),bits(v.z)]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library',type=Path,default=ROOT/'build-multiplayer/libhalo2_kernels.so')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/projectile-boundaries-differential.json')
    args=parser.parse_args()
    report_path=ROOT/'analysis/multiplayer/map-pack/projectile-boundaries-1.1-oracle.json'
    report=json.loads(report_path.read_text())
    for key,path in [('tool_sha256','tools/oracle_projectile_boundaries_1_1.py'),
            ('xbe_sha256','assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),
            ('cache_sha256','assets/disc/maps/lockout.map')]:
        if digest(ROOT/path)!=report[key]:raise ValueError(f'Original authority differs: {path}')
    range_report=ROOT/report['clear_range']['source_report']
    if digest(range_report)!=report['clear_range']['source_report_sha256']:
        raise ValueError('Original full range callback report differs')
    lib=c.CDLL(str(args.library.resolve()))
    normalize=lib.halo2_original_vector_normalize
    normalize.argtypes=[c.POINTER(Vector)];normalize.restype=c.c_float
    flight=lib.halo2_projectile_flight
    flight.argtypes=[c.POINTER(Vector)]*2+[c.c_float]*4+[c.POINTER(Flight)];flight.restype=None
    for row in report['normalization']['cases']:
        actual=vector(row['input_bits']);length=normalize(c.byref(actual))
        if words(actual)!=row['output_vector']['bits'] or bits(length)!=row['returned_length_bits']:
            raise AssertionError(('normalization',row['name'],words(actual),bits(length)))
    source=report['clear_range']
    for row in source['rows']:
        before,after=row['before'],row['after'];actual=Flight()
        position=vector(before['vector_064']['bits']);velocity=vector(before['vector_088']['bits'])
        flight(c.byref(position),c.byref(velocity),value(source['gravity_fixture_bits']),
            value(source['dt_bits']),value(before['field_168_bits']),value(before['field_170_bits']),c.byref(actual))
        if words(actual.position)!=after['vector_064']['bits'] or \
                words(actual.velocity)!=after['vector_088']['bits'] or \
                bits(actual.traveled)!=after['field_168_bits'] or \
                actual.range_exhausted!=bool(after['field_130_int16']):
            raise AssertionError(('clear range',row['tick'],words(actual.position),bits(actual.traveled)))
    args.output.write_text(json.dumps(dict(passed=True,normalization_cases=len(report['normalization']['cases']),
        clear_range_ticks=len(source['rows']),native_library_sha256=digest(args.library),
        original_report_sha256=digest(report_path),tool_sha256=digest(Path(__file__)),scope=__doc__),indent=2)+'\n')
    print(f'Matched {len(report["normalization"]["cases"])} original normalization calls and {len(source["rows"])} clear-range callback states')

if __name__=='__main__': main()
