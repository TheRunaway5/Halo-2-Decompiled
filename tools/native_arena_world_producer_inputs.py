"""Pinned actual World caller/callback arms for the private Arena proof.

The receiver supplies the fields consumed by these authentic arms. This is
not a replacement World constructor, and the wider required dispatcher is
an explicit unexpected-entry stop, never a supplied successful outcome.
"""
import hashlib,re
from native_model_marker_lookup_replacements import function
WORLD='src/physics/physics_live_world_tu15.cpp'
PINS={'build_services':'086959fae536475a9988b9f5d7202595daa8f160aa9eb7a51a469b2ca43c47d9',
      'caller_abi':'26a86717f9b79e758ddfd2d209e312ac80c502dcc4435b04af1ab980259496ca',
      'frame':'e5f2509a36321ce99c1046f9200af7b8ed8cc86152816ba9e70935dffd9eb867',
      'clock':'c746f04320bac20cdef3fa58616ad474e4bc11dacf7ce20c7ebb4b177c9d52df',
      'construction':'467680af4d21b3b70ae5e5daf60afaf510d5f19fd44a33078cb41b9864835c50'}
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def block(source,start):
    a=source.index(start);b=source.index('{',a)+1;depth=1
    while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
    return source[a:b]
def prepare(root,scratch):
    source=(root/WORLD).read_text();actual_source_sha256=digest(source)
    if 'native_tu15_arena_world_publication.hpp'in source:
        # Remove only the separately pinned complete installed producer.
        # The original arm/body pins below remain unchanged.
        from native_arena_world_policy_prototype import canonical_world
        source=canonical_world(source)
    parts={'build_services':function(source,'build_services'),
        'caller_abi':block(source,'    SourcePhysicsWorldRuntimeAbi caller_abi() const {'),
        'frame':block(source,'    SourcePhysicsWorldRuntimeAbi frame(std::span<const std::uint32_t>args) {'),
        'clock':block(source,'class NativeWorldClock final:')}
    a=source.index('        arena=std::make_unique<SourcePhysicsGameArenaNative>');b=source.index('        // Actual32CBF8',a);parts['construction']=source[a:b]
    for name,pin in PINS.items():
        if digest(parts[name])!=pin:raise ValueError('Complete World Arena producer/caller drift '+name)
    required=re.search(r'^    struct WorldRequiredOwner \{.*?^    \};',source,re.M|re.S)
    if not required:raise ValueError('Actual World required-owner callable missing')
    builder=parts['build_services'];cut=builder.index('        static const bool reference_normalize=')
    # All instruction/timestamp/estimate/required-owner arms stay byte-exact.
    # The unused normalization arm is explicitly outside this receiver.
    selected=builder[:cut]+'        return s;\n    }'
    (scratch/'arena_world_clock.inc').write_text(parts['clock']+';\n')
    (scratch/'arena_world_producer.inc').write_text(required[0]+'\n'+selected+'\n'+parts['caller_abi']+'\n'+parts['frame']+'\n')
    return dict(actual_source_sha256=actual_source_sha256,complete_source_sha256=digest(source),complete_arm_pins=PINS,required_owner_sha256=digest(required[0]),normalization_arm_omitted=True,wider_world_dispatch_success_supplied=False,scope='Exact actual World caller and callback/cursor/required certificate arms in a declared receiver, genuine real Arena children; not an entire World/cache constructor or runtime acceptance.')
