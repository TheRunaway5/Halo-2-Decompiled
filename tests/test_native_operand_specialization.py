#!/usr/bin/env python3
"""Differential checks for build-time native operand specialization."""
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from source_layout import source_file


class NativeOperands(unittest.TestCase):
    def setUp(self):
        spec = importlib.util.spec_from_file_location(
            'native_operands', ROOT / 'tools/specialize_native_operands.py')
        self.module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.module)

    def test_instruction_trace_state_and_unknown_operands_match(self):
        source = r'''#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
enum class SourcePhysicsContactImage { MapPack11, TitleUpdate15 };
struct State { SourcePhysicsContactImage image; unsigned value; };
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){case 0x1u:return 0x1u;case 0x100u:return 0x108u;case 0xfffffff0u:return 0xfffffff4u;default:throw std::runtime_error("Unbound original contact executable address");}}
unsigned run(State &s, unsigned dynamic) {
    // selected(0x100u,s.image) must remain a comment.
    const char *literal="selected(0x100u,s.image)";
    unsigned state=s.value;
    for(unsigned i=0;i<16;++i) {
        state=(state+selected(0x100u,s.image))^selected(0xfffffff0u,s.image);
        std::cout<<i<<':'<<state<<':'<<literal<<'\n';
    }
    return state+selected(0x1u,s.image)+selected(dynamic,s.image)+selected(0x999u,s.image);
}
int main() {
    for(unsigned image=0;image<3;++image)for(unsigned dynamic:{0x1u,0x100u,0x777u}) {
        State s{SourcePhysicsContactImage(image),0x12345678u};
        try{const auto value=run(s,dynamic);std::cout<<"value="<<value<<'\n';}
        catch(const std::exception &e){std::cout<<"error="<<e.what()<<'\n';}
    }
}
'''
        specialized, receipt = self.module.specialize(source)
        self.assertEqual(receipt['folded_calls'], 3)
        self.assertEqual(receipt['unmapped_literal_calls'], 1)
        self.assertIn('// selected(0x100u,s.image)', specialized)
        self.assertIn('"selected(0x100u,s.image)"', specialized)
        self.assertIn('selected(dynamic,s.image)', specialized)
        self.assertIn('selected(0x999u,s.image)', specialized)
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            directory = Path(folder)
            outputs = []
            for name, text in (('canonical', source), ('specialized', specialized)):
                cpp = directory / (name + '.cpp'); cpp.write_text(text)
                binary = directory / name
                subprocess.run(['c++', '-std=c++20', '-O2', str(cpp), '-o', str(binary)], check=True)
                outputs.append(subprocess.check_output([str(binary)]))
            self.assertEqual(*outputs)

    def test_real_runtime_modules_preserve_their_source_and_known_bindings(self):
        total = 0
        for name in ('physics/physics_live_support_tu15_generated.cpp',
                     'rendering/shadows/source_shadow_frame_tu15_generated.cpp'):
            source = source_file(ROOT, name).read_text()
            specialized, receipt = self.module.specialize(source)
            if Path(name).name == 'source_shadow_frame_tu15_generated.cpp':
                self.assertGreater(receipt['folded_calls'], 100)
            else:
                self.assertEqual(receipt['folded_calls'], 0)
                self.assertGreater(receipt['instruction_cursor_calls'], 10000)
            self.assertEqual(source.count('if(s.services.instruction){s.services.instruction('),
                specialized.count('if(s.services.instruction){s.services.instruction(') +
                receipt['instruction_cursor_calls'])
            self.assertEqual(receipt['unmapped_literal_calls'], 0)
            self.assertIn('Unbound original contact executable address', specialized)
            self.assertEqual(self.module.specialize(specialized)[0], specialized)
            total += receipt['folded_calls']
        self.assertGreater(total, 10000)

    def test_changed_mapping_grammar_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'mapping'):
            self.module.specialize('std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){return p+1;}')

    def test_instruction_adapter_keeps_diagnostics_and_unadmitted_states(self):
        source = r'''#include "physics/runtime/physics_contact_runtime_internal.hpp"
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){case 0x1u:return 0x1u;default:throw std::runtime_error("Unbound original contact executable address");}}
// if(s.services.instruction){s.services.instruction(0x1u);}
const char *diagnostic="if(s.services.instruction){s.services.instruction(0x1u);}";
if(s.services.instruction){s.services.instruction(0x1u);}
'''
        output, receipt = self.module.specialize(source)
        self.assertEqual(receipt['instruction_cursor_calls'], 1)
        self.assertIn('// if(s.services.instruction){s.services.instruction(0x1u);}', output)
        self.assertIn('"if(s.services.instruction){s.services.instruction(0x1u);}"', output)
        self.assertIn('\ns.instruction_literal(0x1u);\n', output)
        source = source.replace('physics_contact_runtime_internal.hpp', 'unadmitted_state.hpp')
        self.assertEqual(self.module.specialize(source)[1]['instruction_cursor_calls'], 0)


if __name__ == '__main__':
    unittest.main()
