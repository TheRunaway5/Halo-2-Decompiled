#!/usr/bin/env python3
"""Strict private World/Arena producer, source-format and suffix authority."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import native_arena_world_initialize_replacements as binding
import native_arena_world_policy_prototype as producer
import specialize_native_operands as specialization
import validate_native_arena_public_world_tu15 as public_proof
from validate_native_arena_public_world_tu15 import canonicalize_borrowed_links

UNIT='src/physics/physics_game_arena_runtime_generated.cpp'


class WorldArenaReplacements(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.current=(ROOT/UNIT).read_text()
        cls.raw=binding.remove_arena_world_initialize(cls.current)
        cls.prepared=specialization.specialize(cls.raw)[0]

    def profiles(self):
        for prepared,source in((False,self.raw),(True,self.prepared)):
            yield prepared,source,binding.replace_arena_world_initialize(source)

    def refuse(self,source):
        for api in(binding.authenticate,binding.replace_arena_world_initialize,binding.installed_entries):
            with self.subTest(api=api.__name__):
                with self.assertRaises(ValueError):api(source)

    def test_complete_raw_prepared_roundtrip(self):
        for prepared,source,bound in self.profiles():
            with self.subTest(prepared=prepared):
                self.assertEqual(binding.installed_entries(bound),['126640'])
                self.assertEqual(binding.replace_arena_world_initialize(bound),bound)
                self.assertEqual(binding.remove_arena_world_initialize(bound),source)
                body=binding.BODY.search(bound)[0]
                self.assertEqual(binding.digest(body),binding.BOUND_PINS[prepared])
                self.assertEqual(binding.canonical_body(body),binding.BODY.search(source)[0])
                prior=binding.remove_world_only(bound)
                self.assertEqual(binding.replace_arena_world_initialize(prior),bound)
                self.assertEqual(binding.remove_arena_world_initialize(prior),source)
                self.assertNotIn(binding.INCLUDE,prior)

    def test_moved_suffixes(self):
        for prepared,source,bound in self.profiles():
            for pc,label in(*binding.LABELS.items(),(0x1252b8,binding.REP_LABEL)):
                changed=bound.replace(label,'',1)
                owner=binding.BODY.search(changed)
                at=owner.start()+owner[0].index('{')+1+len(binding.prefix(prepared))
                changed=changed[:at]+label+changed[at:]
                with self.subTest(prepared=prepared,pc=hex(pc)):self.refuse(changed)

    def test_entry_format_and_global_ownership(self):
        for prepared,source,bound in self.profiles():
            body=binding.BODY.search(bound)[0]
            prefix=binding.prefix(prepared)
            removed=bound.replace(prefix,'',1)
            end=removed.index('\n}',removed.index('static void run_125280'))
            moved_end=removed[:end]+prefix+removed[end:]
            nested=removed.replace('if(s.services.instruction)', 'if(s.services.instruction)'+prefix,1)
            wrong_owner=bound.replace('native_arena_world_initialize_owner_detail::bridge',
                'native_arena_initialize_owner_detail::bridge',1)
            controls={
                'opposite-tag':bound.replace(prefix,binding.prefix(not prepared),1),
                'mixed-callbacks':bound.replace(binding.callback(prepared,0x125280),binding.callback(not prepared,0x125280),1),
                'moved-end':moved_end,'moved-nested':nested,'wrong-owner':wrong_owner,
                'prior-helper-no-include':bound.replace(binding.INCLUDE,'',1).replace(prefix,'',1)+prefix,
                'duplicate-outside':bound+prefix,'partial-outside':bound+'\nhalo2::native_arena_world_initialize_owner_detail::bridge<true>(s,child);\n',
                'missing-include':bound.replace(binding.INCLUDE,'',1),
                'moved-include':bound.replace(binding.INCLUDE,'',1)+binding.INCLUDE,
                'body':bound.replace(body,body.replace('0xff800u','0xff801u',1),1),
                'resolver':bound.replace('case 0x125280u:return 0x126640u;','case 0x125280u:return 0x126641u;',1),
                'dispatcher':bound.replace('s.required(site,target,tail);}','s.required(site,target+1u,tail);}',1),
                'export':bound.replace('case 0x125280u:run_125280(s);break;','case 0x125280u:run_125280(s);return;',1),
            }
            for label,changed in controls.items():
                with self.subTest(prepared=prepared,mutation=label):
                    self.assertNotEqual(changed,bound)
                    self.refuse(changed)
            self.refuse(source+prefix)
            self.refuse(source+'\nhalo2::native_arena_world_initialize_owner_detail::bridge<true>(s,child);\n')

    def test_late_retag_only_owned_source(self):
        raw_bound=binding.replace_arena_world_initialize(self.raw)
        intermediate,receipt=specialization.specialize(raw_bound)
        # The current production specializer has no private Arena retag hook.
        prepared=binding.retag_specialized(raw_bound,intermediate)
        self.assertEqual(binding.installed_entries(prepared),['126640'])
        self.assertEqual(binding.digest(binding.BODY.search(prepared)[0]),binding.BOUND_PINS[True])
        repeated=specialization.specialize(prepared)[0]
        self.assertEqual(binding.retag_specialized(prepared,repeated),prepared)
        unbound=specialization.specialize(self.raw)[0]
        self.assertEqual(binding.retag_specialized(self.raw,unbound),unbound)
        self.assertNotIn(binding.prefix(True),unbound)
        for changed in(intermediate+'\nhalo2::native_arena_world_initialize_owner_detail::bridge<true>(s,child);',
                       intermediate.replace('0xff800u','0xff801u',1)):
            with self.assertRaises(ValueError):binding.retag_specialized(raw_bound,changed)

    def test_fixed_mappack_not_an_owner(self):
        source=(ROOT/'src/gameplay/units/death_startup_runtime_generated.cpp').read_text()
        self.refuse(source)

    def test_exact_actual_producers(self):
        for name,install,canonical in((
            'src/physics/physics_live_world_tu15.cpp',producer.world_source,producer.canonical_world),(
            'src/physics/physics_game_arena_native.cpp',producer.arena_source,producer.canonical_arena)):
            source=(ROOT/name).read_text();baseline=canonical(source);bound=install(source)
            with self.subTest(source=name):
                self.assertEqual(install(bound),bound)
                self.assertEqual(canonical(bound),baseline)
                self.assertEqual(install(baseline),bound)
                for changed in(bound+'\n',bound[len(producer.INCLUDE):],producer.INCLUDE+bound):
                    for api in(install,canonical):
                        with self.assertRaises(ValueError):api(changed)

    def test_borrowed_helpers_survive_disposable_donor(self):
        # Execute the real configure-time predicate pin both before and after
        # the disposable donor disappears. Original reference bytes stay put.
        module=(ROOT/'cmake/ModelSkinGpuAdmission.cmake').read_text()
        predicate=module[:module.index('    foreach(admission_target')]+"endif()\n"
        original=(ROOT/'tests/helpers').resolve(strict=True)
        reference=original/'model_skin_gpu_admission_reference_20261023.hpp'
        pin=public_proof.sha(reference)
        for mode in('copied-equal','copied-distinct','copied-nested-foreign-link'):
            with self.subTest(mode=mode),tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'))as folder:
                scratch=Path(folder);owner=scratch/'current-owner';owner.mkdir()
                foreign=scratch/'.local/test-scratch/foreign-owner';donor=foreign/'source'
                stable=owner/'stable-root';shutil.copytree(original,stable/'tests/helpers')
                shutil.copytree(original,donor/'tests/helpers')
                if mode=='copied-distinct':
                    (donor/'tests/helpers/extra-reference.txt').write_text('exact donor-only reference\n')
                    (donor/'tests/helpers/empty-reference-directory').mkdir()
                if mode=='copied-nested-foreign-link':
                    shared=foreign/'nested';shared.mkdir();(shared/'payload.txt').write_text('nested exact reference\n')
                    for location in(stable,donor):(location/'tests/helpers/nested').symlink_to(shared,target_is_directory=True)
                sources=[]
                for label in('fixed','legacy'):
                    source=owner/label;(source/'tests').mkdir(parents=True)
                    (source/'tests/helpers').symlink_to(donor/'tests/helpers',target_is_directory=True)
                    script=predicate.replace('${CMAKE_CURRENT_SOURCE_DIR}','${ARENA_REFERENCE_SOURCE}')
                    (source/'verify.cmake').write_text(script)
                    (source/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(ArenaReferenceLifetime NONE)\nset(BUILD_TESTING ON)\nset(CMAKE_SYSTEM_PROCESSOR x86_64)\n'+predicate+
                        'add_custom_target(verify_reference ALL COMMAND "${CMAKE_COMMAND}" "-DARENA_REFERENCE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}" -DBUILD_TESTING=ON -DCMAKE_SYSTEM_PROCESSOR=x86_64 -P "${CMAKE_CURRENT_SOURCE_DIR}/verify.cmake" VERBATIM)\n')
                    sources.append(source)
                fixed,legacy=sources
                with mock.patch.dict(os.environ,{'HALO2_TEST_SCRATCH':str(owner)}),mock.patch.object(public_proof,'ROOT',stable):
                    rows=canonicalize_borrowed_links(fixed)
                if mode=='copied-equal':
                    self.assertTrue(rows[0]['matched_stable_root_exactly'])
                    self.assertEqual(rows[0]['original_owner'],str(stable/'tests/helpers'))
                else:
                    self.assertTrue(rows[0]['materialized_owned_copy'])
                    self.assertFalse((fixed/'tests/helpers').is_symlink())
                def configure(source):
                    return subprocess.run(['cmake','-S',str(source),'-B',str(owner/(source.name+'-build'))],capture_output=True,text=True)
                def build(source):
                    return subprocess.run(['cmake','--build',str(owner/(source.name+'-build'))],capture_output=True,text=True)
                self.assertEqual(configure(legacy).returncode,0)
                self.assertEqual(build(legacy).returncode,0)
                shutil.rmtree(foreign)
                after=configure(fixed);self.assertEqual(after.returncode,0,after.stdout+after.stderr)
                built=build(fixed);self.assertEqual(built.returncode,0,built.stdout+built.stderr)
                if mode=='copied-distinct':
                    self.assertTrue((fixed/'tests/helpers/empty-reference-directory').is_dir())
                if mode=='copied-nested-foreign-link':
                    self.assertEqual((fixed/'tests/helpers/nested/payload.txt').read_text(),'nested exact reference\n')
                failed=configure(legacy);self.assertNotEqual(failed.returncode,0)
                self.assertIn('model_skin_gpu_admission_reference_20261023.hpp',failed.stderr)
                failed_build=build(legacy);self.assertNotEqual(failed_build.returncode,0)
                self.assertIn('model_skin_gpu_admission_reference_20261023.hpp',failed_build.stdout+failed_build.stderr)
        self.assertEqual(public_proof.sha(reference),pin)

    def test_actual_generated_input_foreign_owner_refusal(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'))as folder:
            base=Path(folder);owner=base/'current-owner';build=owner/'build';build.mkdir(parents=True)
            foreign=base/'.local/test-scratch/foreign-owner';foreign.mkdir(parents=True)
            source=foreign/'copied-header.hpp';source.write_text('exact copied header\n')
            stable=owner/'stable-name.hpp';stable.symlink_to(source)
            cache=build/'CMakeCache.txt';dep=build/'actual.cpp.o.d'
            cache.write_text('CURRENT_SOURCE:PATH='+str(owner)+'\n')
            dep.write_text('actual.cpp.o: '+str(owner/'source.cpp')+'\n')
            self.assertEqual(public_proof.audit_generated_inputs(build,owner)['foreign_scratch_inputs'],[])
            for name,spelling in(('copied-directory',source),('indirect-stable-spelling',stable),('missing-foreign',foreign/'removed-header.hpp')):
                for target in(cache,dep):
                    with self.subTest(case=name,metadata=target.name):
                        old=target.read_text();target.write_text('OBJECT: '+str(spelling)+'\n')
                        with self.assertRaisesRegex(ValueError,'foreign disposable owner'):
                            public_proof.audit_generated_inputs(build,owner)
                        target.write_text(old)


if __name__=='__main__':unittest.main()
