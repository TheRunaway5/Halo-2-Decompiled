"""Explicit MCC-only inputs for the native ONLINE HTTP fixtures.

This reads a private import with independent converted-layout evidence for
this qualification route. The runtime also supports an exact-profile fallback
without that sidecar; this runner does not duplicate or qualify that path.
It never imports, repairs, selects a global cache, emits a capability, or
substitutes legacy UI sources. Native Cache/FrontendPainter still perform their
actual metadata, raster and layout admission.
"""
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path, PurePosixPath
import re


def require(condition, message):
    if not condition:
        raise ValueError(message)


def owned(cache, relative):
    require(isinstance(relative, str) and relative and "\\" not in relative and
            not any(c in relative for c in "\t\r\n\0"), "Invalid MCC UI resource name")
    parts = relative.split("/")
    require(not PurePosixPath(relative).is_absolute() and
            all(part not in ("", ".", "..") for part in parts), "MCC UI resource escapes its owner")
    path = cache.joinpath(*parts)
    require(all(not parent.is_symlink() for parent in (path, *path.parents) if parent != cache and cache in parent.parents),
            "MCC UI resource has a symbolic-link owner: " + relative)
    require(path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(cache),
            "Incomplete MCC UI cache: " + relative)
    return path


def metadata(path, limit=4 * 1024 * 1024):
    require(0 < path.stat().st_size <= limit, "MCC UI metadata exceeds its bounded owner: " + path.name)
    def unique(pairs):
        result = {}
        for key, value in pairs:
            require(key not in result, "Duplicate MCC UI metadata key: " + key)
            result[key] = value
        return result
    value = json.loads(path.read_text(), object_pairs_hook=unique)
    require(isinstance(value, dict), "MCC UI metadata is not an object: " + path.name)
    return value


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def checked_file(cache, name, row, seen):
    require(isinstance(row, dict) and type(row.get("bytes")) is int and row["bytes"] > 0 and
            isinstance(row.get("sha256"), str) and re.fullmatch(r"[0-9a-f]{64}", row["sha256"]),
            "Invalid MCC UI resource integrity record: " + str(name))
    identity = (row["bytes"], row["sha256"])
    if name in seen:
        require(seen[name] == identity, "Conflicting MCC UI resource identity: " + name)
        return
    path = owned(cache, name)
    require(path.stat().st_size == row["bytes"] and sha(path) == row["sha256"],
            "MCC UI resource differs from its import: " + name)
    seen[name] = identity


def native_prompt_fonts(root, cache, provenance, fonts, seen):
    """Authenticate native MCC control-font owners, without authored overlays."""
    profile_path = Path(root) / "tools/data/gameplay-font-reference.json"
    require(profile_path.is_file() and not profile_path.is_symlink() and
            sha(profile_path) == "11acb229ef47207f7b4584cc225cacc1a582026d4706802494a3aa73ac7184a1",
            "Current authenticated MCC font profile is unavailable or changed")
    profile = metadata(profile_path)
    require(profile.get("schema") == "H2_MCC_GAMEPLAY_FONT_LAYOUT_V1" and
            isinstance(profile.get("mcc_names"), list) and len(profile["mcc_names"]) == 12 and
            isinstance(profile.get("mcc_files"), dict) and len(profile["mcc_files"]) == 8,
            "Incomplete native MCC font profile")
    table_name = "halo2/h2_fonts/font_table.txt"
    table = owned(cache, table_name)
    require(table.stat().st_size <= 4096 and table.read_text().splitlines() == profile["mcc_names"],
            "MCC native prompt selector table differs from its authenticated owner")
    sources = provenance.get("source_files")
    require(isinstance(sources, dict), "MCC prompt fonts lack their copied-source provenance")
    for name, row in profile["mcc_files"].items():
        require(isinstance(name, str) and PurePosixPath(name).name == name and "\\" not in name,
                "MCC prompt font profile has a foreign path")
        source_name, copied_name = "h2_fonts/" + name, "halo2/h2_fonts/" + name
        expected = {key: row.get(key) for key in ("bytes", "sha256")}
        require(source_name in sources and copied_name in fonts and
                isinstance(sources[source_name], dict) and isinstance(fonts[copied_name], dict) and
                all(sources[source_name].get(key) == fonts[copied_name].get(key) == expected[key]
                    for key in expected), "MCC prompt font differs from its native source profile: " + name)
        checked_file(cache, copied_name, expected, seen)
    # Legacy port_authored_prompts metadata and its files may remain in an old
    # cache. The native route neither reads nor selects that separate owner.


