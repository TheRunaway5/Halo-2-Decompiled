#!/usr/bin/env python3
"""Compare the sole native281530/281F10 binding on authored TU1.5 pair inputs.

Reuses the original physical payload-filter harness. All executable, bank and
compiler products belong to run_test.py; only small proof reports survive.
"""
import hashlib
import inspect
import os
from pathlib import Path
import sys

import validate_native_activation_payload_filter_tu15 as parent


def once(text, old, new):
    assert text.count(old) == 1, old[:120]
    return text.replace(old, new)


def main():
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    root = parent.ROOT
    observer_path = root / "tools/observe_native_activation_payload_filter_tu15.cpp"
    observer = observer_path.read_text()
    observer = once(observer,
        'const auto private_first=capture.at("private_returned_stack")',
        'const auto private_first=capture.at("private_returned_stack")')
    observer = once(observer,
        'put(sp,0x1ccacau);put(sp+4u,pair[0]);put(sp+8u,pair[1]);entry.rip=0x281f10u;entry.rsp=sp;',
        'const auto world=original.word(0x52be48u),dispatcher=original.word(world+0x250u),result_byte=sp+0x80u;'
        'put(sp,0x1ccacau);put(sp+4u,result_byte);put(sp+8u,pair[0]);put(sp+12u,pair[1]);'
        'entry.rip=0x281530u;entry.rsp=sp;entry.rcx=dispatcher;')
    observer = once(observer,
        'NativeTu15ActivationPayloadFilterStats stats;\n  const auto accepted=native_tu15_activation_payload_filter(memory,{pair[0]},{pair[1]},fp,{},&stats);',
        'NativeTu15DiscretePairBindingActivity binding;\n  const auto returned=native_tu15_test_bound_discrete_pair(memory,dispatcher,result_byte,{pair[0]},{pair[1]},fp,{},&binding);'
        'const auto&stats=binding.payload;const auto accepted=std::to_integer<unsigned>(native.read(result_byte,1)[0]);'
        'check(binding.attempts==1&&binding.completed==1&&binding.refusals==0&&binding.unbound_filters==0&&binding.unbound_motion_types==0,"Native discrete activity differs");')
    observer = once(observer, '(expected.rax&255u)==accepted',
                    'expected.rax==returned&&original.read(result_byte,1)==native.read(result_byte,1)')
    observer = once(observer, '{"original_AL",expected.rax&255u}',
                    '{"original_return",expected.rax},{"native_return",returned},{"formal_result_byte",result_byte}')
    # The formal output byte has an explicit consumer even though it is in
    # caller-owned private DATA. It is directly compared above, without a
    # fabricated EAX/GP image or a CPU child on the native side.
    private = scratch / "discrete-observer.cpp"
    private.write_text('#include "halo2/native_tu15_discrete_pair_binding.hpp"\n' + observer)
    source = inspect.getsource(parent.main)
    source = once(source,
        "observer=ROOT/('tools/observe_native_activation_body_datum_tu15.cpp'if a.property else'tools/observe_native_activation_payload_filter_tu15.cpp')",
        "observer=Path(" + repr(str(private)) + ")")
    source = once(source,
        'for owner in(0x281f10,0x1d7530,',
        'for owner in(0x281530,0x281f10,0x1d7530,')
    source = once(source,
        'str(f.relative_to(ROOT)):sha(f)for f in files',
        'str(f.relative_to(ROOT)if f.is_relative_to(ROOT)else f):sha(f)for f in files')
    source = once(source,
        'files=[ROOT/f\'src/physics/native_tu15_{name}.cpp\'for name in units]',
        'files=[ROOT/"include/halo2/native_tu15_discrete_pair_binding.hpp",ROOT/"tools/validate_native_discrete_pair_binding_tu15.py",ROOT/"tools/observe_native_activation_payload_filter_tu15.cpp"]+[ROOT/f\'src/physics/native_tu15_{name}.cpp\'for name in units]')
    # Final pin verification accepts an absolute owned diagnostic source;
    # production sources remain pinned relative to the actual Root checkout.
    source = once(source,
        'unchanged=all(sha(ROOT/f)==v for f,v in pins.items())',
        'unchanged=all(sha(ROOT/f)==v for f,v in pins.items())')
    namespace = dict(parent.__dict__)
    namespace.update(__file__=__file__)
    exec(source, namespace)
    return namespace["main"]()


if __name__ == "__main__":
    raise SystemExit(main())
