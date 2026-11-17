#!/usr/bin/env python3
"""Compare 107140 with complete current/fresh Primary bodies in both formats."""
import re
import subprocess
import sys
from dataclasses import replace
from functools import partial

from specialize_native_operands import specialize
from validate_native_flight_property_query_source_tu15 import (
    DISPATCH, GEN, ROOT, STATE, STATE_PINS, UNIT, SourceProfile, digest, main, one)
from validate_native_object_record_ratio_frame_tu15 import corpus

FRAME = 'src/gameplay/objects/native_tu15_object_record_ratio_source_frame.hpp'
DRIVER = 'tests/native_tu15_object_record_ratio_source_tests.cpp'
BODY = re.compile(r'^static void run_107140\(WorldState &s\) \{.*?^\}',re.M|re.S)
PARENT = re.compile(r'^static void run_1046c0\(WorldState &s\) \{.*?^\}',re.M|re.S)
RESOLVER = re.compile(r'^std::uint32_t selected\([^\n]+',re.M)
SIGN_EXTEND = re.compile(r'^std::uint32_t sign_extend\(.*?^\}',re.M|re.S)
BODY_PINS = ('1e4f91fcb47980f7d5fa69c31a93469f0d76bd7f21548d84d18ec7fa921c5659',
             'fc583b34bd0249e4bba259cbf71eeda58493b95472e9e4514316f02b79e4ab47')
PARENT_PINS = ('84a26140524e26d4dddc5eea8b3bf3e7205f8fcd161c034d49e55ef5f6fcfe9c',
               '32e7c8d5885c51767fdc5bba6dd66cf4ed67516d133fb25ab16e0c77cad36a42')
DISPATCH_PINS = ('5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
                 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849',
                 '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052',
                 '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851')
RESOLVER_PIN = '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4'
SIGN_EXTEND_PIN = 'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b'


def prepare(scratch, require_binding=False, *, ordered=False):
    import native_object_record_ratio_replacements as binding
    fresh, manifest = scratch/'fresh.cpp', scratch/'generation.json'
    command = [sys.executable,'-B',str(ROOT/GEN),'--output',str(fresh),'--manifest',str(manifest)]
    generated = subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=600)
    if generated.returncode:
        raise ValueError('Genuine Primary generation failed: '+generated.stderr[-8000:])
    raw = [(ROOT/UNIT).read_text(),fresh.read_text()]
    if require_binding:
        for text in raw:
            binding.installed_entries(text)
            binding.installed_entries(specialize(text)[0])
    from native_object_ratio_caller_replacements import remove_object_ratio_caller
    raw = [binding.remove_object_record_ratio(remove_object_ratio_caller(text)) for text in raw]
    variants = raw+[specialize(text)[0] for text in raw]
    bodies = [one(BODY,text,'107140 body') for text in variants]
    parents = [one(PARENT,text,'1046C0 parent') for text in variants]
    states = [one(STATE,text,'Primary LiveState') for text in variants]
    dispatches = [one(DISPATCH,text,'Primary dispatcher') for text in variants]
    resolvers = [one(RESOLVER,text,'Primary resolver') for text in variants]
    helpers = [one(SIGN_EXTEND,text,'Primary sign-extension helper') for text in variants]
    for label, rows, pins in (('body',bodies,(BODY_PINS[0],)*2+(BODY_PINS[1],)*2),
                              ('parent',parents,(PARENT_PINS[0],)*2+(PARENT_PINS[1],)*2),
                              ('state',states,STATE_PINS*2),('dispatch',dispatches,DISPATCH_PINS),
                              ('resolver',resolvers,(RESOLVER_PIN,)*4),('sign-extension',helpers,(SIGN_EXTEND_PIN,)*4)):
        if tuple(map(digest,rows)) != pins:
            raise ValueError('Complete 107140 '+label+' context changed')
    calls = ((0x104815,0x10481a),(0x104b6c,0x104b71))
    for parent in parents:
        actual = re.findall(r's\.push\((0x[0-9a-f]+)u,(0x[0-9a-f]+)u\);run_107140\(s\);',parent)
        if tuple(tuple(int(value,16) for value in pair) for pair in actual) != calls:
            raise ValueError('Genuine 1046C0 callers changed')
    sites = [re.findall(r's\.(?:services\.instruction|instruction_literal)\((0x[0-9a-f]+u)\)',body) for body in bodies]
    native = set(re.findall(r'0x1071[0-9a-f]{2}u',(ROOT/FRAME).read_text()))
    if len(set(sites[0]))!=36 or any(row!=sites[0] for row in sites) or set(sites[0])!=native:
        raise ValueError('107140 source callback boundaries changed')
    retained = [binding.apply_ordered_shifts(body) for body in bodies] if ordered else bodies
    if ordered and [binding.remove_ordered_shifts(body) for body in retained]!=bodies:
        raise ValueError('Proposed source shifts did not reverse exactly')
    chunks = ['#define HALO2_OBJECT_RECORD_RATIO_ORDERED_PROOF 1'] if ordered else []
    for variant,name in enumerate(('current','fresh')):
        chunks += ['namespace '+name+' {','using namespace halo2::physics_world_runtime_detail;',
                   states[variant],helpers[variant],retained[variant].replace('run_107140(','raw(',1),
                   retained[variant+2].replace('run_107140(','literal(',1),'}',
                   'static void run_reference('+name+'::WorldState&s,unsigned v){if(v&2u)'+name+'::literal(s);else '+name+'::raw(s);}']
    (scratch/'object_record_ratio_reference.inc').write_text('\n'.join(chunks)+'\n')
    return dict(generation=command,generation_manifest_sha256=digest(manifest.read_bytes()),
                body_sha256=list(map(digest,bodies)),state_sha256=list(map(digest,states)),
                dispatch_sha256=DISPATCH_PINS,resolver_sha256=RESOLVER_PIN,sign_extend_sha256=SIGN_EXTEND_PIN,
                actual_parent_sha256=list(map(digest,parents)),actual_parent_calls=calls,
                sites=36,registry_service=False,scoped_ordered_shifts=ordered,
                reference_body_sha256=list(map(digest,retained)))


PROFILE = SourceProfile(UNIT,GEN,FRAME,DRIVER,prepare,corpus,'Object record ratio',(
    ('added-lower-bound','return !s.flags.zf&&s.flags.sf==s.flags.of;',
     'return !(s.gp[0]&0x80000000u)&&!s.flags.zf&&s.flags.sf==s.flags.of;'),
    ('wrong-stack-cleanup','s.pop();s.gp[4]+=4u;','s.pop();s.gp[4]+=8u;'),
    ('unsigned-numerator','signed_word(read(g[1]+2u,2))','read(g[1]+2u,2)'),
    ('wrong-record-branch','return s.flags.zf?Record::constant:Record::other;',
     'return s.flags.zf?Record::other:Record::constant;'),
),extra_pins=('tools/validate_native_object_record_ratio_source_tu15.py',
              'tools/validate_native_object_record_ratio_frame_tu15.py',
              'tools/native_object_record_ratio_replacements.py'))

ORDERED_PROFILE = replace(PROFILE,prepare=partial(prepare,ordered=True),controls=PROFILE.controls+tuple(
    (f'modeled-shift-{count}',f'shift_eax<{count}u>()',f's.flags.shift(s.reg(0,4,0),{count}u,4)')
    for count in (4,6)))


if __name__=='__main__':
    ordered='--ordered-source' in sys.argv
    if ordered:
        sys.argv.remove('--ordered-source')
    raise SystemExit(main(ORDERED_PROFILE if ordered else PROFILE))
