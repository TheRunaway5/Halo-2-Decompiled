#!/usr/bin/env python3
"""Compile the bounded, authenticated Cairo TU1.5 script authoring deltas."""
import argparse
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]

def generated(p):
    if (p['schema'], p['scenario_path'], p['mcc_dll_sha256'], p['xbox_tu15_xbe_sha256'],
        p['retired_row_hex']) != (1, r'scenarios\solo\01b_spacestation\01b_spacestation',
        'de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946',
        '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',
        '0000'+'ba'*18):
        raise ValueError('unverified Cairo profile identity or free-node representation')
    expected = [('x02_scene_01', 0xeaa0072d, [0xea2b06b8]),
                ('c01_outro_scene_01', 0x82291eb5, [0x81c71e53]),
                ('x02_scene_06', 0xf85314e0, [0xf7ca1457, 0xf84f14dc])]
    if len(p['subtrees']) != len(expected):
        raise ValueError('unverified Cairo subtree count')
    q=json.dumps
    lines=['// Generated from tools/mcc-cairo-vanilla-scripts.json.',
           '// Original TU1.5 16E510 defines zero-salt / BA-poisoned free rows.',
           'constexpr std::array<std::byte,20> cairo_retired_row{{'+
           ','.join('std::byte{0x'+p['retired_row_hex'][i:i+2]+'}' for i in range(0,40,2))+'}};']
    for n,(t,e) in enumerate(zip(p['subtrees'],expected)):
        if (t['name'],t['root'],[x['removed_call'] for x in t['patches']]) != e or t['type']!=3 or t['return_type']!=4:
            raise ValueError('unverified Cairo subtree identity')
        rows={r['datum']:bytes.fromhex(r['raw']) for r in t['rows']}
        if len(rows)!=len(t['rows']) or t['root'] not in rows:
            raise ValueError('duplicate or missing Cairo syntax row')
        for datum,raw in rows.items():
            if len(raw)!=20 or int.from_bytes(raw[:2],'little') != datum>>16:
                raise ValueError('Cairo row extent or full salted identity differs')
        retired=set()
        for patch in t['patches']:
            if any(d not in rows for d in [patch['predecessor'],*patch['retired']]):
                raise ValueError('Cairo patch escapes verified subtree')
            if int.from_bytes(rows[patch['predecessor']][8:12],'little') != patch['removed_call'] or \
               int.from_bytes(rows[patch['removed_call']][8:12],'little') != patch['next']:
                raise ValueError('Cairo predecessor/successor proof differs')
            owned=set(patch['retired'])
            if patch['removed_call'] not in owned or retired & owned:
                raise ValueError('Cairo retired closures overlap')
            for datum,raw in rows.items():
                edges=[int.from_bytes(raw[8:12],'little')]
                if not int.from_bytes(raw[6:8],'little')&1:
                    edges.append(int.from_bytes(raw[16:20],'little'))
                for target in edges:
                    if target in owned and datum not in owned and \
                       (datum,target)!=(patch['predecessor'],patch['removed_call']):
                        raise ValueError('Cairo retired closure has an additional incoming edge')
            retired |= owned
        lines += [f'constexpr std::array<CairoRow,{len(rows)}> cairo_rows_{n}{{{{']
        for datum,raw in rows.items():
            lines.append('    {'+str(datum)+',{{'+','.join('std::byte{'+str(b)+'}' for b in raw)+'}}},')
        lines += ['}};',f'constexpr std::array<CairoString,{len(t["strings"])}> cairo_strings_{n}{{{{']
        lines += ['    {'+str(s['offset'])+','+q(s['text'])+'},' for s in t['strings']]
        lines += ['}};',f'constexpr std::array<CairoBinding,{len(t["bindings"])}> cairo_bindings_{n}{{{{']
        for b in t['bindings']:
            v=b['binding']; typ=extra=0; group=''
            if 'group' in v: kind='Tag';name=v['path'];group=v['group']
            elif 'string_id' in v: kind='StringId';name=v['string_id']
            elif 'object_name' in v: kind='Object';name=v['object_name'];typ=v['object_type'];extra=v['placement']
            elif 'cutscene_flag' in v:kind='Flag';name=v['cutscene_flag']
            elif 'external_global' in v:kind='External';name=v['external_global'];typ=v['index']
            elif 'global' in v:kind='Global';name=v['global'];typ=v['declared_type']
            else:raise ValueError('unverified Cairo binding kind')
            if b['datum'] not in rows or int.from_bytes(rows[b['datum']][16:20],'little')!=b['value']:
                raise ValueError('Cairo binding differs from full row')
            lines.append(f'    {{{b["datum"]},{b["value"]},CairoBindingKind::{kind},{q(name)},{q(group)},{typ},{extra}}},')
        lines += ['}};',f'constexpr std::array<CairoStatic,{len(t["static_calls"])}> cairo_statics_{n}{{{{']
        lines += [f'    {{{s["index"]},{q(s["name"])}}},' for s in t['static_calls']]
        lines += ['}};',f'constexpr std::array<CairoPatch,{len(t["patches"])}> cairo_patches_{n}{{{{']
        lines += [f'    {{{x["predecessor"]},{x["removed_call"]},{x["next"]}}},' for x in t['patches']]
        lines += ['}};',f'constexpr std::array<std::uint32_t,{len(retired)}> cairo_retired_{n}{{{{'+','.join(map(str,sorted(retired)))+'}};']
    lines += ['constexpr std::array<CairoSubtree,3> cairo_subtrees{{']
    for n,t in enumerate(p['subtrees']):
        lines.append(f'    {{{q(t["name"])},{t["type"]},{t["return_type"]},{t["root"]},cairo_rows_{n},cairo_strings_{n},cairo_bindings_{n},cairo_statics_{n},cairo_patches_{n},cairo_retired_{n}}},')
    return '\n'.join(lines+['}};'])+'\n'

def main():
    a=argparse.ArgumentParser(description=__doc__);a.add_argument('--check',action='store_true');o=a.parse_args()
    result=generated(json.loads((ROOT/'tools/mcc-cairo-vanilla-scripts.json').read_text()))
    output=ROOT/'src/campaign/mcc_cairo_vanilla_scripts.inc'
    if o.check:
        if output.read_text()!=result:raise SystemExit('compiled Cairo vanilla profile is stale')
    else:output.write_text(result)
if __name__=='__main__':main()
