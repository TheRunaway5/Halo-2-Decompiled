"""Authenticate complete TU1.5 datum-delete bodies and observation domains."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

INCLUDE='#include "physics/runtime/native_tu15_datum_delete_abi.hpp"\n'
BODY_PINS=('e717d1ead3675d4ca88cc47eb2454fb0be8f80608f50317537e0201bf6706246',
           '355465eabab3185888c1005836682fb0757c6708268888cf4f89c1e5eed2becd')
POOL_PIN='d51da3f2947725ff87d53ecbaabc6114f32d5e1395f4102d2440b816f06d0b24'
RESOLVER_PINS=('5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
               '03e2514fde9d92aa28b30dc7c13ef0cc2fcaf7c320d20644d66603ba6e6b640a')
POOL_RESOLVER_PIN='be2c1863908128002b1b89cb7fae12467fde560f5f6c6416afd378731054b640'


def prefix(pool=False):
    return '    // Shared DATA owner retains this route\'s store-site domain.\n'+\
        '    if(halo2::native_datum_delete_detail::run(s,'+('true' if pool else 'false')+'))return;\n'


def canonical_body(value,pool=False):
    start=value.index('{\n')+2;marker=prefix(pool)
    return value[:start]+value[start+len(marker):]if value[start:].startswith(marker)else value


def install_native_datum_delete(source):
    # The two State/MapPack11-only copies retain their exact-image reference
    # route; this binder admits only the independently qualified TU1.5 owner.
    matches=list(re.finditer(r'static void run_(16e8a0|16d4c0)\(WorldState &s\) \{.*?\n\}',source,re.S))
    for match in reversed(matches):
        pool=match[1]=='16d4c0';current=match[0];original=canonical_body(current,pool)
        if hashlib.sha256(original.encode()).hexdigest()not in((POOL_PIN,)if pool else BODY_PINS):
            raise ValueError('Complete datum-delete body drift '+match[1])
        if pool:
            resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
            if not resolver or hashlib.sha256(resolver[0].encode()).hexdigest()!=POOL_RESOLVER_PIN:
                raise ValueError('Complete datum-delete pool resolver drift')
            mapping={int(a,16):int(b,16) for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver[0])}
            operands={int(a,16)for a in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',original)}
            if any(mapping.get(pc)!=pc+0x13e0 for pc in operands):
                raise ValueError('Datum-delete pool instruction mapping drift')
        else:require_selected_identity(source,original,RESOLVER_PINS)
        if current==original:
            start=current.index('{\n')+2
            source=source[:match.start()]+current[:start]+prefix(pool)+current[start:]+source[match.end():]
    if matches and INCLUDE not in source:
        anchor='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
        if source.count(anchor)!=1:raise ValueError('Missing datum-delete include seam')
        source=source.replace(anchor,anchor+INCLUDE,1)
    return source
