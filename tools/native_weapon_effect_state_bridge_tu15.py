"""Install complete native effect owners in their TU1.5 callers."""
INCLUDE='#include "halo2/native_weapon_effect_dispatch_tu15.hpp"\n'


def _install_native_effect(source,entries):
    for entry in entries:
        marker='static void run_'+entry+'(WorldState &s) {'
        if marker not in source:continue
        assert source.count(marker)==1
        bridge='''
    // The complete native owner retains all real lower calls and their FP ABI.
    if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
        auto abi=s.save();
        try {native_weapon_effect_dispatch_tu15_run(s.memory,0xENTRYu,abi,s.services,s.observe);}
        catch(...) {s.load(abi);throw;}
        s.load(abi);return;
    }
'''.replace('ENTRY',entry)
        if marker+bridge not in source:source=source.replace(marker,marker+bridge,1)
        if INCLUDE not in source:
            anchor='#include "physics/runtime/physics_world_runtime_internal.hpp"\n';assert source.count(anchor)==1
            source=source.replace(anchor,anchor+INCLUDE,1)
    return source


def install_native_effect_state(source):
    return _install_native_effect(source,('17b130','17dc60'))


def install_native_effect_slot_scan(source):
    from native_effect_slot_scan_replacements import install_native_effect_slot_scan as authenticated
    return authenticated(source)


def install_native_effect_frame(source):
    return install_native_effect_slot_scan(_install_native_effect(source,('179df0','177830','179ed0')))


def install_native_effect_advance(source):
    return _install_native_effect(source,('17c140',))


def install_native_effect_next_event(source):
    return _install_native_effect(source,('17c020',))


def install_native_effect_random_range(source):
    return _install_native_effect(source,('259b0',))


def install_native_effect_creation(source):
    return _install_native_effect(source,('178e30',))


def install_native_effect_initialization(source):
    return _install_native_effect(source,('17b0d0','17e5e0'))


def install_native_effect_allocation(source):
    return _install_native_effect(source,('17afb0','17b960'))


def install_native_effect_earlier_owners(source):
    source=_install_native_effect(source,('bbe00','106f90','108a70','1791c0',
        '179610','17a1a0','17dff0','18be50','18c4b0','cabc0'))
    if 'static void run_cabc0(WorldState &s) {' not in source:return source
    # CABC0's real callers need the world's existing ZoomScope and committed
    # reset callback. Preserve the authentic call site and already-pushed RET.
    import re
    def required(match):
        site=int(match[2],16);ret=int(match[3],16)
        assert ret==site+5,'CABC0 caller lost its authenticated return address'
        return match[1]+('if(s.image==SourcePhysicsContactImage::TitleUpdate15)'
            f's.required(0x{site:x}u,0xcabc0u,false);else run_cabc0(s);')
    source=re.sub(r'(s\.push\(0x([0-9a-f]+)u,selected\(0x([0-9a-f]+)u,s\.image\)\);)run_cabc0\(s\);',required,source)
    old='if(target==selected(0xcabc0u,s.image)){run_cabc0(s);return;}'
    new='if(target==selected(0xcabc0u,s.image)){if(s.image==SourcePhysicsContactImage::TitleUpdate15){s.required(site,target,tail);return;}run_cabc0(s);return;}'
    return source.replace(old,new)


def install_native_effect_location_copies(source):
    return _install_native_effect(source,('17dd50','17ddc0'))


def install_native_effect_marker_matrix(source):
    return _install_native_effect(source,('bad60',))


def install_native_effect_marker_parent(source):
    return _install_native_effect(source,('bacd0',))


def install_native_effect_marker_mode(source):
    return _install_native_effect(source,('fc510',))


def install_native_effect_marker_lookup(source):
    return _install_native_effect(source,('1e11b0',))


def install_native_effect_marker_emission(source):
    return _install_native_effect(source,('1e1200',))
