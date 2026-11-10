"""Offline actual-source children for the private complete16C000 ABI probe.

The unchanged i386 image remains the independent authority. These complete
retained child algorithms execute against the live native caller's State and
banks; no recorded child results or ABI tuples are substituted.
"""
import hashlib
import re
from pathlib import Path
from generated_routine_ownership import function_bodies


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def prepare(root, scratch):
    from native_bsp_segment_setup_replacements import canonical_body as setup
    from native_bsp_segment_replacements import canonical_body as segment, ORIGINAL_SPILL, ORDERED_SPILL
    from native_bsp_leaf_surface_replacements import canonical_body as leaf
    from native_bsp_surface_2d_replacements import canonical_body as tree
    from native_bsp_surface_polygon_replacements import canonical_body as polygon
    primary='src/physics/physics_live_support_tu15_generated.cpp'
    ai='src/campaign/ai/source_ai_frame_tu15_generated.cpp'
    owners=((0x120fa0,primary,lambda text:text),(0x1e68e0,primary,setup),
            (0x1e6980,primary,segment),(0x1e6cb0,primary,leaf),
            (0x2a0230,ai,tree),(0x1e6e80,'src/runtime/source_map_world_support_tu15_generated.cpp',polygon))
    pieces=[];receipts=[]
    for entry,path,strip in owners:
        source=(root/path).read_text()
        body=next(body for match,body in function_bodies(source)if int(match[1],16)==entry)
        original=strip(body)
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
        mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver[0])}
        operands={int(value,16)for value in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',original)}
        if any(mapping.get(value)!=value for value in operands):
            raise ValueError('Retained query child is not its identity TU1.5 namespace: '+path)
        projected=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',original.replace(ORIGINAL_SPILL,ORDERED_SPILL))
        pieces.append(projected)
        receipts.append(dict(entry=entry,source=path,complete_body_sha256=digest(body),
            retained_body_sha256=digest(original),resolver_sha256=digest(resolver[0]),projected_body_sha256=digest(projected)))
    # The World owner explicitly records both physical image PCs. Select the
    # actual TU1.5 arm of each existing observation/operand, without modifying
    # any arithmetic or the independent executable's instructions.
    path='src/physics/runtime/physics_world_runtime_generated.cpp';source=(root/path).read_text()
    complete=re.search(r'static void run_142d00\(ObservedWorldState &s\) \{.*?\n\}',source,re.S)[0]
    from native_affine_inverse_replacements import canonical_body as inverse_body, prefix as inverse_prefix, replace_affine_inverse
    original=inverse_body(complete)
    if complete!=original:
        opening=complete.index('{')+1
        if not complete[opening:].startswith(inverse_prefix('world'))or replace_affine_inverse(source)!=source:
            raise ValueError('Retained inverse copy requires its exact authenticated World native prefix')
    if digest(original)!='cd2162d2861fc0cb302081120dcc26518f64d137394aff7268aaae47b569d299':
        raise ValueError('Complete retained inverse-frame owner changed')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
    projected=original.replace('run_142d00(ObservedWorldState &s)','run_144090(WorldState &s)',1)
    projected=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',projected)
    projected=re.sub(r'selected\(0x([0-9a-f]+)u,s.image\)',lambda match:f'0x{mapping[int(match[1],16)]:x}u',projected)
    pieces.append(projected)
    receipts.append(dict(entry=0x144090,source=path,complete_body_sha256=digest(complete),retained_body_sha256=digest(original),
        resolver_sha256=digest(resolver),projected_body_sha256=digest(projected),actual_image='TU1.5'))
    declarations='\n'.join(f'static void run_{entry:x}(WorldState&);'for entry in (0x120fa0,0x144090,0x1e68e0,0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80))
    dispatch='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){switch(target){'+''.join(f'case 0x{entry:x}u:run_{entry:x}(s);return;'for entry in (0x120fa0,0x144090,0x1e68e0,0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80))+'default:throw std::runtime_error("Unowned retained query child");}}'
    extension='std::uint32_t sign_extend(std::uint32_t v,unsigned width){return std::uint32_t(width==1?std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))):std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}'
    (scratch/'camera_query_children.inc').write_text(declarations+'\n'+dispatch+'\n'+extension+'\n'+'\n'.join(pieces))
    return receipts
