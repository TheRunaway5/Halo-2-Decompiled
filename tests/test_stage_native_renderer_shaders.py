"""Production-package ownership tests using real compiled shader modules."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import stage_native_renderer_shaders as staging


class ProductionShaderStageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler=shutil.which('glslangValidator')
        if not compiler:raise unittest.SkipTest('Vulkan shader compiler unavailable')
        cls.root=Path(os.environ['HALO2_TEST_SCRATCH'])/'production-shader-stage-tests';cls.root.mkdir()
        cls.code={}
        for name,source in {
            'good':'#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1);}',
            'other':'#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(.25);}',
            'interpreter':'#version 450\nlayout(set=0,binding=0,std140)uniform Hidden{uvec4 x[136];}b;layout(location=0)out vec4 color;void main(){color=vec4(float(b.x[0].y));}',
        }.items():
            path=cls.root/(name+'.frag');path.write_text(source)
            output=path.with_suffix('.spv')
            result=subprocess.run([compiler,'-V',str(path),'-o',str(output)],capture_output=True,text=True)
            if result.returncode:raise RuntimeError(result.stdout+result.stderr)
            cls.code[name]=output.read_bytes()
        cls.serial=0

    def owner(self):
        type(self).serial+=1
        parent=self.root/str(self.serial);build=parent/'build';source=parent/'source'
        (build/'shaders').mkdir(parents=True);(source/'cmake').mkdir(parents=True);(source/'shaders').mkdir()
        (source/'shaders/good.frag').write_text('#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1);}')
        (source/'cmake/NativeRendererProductionShaders.cmake').write_text('set(native_renderer_fixed_shaders\n good.frag.spv\n)\n')
        (build/'CMakeCache.txt').write_text('CMAKE_HOME_DIRECTORY:INTERNAL='+str(source)+'\n')
        raw=self.code['good'];pin=hashlib.sha256(raw).hexdigest()
        names=['good.frag.spv','native_world/f-'+pin+'.frag.spv','native-fragments/hud/'+pin+'.frag.spv']
        for name in names:
            path=build/'shaders'/name;path.parent.mkdir(exist_ok=True,parents=True);path.write_bytes(raw)
        (build/'native-world-kernel-catalog').mkdir();(build/'native-fragment-catalog').mkdir()
        (build/'native-world-kernel-catalog/compiled.json').write_text(json.dumps({
            'schema':'HALO2_NATIVE_WORLD_KERNEL_COMPILED_V1','runtime_instruction_interpreter':False,
            'stages':[{'path':names[1],'bytes':len(raw),'sha256':pin}]}))
        (build/'native-fragment-catalog/compiled.json').write_text(json.dumps({
            'schema':'HALO2_COMPILED_NATIVE_FRAGMENT_KERNELS_V1','passed':True,
            'modules':[{'path':names[2],'bytes':len(raw),'sha256':pin}]}))
        (build/'production-shaders.txt').write_text('\n'.join(sorted(names))+'\n')
        return build,source,parent/'staged',names

    def test_only_authored_fixed_and_complete_generated_inventory_is_staged(self):
        build,source,output,names=self.owner()
        (build/'shaders/texture_program.comp.spv').write_bytes(self.code['good'])
        result=staging.stage_production_shaders(build,output)
        self.assertTrue(result['passed']);self.assertEqual(result['module_count'],len(names))
        self.assertEqual({p.relative_to(output).as_posix() for p in output.rglob('*.spv')},set(names))
        self.assertFalse((output/'texture_program.comp.spv').exists())

    def test_generated_inventory_cannot_promote_an_undeclared_probe(self):
        build,source,output,names=self.owner()
        probe=build/'shaders/texture_program.comp.spv';probe.write_bytes(self.code['good'])
        with (build/'production-shaders.txt').open('a')as file:file.write('texture_program.comp.spv\n')
        with self.assertRaises((ValueError,FileNotFoundError)):
            staging.stage_production_shaders(build,output)
        self.assertFalse(output.exists())

    def test_fixed_policy_cannot_claim_a_test_origin_as_renderer_source(self):
        build,source,output,names=self.owner();(source/'tests').mkdir()
        (source/'tests/texture_program.comp').write_text('test-only shader')
        (source/'cmake/NativeRendererProductionShaders.cmake').write_text('set(native_renderer_fixed_shaders good.frag.spv texture_program.comp.spv)\n')
        (build/'shaders/texture_program.comp.spv').write_bytes(self.code['good'])
        with (build/'production-shaders.txt').open('a')as file:file.write('texture_program.comp.spv\n')
        with self.assertRaises((ValueError,FileNotFoundError)):
            staging.stage_production_shaders(build,output)
        self.assertFalse(output.exists())

    def test_declared_module_cannot_disappear_from_generated_inventory(self):
        build,source,output,names=self.owner()
        (build/'production-shaders.txt').write_text('\n'.join(names[1:])+'\n')
        with self.assertRaises(ValueError):staging.stage_production_shaders(build,output)
        self.assertFalse(output.exists())

    def test_compiled_manifest_hash_is_not_replaced_by_hashing_changed_input(self):
        build,source,output,names=self.owner()
        (build/'shaders'/names[1]).write_bytes(self.code['other'])
        with self.assertRaises(ValueError):staging.stage_production_shaders(build,output)
        self.assertFalse(output.exists())

    def test_duplicate_noncanonical_escape_and_symlink_owners_are_rejected(self):
        for suffix in ('good.frag.spv','./good.frag.spv','../escape.spv','/absolute.frag.spv'):
            build,source,output,names=self.owner()
            with (build/'production-shaders.txt').open('a')as file:file.write(suffix+'\n')
            with self.subTest(suffix=suffix),self.assertRaises((ValueError,FileNotFoundError)):
                staging.stage_production_shaders(build,output)
            self.assertFalse(output.exists())
        build,source,output,names=self.owner()
        path=build/'shaders'/names[0];path.unlink();path.symlink_to(self.root/'good.spv')
        with self.assertRaises(ValueError):staging.stage_production_shaders(build,output)

    def test_missing_or_interpreting_declared_shader_fails_before_staging(self):
        for kind in ('missing','interpreter'):
            build,source,output,names=self.owner();path=build/'shaders'/names[0]
            if kind=='missing':path.unlink()
            else:path.write_bytes(self.code[kind])
            with self.subTest(kind=kind),self.assertRaises((ValueError,FileNotFoundError)):
                staging.stage_production_shaders(build,output)
            self.assertFalse(output.exists())

    def test_shader_change_during_copy_removes_incomplete_output_owner(self):
        build,source,output,names=self.owner();real_copy=shutil.copy2
        def mutate(source,target,*args,**kwargs):
            Path(source).write_bytes(self.code['other']);return real_copy(source,target,*args,**kwargs)
        with patch.object(staging.shutil,'copy2',side_effect=mutate),self.assertRaises(ValueError):
            staging.stage_production_shaders(build,output)
        self.assertFalse(output.exists())

    def test_inventory_change_during_copy_is_not_republished_as_matching_receipt(self):
        build,source,output,names=self.owner();real_copy=shutil.copy2
        def mutate(source,target,*args,**kwargs):
            result=real_copy(source,target,*args,**kwargs)
            (build/'production-shaders.txt').write_text('forged.frag.spv\n')
            return result
        with patch.object(staging.shutil,'copy2',side_effect=mutate),self.assertRaises(ValueError):
            staging.stage_production_shaders(build,output)
        self.assertFalse(output.exists())


if __name__=='__main__':unittest.main()
