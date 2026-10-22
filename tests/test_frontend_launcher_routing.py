"""Exercise the compiled launcher with synthetic admitted argv-capture owners."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest

R=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(R/'tools'))
sys.path.insert(0,str(R/'tests'))
import frontend_runtime_selection as selection
import native_gameplay_test as admission
import test_frontend_capability as fixtures

LAUNCHER=None
SHADERS=('world.vert.spv','world.frag.spv','world_emblem.geom.spv','world_emblem.frag.spv',
    'fog.vert.spv','fog.frag.spv','water_copy.frag.spv','bsp_patchy_noise.frag.spv',
    'bsp_patchy_composite.frag.spv','patchy_noise.frag.spv','patchy_composite.frag.spv',
    'bloom.vert.spv','bloom.frag.spv','crt_oled.frag.spv','display.frag.spv',
    'overlay.vert.spv','overlay.frag.spv','ui.vert.spv','ui.frag.spv','hud.frag.spv')


class FrontendLauncherRoutingTests(unittest.TestCase):
    def setUp(self):
        if LAUNCHER is None:self.skipTest('Pass the compiled launcher with --launcher for routing validation')
        f=fixtures.FrontendCapabilityTests();f.setUp();self.addCleanup(f.doCleanups)
        self.f=f;self.root=f.root;self.cache=f.cache;self.maps=self.cache/'halo2/h2_maps_win64_dx11'
        self.launcher=self.root/'Halo2-Launcher';shutil.copyfile(LAUNCHER,self.launcher);self.launcher.chmod(0o700)
        tools=self.root/'tools';tools.mkdir()
        (tools/'setup_assets.py').write_text('# Fixture root sentinel; no setup runs.\n')
        for name in ('native_gameplay_test.py','frontend_runtime_selection.py','packaged_shader_defaults.py',
                     'native_renderer_shaders.py'):
            shutil.copyfile(R/'tools'/name,tools/name)
        for name in ('lockout.map','single_player_shared.map'):(self.maps/name).write_bytes(b'synthetic MCC metadata')
        for name in ('bitmap-resources.json','tag-conversion.json','loading/mcc-loading-assets.json'):
            path=self.cache/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text('{}')
        (self.cache/'provenance.json').write_text(json.dumps(dict(schema='H2_MCC_TU15_PREPARATION_V1',
            external_installation_requirement='Halo MCC only',runtime_active=False,full_tu15_parity=False)))
        (self.root/'assets/mcc-source.tsv').write_text('H2_MCC_ASSET_SOURCE_V1\ncache\t'+self.cache.relative_to(self.root).as_posix()+'\n')
        self.gameplay=self.root/'halo2_native';self.game_shaders=self.root/'shaders';self.game_shaders.mkdir()
        for name in (*SHADERS,*(f'fixture-{n}.spv' for n in range(77))):
            (self.game_shaders/name).write_bytes(name.encode())
        self.capture(self.gameplay,self.game_shaders,'gameplay')
        self.capture(f.runtime,f.shaders,'frontend')
        f.report['runtime']['sha256']=f.digest(f.runtime);f.publish()
        self.native=dict(schema=admission.SCHEMA,capability=admission.CAPABILITY,
            runtime=dict(path=str(self.gameplay),sha256=f.digest(self.gameplay)),
            shaders=dict(binding='compiled-default',directory=str(self.game_shaders),
                files={p.name:f.digest(p) for p in self.game_shaders.iterdir()}),
            map=dict(path=admission.MAP,sha256=f.digest(self.cache/admission.MAP)),
            validation=dict(passed=True,scope=admission.SCOPE,report_sha256='2'*64))
        self.gameplay.with_name(self.gameplay.name+'.native-test.json').write_text(json.dumps(self.native))
        self.record=dict(schema=selection.SCHEMA,runtime=f.runtime.relative_to(self.root).as_posix(),
            runtime_sha256=f.digest(f.runtime),capability_sha256=f.digest(f.sidecar))
        self.manifest=self.root/selection.SELECTION;self.manifest.write_text(json.dumps(self.record))
        self.env=os.environ.copy();self.env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',PYTHONDONTWRITEBYTECODE='1')

    @staticmethod
    def capture(path,shaders,owner):
        path.write_text('#!'+sys.executable+'\n# '+str(shaders)+'\nimport json,sys\nprint("ROUTED "+json.dumps(dict(owner='+repr(owner)+',argv=sys.argv)))\n')
        path.chmod(0o700)

    def run_launcher(self,*args):
        result=subprocess.run([str(self.launcher),*args],cwd=self.root,env=self.env,
            capture_output=True,text=True,timeout=30)
        self.assertEqual(result.returncode,0,(args,result.stdout,result.stderr))
        rows=[json.loads(line[7:]) for line in result.stdout.splitlines() if line.startswith('ROUTED ')]
        self.assertEqual(len(rows),1,(args,result.stdout,result.stderr))
        return rows[0]

    def test_ordinary_play_uses_frontend_owner_and_authored_menu_paths(self):
        before={p:self.f.digest(p) for p in (self.gameplay,self.f.runtime,self.manifest)}
        row=self.run_launcher('--launch-game','--hidden','--mute','--frames','6')
        self.assertEqual(row['owner'],'frontend');argv=row['argv']
        self.assertEqual(argv[0],str(self.f.runtime))
        for option,value in (('--menu-screen','9'),('--menu-maps',str(self.maps))):
            self.assertEqual(argv[argv.index(option)+1],value)
        self.assertNotIn('--multiplayer',argv);self.assertNotIn('--mp-source-object-world',argv)
        self.assertEqual(before,{p:self.f.digest(p) for p in before})

    def test_native_test_and_explicit_gameplay_keep_original_gameplay_owner(self):
        for option in ('--native-test','--multiplayer'):
            with self.subTest(option=option):
                row=self.run_launcher(option,'--hidden','--frames','6')
                self.assertEqual(row['owner'],'gameplay');argv=row['argv']
                self.assertEqual(argv[0],str(self.gameplay));self.assertIn('--multiplayer',argv)
                self.assertNotIn('--menu-screen',argv)
                self.assertEqual(argv[argv.index('--map')+1],str(self.maps/'lockout.map'))
                self.assertEqual(argv[argv.index('--mp-source-object-world')+1],str(self.root/'saves/native-mcc-test'))

    def test_info_names_both_actual_owners_without_dispatching(self):
        result=subprocess.run([str(self.launcher),'--launcher-info'],cwd=self.root,env=self.env,
            capture_output=True,text=True,timeout=30)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertIn('Runtime: '+str(self.gameplay)+'\n',result.stdout)
        self.assertIn('Frontend runtime: '+str(self.f.runtime)+'\n',result.stdout)
        self.assertIn('Native MCC frontend: available',result.stdout)
        self.assertIn('Play: MCC title screen and menus',result.stdout);self.assertNotIn('ROUTED ',result.stdout)

    def test_stale_explicit_frontend_blocks_play_but_preserves_native_test(self):
        self.manifest.write_text(json.dumps(dict(self.record,runtime_sha256='0'*64)))
        info=subprocess.run([str(self.launcher),'--launcher-info'],cwd=self.root,env=self.env,
            capture_output=True,text=True,timeout=30)
        self.assertEqual(info.returncode,0,info.stderr)
        self.assertIn('Play: unavailable (selected independent MCC frontend is unavailable or changed)',info.stdout)
        self.assertIn('Native MCC gameplay test: available',info.stdout)
        self.assertIn('Native MCC frontend: unavailable',info.stdout)
        self.assertNotIn('Play: native Lockout gameplay test',info.stdout)
        self.assertNotIn('ROUTED ',info.stdout)
        result=subprocess.run([str(self.launcher),'--launch-game'],cwd=self.root,env=self.env,
            capture_output=True,text=True,timeout=30)
        self.assertNotEqual(result.returncode,0)
        self.assertIn('selected independent MCC frontend is unavailable or changed',result.stderr)
        self.assertNotIn('ROUTED ',result.stdout)
        self.assertEqual(self.run_launcher('--native-test')['owner'],'gameplay')

    def test_no_selection_retains_existing_single_runtime_routes(self):
        self.manifest.unlink()
        self.assertEqual(self.run_launcher('--launch-game')['owner'],'gameplay')
        report=dict(self.f.report,runtime=self.native['runtime'],shaders=self.native['shaders'])
        self.gameplay.with_name(self.gameplay.name+'.frontend-test.json').write_text(json.dumps(report))
        row=self.run_launcher('--launch-game')
        self.assertEqual(row['owner'],'gameplay');self.assertIn('--menu-screen',row['argv'])
        self.assertNotIn('--multiplayer',row['argv'])


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--launcher',type=Path,required=True)
    args,remaining=parser.parse_known_args()
    LAUNCHER=args.launcher.resolve(strict=True)
    unittest.main(argv=[sys.argv[0],*remaining])
