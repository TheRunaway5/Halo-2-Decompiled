"""Bind 250B50 through RET around the genuine 1458A0 and 1459B0 calls."""

import hashlib

import native_particle_transform_basis_replacements as basis


INCLUDE = '#include "rendering/effects/native_tu15_particle_transform_projection.hpp"\n'
BASIS_PREPARED_PIN = '4c84212c9fe26f34efa85d685faa18adb078b75fe623b76c7614e3087fa088d5'
STAGE = '250b50'
NAMESPACE = 'halo2::native_particle_transform_projection_detail'
ROUTES = {'250b50':'project', '250bf8':'rotation', '250c11':'rotate_basis',
          '250c1c':'read_projected', '250c2e':'add_translation', '250c4d':'leave'}
RESUMES = ('250b55','250b5b','250b60','250b67','250b6c','250b71','250b75','250b7a','250b7e',
           '250b86','250b8a','250b93','250b98','250ba1','250ba6','250baf','250bb4','250bbd',
           '250bc2','250bc8','250bd6','250bdc','250be2','250bea','250bf2','250bfe','250c04',
           '250c06','250c07','250c08','250c0c','250c17','250c22','250c28','250c32','250c37',
           '250c3c','250c40','250c41','250c42','250c47','250c4c')
EXISTING = ('250b50','250bf8','250c2e','250c4d')
CALLS = (('250c0c','250c11','1458a0'), ('250c17','250c1c','1459b0'))
JUMP = 'native_particle_projection_resume'


def install(canonical, output):
    if not basis.admitted(canonical):
        return output, []
    found = basis.world.body(output,basis.OWNER)
    if not found or hashlib.sha256(found[0].encode()).hexdigest()!=BASIS_PREPARED_PIN:
        return output, []
    body = found[0]
    start = body.index('L250b50: ')
    prefix, tail = body[:start], body[start:]
    tail = tail.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')
    tail = tail.replace('_mm_add_ss(', 'halo2::native_world_leaves_detail::ordered_add_ss(')
    for site,returned,callee in CALLS:
        before=f's.push(0x{site}u,0x{returned}u);run_{callee}(s);'
        if tail.count(before)!=1:
            raise ValueError(f'Particle projection genuine call {site} drifted')
        tail=tail.replace(before,f'{basis.NAMESPACE}::push_saved(s,0x{site}u,0x{returned}u);run_{callee}(s);',1)
    for site,word in (('250c04','0x0u'),('250c06','s.reg(0,4,0)'),('250c07','s.reg(1,4,0)')):
        before=f's.push(0x{site}u,{word});'
        if tail.count(before)!=1:
            raise ValueError(f'Particle projection stack argument at {site} drifted')
        tail=tail.replace(before,f'{basis.NAMESPACE}::push_saved(s,0x{site}u,{word});',1)
    for site in (*RESUMES,*ROUTES):
        label=f'L{site}:'
        instruction=f's.instruction_literal(0x{site}u);'
        before=f'{label} {instruction}' if site in EXISTING else '\n'+instruction
        if tail.count(before)!=1:
            raise ValueError(f'Particle projection original continuation {site} drifted')
        hook=(f'\n    if ((projection_next={NAMESPACE}::bridge<{NAMESPACE}::{ROUTES[site]}>(s))) '
              f'goto {JUMP};\n' if site in ROUTES else ' ')
        tail=tail.replace(before, ('\n' if site not in EXISTING else '')+label+hook+instruction,1)
    inside=prefix.index('{')+1
    prefix=prefix[:inside]+'\n    std::uint32_t projection_next{};'+prefix[inside:]
    end=tail.rindex('\n}')
    control=(f'\n{JUMP}:\n'
             '    switch (projection_next) {\n'
             f'    case {NAMESPACE}::done: return;\n'
             + ''.join(f'    case 0x{site}u: goto L{site};\n' for site in (*RESUMES,*ROUTES))
             + '    }\n'
             '    throw std::logic_error("Unexpected particle projection continuation");')
    return INCLUDE+output[:found.start()]+prefix+tail[:end]+control+tail[end:]+output[found.end():],[STAGE]
