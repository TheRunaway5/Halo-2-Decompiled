import hashlib
import json
import os
from pathlib import Path
import shutil
import shlex
import struct
import subprocess
import sys
import time
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from compile_native_fragment_kernels import controls,key,source_for,entries,write_header
from audit_native_renderer_kernels import audit_spirv

class NativeFragmentFamilies(unittest.TestCase):
    def test_identical_revalidation_completes_all_outputs_then_make_is_idle(self):
        make=shutil.which('make')
        if not make:self.skipTest('This generated-Makefile regression requires make')
        toolchain=[shutil.which(name) for name in ('glslangValidator','spirv-opt','spirv-val')]
        self.assertTrue(all(toolchain),'Native kernel toolchain is required')
        work=Path(os.environ['HALO2_TEST_SCRATCH'])/'fragment-revalidation';work.mkdir()
        original=json.loads((ROOT/'shaders/native-programs/fragments/effects.json').read_bytes())
        rows=[next(row for row in original['entries'] if row['family']==family) for family in ('hud','world-fixed')]
        registry=work/'registry.json';registry.write_text(json.dumps({'schema':original['schema'],'entries':rows}))
        header=work/'catalog/native_fragment_kernels.generated.hpp';write_header(header,entries([registry]))
        manifest=work/'compiled.json';dependency=work/'guard-input';dependency.write_text('unchanged program input\n')
        command=[sys.executable,'-B',str(ROOT/'tools/compile_native_fragment_kernels.py'),
            '--registry',str(registry),'--shader-root',str(ROOT/'shaders'),
            '--shader-output-dir',str(work/'shaders'),'--header',str(header),'--manifest',str(manifest)]
        for option,tool in zip(('--compiler','--optimizer','--validator'),toolchain):command += [option,tool]
        counter=work/'invocations';driver=work/'compile.py'
        driver.write_text('import subprocess\nfrom pathlib import Path\np=Path('+repr(str(counter))+')\n'
            'p.write_text(str(int(p.read_text())+1) if p.exists() else "1")\n'
            'subprocess.run('+repr(command)+',check=True)\n')
        outputs=[work/'shaders'/row['path'] for row in entries([registry])]
        escape=lambda path:str(path).replace('$','$$').replace('#','\\#').replace(' ','\\ ')
        makefile=work/'Makefile';makefile.write_text('.PHONY: all\nall: '+
            ' '.join(escape(path) for path in [manifest,*outputs])+'\n'+escape(manifest)+': '+
            ' '.join(escape(path) for path in [header,dependency])+'\n\t'+
            shlex.quote(sys.executable)+' '+shlex.quote(str(driver))+'\n'+
            ' '.join(escape(path) for path in outputs)+': '+escape(manifest)+'\n\t@test -f "$@"\n')
        def build(*args):
            result=subprocess.run([make,'--no-print-directory','-f',str(makefile),*args,'all'],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        build()
        files=[header,manifest,*outputs];before={path:hashlib.sha256(path.read_bytes()).hexdigest() for path in files}
        header_state=(header.stat().st_ino,header.stat().st_mtime_ns)
        newer=max(time.time_ns(),manifest.stat().st_mtime_ns+1)
        os.utime(dependency,ns=(newer,newer));build()
        self.assertEqual(counter.read_text(),'2')
        self.assertEqual(before,{path:hashlib.sha256(path.read_bytes()).hexdigest() for path in files})
        self.assertEqual(header_state,(header.stat().st_ino,header.stat().st_mtime_ns))
        for path in [manifest,*outputs]:self.assertGreater(path.stat().st_mtime_ns,newer,str(path))
        completed={path:(path.stat().st_ino,path.stat().st_mtime_ns) for path in files}
        build('--question');build()
        self.assertEqual(counter.read_text(),'2','Unchanged native programs compiled again')
        self.assertEqual(completed,{path:(path.stat().st_ino,path.stat().st_mtime_ns) for path in files})

    def test_complete_control_identity_only_excludes_colors(self):
        program=list(range(60));baseline=controls(program)
        for i in range(60):
            changed=program.copy();changed[i]^=0x80000000
            self.assertEqual(key(controls(changed))==key(baseline),10<=i<26 or i in (43,44))
        for bad in ([],[0]*59,[0]*61,[False]*60,[-1]*60):
            with self.assertRaises(ValueError):controls(bad)

    def test_static_lightmap_branch_is_preserved(self):
        words=json.loads((ROOT/'shaders/native-programs/world/programs.json').read_text())['entries'][0]['controls']
        for family in ('world','world-fixed','world-emblem'):
            source=source_for(ROOT/'shaders',{'family':family,'controls':words})
            self.assertIn('else if(material.flags.x!=0u && lightmapped!=0u)',source)
            self.assertNotIn('material.flags.w',source)
            self.assertNotIn('word(',source)

    def test_all_fog_kernels_preserve_source_stage_arithmetic(self):
        compiler=shutil.which('glslangValidator');validator=shutil.which('spirv-val')
        self.assertTrue(compiler and validator,'Native kernel toolchain is required')
        work=Path(os.environ['HALO2_TEST_SCRATCH'])/'fog-operation-boundaries';work.mkdir()
        registry=json.loads((ROOT/'shaders/native-programs/fragments/atmosphere.json').read_bytes())
        rows=[row for row in registry['entries'] if row['family']=='fog']
        self.assertTrue(rows,'The complete source-owned fog family is required')
        world=json.loads((ROOT/'shaders/native-programs/world/programs.json').read_bytes())['entries'][0]
        self.assertNotIn('precise vec3',source_for(ROOT/'shaders',
            {'family':'world','controls':world['controls']}))
        for row in rows:
            source=source_for(ROOT/'shaders',row)
            self.assertIn('precise vec3',source)
            path=work/(key(controls(row['controls']))+'.frag');raw=path.with_suffix('.spv')
            path.write_text(source)
            for command in ([compiler,'-V','--target-env','vulkan1.3','-I'+str(ROOT/'shaders'),str(path),'-o',str(raw)],
                            [validator,'--target-env','vulkan1.3',str(raw)]):
                result=subprocess.run(command,capture_output=True,text=True)
                self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            words=struct.unpack('<'+str(raw.stat().st_size//4)+'I',raw.read_bytes())
            protected=set();arithmetic={};position=5
            while position<len(words):
                count,opcode=words[position]>>16,words[position]&65535
                self.assertGreater(count,0)
                if opcode==71 and words[position+2]==42:protected.add(words[position+1])
                if opcode in (129,131,133,136,142,148):arithmetic[words[position+2]]=opcode
                position+=count
            protected_operations={arithmetic[identity] for identity in protected if identity in arithmetic}
            self.assertIn(129,protected_operations,'Fog SUM lost its separate source addition')
            self.assertTrue(protected_operations&{133,142},'Fog AB/CD lost their separate source multiplication')

    def test_menu_and_pstencil_modules_have_no_instruction_bank_reads(self):
        compiler=shutil.which('glslangValidator');optimizer=shutil.which('spirv-opt');validator=shutil.which('spirv-val')
        if not all((compiler,optimizer,validator)):self.fail('Native kernel toolchain is required')
        work=Path(os.environ['HALO2_TEST_SCRATCH'])/'fragment-family-contract';work.mkdir()
        words=json.loads((ROOT/'shaders/native-programs/world/programs.json').read_text())['entries'][0]['controls']
        for family in ('menu','hud','pstencil-normal','pstencil-normal-utility','pstencil-texture','pstencil-light-utility','pstencil-dependent-light'):
            source=source_for(ROOT/'shaders',{'family':family,'controls':words})
            self.assertNotIn('originalPhase',source)
            self.assertNotIn('menuWorldPixelProgram.program',source)
            text=work/(family+'.frag');raw=work/(family+'.raw.spv');binary=work/(family+'.spv')
            text.write_text(source)
            for command in ([compiler,'-V','--target-env','vulkan1.3','-I'+str(ROOT/'shaders'),str(text),'-o',str(raw)],
                            [optimizer,'--preserve-interface','-O',str(raw),'-o',str(binary)],
                            [validator,'--target-env','vulkan1.3',str(binary)]):
                result=subprocess.run(command,capture_output=True,text=True)
                self.assertEqual(result.returncode,0,family+': '+result.stdout+result.stderr)
            contract=audit_spirv(binary.read_bytes())
            self.assertTrue(contract['passed'],(family,contract))

if __name__=='__main__':unittest.main()
