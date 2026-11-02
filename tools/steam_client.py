"""Discover installed Steam content and hand explicit download requests to Steam.

No Steam account handling or manifest edits: Steam owns installation, licensing,
download queues and DLC selection. Detection is advisory; onboarding still
verifies every imported source through its existing readers.
"""
from dataclasses import dataclass
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

MCC = 976730
HALO2 = 1064270
HALO2_MULTIPLAYER = 1097223
H2EK = 1613450
INSTALL_IDS = (MCC, HALO2, HALO2_MULTIPLAYER, H2EK)


def host_path(value, environment):
    """A Proton process reads the host's Steam library through Wine's Z drive."""
    value = str(value)
    if os.name == 'nt' and environment.get('STEAM_COMPAT_CLIENT_INSTALL_PATH') and value.startswith('/'):
        value = 'Z:' + value
    return Path(value).expanduser()


def _windows_libraries():
    import winreg
    for hive, key, name in ((winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam', 'SteamPath'),
                            (winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\WOW6432Node\Valve\Steam', 'InstallPath')):
        try:
            with winreg.OpenKey(hive, key) as registry:
                yield Path(winreg.QueryValueEx(registry, name)[0])
        except OSError:
            pass


def libraries(*, home=None, environment=None, extra_libraries=()):
    home = Path.home() if home is None else Path(home)
    env = os.environ if environment is None else environment
    candidates = [home/'.local/share/Steam', home/'.steam/steam', home/'.steam/root',
                  home/'.var/app/com.valvesoftware.Steam/.local/share/Steam',
                  home/'Library/Application Support/Steam', *map(Path, extra_libraries)]
    if env.get('STEAM_COMPAT_CLIENT_INSTALL_PATH'):
        candidates.insert(0, host_path(env['STEAM_COMPAT_CLIENT_INSTALL_PATH'], env))
    for key in ('PROGRAMFILES(X86)', 'ProgramFiles(x86)', 'PROGRAMFILES', 'ProgramFiles', 'LOCALAPPDATA'):
        if env.get(key):
            candidates.append(Path(env[key])/'Steam')
    if os.name == 'nt':
        candidates.extend(_windows_libraries())
    visited = set()
    for candidate in candidates:
        if len(visited) >= 128:
            break
        try:
            library = host_path(candidate, env).resolve()
            if library in visited:
                continue
            visited.add(library)
            for config in (library/'steamapps/libraryfolders.vdf', library/'config/libraryfolders.vdf'):
                if config.is_file() and config.stat().st_size <= 1024**2:
                    text = config.read_text(encoding='utf-8', errors='replace')
                    paths = re.findall(r'"(?:path|[0-9]+)"\s+"([^"\r\n]+)"', text)
                    for name in paths:
                        path = host_path(name.replace('\\\\', '\\'), env)
                        if path.is_absolute():
                            candidates.append(path)
            yield library
        except OSError:
            continue


def app_directories(library, app_id, default):
    names = [default]
    manifest = library/f'steamapps/appmanifest_{app_id}.acf'
    if manifest.is_file() and manifest.stat().st_size <= 1024**2:
        match = re.search(r'"installdir"\s+"([^"\r\n]+)"', manifest.read_text(encoding='utf-8', errors='replace'))
        if match:
            name = match[1]
            if name not in ('.', '..') and not any(c in name for c in '/\\:'):
                names.insert(0, name)
    return [library/'steamapps/common'/name for name in dict.fromkeys(names)]


def _present(path):
    return path.is_file() and path.stat().st_size > 0


def _installing(library, app_id):
    manifest = library/f'steamapps/appmanifest_{app_id}.acf'
    if not manifest.is_file() or manifest.stat().st_size > 1024**2:
        return False
    match = re.search(r'"StateFlags"\s+"([0-9]+)"', manifest.read_text(encoding='utf-8', errors='replace'))
    return bool(match and int(match[1]) != 4)


@dataclass(frozen=True)
class InstalledContent:
    mcc: bool
    halo2: Path | None
    campaign: bool
    multiplayer: bool
    h2ek: Path | None
    h2ek_ready: bool
    pending: bool = False

    @property
    def download_ids(self):
        return tuple(app for app, installed in ((MCC, self.mcc), (HALO2, self.campaign),
                     (HALO2_MULTIPLAYER, self.multiplayer), (H2EK, self.h2ek is not None)) if not installed)

    @property
    def ready(self):
        return self.campaign and self.multiplayer and self.h2ek_ready and not self.pending

    def message(self):
        missing = []
        if not self.mcc:
            missing.append('Halo MCC')
        if not self.campaign:
            missing.append('Halo 2: Anniversary (Campaign DLC)')
        if not self.multiplayer:
            missing.append('MCC - H2: Multiplayer DLC')
        if self.h2ek is None:
            missing.append('Halo 2 Mod Tools (H2EK)')
        if missing:
            text = 'Not found: ' + '; '.join(missing) + '. Download the missing content from Steam.'
            if self.mcc and (not self.campaign or not self.multiplayer):
                text = 'Found MCC, but Halo 2 content is incomplete. ' + text
        else:
            text = 'Found MCC Halo 2 and Halo 2 Mod Tools (H2EK).'
        if self.h2ek is not None and not self.h2ek_ready:
            text += ' H2EK is downloaded; extract H2EK.7z with the kit’s Extract step, then scan again.'
        elif self.ready:
            text += ' Prepare your local cache.'
        if self.pending:
            text += ' Steam is still installing or updating content. Wait for its downloads to finish.'
        return text


def inspect_content(*, mcc_source=None, home=None, environment=None, extra_libraries=()):
    from audit_rendering import FormatError
    from mcc_content import directory, discover
    from h2ek_loading_ambience import discover as discover_kit, SOURCE_FILES, TEXTURE_SOURCE
    env = os.environ if environment is None else environment
    roots = list(libraries(home=home, environment=env, extra_libraries=extra_libraries))
    mcc = any(path.is_dir() and ((path/'MCC').is_dir() or (path/'halo2').is_dir())
              for library in roots for path in app_directories(library, MCC, 'Halo The Master Chief Collection'))
    try:
        source = directory(mcc_source) if mcc_source else discover(home=home, environment=env,
                                            extra_libraries=extra_libraries, include_store=False)
    except (OSError, ValueError, FormatError):
        source = None
    maps = source/'h2_maps_win64_dx11' if source else None
    campaign = bool(maps and all(_present(maps/name) for name in ('textures.dat', 'shared.map', 'mainmenu.map',
                          'single_player_shared.map', '00a_introduction.map')) and _present(source/'h2_fonts/font_table.txt'))
    multiplayer = bool(maps and all(_present(maps/name) for name in ('textures.dat', 'shared.map', 'mainmenu.map',
                            'lockout.map')) and _present(source/'h2_fonts/font_table.txt'))
    try:
        kit = discover_kit(home=home, environment=env, extra_libraries=extra_libraries)
    except (OSError, ValueError, FormatError):
        kit = None
    if kit is None:
        candidates = [Path(env['HALO2_H2EK_ROOT']).expanduser()] if env.get('HALO2_H2EK_ROOT') else []
        candidates += [path for library in roots for path in app_directories(library, H2EK, 'H2EK')]
        kit = next((path for path in candidates if _present(path/'H2EK.7z')), None)
    kit_ready = bool(kit and _present(kit/TEXTURE_SOURCE) and any(_present(kit/name) for name in SOURCE_FILES))
    pending = any((source is not None and source.parent in app_directories(library, MCC, 'Halo The Master Chief Collection')
                   and _installing(library, MCC)) or
                  (kit is not None and kit in app_directories(library, H2EK, 'H2EK') and _installing(library, H2EK))
                  for library in roots)
    return InstalledContent(mcc or source is not None, source, campaign, multiplayer, kit, kit_ready, pending)


def open_install(app_id):
    if type(app_id) is not int or app_id not in INSTALL_IDS:
        raise ValueError('Unknown Steam onboarding content')
    uri = f'steam://install/{app_id}'
    if os.name == 'nt':
        # Proton registers this protocol with its built-in Steam bridge too.
        os.startfile(uri)
        return
    if sys.platform == 'darwin':
        command = ['/usr/bin/open', uri]
    elif shutil.which('steam'):
        command = [shutil.which('steam'), uri]
    elif shutil.which('xdg-open'):
        command = [shutil.which('xdg-open'), uri]
    else:
        raise RuntimeError('Steam could not be opened. Install or start Steam, then try Download again.')
    # This is the user's Steam client, which owns its own lifetime. Do not tie
    # its process or download queue to the onboarding window's lifetime.
    subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                     stderr=subprocess.DEVNULL, start_new_session=True)


def request_downloads(content):
    sent = []
    for app_id in content.download_ids:
        try:
            open_install(app_id)
        except (OSError, RuntimeError) as error:
            raise RuntimeError('Could not open the Steam installer. Start Steam and retry Download. '
                               f'{len(sent)} installation request(s) were already sent.') from error
        sent.append(app_id)
    return tuple(sent)
