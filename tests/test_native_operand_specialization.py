#!/usr/bin/env python3
"""Differential checks for build-time native operand specialization."""
import importlib.util
import os
import re
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

    def test_object_node_binder_uses_only_explicit_sibling_imports(self):
        # Reproduce the BSP/custom-build loader, including the transitive
        # descriptor/lifecycle checks, with no tools import-search entry.
        script=r'''
import importlib.util
from pathlib import Path
import sys
root=Path(sys.argv[1]);before=tuple(sys.path)
assert str(root/'tools')not in sys.path
def load(name,file):
    spec=importlib.util.spec_from_file_location(name,root/'tools'/file)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
specializer=load('isolated_specializer','specialize_native_operands.py')
helper=load('isolated_object_nodes','native_object_node_rebuild_replacements.py')
for path in ('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp'):
    source=(root/path).read_text()
    assert set(helper.installed_entries(source))=={'bf0c0','bfb90','b8780','b87b0'}
    assert helper.install_native_object_nodes(source)==source
    specialized,receipt=specializer.specialize(source)
    assert receipt['native_object_node_rebuild_entries']==['bf0c0']
    assert set(receipt['native_object_animation_entries'])=={'bfb90','b8780','b87b0'}
    assert set(helper.installed_entries(specialized))=={'bf0c0','bfb90','b8780','b87b0'}
    owner=helper.body(source,'bf0c0');replacement=owner[0].replace('0x60u','0x61u',1)
    if replacement==owner[0]:raise AssertionError('Deliberate body mutation seam missing')
    changed=source[:owner.start()]+replacement+source[owner.end():]
    try:helper.authenticate(changed)
    except ValueError as error:assert str(error)=='Complete object node body drift bf0c0',str(error)
    else:raise AssertionError('Object body drift accepted')
assert tuple(sys.path)==before
assert not({'native_selected_operand_binding','native_node_descriptor_lookup_replacements',
    'native_node_resource_lifecycle_replacements','native_halfword_bit_query_replacements'}&sys.modules.keys())
print('isolated object sibling imports passed')
'''
        subprocess.run([sys.executable,'-I','-B','-c',script,str(ROOT)],check=True)

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

    def test_main_heap_bindings_require_exact_bodies_and_selected_addresses(self):
        source=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text()
        _,receipt=self.module.specialize(source)
        self.assertEqual(set(receipt['native_main_heap_entries']),
            {entry[2] for entry in self.module.MAIN_HEAP_ENTRIES.values()})
        for name,(_,_,actual) in self.module.MAIN_HEAP_ENTRIES.items():
            body=self.module.leaf_body(source,name)
            changed=body[0].replace('4,0);','2,0);',1)
            self.assertNotEqual(body[0],changed)
            _,refused=self.module.specialize(source[:body.start()]+changed+source[body.end():])
            self.assertNotIn(actual,refused['native_main_heap_entries'])
        changed=source.replace('case 0x22e42au:return 0x232d2au;',
            'case 0x22e42au:return 0x232d2bu;',1)
        self.assertNotEqual(source,changed)
        # The installed MOPP owner authenticates this complete shared resolver.
        with self.assertRaisesRegex(ValueError,'Complete309940 MOPP.*drift'):
            self.module.specialize(changed)

    def test_main_heap_allocation_bindings_require_exact_source_and_tables(self):
        source=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text()
        _,receipt=self.module.specialize(source)
        self.assertEqual(set(receipt['native_main_heap_allocation_entries']),
            {entry[2] for entry in self.module.MAIN_HEAP_ALLOCATION_ENTRIES.values()})
        for name,(_,_,actual) in self.module.MAIN_HEAP_ALLOCATION_ENTRIES.items():
            body=self.module.leaf_body(source,name)
            changed=body[0].replace('4,0);','2,0);',1)
            self.assertNotEqual(body[0],changed)
            _,refused=self.module.specialize(source[:body.start()]+changed+source[body.end():])
            self.assertNotIn(actual,refused['native_main_heap_allocation_entries'])
        for before,after,actual in (('0x22e35cu:return 0x232c5cu','0x22e35cu:return 0x232c60u','232ad0'),
            ('0x22e470u:return 0x232d70u','0x22e470u:return 0x232d74u','232d30'),
            ('0x22e411u:return 0x232d11u','0x22e411u:return 0x232d12u','232d00')):
            changed=source.replace(before,after,1);self.assertNotEqual(source,changed)
            with self.assertRaisesRegex(ValueError,'Complete309940 MOPP.*drift'):
                self.module.specialize(changed)

    def test_raw_allocator_bindings_require_whole_bodies(self):
        source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
        _,receipt=self.module.specialize(source)
        self.assertEqual(set(receipt['native_raw_allocator_entries']),set(self.module.RAW_ALLOCATOR_ENTRIES))
        for name in self.module.RAW_ALLOCATOR_ENTRIES:
            body=self.module.leaf_body(source,name)
            changed=body[0].replace('4,0);','2,0);',1);self.assertNotEqual(body[0],changed)
            _,refused=self.module.specialize(source[:body.start()]+changed+source[body.end():])
            self.assertNotIn(name,refused['native_raw_allocator_entries'])

    def test_actual_allocator_owners_require_complete_dispatch_context(self):
        source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
        _,receipt=self.module.specialize(source)
        self.assertEqual(set(receipt['native_allocator_actual_owner_entries']),
            {f'{owner:x}' for owner in self.module.ALLOCATOR_ACTUAL_OWNER_BYTES})
        dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
        changed=dispatch[0].replace('switch(target)','switch(target+0u)',1)
        self.assertNotEqual(changed,dispatch[0])
        # The bound page parent additionally authenticates this complete
        # dispatcher. Its whole-owner rejection precedes any refusal receipt.
        with self.assertRaisesRegex(ValueError,'Complete13FE50 dispatcher drift'):
            self.module.specialize(source[:dispatch.start()]+changed+source[dispatch.end():])

    def test_page_window_binding_requires_complete_body(self):
        source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
        self.assertEqual(self.module.specialize(source)[1]['native_page_window_entries'],['13fe00'])
        body=self.module.leaf_body(source,'13fe00')
        changed=body[0].replace('0x38u','0x3cu',1);self.assertNotEqual(body[0],changed)
        with self.assertRaisesRegex(ValueError,'page-window body drift'):
            self.module.specialize(source[:body.start()]+changed+source[body.end():])

    def test_page_retire_bindings_require_complete_bodies(self):
        for name in ('src/physics/physics_live_support_tu15_generated.cpp',
                     'src/gameplay/units/source_combat_action_tu15_generated.cpp'):
            source=(ROOT/name).read_text()
            self.assertEqual(self.module.specialize(source)[1]['native_page_retire_entries'],['140310'])
            body=self.module.leaf_body(source,'140310')
            changed=body[0].replace('0x64u','0x68u',1);self.assertNotEqual(body[0],changed)
            with self.assertRaisesRegex(ValueError,'page-retire body drift'):
                self.module.specialize(source[:body.start()]+changed+source[body.end():])

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
