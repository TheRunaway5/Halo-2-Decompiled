#!/usr/bin/env python3
"""Build-only native fragment compiler contracts, including actual GLSL validation."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from native_world_fragment_kernel import lower_fragment_program,build_fragment_source


class NativeFragmentKernelTests(unittest.TestCase):
    def test_complete_unsigned_program_required(self):
        for words in ([],[0]*59,[0]*61,[0]*59+[-1],[0]*59+[True],[0]*59+[2**32]):
            with self.assertRaises(ValueError):lower_fragment_program(words)

    def test_unproved_modes_and_dot_encodings_reject(self):
        words=[0]*60;words[54]=7
        with self.assertRaises(ValueError):lower_fragment_program(words)
        words[54]=3|(17<<5);words[55]=3
        with self.assertRaises(ValueError):lower_fragment_program(words)

    def test_generated_program_has_no_control_word_or_stage_dispatch(self):
        words=[0]*60;words[53]=2|0x11000;words[54]=3|(17<<5)|(9<<10)
        words[56]=0;words[34]=0x08090809;words[45]=0x00000c00
        source=build_fragment_source((ROOT/'shaders/world.frag').read_text(),lower_fragment_program(words))
        self.assertNotIn('word(',source)
        self.assertNotIn('switch(',source)
        self.assertNotIn('for(uint stage=',source)
        self.assertIn('if(material.flags.y',source)
        self.assertIn('else if(material.flags.x!=0u && lightmapped!=0u)',source)
        self.assertNotIn('material.flags.w',source)
        self.assertIn('product1=dot(stageCoordinates[1]',source)
        self.assertIn('samples[2]=fetchStage2D(2u,vec2(product1,product2))',source)
        self.assertIn('argb(material.program[2][2])',source)

    def test_generated_supported_texture_and_combiner_forms_compile(self):
        compiler=shutil.which('glslangValidator');validator=shutil.which('spirv-val')
        if not compiler or not validator:self.skipTest('GLSL/SPIR-V build tools unavailable')
        scratch=Path(os.environ['HALO2_TEST_SCRATCH'])/'fragment-kernel-contracts';scratch.mkdir()
        # Dot-pad/reflection layouts, signed bump, both dependent selectors,
        # projected3D/cube, coordinate kill and passthrough are distinct paths.
        modes=[(0,0,0,0),(1,2,3,4),(3,6,15,16),(3,17,9,0),
               (3,17,17,14),(3,17,11,12),(3,17,17,12),(5,5,5,5)]
        for index,stages in enumerate(modes):
            words=[0]*60;words[54]=sum(mode<<(5*i) for i,mode in enumerate(stages))
            words[53]=2|0x1100;words[42]=0x8421
            words[34]=0x08090809;words[45]=0x00000c00
            words[35]=0x2c4c6c8c;words[46]=0x0005cccd
            words[0]=0x18191819;words[26]=0x00000c00
            words[8]=0x040c0200;words[9]=0x00001c00
            source=build_fragment_source((ROOT/'shaders/world.frag').read_text(),lower_fragment_program(words))
            path=scratch/f'{index}.frag';binary=scratch/f'{index}.spv';path.write_text(source)
            result=subprocess.run([compiler,'-V','--target-env','vulkan1.3','-I'+str(ROOT/'shaders'),str(path),'-o',str(binary)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            result=subprocess.run([validator,'--target-env','vulkan1.3',str(binary)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)


if __name__=='__main__':unittest.main()
