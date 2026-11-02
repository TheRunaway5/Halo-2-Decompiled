"""Bind SHA-qualified literal30BD0 bodies to the semantic native owner.

The original body remains byte-exact behind strict admission and the owner's
reference switch. Profile choice follows the literal operands, never a guess
from the generated family's name or its selected image.
"""
import hashlib
import re

INCLUDE='#include "physics/runtime/native_tu15_normalize_vector_bridge.hpp"\n'
PROFILES={
    '9b753539d3d6eecfb3e24cfcc6c9501db34d7d563e5ff09fc7f2e3ad878b15e5':'Mp11Literal',
    'cdca761e7879a9c99b6029ee6caaada0b15c801a32fc33fd6a3b8c2797557985':'Tu15Literal',
    '0fb4bfcf1d502798571206228687f996c185d88417493acb49d9012f9fd876e1':'ImageSelected',
    '66fb612a582a2d8bc55c1956b0d68f53ab60738631c43d26b924e41ebfb9d75e':'Tu15Literal',
    '17d1015517b4161dfc533ea26f68bf05e558a28e424b9452a7b2aff5e76bcbb9':'Tu15Literal',
}
PATTERN=r'static void run_30bd0\(\w+ &s\) \{.*?\n\}'


def prefix(profile):
    if profile not in set(PROFILES.values()):raise ValueError('Unknown literal30BD0 constants profile')
    return ('    // BEGIN native normalize30BD0\n'
            f'    if(native_tu15_normalize_vector_try(s,NativeTu15NormalizeProfile::{profile}))return;\n'
            '    // END native normalize30BD0\n')


def literal_body(body):
    markers=re.findall(r'    // BEGIN native normalize30BD0\n.*?    // END native normalize30BD0\n',body,re.S)
    if len(markers)>1:raise ValueError('Multiple native30BD0 prefixes')
    if markers and markers[0] not in [prefix(p)for p in set(PROFILES.values())]:
        raise ValueError('Changed native30BD0 admission prefix')
    return body.replace(markers[0],'',1)if markers else body


def install_native_normalizer(source,profile='Tu15Literal'):
    matches=list(re.finditer(PATTERN,source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one literal30BD0 body')
    body=matches[0].group();literal=literal_body(body);sha=hashlib.sha256(literal.encode()).hexdigest()
    if PROFILES.get(sha)!=profile:
        raise ValueError(f'Unqualified/changed literal30BD0 or constants profile: {sha}, {profile}')
    marker=prefix(profile);start=literal.index('{\n')+2
    replacement=literal[:start]+marker+literal[start:]
    # Retain every original label and the canonical unreachable trailer used
    # by the production partitioner. This adds no external native symbol.
    source=source[:matches[0].start()]+replacement+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
