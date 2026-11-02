"""Installed-content discovery and real OS dispatch boundaries, without downloads."""
import os
from pathlib import Path, PosixPath
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import steam_client as client


class SteamContentTests(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(prefix='steam-content-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name).resolve()
        self.home = self.root/'home'
        self.steam = self.home/'.local/share/Steam'
        self.common = self.steam/'steamapps/common'
        self.mcc = self.common/'Halo The Master Chief Collection'
        self.kit = self.common/'H2EK'
        registry = patch.object(client, '_windows_libraries', return_value=[])
        registry.start()
        self.addCleanup(registry.stop)

    def files(self, directory, names):
        for name in names:
            path = directory/name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'content discovery fixture, not authenticated assets')

    def halo2(self, *, multiplayer=True):
        names = ['h2_maps_win64_dx11/'+n for n in ('textures.dat', 'shared.map', 'mainmenu.map',
                  'single_player_shared.map', '00a_introduction.map')]
        if multiplayer:
            names += ['h2_maps_win64_dx11/lockout.map']
        self.files(self.mcc/'halo2', names+['h2_fonts/font_table.txt'])

    def inspect(self):
        return client.inspect_content(home=self.home, environment={})

    def test_missing_everything_targets_parent_game_both_dlc_and_tool(self):
        content = self.inspect()
        self.assertEqual(content.download_ids, (976730, 1064270, 1097223, 1613450))
        self.assertIn('DLC', content.message())
        self.assertIn('H2EK', content.message())
        self.assertFalse(content.ready)

    def test_mcc_manifest_without_halo2_does_not_count_as_halo2_installed(self):
        self.files(self.mcc, ['MCC/Content/Paks/MCC-WindowsNoEditor.pak'])
        (self.steam/'steamapps/appmanifest_976730.acf').write_text('"installdir" "Halo The Master Chief Collection"')
        content = self.inspect()
        self.assertTrue(content.mcc)
        self.assertFalse(content.campaign)
        self.assertIsNone(content.halo2)
        self.assertEqual(content.download_ids, (1064270, 1097223, 1613450))

    def test_campaign_without_multiplayer_requests_only_missing_multiplayer_and_tool(self):
        self.halo2(multiplayer=False)
        content = self.inspect()
        self.assertTrue(content.campaign)
        self.assertFalse(content.multiplayer)
        self.assertEqual(content.download_ids, (1097223, 1613450))

    def test_archive_only_h2ek_is_installed_and_needs_extraction_not_another_download(self):
        self.halo2()
        self.files(self.kit, ['H2EK.7z'])
        content = self.inspect()
        self.assertEqual(content.h2ek, self.kit)
        self.assertFalse(content.h2ek_ready)
        self.assertFalse(content.ready)
        self.assertEqual(content.download_ids, ())
        self.assertIn('extract H2EK.7z', content.message())

    def test_complete_content_needs_no_install_requests(self):
        self.halo2()
        self.files(self.kit, ['tool.exe', 'loading_dx11.bin'])
        content = self.inspect()
        self.assertTrue(content.ready)
        with patch.object(client, 'open_install') as launch:
            self.assertEqual(client.request_downloads(content), ())
        launch.assert_not_called()

    def test_present_files_during_a_steam_update_are_not_reported_as_ready(self):
        self.halo2()
        self.files(self.kit, ['tool.exe', 'loading_dx11.bin'])
        manifest = self.steam/'steamapps/appmanifest_976730.acf'
        manifest.write_text('"StateFlags" "1026"')
        content = self.inspect()
        self.assertTrue(content.pending)
        self.assertFalse(content.ready)
        self.assertIn('downloads to finish', content.message())
        manifest.write_text('"StateFlags" "4"')
        self.assertTrue(self.inspect().ready)

    def test_zero_length_download_placeholders_do_not_count_as_content(self):
        self.halo2()
        (self.mcc/'halo2/h2_maps_win64_dx11/lockout.map').write_bytes(b'')
        self.assertIn(client.HALO2_MULTIPLAYER, self.inspect().download_ids)

    def test_external_library_and_nonstandard_installdir_are_shared_by_both_detectors(self):
        library = self.root/'external library'
        (self.steam/'steamapps').mkdir(parents=True)
        (self.steam/'steamapps/libraryfolders.vdf').write_text('"1" { "path" "'+str(library).replace('\\','\\\\')+'" }')
        self.mcc = library/'steamapps/common/Custom MCC'
        self.kit = library/'steamapps/common/Custom H2EK'
        self.halo2()
        self.files(self.kit, ['guerilla.exe', 'loading_dx11.bin'])
        (library/'steamapps/appmanifest_976730.acf').write_text('"installdir" "Custom MCC"')
        (library/'steamapps/appmanifest_1613450.acf').write_text('"installdir" "Custom H2EK"')
        content = self.inspect()
        self.assertTrue(content.ready)
        self.assertEqual(content.halo2, self.mcc/'halo2')
        self.assertEqual(content.h2ek, self.kit)

    def test_library_cycles_and_manifest_path_escape_are_bounded(self):
        (self.steam/'steamapps').mkdir(parents=True)
        value = str(self.steam).replace('\\', '\\\\')
        (self.steam/'steamapps/libraryfolders.vdf').write_text('"1" { "path" "'+value+'" }')
        (self.steam/'steamapps/appmanifest_1613450.acf').write_text('"installdir" "../outside"')
        libraries = list(client.libraries(home=self.home, environment={}))
        self.assertEqual(len(libraries), len(set(libraries)))
        self.assertEqual(client.app_directories(self.steam, client.H2EK, 'H2EK'), [self.kit])

    def test_request_failure_preserves_partial_handoff_and_is_actionable(self):
        with patch.object(client, 'open_install', side_effect=[None, OSError('no protocol handler')]) as launch:
            with self.assertRaisesRegex(RuntimeError, '1 installation request'):
                client.request_downloads(self.inspect())
        self.assertEqual([c.args[0] for c in launch.call_args_list], [976730, 1064270])


class SteamDispatchTests(unittest.TestCase):
    def test_linux_uses_installed_client_with_fixed_uri_and_no_shell(self):
        with patch.object(client.os, 'name', 'posix'), patch.object(client.sys, 'platform', 'linux'), \
             patch.object(client.shutil, 'which', return_value='/usr/bin/steam'), \
             patch.object(client.subprocess, 'Popen') as launch:
            client.open_install(1613450)
        self.assertEqual(launch.call_args.args[0], ['/usr/bin/steam', 'steam://install/1613450'])
        self.assertNotIn('shell', launch.call_args.kwargs)

    def test_linux_registered_protocol_handler_supports_flatpak_steam(self):
        def which(name):
            return '/usr/bin/xdg-open' if name == 'xdg-open' else None
        with patch.object(client.os, 'name', 'posix'), patch.object(client.sys, 'platform', 'linux'), \
             patch.object(client.shutil, 'which', side_effect=which), \
             patch.object(client.subprocess, 'Popen') as launch:
            client.open_install(1064270)
        self.assertEqual(launch.call_args.args[0], ['/usr/bin/xdg-open', 'steam://install/1064270'])

    def test_windows_and_proton_delegate_to_registered_steam_protocol(self):
        with patch.object(client.os, 'name', 'nt'), patch.object(client.os, 'startfile', create=True) as launch:
            client.open_install(1097223)
        launch.assert_called_once_with('steam://install/1097223')

    def test_arbitrary_ids_cannot_be_sent_to_steam(self):
        for app_id in ('976730', '../x', True, -1, 123):
            with self.assertRaises(ValueError):
                client.open_install(app_id)

    @unittest.skipIf(os.name == 'nt', 'POSIX fixture for the Proton path conversion boundary')
    def test_proton_host_library_paths_map_to_z_drive(self):
        with patch.object(client.os, 'name', 'nt'), patch.object(client, 'Path', PosixPath):
            path = client.host_path('/home/user/Steam', {'STEAM_COMPAT_CLIENT_INSTALL_PATH':'/home/user/Steam'})
        self.assertEqual(str(path), 'Z:/home/user/Steam')


if __name__ == '__main__':
    unittest.main()
