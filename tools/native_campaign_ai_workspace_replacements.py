"""Group the authenticated AI workspace clear sites in the build-local unit."""

import hashlib
import re


OWNERS = (
    ('1c58e0', '1c5993', 'try_clear',
     '0e8cca0d6c846ebbbf1c5ec5885b4fe605812cafb63cb2c352baf6cc79453504'),
    ('27b2b0', '27b2dd', 'try_clear_path_hash',
     'b37a3fb8a99befd39aa4aabce3cce66482dcd501f3547921bc1069f4b2773e0b'),
)
RESOLVER = re.compile(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}', re.S)
RESOLVER_SHA256 = '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657'
INCLUDE = '#include "campaign/ai/native_campaign_ai_workspace_clear.hpp"\n'


PATH_RING_OWNERS = (
    ('27b340', '486c3b678584c6802bbd3d352f1cf3d77128a6338c3477e230500bc4b5e15b32'),
    ('27bd30', '6aff17073c71c069a303f708d9dc99ce37ef36e57d2d6386d6f0ec22f1f050e4'),
)
PATH_RING_INCLUDE = '#include "campaign/ai/native_campaign_ai_path_edge_ring.hpp"\n'


def install_path_ring(canonical, output):
    resolver = RESOLVER.search(canonical)
    if not resolver or hashlib.sha256(resolver[0].encode()).hexdigest() != RESOLVER_SHA256:
        return output, []
    for entry, pin in PATH_RING_OWNERS:
        canonical_body = re.search(rf'^static void run_{entry}\(WorldState &s\) \{{.*?^\}}',
                                   canonical, re.M | re.S)
        if not canonical_body or hashlib.sha256(canonical_body[0].encode()).hexdigest() != pin:
            return output, []

    marker = '    using physics_world_runtime_detail::WorldState::WorldState;\n'
    state = marker + '    std::uint32_t path_edge_ring_caller_sp{};\n'
    call = re.compile(r'^(s\.instruction_literal\(0x27b447u\); '
                      r's\.push\(0x27b447u,[^\n]+?\);)run_27bd30\(s\);$', re.M)
    admission = 'L27c2b2: if(halo2::native_campaign_ai_path_edge_ring::try_run(s))goto L27c3e7;\n'
    tail = 'L27c2b2: s.instruction_literal(0x27c2b2u); '
    if PATH_RING_INCLUDE in output and state in output and admission in output and not call.search(output):
        return output, ['27c2b2']
    if output.count(marker) != 1 or output.count(tail) != 1 or len(call.findall(output)) != 1:
        raise ValueError('Prepared 27C2B2 ring or its authenticated caller differs')
    output = output.replace(marker, state).replace(tail, admission + 's.instruction_literal(0x27c2b2u); ')
    output = call.sub(r'\1halo2::native_campaign_ai_path_edge_ring::call_from_search(s,run_27bd30);', output)
    return PATH_RING_INCLUDE + output, ['27c2b2']


def install(canonical, output):
    resolver = RESOLVER.search(canonical)
    if not resolver or hashlib.sha256(resolver[0].encode()).hexdigest() != RESOLVER_SHA256:
        return output, []
    installed = []
    for entry, pc, operation, pin in OWNERS:
        body = re.compile(rf'^static void run_{entry}\(WorldState &s\) \{{.*?^\}}', re.M | re.S)
        original = body.search(canonical)
        if not original or hashlib.sha256(original[0].encode()).hexdigest() != pin:
            continue
        site = f's.instruction_literal(0x{pc}u); '
        scalar = (f'while(s.gp[1]){{w(s.memory,s.observe,0x{pc}u,s.gp[7],4,s.gp[0]);'
                  'const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;'
                  f'--s.gp[1];if(s.services.instruction)s.services.instruction(0x{pc}u);}}')
        bridge = f'if(!halo2::native_campaign_ai_workspace_clear::{operation}(s)) '
        prepared = body.search(output)
        if prepared and prepared[0].count(site + scalar) == 1:
            bound = prepared[0].replace(site + scalar, site + bridge + scalar)
            output = output[:prepared.start()] + bound + output[prepared.end():]
        elif not prepared or prepared[0].count(site + bridge + scalar) != 1:
            raise ValueError(f'Prepared {pc.upper()} store loop differs from the authenticated owner')
        installed.append(pc)
    if installed and INCLUDE not in output:
        output = INCLUDE + output
    return output, installed
