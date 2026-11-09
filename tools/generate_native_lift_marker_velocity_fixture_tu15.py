#!/usr/bin/env python3
"""Generate the physical lift fixture with typed marker request boundaries."""
import inspect
import json
import os
from pathlib import Path

import generate_native_lift_velocity_parent_fixture_tu15 as base


def replace(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


def physical(fixture, paired, scratch, code):
    source = base.OBSERVER.read_text()
    source = replace(source, 'auto r=regs(child);J active;', 'auto r=regs(child);J active,marker;')
    source = replace(source, '   if(!active.is_null()&&actual==active.at("continuation")){', r'''
   if(!marker.is_null()&&actual==marker.at("continuation")){
    check(std::uint32_t(r.rsp)==marker.at("caller_sp").get<std::uint32_t>()+12u,"Marker RET12 differs");
    marker["return_step"]=step;marker["count"]=std::uint32_t(r.rax)&0xffffu;
    result["marker_deliveries"].push_back(std::move(marker));marker=nullptr;
   }
   if(actual==0x1da9a6u||actual==0x1db3a4u){
    check(marker.is_null(),"Nested original marker phase");const auto sp=std::uint32_t(r.rsp);
    marker=J{{"site",actual},{"call_step",step},{"continuation",actual+5u},{"caller_sp",sp},
      {"object",read_word(child,sp)},{"marker",read_word(child,sp+4u)},
      {"capacity",read_word(child,sp+8u)&0xffffu},{"direct",(r.rcx&0xffu)!=0},
      {"destination",std::uint32_t(r.rbx)}};
   }
   if(!active.is_null()&&actual==active.at("continuation")){''')
    source = replace(source, 'check(active.is_null(),"Original velocity delivery did not return to its caller");',
                     'check(active.is_null()&&marker.is_null(),"Original delivery did not return to its caller");')
    original = base.OBSERVER
    base.OBSERVER = scratch / 'marker-observer.cpp'
    base.OBSERVER.write_text(source)
    try:
        hardware, velocity = base.physical(fixture, paired, scratch, code)
    finally:
        base.OBSERVER = original
    rows = json.loads((scratch / 'hardware/trace.json').read_text())['cases']
    return hardware, {row['id']: (velocity[row['id']], row.get('marker_deliveries', [])) for row in rows}


def classify(case, deliveries, actual, owner_sites, code):
    velocity, markers = deliveries
    source = inspect.getsource(base.classify)
    source = replace(source, 'def classify(case, deliveries, actual, owner_sites, code):',
                     'def classify(case, deliveries, markers, actual, owner_sites, code):')
    source = replace(source, '    def check_raw(p, raw):', '''    marker_steps = {}
    for marker in markers:
        assert marker['destination']==marker['caller_sp']+0x118 and marker['capacity']==1 and not marker['direct']
        assert case['instructions'][marker['call_step']]==marker['site']
        assert actual[case['instructions'][marker['call_step']+1]]==0xbad60
        assert actual[case['instructions'][marker['return_step']]]==marker['continuation']
        marker['defined_bytes']={}
        for step in range(marker['call_step'],marker['return_step']):
            assert step not in marker_steps and step not in delivery_for_step
            marker_steps[step]=marker

    def check_raw(p, raw):''')
    source = replace(source, '        kept = []', "        marker = marker_steps.get(step)\n        kept = []")
    source = replace(source, '            mask = sum(', '''            if marker:
                start = marker['destination']
                if case['stack_begin'] <= p and p+len(written) <= marker['caller_sp']:
                    private = True
                else:
                    assert start <= p and p+len(written) <= start+0x70, (marker,event)
                    assert not private
                    marker['defined_bytes'].update({p-start+i:v for i,v in enumerate(written)})
            mask = sum(''')
    source = replace(source, '            if not private:\n                kept.append',
                     '            if not private and not marker:\n                kept.append')
    source = replace(source, "    case['velocity_calls'] =", "    for marker in markers:\n        assert marker['defined_bytes']\n        marker['defined_bytes']=sorted(marker['defined_bytes'].items())\n    case['marker_deliveries']=markers\n    case['velocity_calls'] =")
    env = dict(base.__dict__)
    exec(source, env)
    return env['classify'](case, velocity, markers, actual, owner_sites, code)


def main():
    source = inspect.getsource(base.main)
    source = replace(source, "fixture_path = scratch / 'lift-velocity-parent.json'", "fixture_path = scratch / 'lift-marker-velocity-parent.json'")
    source = replace(source, "            semantic_stores=sum(", "            marker_deliveries=sum(len(c['marker_deliveries']) for c in fixture['cases']),\n            semantic_stores=sum(")
    env = dict(base.__dict__, physical=physical, classify=classify, __file__=__file__, __doc__=__doc__)
    exec(source, env)
    env['main']()


if __name__ == '__main__':
    main()
