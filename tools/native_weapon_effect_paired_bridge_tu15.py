"""Native weapon owners behind authenticated paired-image caller APIs.

The mappings retain independently checked original operands and PC domains.
Literal older-image bodies and canonical required-owner sites remain.
"""
import re

INCLUDE_BEGIN='// BEGIN native paired weapon owner include\n'
INCLUDE_END='// END native paired weapon owner include\n'
BEGIN='    // BEGIN native paired weapon owner\n'
END='    // END native paired weapon owner\n'
# Canonical entry, TU1.5 entry, final instruction, authenticated instruction count.
OWNERS=((0xbacc0,0xbbe00,0xbbf1c,76),(0xc9a40,0xcabc0,0xcad35,108),
        (0x177da0,0x1791c0,0x17922b,38),(0x1781f0,0x179610,0x1796f0,58),
        (0xb9c20,0xbad60,0xbaf05,118),(0xb9b90,0xbacd0,0xbad51,44),
        (0x1dc8f0,0x1e11b0,0x1e11f2,28),
        (0x1dc940,0x1e1200,0x1e135a,106))
OLD_DISPATCH='if(target==selected(0xc9a40u,s.image)){run_c9a40(s);return;}'
NEW_DISPATCH='if(target==selected(0xc9a40u,s.image)){if(s.image==SourcePhysicsContactImage::TitleUpdate15){s.required(site,target,tail);return;}run_c9a40(s);return;}'


def _bridge(entry,target,last,mode):
    delta=target-entry
    text=BEGIN+'''    if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
        auto abi=s.save();auto services=s.services;auto observe=s.observe;
        // Only this owner's PCs change domains. Lower owners keep their
        // existing observation contracts and actual TU1.5 targets/RET words.
'''
    condition='!s.selected_observation_pcs' if mode=='damage-observed' else 'true'
    text+='''        if(CONDITION) {
            if(s.services.instruction)services.instruction=[&](auto pc){s.services.instruction(pc>=TARGETu && pc<=LASTu?pc-DELTAu:pc);};
            if(s.observe)observe=[&](const auto &event){auto canonical=event;if(canonical.instruction>=TARGETu && canonical.instruction<=LASTu)canonical.instruction-=DELTAu;s.observe(canonical);};
        }
        if(s.services.required_owner)services.required_owner=[&](auto &memory,const auto &call,auto &state,const auto &){
            auto canonical=call;if(canonical.instruction>=TARGETu && canonical.instruction<=LASTu)canonical.instruction-=DELTAu;
            s.services.required_owner(memory,canonical,state,s.observe);
        };
        try {native_weapon_effect_dispatch_tu15_run(s.memory,TARGETu,abi,services,observe);}
        catch(...) {s.load(abi);throw;}
        s.load(abi);return;
    }
'''.replace('CONDITION',condition).replace('TARGET',hex(target)).replace('LAST',hex(last)).replace('DELTA',hex(delta))
    return text+END


def install_native_paired_weapon_owners(source,mode):
    assert mode in ('biped','damage-canonical','damage-observed','vehicle')
    state='ObservedDamageState' if mode=='damage-observed' else ('MotionState' if mode=='vehicle' else 'WorldState')
    installed=False
    owners=OWNERS[:1] if mode=='biped' else (tuple(row for row in OWNERS if row[0] in (0xb9c20,0xb9b90,0x1dc8f0,0x1dc940)) if mode=='vehicle' else OWNERS)
    for entry,target,last,count in owners:
        marker=f'static void run_{entry:x}({state} &s) {{\n'
        if marker not in source:continue
        assert source.count(marker)==1
        body=source.split(marker,1)[1].split('\n}',1)[0]
        if mode=='damage-observed':
            pairs={(int(a,16),int(b,16)) for a,b in re.findall(r'damage_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)',body)}
            assert len(pairs)==count and all(b-a==target-entry for a,b in pairs)
            sites={a for a,b in pairs}
        else:sites={int(p,16) for p in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',body)}
        assert len(sites)==count and min(sites)==entry and max(sites)==last-(target-entry)
        bridge=_bridge(entry,target,last,mode)
        if marker+bridge not in source:
            assert BEGIN not in body,'Remove the previous declared bridge before changing observation mode'
            source=source.replace(marker,marker+bridge,1)
        installed=True
    if installed and '#include "halo2/native_weapon_effect_dispatch_tu15.hpp"' not in source:
        anchor='#include "physics/vehicles/physics_vehicle_motion_internal.hpp"\n' if mode=='vehicle' else '#include "physics/runtime/physics_world_runtime_internal.hpp"\n';assert source.count(anchor)==1
        source=source.replace(anchor,anchor+INCLUDE_BEGIN+'#include "halo2/native_weapon_effect_dispatch_tu15.hpp"\n'+INCLUDE_END,1)
    if mode.startswith('damage'):
        def route(match):
            site=int(match[2] or match[4],16);ret=int(match[5],16);assert ret==site+5
            return match[1]+f'if(s.image==SourcePhysicsContactImage::TitleUpdate15)s.required(0x{site:x}u,selected(0xc9a40u,s.image),false);else run_c9a40(s);'
        source=re.sub(r'(s\.push\((?:damage_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)|0x([0-9a-f]+)u),selected\(0x([0-9a-f]+)u,s\.image\)\);)run_c9a40\(s\);',route,source)
        source=source.replace(OLD_DISPATCH,NEW_DISPATCH)
    return source


def strip_native_paired_weapon_owners(source):
    source=re.sub(re.escape(BEGIN)+r'.*?'+re.escape(END),'',source,flags=re.S)
    source=re.sub(re.escape(INCLUDE_BEGIN)+r'.*?'+re.escape(INCLUDE_END),'',source,flags=re.S)
    source=re.sub(r'if\(s\.image==SourcePhysicsContactImage::TitleUpdate15\)s\.required\(0x[0-9a-f]+u,selected\(0xc9a40u,s\.image\),false\);else run_c9a40\(s\);','run_c9a40(s);',source)
    return source.replace(NEW_DISPATCH,OLD_DISPATCH)
