#!/usr/bin/env python3
"""Observe BDEF0's primitive sphere in untouched cold object constructors.

Reuses the independently retained factory harness without changing it. Only
observations and distinct report destinations are added; no original routine
or existing evidence file is patched. Registration is deliberately not run.
"""
import argparse, hashlib, json, struct, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('kind', choices=['scenery', 'machine', 'prop'])
    parser.add_argument('--binary', type=Path, required=True)
    args = parser.parse_args()
    source = ROOT / f'analysis/multiplayer-shading/{args.kind}-cold-bounds-original.py'
    destination = ROOT / f'analysis/multiplayer-shading/{args.kind}-cold-spatial-original.json'
    text = source.read_text()
    marker = "  color_size=struct.unpack('<h',u.mem_read(HEAP+0x124,2))[0]"
    if text.count(marker) != 1:
        raise RuntimeError('original cold harness observation seam changed')
    observation = """  record['primitive_sphere_words']=list(struct.unpack('<4I',u.mem_read(HEAP+0x40,16)))
  record['dynamic_sphere_words']=list(struct.unpack('<4I',u.mem_read(HEAP+0x50,16)))
  record['location_words']=list(struct.unpack('<2I',u.mem_read(HEAP+0x28,8)))
  record['root_position_words']=list(struct.unpack('<3I',u.mem_read(HEAP+0x64,12)))
  record['before_registration_flags']=read(HEAP+4)
  definition_at=c.tags[r['definition']]['offset']
  record['spatial_definition_words']=[struct.unpack_from('<H',c.data,definition_at+2)[0],*struct.unpack_from('<4I',c.data,definition_at+0x20)]
"""
    text = text.replace(marker, observation + marker)
    text = text.replace(f'{args.kind}-cold-bounds-original.json', destination.name)
    # The existing scenery harness additionally probes animation refresh. This
    # report observes only the cold constructor, before B8890 registration.
    if args.kind == 'scenery':
        start = text.index("  control_offset=record['initial_control_offset']")
        end = text.index(" print('cold scenery cached state'", start)
        text = text[:start] + text[end:]
    namespace = {'__name__': '__main__', '__file__': str(source)}
    exec(compile(text, str(source), 'exec'), namespace)
    report = json.loads(destination.read_text())
    records = report['records']
    fixture_rows = []
    expected = []
    for record in records:
        # BC280 zeroes the allocation and B8890 has not yet registered it. No
        # retained primitive suppression may occur at this observation point.
        assert record['before_registration_flags'] & 0x100 == 0
        assert record['location_words'] == [0xffffffff, 0xffffffff]
        definition = record['spatial_definition_words']
        query = record['initial_sphere_words']
        fixture_rows.append(' '.join(map(str, [definition[0], *definition[1:],
            *query, record['object_scale_word'], *([0]*4), 0])))
        expected.append([*record['dynamic_sphere_words'],
            *record['primitive_sphere_words'], 1])
    output = subprocess.check_output([str(args.binary.resolve()), '--bounds'],
        input='\n'.join(fixture_rows)+'\n', text=True)
    actual = [list(map(int, line.split())) for line in output.splitlines()]
    if len(actual) != len(expected):
        raise RuntimeError('cold spatial native fixture count changed')
    failures = [dict(map=records[i]['map'], placement=records[i]['placement'],
        original=a, native=b) for i, (a,b) in enumerate(zip(expected,actual)) if a != b]
    report['native_sha256'] = hashlib.sha256(args.binary.read_bytes()).hexdigest()
    report['cold_harness_sha256'] = hashlib.sha256(source.read_bytes()).hexdigest()
    report['spatial_words_compared'] = len(expected)*9
    report['spatial_exact'] = len(expected)-len(failures)
    report['spatial_failures'] = failures[:8]
    report['spatial_scope'] = ('Untouched factory/B7B40/BDEF0 up to B8890 entry; '
        'actual object+30/+3C, +40/+4C and +50/+5C, root-definition+2/+20/+24 '
        'and scale words. All objects independently reset, source fixture '
        'full handle 12340000, no selected BSP, no location producer or '
        'registration chronology. The cold location remains NONE. Successful '
        'allocator/resident authored-animation services retain the original '
        'harness boundaries. Native comparison starts after the independently '
        'executed original query-sphere producer; it does not re-prove node posing.')
    destination.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(kind=args.kind, fixtures=len(records),
        words=report['spatial_words_compared'], exact=report['spatial_exact'],
        failures=failures[:3]), indent=2))
    assert not failures

if __name__ == '__main__':
    main()