def validate_cache(root, selected):
    cache = Path(selected).expanduser()
    require(not cache.is_symlink(), "--mcc-root requires the direct completed cache, not a symbolic link")
    cache = cache.resolve(strict=True)
    require(cache.is_dir() and cache.parent.name == "imported" and cache.parent.parent.name == "assets" and
            not cache.name.startswith("."), "--mcc-root requires a completed private cache under assets/imported")
    provenance = metadata(owned(cache, "provenance.json"))
    require(provenance.get("schema") == "H2_MCC_TU15_PREPARATION_V1" and
            provenance.get("external_installation_requirement") == "Halo MCC only",
            "--mcc-root is not an MCC-only prepared cache")
    maps = "halo2/h2_maps_win64_dx11/"
    for name in ("mainmenu.map", "shared.map", "single_player_shared.map", "lockout.map"):
        with owned(cache, maps + name).open("rb") as stream:
            require(stream.read(8) == b"daeh\x0d\0\0\0", "MCC UI input is not a version-13 cache: " + name)
    for name in (maps + "textures.dat", maps + "sounds_neutral.dat", "halo2/h2_fonts/font_table.txt",
                 "bitmap-resources.json", "tag-conversion.json", "loading/mcc-loading-assets.json"):
        require(owned(cache, name).stat().st_size > 0, "Empty MCC UI dependency: " + name)

    layout_name = "presentation/ui/native-layout.json"
    require((cache / layout_name).exists(),
            "This ONLINE qualification route requires independent native-layout.json sidecar evidence; "
            "the runtime's exact-profile fallback is supported but is not exercised by this runner")
    layout_path = owned(cache, layout_name)
    layout = metadata(layout_path, 2 * 1024 * 1024)
    entry = provenance.get("native_ui_layout")
    require(isinstance(entry, dict) and entry.get("complete") is True and entry.get("path") == layout_name and
            entry.get("schema") == "H2_MCC_NATIVE_UI_LAYOUT_V1" and layout.get("schema") == entry["schema"],
            "MCC UI cache has no completed native layout conversion")
    checked_file(cache, layout_name, entry, {})
    profile_path = Path(root) / "resources/frontend/mcc-native-layout.json"
    require(profile_path.is_file() and not profile_path.is_symlink(), "Current port MCC UI profile is unavailable")
    profile = metadata(profile_path, 2 * 1024 * 1024)
    profile_hash, menu_hash = sha(profile_path), sha(owned(cache, maps + "mainmenu.map"))
    require(profile.get("schema") == entry["schema"] and layout.get("layout") == profile.get("layout") and
            layout.get("source_profile_sha256") == entry.get("source_profile_sha256") == profile_hash and
            layout.get("source_mainmenu_sha256") == entry.get("source_mainmenu_sha256") == menu_hash,
            "MCC UI layout belongs to a different port profile or mainmenu owner")

    menu_name = "ui/mcc-menu-assets.json"
    menu = metadata(owned(cache, menu_name))
    menu_entry = provenance.get("menu_resources")
    require(isinstance(menu_entry, dict) and menu_entry.get("complete") is True and menu_entry.get("path") == menu_name and
            menu.get("schema") == "H2_MCC_MENU_RESOURCE_CACHE_V1" and menu.get("complete") is True and
            menu.get("external_vista_or_xbox_required") is False, "MCC UI resource assembly is incomplete or requires a legacy donor")
    for name, key in (("vista-menu-inventory.json", "menu_inventory_sha256"), ("bitmap-resources.json", "bitmap_inventory_sha256")):
        require(sha(owned(cache, name)) == menu.get(key), "MCC UI resource inventory belongs to a different import")
    pages, fonts = menu.get("pages"), menu.get("fonts")
    require(isinstance(pages, list) and pages and isinstance(fonts, dict) and fonts and
            menu.get("frame_owners") == menu_entry.get("frame_owners") == len(pages) and
            menu_entry.get("fonts") == len(fonts), "Incomplete MCC UI frame/font census")
    seen = {}
    for page in pages:
        require(isinstance(page, dict) and isinstance(page.get("path"), str) and page["path"].startswith("ui/bitmaps/"),
                "MCC UI page has a foreign resource owner")
        checked_file(cache, page["path"], {"bytes": page.get("bytes"), "sha256": page.get("dds_sha256")}, seen)
    require(menu.get("unique_dds_pages") == menu_entry.get("unique_dds_pages") == len(seen), "Incomplete MCC UI unique-page census")
    for name, row in fonts.items():
        require(name.startswith("halo2/h2_fonts/"), "MCC font has a foreign resource owner")
        checked_file(cache, name, row, seen)

    native_prompt_fonts(root, cache, provenance, fonts, seen)
    return cache


@dataclass(frozen=True)
class UiArguments:
    source: tuple[str, ...]
    forwarded: tuple[str, ...]
    mcc_cache: Path | None = None


def ui_arguments(root, arguments):
    forwarded, selected = [], None
    args = iter(arguments)
    for option in args:
        if option == "--mcc-root" or option.startswith("--mcc-root="):
            require(selected is None, "Supply --mcc-root only once")
            selected = next(args, None) if option == "--mcc-root" else option.split("=", 1)[1]
            require(selected and not selected.startswith("--"), "--mcc-root requires a completed private cache path")
        else:
            forwarded.append(option)
    root = Path(root)
    if selected is None:
        return UiArguments((str(root / "assets/disc/maps"), "--asset-sources", str(root / "assets/asset-sources.tsv")), tuple(forwarded))
    for option in forwarded:
        require(not any(option == key or option.startswith(key + "=") for key in
                        ("--asset-sources", "--ui-assets", "--menu-ui-assets")),
                "--mcc-root cannot be combined with a legacy or replacement UI selector")
    try:
        cache = validate_cache(root, selected)
    except (OSError, ValueError, KeyError, TypeError) as error:
        raise ValueError("Invalid --mcc-root: " + str(error)) from error
    return UiArguments((str(cache / "halo2/h2_maps_win64_dx11"),), tuple(forwarded), cache)
