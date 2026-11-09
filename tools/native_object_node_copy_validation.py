"""Compile authenticated private object-node default prefixes with CPU fixtures.

Only the four new owner bodies come from each complete source copy. Other real
CPU algorithms/entry stops are shared explicitly; the fixture does not execute
every function in the authenticated complete production dispatcher.
"""
import hashlib
from pathlib import Path
import re
from generated_routine_ownership import function_bodies
import native_object_node_rebuild_replacements as binding

ROOT=Path(__file__).resolve().parents[1]
SOURCES=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp')
def prepare(scratch,entries):
    result=[]
    for label,path in zip(('primary','frame'),SOURCES):
        source=(ROOT/path).read_text();bound=binding.install_native_object_nodes(source)
        if binding.install_native_object_nodes(bound)!=bound:raise ValueError('Private object node default binder lost idempotency')
        original={match[1]:body for match,body in function_bodies(source)};installed={match[1]:body for match,body in function_bodies(bound)}
        if any(installed[name]!=body for name,body in original.items()if name not in binding.BODY_PINS):raise ValueError('Object node default binder changed unrelated owner')
        text='using reference::sign_extend;\n'
        text+=''.join(f'static void run_{name}(WorldState&);\n'for name in entries)
        text+='static void dispatch(WorldState&,std::uint32_t,std::uint32_t,bool);\n'
        for name in entries:
            if name not in binding.BODY_PINS:text+=f'static void run_{name}(WorldState&s){{reference::run_{name}(s);}}\n'
        for name in binding.BODY_PINS:
            body=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',installed[name])
            normalized=binding.canonical_body(name,body)
            if hashlib.sha256(normalized.encode()).hexdigest()not in binding.BODY_PINS[name]:raise ValueError('Default object node literal normalization drift')
            if name!='bf0c0':
                at=body.index('{\n')+2;body=body[:at]+f'note(s,0x{name}u);\n'+body[at:]
            text+=body+'\n'
        text+='static void dispatch(WorldState&s,std::uint32_t owner,std::uint32_t site,bool tail){switch(owner){'+''.join(f'case 0x{name}u:run_{name}(s);return;'for name in binding.BODY_PINS)+'default:reference::dispatch(s,owner,site,tail);return;}}\n'
        (scratch/f'object_node_rebuild_default_{label}.inc').write_text(text)
        result.append(dict(copy=label,source=path,source_sha256=hashlib.sha256(source.encode()).hexdigest(),private_bound_sha256=hashlib.sha256(bound.encode()).hexdigest(),
            source_already_installed=source==bound,
            installed_entries=binding.installed_entries(bound),compiled_fragment_sha256=hashlib.sha256(text.encode()).hexdigest(),
            unrelated_owner_body_bytes_preserved=True,whole_dispatcher_authenticated=True,whole_dispatcher_executed=False))
    return result
