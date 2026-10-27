#!/usr/bin/env python3
"""AOT CPU prefixes execute the actual source words through fixed functions."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from native_cpu_vertex_kernel import build_position_header, position_prefix, _program


def source_registry():
    registry = json.loads((ROOT / 'shaders/native-programs/world/programs.json').read_text())
    if registry.get('cpu_vertex_programs'):
        return registry
    # The existing finite water/sky sources also exercise the exact same AOT
    # compiler before the expanded offline all-class inventory is generated.
    rows = {row['vertex_program_hex']: row for row in registry['entries']}
    return {'cpu_vertex_programs': list(rows.values())}


class NativeCpuPositionTests(unittest.TestCase):
    def test_complete_source_identity_and_extent(self):
        row = source_registry()['cpu_vertex_programs'][0]
        bad = dict(row)
        bad['vertex_program_hex'] = row['vertex_program_hex'][:-2]
        with self.assertRaises(ValueError):
            _program(bad)
        bad = dict(row)
        bad['vertex_instructions'] = []
        with self.assertRaises(ValueError):
            _program(bad)
        with self.assertRaises(ValueError):
            build_position_header({'cpu_vertex_programs': [row, row]})

    def test_compiled_prefixes_match_independent_cpu_reference(self):
        compiler = shutil.which('c++')
        if not compiler:
            self.skipTest('C++ compiler is unavailable')
        owner = Path(os.environ['HALO2_TEST_SCRATCH']) / 'native-cpu-position'
        owner.mkdir()
        header, report = build_position_header(source_registry())
        self.assertGreater(report['position_kernels'], 0)
        (owner / 'native_position_kernels.generated.hpp').write_text(header)
        source = r'''
#include "native_position_kernels.generated.hpp"
#include "halo2/pstencil/geometry.hpp"
#include <bit>
#include <iostream>
int main() {
    unsigned cases=0,words=0,rejections=0,program=0;
    for(const auto& entry:halo2::generated_native_position_kernels::entries) {
        halo2::VertexProgram original;
        original.count=unsigned(entry.program.size());
        std::copy(entry.program.begin(),entry.program.end(),original.instructions.begin());
        const auto prefix=halo2::pstencil::position_prefix(original);
        for(unsigned sample=0;sample<8;++sample) {
            halo2::VertexProgramState expected;
            for(unsigned i=0;i<expected.constants.size();++i)for(unsigned a=0;a<4;++a)
                expected.constants[i][a]=float(int((i*17+a*5+sample*3)%29)-14)/16.f;
            for(unsigned i=0;i<expected.attributes.size();++i)for(unsigned a=0;a<4;++a)
                expected.attributes[i][a]=float(int((i*7+a*3+sample*5)%19)-9)/16.f;
            auto actual=expected;
            bool expected_error=false,actual_error=false;
            try {halo2::execute_vertex_program(prefix,expected);}catch(const halo2::FormatError&){expected_error=true;}
            try {halo2::pstencil::execute_native_position_prefix(original,actual);}catch(const halo2::FormatError&){actual_error=true;}
            if(expected_error!=actual_error)return 2;
            if(expected_error)continue;
            for(unsigned i=0;i<expected.outputs.size();++i)for(unsigned a=0;a<4;++a) {
                if(std::bit_cast<std::uint32_t>(expected.outputs[i][a])!=
                   std::bit_cast<std::uint32_t>(actual.outputs[i][a])) {
                    std::cerr<<"Native CPU prefix "<<program<<" sample "<<sample<<" output "<<i<<'.'<<a
                        <<" expected bits "<<std::bit_cast<std::uint32_t>(expected.outputs[i][a])
                        <<" actual bits "<<std::bit_cast<std::uint32_t>(actual.outputs[i][a])<<'\n';return 3;
                }
                ++words;
            }
            ++cases;
        }
        auto changed=original;changed.instructions[0][0]^=0xffffffffu;
        halo2::VertexProgramState rejected;
        try {halo2::pstencil::execute_native_position_prefix(changed,rejected);return 5;}
        catch(const halo2::FormatError&){++rejections;}
        ++program;
    }
    if(!cases || !words)return 4;
    std::cout<<cases<<" native CPU prefix cases; "<<words<<" exact output words; "<<rejections<<" complete-identity rejection controls\n";
}
'''
        unit = owner / 'probe.cpp'
        unit.write_text(source)
        binary = owner / 'probe'
        compiled=subprocess.run([compiler, '-std=c++20', '-O2', '-ffp-contract=off',
            '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
            '-I'+str(ROOT/'include'), '-I'+str(owner), str(unit),
            str(ROOT/'src/rendering/vertex_program.cpp'), str(ROOT/'tests/reference/vertex_program_reference.cpp'),
            str(ROOT/'src/pstencil/geometry.cpp'),
            '-o', str(binary)], capture_output=True, text=True)
        self.assertEqual(compiled.returncode,0,compiled.stdout+compiled.stderr)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertIn('exact output words', result.stdout)
        print(result.stdout.strip())
        print('Owned scratch:', owner.parent)


if __name__ == '__main__':
    unittest.main()
