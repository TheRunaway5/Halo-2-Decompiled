"""Authenticate the complete TU1.5 resource request before native binding."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

INCLUDE = '#include "rendering/native_tu15_geometry_request_abi.hpp"\n'
PREFIX = ('\n    if(native_geometry_request_detail::run(s,[](auto &v,std::uint32_t target,std::uint32_t site){\n'
          '        dispatch(v,target,site,false);}))return;\n')
PUBLISH_PREFIX = '\n    if(native_geometry_publish_detail::run(s))return;\n'
PUBLISH_INCLUDE = '#include "rendering/native_tu15_geometry_publish_abi.hpp"\n'
PUBLISH_PINS = {'1d989e24bf5d51a95544a92a12425019aab66e1a7605d38eb2d545e0f7da7e4a'}
# Full canonical body identities, including service targets and frame fields.
PINS = {'531100d5b62389564bb75c0128a2727415d1eb458f8aad11ba37347a7a74713a',
        '063663568dd71d11e8afb2e13eccf770bf82bfc1f47d742ee368f0a1c73bfdb6'}
SELECTED_PINS = {'1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
                 'dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981'}


def canonical_body(body):
    start = body.index('{') + 1
    prefix=PUBLISH_PREFIX if body.startswith('static void run_22a680(') else PREFIX
    return body[:start] + body[start + len(prefix):] if body[start:].startswith(prefix) else body


def install_native_geometry_request(source):
    return _install(source,'130980',PREFIX,INCLUDE,PINS)


def install_native_geometry_publish(source):
    return _install(source,'22a680',PUBLISH_PREFIX,PUBLISH_INCLUDE,PUBLISH_PINS)


def _install(source,entry,prefix,include,pins):
    matches = list(re.finditer(r'static void run_'+entry+r'\((\w+) &s\) \{.*?\n\}', source, re.S))
    if not matches:
        return source
    if len(matches) != 1:
        raise ValueError('Expected one complete TU1.5 resource request')
    match = matches[0]
    body, state = match[0], match[1]
    original = canonical_body(body)
    digest = hashlib.sha256(original.replace(state + ' &s', 'STATE &s').encode()).hexdigest()
    if digest not in pins:
        raise ValueError('Complete'+entry+' resource body changed before native binding')
    require_selected_identity(source,original,SELECTED_PINS)
    if original == body:
        start = body.index('{') + 1
        source = source[:match.start()] + body[:start] + prefix + body[start:] + source[match.end():]
    if include not in source:
        source = include + source
    return source
