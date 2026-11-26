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

    def test_codec_interpolation_exact_format_and_copied_siblings(self):
        subprocess.run([sys.executable,'-B',str(ROOT/'tests/test_native_codec_rotation_interpolation_bindings.py')],check=True)

    def test_codec_translation_interpolation_exact_format_and_copied_siblings(self):
        subprocess.run([sys.executable,'-B',str(ROOT/'tests/test_native_codec_translation_interpolation_bindings.py')],check=True)

    def test_codec_scale_interpolation_exact_format_and_copied_siblings(self):
        subprocess.run([sys.executable,'-B',str(ROOT/'tests/test_native_codec_scale_interpolation_bindings.py')],check=True)

    def test_codec_copy_exact_groups_and_copied_pure_helper(self):
        subprocess.run([sys.executable,'-B',str(ROOT/'tests/test_native_codec_copy_bindings.py')],check=True)

    def test_projectile_query_retag_and_copied_helper(self):
        import native_projectile_function_query_replacements as query
        owner=ROOT/'src/gameplay/units/source_combat_action_tu15_generated.cpp'
        raw=owner.read_text();prepared,receipt=self.module.specialize(raw)
        self.assertEqual(receipt['native_projectile_function_query_entries'],['100e70'])
        self.assertEqual(query.installed_entries(prepared),['100e70'])
        self.assertEqual(self.module.specialize(prepared)[0],prepared)
        from native_object_definition_mode_query_replacements import remove_object_definition_mode_query
        from native_object_table_ratio_replacements import remove_object_table_ratio
        def remove_queries(text):
            return query.remove_projectile_function_query(remove_object_definition_mode_query(remove_object_table_ratio(text)))
        self.assertEqual(remove_queries(prepared),self.module.specialize(remove_queries(raw))[0])
        for original,changed in ((raw,raw.replace(query.prefix(False),query.prefix(True),1)),
                                 (prepared,prepared.replace(query.prefix(True),query.prefix(False),1)),
                                 (raw,raw.replace(query.ORDERED_MULTIPLY,query.LEGACY_MULTIPLY,1)),
                                 (raw,query.INCLUDE+raw.replace(query.INCLUDE,'',1))):
            self.assertNotEqual(original,changed)
            with self.assertRaisesRegex(ValueError,'projectile query|projectile fallback'):
                self.module.specialize(changed)
        script=r'''
import importlib.util,sys
from pathlib import Path
before=tuple(sys.path)
spec=importlib.util.spec_from_file_location('copied_projectile',sys.argv[1])
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
assert helper.installed_entries(Path(sys.argv[2]).read_text())==['100e70']
assert tuple(sys.path)==before
assert not any(name.startswith(('validate_native_','audit_native_')) for name in sys.modules)
'''
        with tempfile.TemporaryDirectory(prefix='halo2-projectile-binding-')as directory:
            helper=Path(directory)/'native_projectile_function_query_replacements.py'
            helper.write_bytes((ROOT/'tools'/helper.name).read_bytes())
            subprocess.run([sys.executable,'-I','-B','-c',script,str(helper),str(owner)],check=True)

    def test_flight_query_retags_each_exact_owner_and_copied_helpers(self):
        import native_flight_property_query_replacements as primary
        import native_death_cycle_flight_query_replacements as death
        from native_object_record_ratio_replacements import remove_object_record_ratio
        def remove_primary(text):
            return primary.remove_flight_property_query(remove_object_record_ratio(text))
        owners=(('src/physics/physics_live_support_tu15_generated.cpp',primary,
                 remove_primary,'native_flight_property_query_entries'),
                ('src/gameplay/units/death_cycle_runtime_tu15_generated.cpp',death,
                 death.remove_death_cycle_flight_query,'native_death_cycle_flight_query_entries'))
        script=r'''
import importlib.util,sys
from pathlib import Path
before=tuple(sys.path)
spec=importlib.util.spec_from_file_location('copied_flight_query',sys.argv[1])
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
assert helper.installed_entries(Path(sys.argv[2]).read_text())==['e0900']
assert tuple(sys.path)==before
assert not any(name.startswith(('validate_native_','audit_native_')) for name in sys.modules)
'''
        with tempfile.TemporaryDirectory(prefix='halo2-flight-query-binding-',dir=os.environ.get('HALO2_TEST_SCRATCH')) as directory:
            helpers=('native_flight_property_query_replacements.py','native_death_cycle_flight_query_replacements.py',
                     'native_object_type_query_replacements.py')
            for name in helpers:(Path(directory)/name).write_bytes((ROOT/'tools'/name).read_bytes())
            for index,(name,binding,remove,receipt_key) in enumerate(owners):
                path=ROOT/name;raw=path.read_text();prepared,receipt=self.module.specialize(raw)
                self.assertEqual(receipt[receipt_key],['e0900'])
                self.assertEqual(receipt[owners[1-index][3]],[])
                self.assertEqual(binding.installed_entries(prepared),['e0900'])
                self.assertEqual(self.module.specialize(prepared)[0],prepared)
                self.assertEqual(remove(prepared),self.module.specialize(remove(raw))[0])
                for changed in (raw.replace(binding.prefix(False,False),binding.prefix(False,True),1),
                                raw+'\nusing namespace halo2::'+binding.OWNER+';\n'):
                    self.assertNotEqual(changed,raw)
                    with self.assertRaisesRegex(ValueError,'E0900'):
                        self.module.specialize(changed)
                subprocess.run([sys.executable,'-I','-B','-c',script,str(Path(directory)/helpers[index]),str(path)],check=True)

    def test_map_world_property_query_pair_retag_and_copied_helper(self):
        import native_map_world_property_query_replacements as binding
        from native_device_property_query_replacements import remove_device_property_query
        owner=ROOT/'src/runtime/source_map_world_support_tu15_generated.cpp'
        raw=owner.read_text();prepared,receipt=self.module.specialize(raw)
        self.assertEqual(receipt['native_map_world_property_query_entries'],list(binding.ENTRIES))
        self.assertEqual(self.module.specialize(prepared)[0],prepared)
        self.assertEqual(binding.remove_map_world_property_queries(remove_device_property_query(prepared)),
                         self.module.specialize(binding.remove_map_world_property_queries(remove_device_property_query(raw)))[0])
        for original,bad in ((raw,raw.replace(binding.prefix('11e490',False),binding.prefix('11e490',True),1)),
                             (prepared,prepared.replace(binding.prefix('1db800',True),'',1)),
                             (raw,raw.replace(binding.ORDERED_MUL,binding.OLD_MUL,1))):
            self.assertNotEqual(original,bad)
            with self.assertRaisesRegex(ValueError,'MapWorld property'):self.module.specialize(bad)
        script=r'''
import importlib.util,sys
from pathlib import Path
before=tuple(sys.path)
spec=importlib.util.spec_from_file_location('copied_map_world_properties',sys.argv[1])
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
assert module.installed_entries(Path(sys.argv[2]).read_text())==['11e490','1db800']
assert tuple(sys.path)==before
assert not any(name.startswith(('validate_native_','audit_native_')) for name in sys.modules)
'''
        with tempfile.TemporaryDirectory(prefix='halo2-map-world-properties-')as directory:
            helper=Path(directory)/'native_map_world_property_query_replacements.py'
            helper.write_bytes((ROOT/'tools'/helper.name).read_bytes())
            subprocess.run([sys.executable,'-I','-B','-c',script,str(helper),str(owner)],check=True)

    def test_device_property_query_retag_and_copied_helpers(self):
        import native_device_property_query_replacements as device
        owner=ROOT/'src/runtime/source_map_world_support_tu15_generated.cpp'
        raw=owner.read_text();prepared,receipt=self.module.specialize(raw)
        self.assertEqual(receipt['native_device_property_query_entries'],['10be20'])
        self.assertEqual(receipt['native_map_world_property_query_entries'],list(device.pair.ENTRIES))
        self.assertEqual(device.remove_device_property_query(prepared),
                         self.module.specialize(device.remove_device_property_query(raw))[0])
        self.assertEqual(self.module.specialize(prepared)[0],prepared)
        changed=raw.replace(device.prefix(False,False),device.prefix(False,True),1)
        self.assertNotEqual(changed,raw)
        with self.assertRaisesRegex(ValueError,'10BE20'):
            self.module.specialize(changed)
        script=r'''
import importlib.util,sys
from pathlib import Path
before=tuple(sys.path)
spec=importlib.util.spec_from_file_location('copied_device',sys.argv[1])
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
assert module.installed_entries(Path(sys.argv[2]).read_text())==['10be20']
assert tuple(sys.path)==before
assert not any(name.startswith(('validate_native_','audit_native_')) for name in sys.modules)
'''
        with tempfile.TemporaryDirectory(prefix='halo2-device-property-')as directory:
            for name in ('native_device_property_query_replacements.py','native_map_world_property_query_replacements.py'):
                (Path(directory)/name).write_bytes((ROOT/'tools'/name).read_bytes())
            subprocess.run([sys.executable,'-I','-B','-c',script,
                            str(Path(directory)/'native_device_property_query_replacements.py'),str(owner)],check=True)

    def test_object_definition_mode_query_retag_preserves_projectile(self):
        import native_object_definition_mode_query_replacements as definition
        path=ROOT/'src/gameplay/units/source_combat_action_tu15_generated.cpp'
        raw=path.read_text();prepared,receipt=self.module.specialize(raw)
        self.assertEqual(receipt['native_object_definition_mode_query_entries'],['165590'])
        self.assertEqual(receipt['native_projectile_function_query_entries'],['100e70'])
        from native_object_table_ratio_replacements import remove_object_table_ratio
        self.assertEqual(definition.remove_object_definition_mode_query(remove_object_table_ratio(prepared)),
                         self.module.specialize(definition.remove_object_definition_mode_query(remove_object_table_ratio(raw)))[0])
        self.assertEqual(self.module.specialize(prepared)[0],prepared)
        bad=raw.replace(definition.prefix(False),definition.prefix(True),1)
        self.assertNotEqual(raw,bad)
        with self.assertRaisesRegex(ValueError,'165590'):self.module.specialize(bad)
        with tempfile.TemporaryDirectory(prefix='halo2-definition-mode-') as directory:
            names=('native_object_definition_mode_query_replacements.py','native_projectile_function_query_replacements.py')
            for name in names:(Path(directory)/name).write_bytes((ROOT/'tools'/name).read_bytes())
            script=('import runpy,sys;from pathlib import Path;'
                    'm=runpy.run_path(sys.argv[1]);'
                    'assert m["installed_entries"](Path(sys.argv[2]).read_text())==["165590"]')
            subprocess.run([sys.executable,'-I','-B','-c',script,str(Path(directory)/names[0]),str(path)],check=True)

    def test_node_progress_query_retag_keeps_exact_child_and_public_route(self):
        import native_node_progress_query_replacements as node
        path=ROOT/'src/campaign/ai/source_ai_frame_tu15_generated.cpp'
        raw=path.read_text();prepared,receipt=self.module.specialize(raw)
        self.assertEqual(receipt['native_node_progress_query_entries'],['1cf0e0'])
        self.assertEqual(node.remove_node_progress_query(prepared),
                         self.module.specialize(node.remove_node_progress_query(raw))[0])
        self.assertEqual(self.module.specialize(prepared)[0],prepared)
        bad=raw.replace(node.prefix(False),node.prefix(True),1)
        self.assertNotEqual(raw,bad)
        with self.assertRaisesRegex(ValueError,'1CF0E0'):self.module.specialize(bad)
        with tempfile.TemporaryDirectory(prefix='halo2-node-progress-') as directory:
            helper=Path(directory)/'native_node_progress_query_replacements.py'
            helper.write_bytes((ROOT/'tools'/helper.name).read_bytes())
            script=('import runpy,sys;from pathlib import Path;'
                    'm=runpy.run_path(sys.argv[1]);'
                    'assert m["installed_entries"](Path(sys.argv[2]).read_text())==["1cf0e0"]')
            subprocess.run([sys.executable,'-I','-B','-c',script,str(helper),str(path)],check=True)

    def test_type_query_original_authentication_and_copied_pure_helper(self):
        import native_object_type_query_replacements as query
        source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
        self.assertEqual(query.replace_object_type_query(source),source)
        prepared,receipt=self.module.specialize(source)
        self.assertEqual(receipt['native_object_type_query_entries'],['10de40'])
        self.assertEqual(query.installed_entries(prepared),['10de40'])
        self.assertEqual(self.module.specialize(prepared)[0],prepared)
        controls=(source.replace(query.prefix(False),query.prefix(True),1),
            prepared.replace(query.prefix(True),query.prefix(False),1),
            source.replace('if(s.services.instruction){s.services.instruction(0x10de40u);}','s.instruction_literal(0x10de40u);',1),
            source.replace('NativeObjectTypeQuery10de46: ','',1),
            source+'\n'+query.prefix(False)+'\n')
        for wrong in controls:
            with self.assertRaisesRegex(ValueError,'TypeQuery'):self.module.specialize(wrong)
        script=r'''
import importlib.util,sys
from pathlib import Path
folder=Path(sys.argv[1]);before=tuple(sys.path)
spec=importlib.util.spec_from_file_location('copied_query',folder/'native_object_type_query_replacements.py')
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
assert helper.installed_entries(Path(sys.argv[2]).read_text())==['10de40']
assert tuple(sys.path)==before
assert not any(name.startswith(('validate_native_','audit_native_'))for name in sys.modules)
'''
        with tempfile.TemporaryDirectory(prefix='halo2-query-sibling-')as directory:
            folder=Path(directory);(folder/'native_object_type_query_replacements.py').write_bytes((ROOT/'tools/native_object_type_query_replacements.py').read_bytes())
            subprocess.run([sys.executable,'-I','-B','-c',script,str(folder),str(ROOT/'src/physics/physics_live_support_tu15_generated.cpp')],check=True)

    def test_hull_edge_authenticates_original_format_before_specialization(self):
        import native_solver_hull_edge_replacements as edge
        source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
        self.assertEqual(edge.replace_solver_hull_edge(source),source)
        prepared,receipt=self.module.specialize(source)
        self.assertEqual(receipt['native_solver_hull_edge_entries'],['313470'])
        self.assertEqual(edge.installed_entries(prepared),['313470'])
        self.assertEqual(self.module.specialize(prepared)[0],prepared)
        raw='if(s.services.instruction){s.services.instruction(0x313470u);}'
        literal='s.instruction_literal(0x313470u);'
        self.assertEqual(source.count(raw),1)
        self.assertEqual(prepared.count(literal),1)
        controls=(
            (source.replace(edge.RAW_PREFIX,edge.PREPARED_PREFIX,1),
             'Complete hull-edge prepared body/resolver/dispatcher drift'),
            (source.replace(raw,literal,1),
             'Complete tagged hull-edge body, source format or continuation placement drift'),
            (prepared.replace(edge.PREPARED_PREFIX,edge.RAW_PREFIX,1),
             'Complete tagged hull-edge body, source format or continuation placement drift'),
            (prepared.replace(literal,raw,1),
             'Complete hull-edge prepared body/resolver/dispatcher drift'),
        )
        for changed,error in controls:
            with self.subTest(error=error),self.assertRaisesRegex(ValueError,'^'+re.escape(error)+'$'):
                self.module.specialize(changed)

    def test_hull_initialize_authenticates_input_and_copied_helper(self):
        import native_solver_hull_initialize_replacements as initialize
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
        source=path.read_text()
        self.assertEqual(initialize.replace_solver_hull_initialize(source),source)
        self.assertEqual(initialize.installed_entries(source),['313560'])
        prepared,receipt=self.module.specialize(source)
        self.assertEqual(receipt['native_solver_hull_initialize_entries'],['313560'])
        self.assertEqual(initialize.installed_entries(prepared),['313560'])
        self.assertEqual(self.module.specialize(prepared)[0],prepared)
        raw=initialize.instruction(0x313561,False)
        literal=initialize.instruction(0x313561,True)
        controls=(
            (source,initialize.PREFIXES[False,False],initialize.PREFIXES[True,False]),
            (prepared,initialize.PREFIXES[True,False],initialize.PREFIXES[False,False]),
            (source,raw,literal),
            (prepared,literal,raw),
            (source,initialize.PREFIXES[False,False],initialize.PREFIXES[False,True]),
        )
        for index,(original,before,after) in enumerate(controls):
            self.assertEqual(original.count(before),1)
            changed=original.replace(before,after,1)
            self.assertNotEqual(changed,original)
            error=('Complete hull-initialize body/source-format/spill profile drift'
                   if index==4 else 'Hull-initialize label moved from authored instruction')
            with self.subTest(control=index),self.assertRaisesRegex(ValueError,'^'+re.escape(error)+'$'):
                self.module.specialize(changed)
        script=r'''
import importlib.util,sys
from pathlib import Path
before=tuple(sys.path)
spec=importlib.util.spec_from_file_location('copied_initialize',sys.argv[1])
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
assert helper.installed_entries(Path(sys.argv[2]).read_text())==['313560']
assert tuple(sys.path)==before
'''
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'),
                                         prefix='halo2-hull-initialize-sibling-') as directory:
            helper=Path(directory)/'native_solver_hull_initialize_replacements.py'
            helper.write_bytes((ROOT/'tools'/helper.name).read_bytes())
            subprocess.run([sys.executable,'-I','-B','-c',script,str(helper),str(path)],check=True)

    def test_primary_hull_rejects_markers_outside_their_actual_owner(self):
        from importlib import import_module
        source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
        prepared,_=self.module.specialize(source)
        families=('solver_hull_append','zero_shape_support','box_shape_support',
                  'solver_hull_support_pair','solver_hull_edge','solver_hull_initialize',
                  'solver_hull_line','solver_hull_sample')
        for name in families:
            helper=import_module('native_'+name+'_replacements')
            install=getattr(helper,'replace_'+name)
            unbound=getattr(helper,'remove_'+name)(source)
            body=helper.BODY.search(source)
            prefixes=getattr(helper,'PREFIXES',{}).values()
            prefix=next((p for p in prefixes if p in body[0]),getattr(helper,'PREFIX',None))
            self.assertIn(prefix,body[0])
            first=prefix.index('halo2::')
            last=prefix.index('(',first)
            depth=1
            while depth:
                last+=1
                depth+=int(prefix[last]=='(')-int(prefix[last]==')')
            expression=prefix[first:last+1]
            extras={'bridge':'static void outside_hull_owner(WorldState &s){(void)'+expression+';}\n'}
            labels=getattr(helper,'LABELS',{})
            if labels:
                extras['label']='static void outside_hull_label(){'+next(iter(labels.values()))+'return;}\n'
            for kind,extra in extras.items():
                for phase,text,action in (('raw',source,install),('unbound',unbound,install),
                                           ('raw entries',source,helper.installed_entries),
                                           ('prepared entries',prepared,helper.installed_entries)):
                    where=helper.BODY.search(text).start()
                    with self.subTest(family=name,kind=kind,phase=phase):
                        with self.assertRaisesRegex(ValueError,'Foreign .+ owner or continuation marker'):
                            action(text[:where]+extra+text[where:])
                # Exercise each specializer group at least once; the full
                # four-context audit covers every family separately.
                if (name,kind) in (('solver_hull_append','bridge'),
                                   ('solver_hull_support_pair','label'),
                                   ('solver_hull_line','label')):
                    chosen=unbound if kind=='label' else source
                    where=helper.BODY.search(chosen).start()
                    with self.assertRaisesRegex(ValueError,'Foreign .+ owner or continuation marker|Missing .+ overlay|default not installed'):
                        self.module.specialize(chosen[:where]+extra+chosen[where:])



    def test_hull_clip_authenticates_raw_and_prepared_input(self):
        import native_solver_hull_clip_replacements as clip
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
        raw=path.read_text()
        prepared,receipt=self.module.specialize(raw)
        self.assertEqual(receipt['native_solver_hull_clip_entries'],['3147d0'])
        self.assertEqual(self.module.specialize(prepared)[0],prepared)
        for text,mode in ((raw,False),(prepared,True)):
            self.assertEqual(clip.installed_entries(text),['3147d0'])
            pc=0x3147d1
            body=clip.BODY.search(text)[0]
            controls=(
                text.replace(clip.PREFIXES[mode,False],clip.PREFIXES[not mode,False],1),
                text.replace(body,body.replace(clip.instruction(pc,mode),clip.instruction(pc,not mode),1),1),
                text+'\nstatic void foreign_clip_label(){native_hull_clip_resume_3147d1:return;}\n',
            )
            for changed in controls:
                self.assertNotEqual(changed,text)
                with self.assertRaisesRegex(ValueError,'Hull-clip label moved|Complete hull-clip body|Foreign hull-clip'):
                    self.module.specialize(changed)

    def test_hull_clear_authenticates_raw_and_prepared_input(self):
        import native_solver_hull_clear_replacements as clear
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
        raw=path.read_text()
        prepared,receipt=self.module.specialize(raw)
        self.assertEqual(receipt['native_solver_hull_clear_entries'],['314870'])
        self.assertEqual(self.module.specialize(prepared)[0],prepared)
        for text,mode in ((raw,False),(prepared,True)):
            self.assertEqual(clear.installed_entries(text),['314870'])
            pc=0x314876
            body=clear.BODY.search(text)[0]
            controls=(
                text.replace(clear.PREFIXES[mode],clear.PREFIXES[not mode],1),
                text.replace(body,body.replace(clear.instruction(pc,mode),clear.instruction(pc,not mode),1),1),
                text+'\\nstatic void foreign_clear_label(){native_hull_clear_resume_314876:return;}\\n',
            )
            for changed in controls:
                self.assertNotEqual(changed,text)
                with self.assertRaisesRegex(ValueError,'Hull-clear label moved|Complete hull-clear body|Foreign hull-clear'):
                    self.module.specialize(changed)

    def test_hull_line_and_sample_authenticate_input_and_copied_helpers(self):
        import native_solver_hull_line_replacements as line
        import native_solver_hull_sample_replacements as sample
        import native_solver_hull_minimum_replacements as minimum
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
        source=path.read_text()
        prepared,receipt=self.module.specialize(source)
        self.assertEqual(self.module.specialize(prepared)[0],prepared)
        for helper,entry,name,pc,profile in (
                (line,'3136a0','line',0x3136a1,'FLD C1'),
                (sample,'313300','sample',0x313304,'spill'),
                (minimum,'3131b0','minimum',0x3131b1,'spill')):
            self.assertEqual(helper.installed_entries(source),[entry])
            self.assertEqual(helper.installed_entries(prepared),[entry])
            self.assertEqual(receipt['native_solver_hull_'+name+'_entries'],[entry])
            raw,literal=helper.instruction(pc,False),helper.instruction(pc,True)
            controls=(
                (source,helper.PREFIXES[False,False],helper.PREFIXES[True,False]),
                (prepared,helper.PREFIXES[True,False],helper.PREFIXES[False,False]),
                (source,raw,literal),
                (prepared,literal,raw),
                (source,helper.PREFIXES[False,False],helper.PREFIXES[False,True]),
            )
            for index,(original,before,after) in enumerate(controls):
                self.assertEqual(original.count(before),1)
                changed=original.replace(before,after,1)
                self.assertNotEqual(changed,original)
                error=(f'Complete hull-{name} body/source-format/{profile} profile drift'
                       if index==4 else f'Hull-{name} label moved from authored instruction')
                with self.subTest(entry=entry,control=index),self.assertRaisesRegex(ValueError,'^'+re.escape(error)+'$'):
                    self.module.specialize(changed)
        script=r"""
import importlib.util,sys
from pathlib import Path
folder=Path(sys.argv[1]);source=Path(sys.argv[2]).read_text();before=tuple(sys.path)
for name,entry in (('line','3136a0'),('sample','313300'),('minimum','3131b0')):
    spec=importlib.util.spec_from_file_location('copied_'+name,folder/f'native_solver_hull_{name}_replacements.py')
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    assert helper.installed_entries(source)==[entry]
assert tuple(sys.path)==before
"""
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'),
                                         prefix='halo2-hull-line-sample-siblings-') as directory:
            folder=Path(directory)
            for helper in (line,sample,minimum):
                name=Path(helper.__file__).name
                (folder/name).write_bytes((ROOT/'tools'/name).read_bytes())
            subprocess.run([sys.executable,'-I','-B','-c',script,str(folder),str(path)],check=True)

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

    def test_lookup_binders_load_from_their_copied_siblings(self):
        script=r'''
import importlib.util
from pathlib import Path
import sys
folder=Path(sys.argv[1]);source=Path(sys.argv[2]).read_text();file=sys.argv[3]
before=tuple(sys.path)
assert str(folder)not in sys.path
spec=importlib.util.spec_from_file_location("isolated_lookup",folder/file)
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
assert helper.installed_entries(source)==["bcdf0"]
assert tuple(sys.path)==before
assert "native_object_body_replacements"not in sys.modules
assert not any(name.startswith(("validate_native_","audit_native_"))for name in sys.modules)
'''
        owners={
            "native_object_lookup_copies_replacements.py":(
                ("src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp","native_object_lookup_copies_entries"),
                ("src/gameplay/objects/source_sandbox_special_tu15_generated.cpp","native_object_lookup_copies_entries"),
                ("src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp","native_object_lookup_copies_entries"),
                ("src/campaign/ai/source_ai_frame_tu15_generated.cpp","native_object_lookup_copies_entries")),
            "native_object_material_lookup_replacements.py":(
                ("src/gameplay/objects/source_object_material_tu15_generated.cpp","native_object_material_lookup_entries"),),
            "native_object_lookup_canonical_replacements.py":(
                ("src/gameplay/units/source_direct_damage_runtime_generated.cpp","native_object_lookup_canonical_entries"),
                ("src/gameplay/units/source_vitality_runtime_generated.cpp","native_object_lookup_canonical_entries")),
        }
        for helper,units in owners.items():
            with tempfile.TemporaryDirectory(prefix="halo2-lookup-siblings-")as directory:
                folder=Path(directory)
                for name in(helper,"native_object_body_replacements.py"):
                    (folder/name).write_bytes((ROOT/"tools"/name).read_bytes())
                for unit,key in units:
                    entry="bbcb0"if"canonical"in helper else"bcdf0"
                    isolated=script.replace('["bcdf0"]','["'+entry+'"]')
                    subprocess.run([sys.executable,"-I","-B","-c",isolated,str(folder),str(ROOT/unit),helper],check=True)
                    prepared,receipt=self.module.specialize((ROOT/unit).read_text())
                    self.assertEqual(receipt[key],[entry])
                    self.assertEqual(self.module.specialize(prepared)[1][key],[entry])

    def test_codec_translation_exact_known_include_sibling(self):
        import native_codec_translation_replacements as codec
        from specialize_native_operands import specialize
        for name in codec.inputs.UNITS:
            source=(ROOT/name).read_text()
            for form,text in(('raw',source),('prepared',specialize(source)[0])):
                with self.subTest(owner=name,form=form):
                    self.assertEqual(codec.replace_codec_translation(codec.remove_codec_translation(text)),text)
                    if codec.INTERPOLATION_INCLUDE in text:
                        for wrong in(codec.INTERPOLATION_INCLUDE+text,
                            text.replace(codec.INTERPOLATION_INCLUDE,'',1)+codec.INTERPOLATION_INCLUDE):
                            with self.assertRaisesRegex(ValueError,'sibling placement'):codec.installed_entries(wrong)
                    moved=text.replace(codec.INCLUDE,'',1)+codec.INCLUDE
                    with self.assertRaisesRegex(ValueError,'include placement'):codec.installed_entries(moved)
                    anchor=codec.INTERPOLATION_INCLUDE if codec.INTERPOLATION_INCLUDE in text else codec.ANCHOR
                    wrong=text.replace(anchor,anchor+'#include "unknown_codec_sibling.hpp"\n',1)
                    with self.assertRaisesRegex(ValueError,'include placement'):codec.installed_entries(wrong)

    def test_codec_binder_loads_only_its_copied_pure_siblings(self):
        script=r'''
import importlib.util
from pathlib import Path
import sys
folder=Path(sys.argv[1]);root=Path(sys.argv[2]);before=tuple(sys.path)
assert str(folder)not in sys.path
spec=importlib.util.spec_from_file_location('isolated_codec',folder/'native_codec_translation_replacements.py')
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
for name in helper.inputs.UNITS:
    text=(root/name).read_text()
    assert helper.installed_entries(text)==['2961f0']
    assert helper.replace_codec_translation(text)==text
    assert helper.replace_codec_translation(helper.remove_codec_translation(text))==text
    body=helper.BODY.search(text)[0];label=helper.LABELS[0x2961f5]
    changed=body.replace(label,'',1);target=next(line for line in changed.splitlines()if 'instruction(0x2961f8u)'in line)
    changed=changed.replace(target,label+target,1)
    try:helper.installed_entries(text.replace(body,changed,1))
    except ValueError as error:assert 'Relocated'in str(error)
    else:raise AssertionError('Copied codec accepted a relocated suffix')
assert tuple(sys.path)==before
assert not any(name.startswith(('validate_native_','audit_native_'))for name in sys.modules)
assert not {'native_codec_translation_replacements','native_packed_rotation_replacements','native_packed_rotation_source_inputs','native_model_marker_lookup_replacements','native_halfword_bit_query_replacements'}&sys.modules.keys()
'''
        with tempfile.TemporaryDirectory(prefix='halo2-codec-siblings-')as directory:
            folder=Path(directory)
            for name in('native_codec_translation_replacements.py','native_packed_rotation_replacements.py',
                        'native_packed_rotation_source_inputs.py','native_model_marker_lookup_replacements.py',
                        'native_halfword_bit_query_replacements.py'):
                (folder/name).write_bytes((ROOT/'tools'/name).read_bytes())
            subprocess.run([sys.executable,'-I','-B','-c',script,str(folder),str(ROOT)],check=True)
        for name in('src/physics/physics_live_support_tu15_generated.cpp',
                    'src/campaign/ai/source_ai_frame_tu15_generated.cpp',
                    'src/rendering/models/source_model_render_tu15_generated.cpp'):
            prepared,receipt=self.module.specialize((ROOT/name).read_text())
            self.assertEqual(receipt['native_codec_translation_entries'],['2961f0'])
            self.assertEqual(self.module.specialize(prepared)[1]['native_codec_translation_entries'],['2961f0'])

    def test_packed_codec_policy_edits_regenerate_prepared_aliases(self):
        # Exercise CMake's actual custom-command dependencies. The helper edit
        # belongs to this temporary miniature checkout, never the live tools.
        with tempfile.TemporaryDirectory(prefix="halo2-codec-dependency-")as directory:
            root=Path(directory);(root/"tools").mkdir();(root/"src").mkdir()
            module=(ROOT/"cmake/NativeOperandSpecialization.cmake").read_text()
            helpers=set(re.findall(r"[A-Za-z_]+\.py",module))
            for name in helpers:
                (root/"tools"/name).write_bytes((ROOT/"tools"/name).read_bytes())
            (root/"NativeOperandSpecialization.cmake").write_text(module)
            source=root/"src/probe_generated.cpp"
            source.write_text('std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){case 0x1u:return 0x2u;default:throw std::runtime_error("Unmapped");}}\nunsigned probe(State&s){return selected(0x1u,s.image); }\n')
            (root/"CMakeLists.txt").write_text("cmake_minimum_required(VERSION 3.22)\nproject(CodecDependencies NONE)\nset(Python3_EXECUTABLE \""+sys.executable+"\")\ninclude(NativeOperandSpecialization.cmake)\nhalo2_native_operand_source(prepared src/probe_generated.cpp)\nadd_custom_target(prepared_alias ALL DEPENDS \"${prepared}\")\n")
            build=root/"build"
            subprocess.run(["cmake","-G","Ninja","-S",str(root),"-B",str(build)],check=True,capture_output=True,text=True)
            command=["cmake","--build",str(build),"--target","prepared_alias"]
            subprocess.run(command,check=True,capture_output=True,text=True)
            output=build/"native-operands/probe_generated.cpp"
            before=output.stat().st_mtime_ns;before_bytes=output.read_bytes()
            subprocess.run(command,check=True,capture_output=True,text=True)
            self.assertEqual(output.stat().st_mtime_ns,before)
            policy=root/"tools/native_packed_rotation_source_inputs.py"
            policy.write_text(policy.read_text()+"\n# owned policy dependency witness\n")
            subprocess.run(command,check=True,capture_output=True,text=True)
            self.assertGreater(output.stat().st_mtime_ns,before)
            self.assertEqual(output.read_bytes(),before_bytes)

    def test_reconfigure_keeps_the_explicit_specialized_source(self):
        with tempfile.TemporaryDirectory(prefix='halo2-operand-reconfigure-') as directory:
            root=Path(directory)
            (root/'src').mkdir()
            source=root/'src/probe_generated.cpp'
            source.write_text('std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){return p;}\n')
            module=ROOT/'cmake/NativeOperandSpecialization.cmake'
            (root/'CMakeLists.txt').write_text(f"""cmake_minimum_required(VERSION 3.22)
project(OperandReconfigure LANGUAGES CXX)
set(Python3_EXECUTABLE "{sys.executable}")
include("{module}")
halo2_native_operand_source(prepared src/probe_generated.cpp)
add_library(probe STATIC "${{prepared}}")
halo2_specialize_native_target(probe)
file(GENERATE OUTPUT "${{CMAKE_CURRENT_BINARY_DIR}}/sources.txt" CONTENT "$<TARGET_PROPERTY:probe,SOURCES>")
""")
            build=root/'build'
            configure=['cmake','-G','Ninja','-S',str(root),'-B',str(build)]
            subprocess.run(configure,check=True,capture_output=True,text=True)
            prepared=build/'native-operands/probe_generated.cpp'
            before=(build/'sources.txt').read_text()
            self.assertIn(str(prepared),before.split(';'))
            prepared.parent.mkdir(parents=True,exist_ok=True)
            # Model the existing generated file with its retained dispatch
            # signature. A second configure must leave it on its first rule.
            prepared.write_bytes(source.read_bytes())
            reconfigured=subprocess.run(configure,capture_output=True,text=True)
            self.assertEqual(reconfigured.returncode,0,reconfigured.stderr)
            self.assertEqual((build/'sources.txt').read_text(),before)

    def test_scale_binder_loads_only_its_copied_pure_siblings(self):
        script=r'''
import importlib.util
from pathlib import Path
import sys
folder=Path(sys.argv[1]);root=Path(sys.argv[2]);before=tuple(sys.path)
assert str(folder)not in sys.path
spec=importlib.util.spec_from_file_location('isolated_scale',folder/'native_codec_scale_replacements.py')
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
for name in helper.UNITS:
    text=(root/name).read_text()
    assert helper.installed_entries(text)==['296220']
    assert helper.replace_codec_scale(text)==text
    assert helper.replace_codec_scale(helper.remove_codec_scale(text))==text
    body=helper.BODY.search(text)[0];label=helper.LABELS[0x296225]
    changed=body.replace(label,'',1);target=next(line for line in changed.splitlines()if 'instruction(0x296228u)'in line)
    changed=changed.replace(target,label+target,1)
    try:helper.installed_entries(text.replace(body,changed,1))
    except ValueError as error:assert 'Relocated'in str(error)
    else:raise AssertionError('Copied scale accepted a relocated suffix')
assert tuple(sys.path)==before
assert not any(name.startswith(('validate_native_','audit_native_'))for name in sys.modules)
assert not {'native_codec_scale_replacements','native_model_marker_lookup_replacements','native_sandbox_dispatch_preservation'}&sys.modules.keys()
'''
        with tempfile.TemporaryDirectory(prefix='halo2-scale-siblings-')as directory:
            folder=Path(directory)
            for name in('native_codec_scale_replacements.py','native_model_marker_lookup_replacements.py',
                        'native_sandbox_dispatch_preservation.py'):
                (folder/name).write_bytes((ROOT/'tools'/name).read_bytes())
            subprocess.run([sys.executable,'-I','-B','-c',script,str(folder),str(ROOT)],check=True)
        for name in('src/rendering/models/source_model_render_tu15_generated.cpp',
                    'src/gameplay/objects/source_sandbox_special_tu15_generated.cpp'):
            prepared,receipt=self.module.specialize((ROOT/name).read_text())
            self.assertEqual(receipt['native_codec_scale_entries'],['296220'])
            self.assertEqual(self.module.specialize(prepared)[1]['native_codec_scale_entries'],['296220'])

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
