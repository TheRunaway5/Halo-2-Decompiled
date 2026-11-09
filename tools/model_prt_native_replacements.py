"""Bind the complete authenticated PRT helper bodies to native kernels."""
import hashlib
import re

BODY_SHA256 = {
    0x23520: 'da21d68e7204de2d3fea25c2d27d5f10ce175db67d0d0c61694fcc7d3afb9a09',
    0x23580: '799b7cbd7e8e0e47cbc66066190271288bbd2444be60c87ed818cade787f1e4e',
    0x23c80: '2f01f8a24c78e326f31cb0171453db4fa5733f3ddb3220b385f80d2757d1ae21',
    0x1456f0: '13e05f4fea789b25574df7583a3dd6488bc9f626562249842e155c0e9cb84525',
    0x145d50: '14c31b2fdc7e68bd34c55e9266962e2522306164ee56ccd090e85620ca467ede',
    0x145e60: '1a4f7b250cba23faa85ecdf67674020526c004b87ffb84295c2a40efdf1687f0',
    0x148b20: '66b3b5420a9c63684436d76cba6a8d5a692f143b8e61b933b288cd18879c00c0',
    0x22c10: '6adeb2168adf474e80247c4356e903b5d9233f924e7d79baf2f749d2ef6f908a',
    0x22730: '9c9eab47295f6c76da5fc07bc1c3f57b0b33a14239becb1ee84f6bd5a9f50b08',
    0x146100: 'aaf6838f73e44fd434a12f52006dc855dfee0d5adac0a1172e2d50efc788cabf',
}
# Full reference bodies emitted by the newer literal TU1.5 generator. These
# pins were derived from the selected() bodies above by replacing only
# selected(0xIDENTITYu,s.image) with that identical literal.
LITERAL_BODY_SHA256 = {
    0x23520: 'a49b43b57458ab1060dcd8359e9d334fd79a6982c7ab0b16dba0513e48c4dc73',
    0x23580: '72038c5c0de03b4f9eb3e30377dc41b235ce6dcab0cdd0247caceca24d5271bb',
    0x23c80: '37f2ddb3a99551ecfa2522d8573f565bd105fa6161a6449842a2545c09c75c20',
    0x1456f0: 'e0c05307343a42fabc23daf9817d4130d569b9ed49c84121e19c859f7693b414',
    0x145d50: '9576d16fad83b0ccc54fbdd7f1c75a4e11e7d25be55980665cabd5972d2c4852',
    0x145e60: 'e51b34909cdbce0da23fd2d918a5c6e48d5f95ead80aac9fcb0fc5b3ea6f0b62',
    0x148b20: '7b86f3995e96b9ab049bc9eb34a611299362dd5907e6cd6bfc618edf581a7e2a',
    0x22c10: 'fe4714b47e52d209d82973481d2f8533ac9cb2921e2d1517eadf9b1802ee12c9',
    0x22730: '4e869deb6916321f5dd61069f79c1b52eda4d9bb57c5a307d90da221900c5a2e',
    0x146100: '02cd29076113a819c81651c9844f3c08ce9727df28b7c89011ee641c87125592',
}
SELECTED_SHA256 = '82f64e531376dd5d49c3e201e78a321c9d260240fdeada8a6e68743e39defc79'


def authenticate_body(body, entry, source):
    normalized = body.replace('WorldState &s', 'STATE &s')
    digest = hashlib.sha256(normalized.encode()).hexdigest()
    if digest == LITERAL_BODY_SHA256[entry]:
        return
    if digest != BODY_SHA256[entry]:
        raise ValueError(f'Authenticated PRT helper {entry:x} changed before native binding')
    selected = re.search(r'std::uint32_t selected\(.*?\n', source)
    if not selected or hashlib.sha256(selected.group().encode()).hexdigest() != SELECTED_SHA256:
        raise ValueError('Authenticated model selected identity mapping changed')


def binding_prefix(entry):
    if entry in (0x22c10, 0x22730):
        call = f'prt_interpolation_detail::bridge(s,0x{entry:x}u)'
    else:
        call = {0x23c80: 'prt_projection_detail::bridge(s)',
                0x1456f0: 'prt_matrix_inverse_detail::bridge(s)',
                0x145d50: 'prt_matrix_vector_detail::bridge(s)',
                0x145e60: 'prt_basis_detail::bridge(s)',
                0x148b20: 'prt_scaled_basis_detail::bridge(s)',
                0x146100: 'prt_rotation_detail::bridge(s)'}.get(entry,
                f'prt_helpers_detail::bridge(s,0x{entry:x}u)')
    return ('\n    if(!prt_helpers_detail::reference()) {\n'
            f'        {call};return;\n'
            '    }')


def replace_prt_helpers(source):
    include = '#include "rendering/models/native_tu15_prt_helpers.hpp"\n'
    if include not in source:
        source = include + source
    projection_include = '#include "rendering/models/native_tu15_prt_projection.hpp"\n'
    if projection_include not in source:
        source = projection_include + source
    inverse_include = '#include "rendering/models/native_tu15_prt_matrix_inverse.hpp"\n'
    if inverse_include not in source:
        source = inverse_include + source
    product_include = '#include "rendering/models/native_tu15_prt_matrix_vector.hpp"\n'
    if product_include not in source:
        source = product_include + source
    basis_include = '#include "rendering/models/native_tu15_prt_basis.hpp"\n'
    if basis_include not in source:
        source = basis_include + source
    scaled_include = '#include "rendering/models/native_tu15_prt_scaled_basis.hpp"\n'
    if scaled_include not in source:
        source = scaled_include + source
    interpolation_include = '#include "rendering/models/native_tu15_prt_interpolation.hpp"\n'
    if interpolation_include not in source:
        source = interpolation_include + source
    rotation_include = '#include "rendering/models/native_tu15_prt_rotation_bridge.hpp"\n'
    if rotation_include not in source:
        source = rotation_include + source
    for entry in BODY_SHA256:
        pattern = rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}'
        matches = list(re.finditer(pattern, source, re.S))
        if len(matches) != 1:
            raise ValueError(f'Expected one authenticated PRT helper {entry:x}')
        body = matches[0].group()
        prefix = binding_prefix(entry)
        start = body.index('{') + 1
        bound = body[start:].startswith(prefix)
        original = body[:start] + body[start + len(prefix):] if bound else body
        # Authenticate every operation, operand, branch and store, including
        # reference bodies that have already received the native prefix.
        authenticate_body(original, entry, source)
        if bound:
            continue
        source = source.replace(body, body[:start] + prefix + body[start:], 1)
    return source
