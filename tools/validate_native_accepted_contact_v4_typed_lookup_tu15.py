#!/usr/bin/env python3
"""Positive V4 typed lookup reader on retained, independently executed TU1.5 inputs."""
import inspect

import validate_native_accepted_contact_v4_public_regressions_tu15 as public

TOOL = 'tools/validate_native_accepted_contact_v4_typed_lookup_tu15.py'
CASES = r'actual-warthog_[1-3]_[0-3]'


def typed_driver():
    text = (public.ROOT / 'tools/observe_native_body_constraint_leaves_tu15.cpp').read_text()
    old = '  const auto result=native_tu15_find_model_constraint(memory,{component},{sp-0x400u},mode&1u,fp,&lookup_stats);'
    assert text.count(old) == 1
    text = text.replace(old, '''  struct Reader final:NativeTu15ModelConstraintLookupRequestData {
   const SourcePhysicsContactMemory& memory;mutable J phases=J::array();
   explicit Reader(const SourcePhysicsContactMemory&m):memory(m){}
   std::uint32_t word(std::uint32_t request,std::uint32_t offset,unsigned width)const override {
    std::array<std::byte,4>bytes{};memory.read(request+offset,std::span(bytes).first(width));
    phases.push_back(J::array({offset,width}));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
   }
  }reader(memory);
  const auto result=native_tu15_find_model_constraint_data(memory,{component},{sp-0x400u},mode&1u,fp,reader,&lookup_stats);''')
    old = '{"lookup_calls",lookup_stats.calls},'
    assert text.count(old) == 1
    return text.replace(old, '{"typed_request_phases",reader.phases},' + old)


def main():
    text = inspect.getsource(public.main)
    text = text.replace('    args = p.parse_args()', '    args = p.parse_args();assert args.mode=="lookup"')
    text = text.replace('source_overlay_only=True, unchanged_harness_logic=True,',
                        'source_overlay_only=False, unchanged_original_harness_logic=True, typed_request_reader=True,')
    text = text.replace('tools = (*COMMON_TOOLS,', 'tools = (*COMMON_TOOLS, TOOL,')
    old = "            sys.argv.extend(('--leaf', 'lookup', '--case', args.lookup_case))"
    assert text.count(old) == 1
    text = text.replace(old, old + '''
            main_source=inspect.getsource(harness.main)
            marker='prefix+observer.read_text()';assert main_source.count(marker)==1
            patched=main_source.replace(marker,'prefix+TYPED_DRIVER')
            namespace=dict(harness.__dict__,TYPED_DRIVER=typed_driver());exec(patched,namespace)
            harness.main=namespace['main']''')
    env = dict(public.__dict__, DEFAULT_LOOKUP_CASES=CASES, TOOL=TOOL, inspect=inspect,
               typed_driver=typed_driver, __file__=__file__, __doc__=__doc__)
    exec(text, env)
    return env['main']()


if __name__ == '__main__':
    raise SystemExit(main())
