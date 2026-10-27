"""Explicit inventory family emitted by the native Vulkan/pStencil/upscaling build.

Legacy capability readers retain their 97-file family. This family replaced
unused compute programs with native pStencil/upscaling stages; count alone
cannot establish that either renderer's complete bank is present.
"""
import argparse
import hashlib
import json
import mmap
import os
from pathlib import Path
import re
import struct


def _identity(value):
    return (value.st_dev, value.st_ino, value.st_size,
            value.st_mtime_ns, value.st_ctime_ns)

PROFILE = "native-vulkan-pstencil-upscaling-v1"
AOT_PROFILE = "native-vulkan-aot-program-kernels-v2"
AOT_PACKAGE_SCHEMA = "HALO2_NATIVE_RENDERER_PRODUCTION_SHADER_PACKAGE_V2"
AOT_PACKAGE_NAME = "production-shaders.json"
# This fixed Vulkan ABI is independent of per-program catalog cardinality.
# The source-policy regression verifies that it matches the actual CMake owner.
AOT_BASE_FIXED_FILES = frozenset("""
bloom.vert.spv crt_oled.frag.spv display.frag.spv fog.vert.spv
light_volume.vert.spv menu_world.geom.spv model_skin.comp.spv
overlay.frag.spv overlay.vert.spv particle.geom.spv particle.vert.spv
particle_directional.vert.spv particle_distortion.vert.spv
particle_model.geom.spv particle_model.vert.spv particle_perpendicular.vert.spv
patchy_noise.vert.spv pstencil/lightmap.frag.spv pstencil/lightmap.vert.spv
pstencil/volume.frag.spv pstencil/volume.vert.spv
source_contrail.geom.spv source_contrail.vert.spv
source_decal.geom.spv source_decal.vert.spv
source_depth_color_writeback.comp.spv source_lens_draw.vert.spv
source_lens_query.frag.spv source_lens_query.vert.spv
source_shadow_filter.frag.spv source_shadow_filter.vert.spv
source_weather_depth_alias.comp.spv source_weather_line.frag.spv
source_weather_line.geom.spv source_weather_line.vert.spv
source_weather_plate.frag.spv source_weather_plate.vert.spv
source_weather_point.frag.spv source_weather_point.vert.spv
ui.frag.spv ui.vert.spv ui_frame.frag.spv upscale_motion.frag.spv
upscale_motion.vert.spv upscale_prepare.comp.spv
vista_loading.frag.spv vista_loading.vert.spv water_copy.frag.spv
world.vert.spv world_prt.vert.spv world_emblem.geom.spv
""".split())
PATH_TRACING_FILES = frozenset({"path_tracing/world.vert.spv", "path_tracing/world.frag.spv"})
PATH_TRACING_VISIBILITY_FILES = frozenset({"path_tracing/depth.frag.spv"})
PATH_TRACING_LIGHTING_FILES = frozenset({"path_tracing/lighting.frag.spv"})
AOT_FIXED_FILES = AOT_BASE_FIXED_FILES | PATH_TRACING_FILES | PATH_TRACING_VISIBILITY_FILES | PATH_TRACING_LIGHTING_FILES
# This mode-availability diagnostic is retained by every engine exposing the
# RT choice. Bind that executable feature to its complete fixed shader pair;
# older qualified v2 executables remain admitted with their original fixed ABI.
PATH_TRACING_EXECUTABLE_MARKER = b"Path Tracing shaders are unavailable in this runtime.\0"
PATH_TRACING_VISIBILITY_MARKER = b"Path Tracing visibility shader is unavailable in this runtime.\0"
PATH_TRACING_LIGHTING_MARKER = b"Path Tracing HDR lighting shader is unavailable in this runtime.\0"
PATH_TRACING_TEMPORAL_MARKER = b"Path Tracing temporal lighting shaders are unavailable in this runtime.\0"
PATH_TRACING_TEMPORAL_FRAGMENTS = frozenset({"path_tracing/world.frag.spv", "path_tracing/lighting.frag.spv"})
PATH_TRACING_SURFACE_MARKER = b"Path Tracing authored surface shaders are unavailable in this runtime.\0"
PATH_TRACING_SURFACE_FRAGMENTS = PATH_TRACING_TEMPORAL_FRAGMENTS | PATH_TRACING_VISIBILITY_FILES
_GENERATED_PATH = re.compile(
    r"(?:native_world/(?:v-[0-9a-f]{64}(?:-normal|-normal-utility)?\.vert|f-[0-9a-f]{64}\.frag)"
    r"|native-fragments/[a-z][a-z0-9-]*/[0-9a-f]{64}\.frag)\.spv")


def _direct_bytes(path, maximum):
    if path.is_symlink() or not path.is_file() or path.resolve(strict=True) != path:
        raise ValueError("Native shader package has no direct file owner")
    with path.open('rb') as stream:
        before = os.fstat(stream.fileno())
        if not 0 < before.st_size <= maximum:
            raise ValueError("Native shader owner exceeds its declared extent")
        raw = stream.read()
        if _identity(before) != _identity(os.fstat(stream.fileno())) or _identity(before) != _identity(path.stat()):
            raise ValueError("Native shader owner changed during validation")
    return raw


def _path_tracing_temporal_abi(ops):
    """Reflect the exact32-byte screen header, independent of symbol names."""
    types, variables, decorations, offsets = {}, [], {}, {}
    for opcode, args in ops:
        if opcode in (21, 23, 29, 30, 32) and args:
            types[args[0]] = (opcode, args[1:])
        elif opcode == 59 and len(args) >= 3:
            variables.append((args[0], args[1], args[2]))
        elif opcode == 71 and len(args) >= 2:
            decorations.setdefault((args[0], args[1]), []).append(tuple(args[2:]))
        elif opcode == 72 and len(args) == 4 and args[2] == 35:
            offsets.setdefault(args[0], []).append((args[1], args[3]))
    candidates = [(pointer, storage) for pointer, identity, storage in variables
                  if decorations.get((identity, 34)) == [(1,)] and
                  decorations.get((identity, 33)) == [(6,)]]
    if len(candidates) != 1:
        return None
    pointer, storage = candidates[0]
    opcode, args = types.get(pointer, (None, ()))
    if opcode != 32 or len(args) != 2 or args[0] != storage:
        return None
    block = args[1]
    # SPIR-V1.0 uses Uniform+BufferBlock for SSBOs;1.3 may use
    # StorageBuffer+Block. Ordinary uniform buffers are a different ABI.
    decoration = 3 if storage == 2 else 2 if storage == 12 else None
    if decoration is None or decorations.get((block, decoration)) != [()]:
        return None
    opcode, members = types.get(block, (None, ()))
    if opcode != 30 or len(members) != 3 or sorted(offsets.get(block, ())) != [(0, 0), (1, 16), (2, 32)]:
        return None
    for member in members[:2]:
        opcode, vector = types.get(member, (None, ()))
        if opcode != 23 or len(vector) != 2 or vector[1] != 4 or types.get(vector[0]) != (21, (32, 0)):
            return None
    opcode, element = types.get(members[2], (None, ()))
    if (opcode != 29 or len(element) != 1 or types.get(element[0]) != (21, (32, 0)) or
            decorations.get((members[2], 6)) != [(4,)]):
        return None
    return 2


def _path_tracing_surface_abi(ops):
    """Reflect the fixed authored four-stage interface and surface record ABI."""
    types, constants, variables, decorations, offsets = {}, {}, [], {}, {}
    for opcode, args in ops:
        if 19 <= opcode <= 39 and args:
            types[args[0]] = (opcode, args[1:])
        elif opcode == 43 and len(args) == 3:
            constants[args[1]] = (args[0], args[2])
        elif opcode == 59 and len(args) >= 3:
            variables.append((args[0], args[1], args[2]))
        elif opcode == 71 and len(args) >= 2:
            decorations.setdefault((args[0], args[1]), []).append(tuple(args[2:]))
        elif opcode == 72 and len(args) == 4 and args[2] == 35:
            offsets.setdefault(args[0], []).append((args[1], args[3]))

    def scalar_constant(identity):
        scalar, value = constants.get(identity, (None, None))
        return value if types.get(scalar) == (21, (32, 0)) else None

    def pointee(pointer, storage):
        opcode, args = types.get(pointer, (None, ()))
        return args[1] if opcode == 32 and len(args) == 2 and args[0] == storage else None

    def binding(number):
        return [(pointer, storage) for pointer, identity, storage in variables
                if decorations.get((identity, 34)) == [(0,)] and
                decorations.get((identity, 33)) == [(number,)]]

    for stage in range(4):
        candidates = binding(stage)
        if len(candidates) != 1:
            return None
        pointer, storage = candidates[0]
        opcode, image = types.get(pointee(pointer, 0), (None, ()))
        if storage != 0 or opcode != 27 or len(image) != 1:
            return None
        opcode, shape = types.get(image[0], (None, ()))
        if (opcode != 25 or len(shape) != 7 or types.get(shape[0]) != (22, (32,)) or
                shape[1:] != (1, 0, 0, 0, 1, 0)):
            return None  # Float sampled 2D, no array, multisampling or depth comparison.

    def array_of_vector(identity, scalar, length):
        opcode, array = types.get(identity, (None, ()))
        if (opcode != 28 or len(array) != 2 or scalar_constant(array[1]) != length or
                decorations.get((identity, 6)) != [(16,)]):
            return False
        opcode, vector = types.get(array[0], (None, ()))
        return opcode == 23 and len(vector) == 2 and vector[1] == 4 and types.get(vector[0]) == scalar

    blocks = []
    for pointer, storage in binding(4):
        block = pointee(pointer, 2)
        opcode, members = types.get(block, (None, ()))
        if (storage == 2 and opcode == 30 and len(members) == 2 and
                decorations.get((block, 2)) == [()] and
                sorted(offsets.get(block, ())) == [(0, 0), (1, 304)] and
                array_of_vector(members[0], (21, (32, 0)), 15) and
                array_of_vector(members[1], (22, (32,)), 8)):
            blocks.append(block)
    if len(blocks) != 1:
        return None
    values = {scalar_constant(identity) for identity in constants}
    return 1 if {0x53525448, 52} <= values else None


def native_module_contract(raw, name):
    """Recheck executable reads and the actual Vulkan stage, not report flags."""
    from audit_native_renderer_kernels import audit_spirv, instructions
    stage = {'.vert.spv': 0, '.geom.spv': 3, '.frag.spv': 4, '.comp.spv': 5}
    models = [model for suffix, model in stage.items() if name.endswith(suffix)]
    if len(models) != 1:
        raise ValueError("Native shader has no admitted Vulkan stage")
    entry_points = []
    ops = instructions(raw)
    for opcode, arguments in ops:
        if opcode == 15:
            if len(arguments) < 3:
                raise ValueError("Native shader entry point is truncated")
            entry = struct.pack('<'+'I'*(len(arguments)-2), *arguments[2:]).split(b'\0', 1)[0]
            entry_points.append((arguments[0], entry))
    if entry_points != [(models[0], b'main')]:
        raise ValueError("Native shader execution model differs from its production stage")
    result = audit_spirv(raw)
    if result['passed'] is not True:
        raise ValueError("Native shader contains executable instruction-bank reads")
    contract = dict(result, execution_model=models[0], entry_point='main')
    # Legacy module contracts retain their exact historical shape. Only the
    # independently derived new ABI adds metadata to a staged module.
    if name in PATH_TRACING_TEMPORAL_FRAGMENTS and _path_tracing_temporal_abi(ops) == 2:
        contract['path_tracing_screen_abi'] = 2
    if name in PATH_TRACING_SURFACE_FRAGMENTS and _path_tracing_surface_abi(ops) == 1:
        contract['path_tracing_surface_abi'] = 1
    return contract


def _bound_shader_requirements(executable):
    executable = Path(executable)
    if executable.is_symlink() or not executable.is_file() or executable.resolve(strict=True) != executable:
        raise ValueError("Native shader catalog has no direct executable owner")
    with executable.open('rb') as stream:
        before = os.fstat(stream.fileno())
        if not before.st_size:
            raise ValueError("Native shader catalog executable is empty")
        with mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
            pattern = re.compile(_GENERATED_PATH.pattern.encode() + rb'\x00')
            paths = {match.group()[:-1].decode() for match in pattern.finditer(data)}
            path_tracing = data.find(PATH_TRACING_EXECUTABLE_MARKER) != -1
            visibility = data.find(PATH_TRACING_VISIBILITY_MARKER) != -1
            lighting = data.find(PATH_TRACING_LIGHTING_MARKER) != -1
            temporal = data.find(PATH_TRACING_TEMPORAL_MARKER) != -1
            surface = data.find(PATH_TRACING_SURFACE_MARKER) != -1
        if _identity(before) != _identity(os.fstat(stream.fileno())) or _identity(before) != _identity(executable.stat()):
            raise ValueError("Native shader catalog executable changed during validation")
    if not paths:
        raise ValueError("Native executable has no bound per-program Vulkan catalog")
    return paths, path_tracing, visibility, lighting, temporal, surface


def validate_native_package(directory, *, expected_sha256=None, executable):
    """Authenticate the exact package and its complete executable-bound catalogs.

    This complements source-origin closure at staging. Its SPIR-V check covers
    executable reads from the complete source program ABIs, not arbitrary future
    bytecode formats. It does not grant gameplay or original-framebuffer parity.
    """
    directory = Path(directory)
    if directory.is_symlink() or directory.resolve(strict=True) != directory:
        raise ValueError("Native shader package requires its canonical directory")
    package = directory / AOT_PACKAGE_NAME
    raw = _direct_bytes(package, 64*1024*1024)
    package_pin = hashlib.sha256(raw).hexdigest()
    if expected_sha256 is not None and package_pin != expected_sha256:
        raise ValueError("Native shader package descriptor changed")
    report = json.loads(raw)
    if (report.get('schema') != AOT_PACKAGE_SCHEMA or report.get('profile') != AOT_PROFILE or
            report.get('passed') is not True or report.get('runtime_instruction_interpreter') is not False):
        raise ValueError("Native shader package lacks its strict AOT contract")
    rows = report.get('modules')
    if (not isinstance(rows, list) or not rows or len(rows) > 20000 or
            type(report.get('module_count')) is not int or report['module_count'] != len(rows)):
        raise ValueError("Native shader package has an invalid complete module count")
    files, generated, identities, contracts = {}, set(), {}, {}
    for row in rows:
        name = row.get('path')
        if not isinstance(name, str) or name in files:
            raise ValueError("Native shader package duplicates a module owner")
        if name not in AOT_FIXED_FILES:
            if not _GENERATED_PATH.fullmatch(name):
                raise ValueError("Native shader package contains an undeclared production module")
            generated.add(name)
        size, pin = row.get('bytes'), row.get('sha256')
        if (type(size) is not int or not 0 < size <= 32*1024*1024 or
                not isinstance(pin, str) or not re.fullmatch('[0-9a-f]{64}', pin)):
            raise ValueError("Native shader package lacks an immutable module identity")
        path = directory/name
        identity = _identity(path.stat())
        data = _direct_bytes(path, 32*1024*1024)
        if identity != _identity(path.stat()):
            raise ValueError("Native shader changed before its contract was checked")
        identities[path] = identity
        if len(data) != size or hashlib.sha256(data).hexdigest() != pin:
            raise ValueError("Native shader bytes differ from their package identity")
        contracts[name] = native_module_contract(data, name)
        if row.get('native_contract') != contracts[name]:
            raise ValueError("Native shader package falsified its executable contract")
        files[name] = pin
    bound_generated, path_tracing, visibility, lighting, temporal, surface = _bound_shader_requirements(executable)
    fixed = AOT_BASE_FIXED_FILES
    if path_tracing or visibility or lighting or temporal or surface or (PATH_TRACING_FILES | PATH_TRACING_VISIBILITY_FILES | PATH_TRACING_LIGHTING_FILES) & files.keys():
        fixed |= PATH_TRACING_FILES
    if visibility or lighting or temporal or surface or (PATH_TRACING_VISIBILITY_FILES | PATH_TRACING_LIGHTING_FILES) & files.keys():
        fixed |= PATH_TRACING_VISIBILITY_FILES
    if lighting or temporal or surface or PATH_TRACING_LIGHTING_FILES & files.keys():
        fixed |= PATH_TRACING_LIGHTING_FILES
    if not fixed <= files.keys():
        raise ValueError("Native shader package omits its fixed Vulkan ABI")
    if temporal and any(contracts[name].get('path_tracing_screen_abi') != 2
                        for name in PATH_TRACING_TEMPORAL_FRAGMENTS):
        raise ValueError("Path Tracing temporal shader lacks its 32-byte screen ABI")
    if surface and any(contracts[name].get('path_tracing_surface_abi') != 1
                       for name in PATH_TRACING_SURFACE_FRAGMENTS):
        raise ValueError("Path Tracing shader lacks its authored surface ABI")
    if generated != bound_generated:
        raise ValueError("Native shader package differs from its executable-bound program catalogs")
    inventory_pin = hashlib.sha256(('\n'.join(sorted(files))+'\n').encode()).hexdigest()
    if report.get('inventory_sha256') != inventory_pin:
        raise ValueError("Native shader package inventory identity differs")
    files[AOT_PACKAGE_NAME] = package_pin
    actual = set()
    for path in directory.rglob('*'):
        if path.is_symlink():
            raise ValueError("Native shader package contains an alias")
        if path.is_file():
            actual.add(path.relative_to(directory).as_posix())
    if (actual != files.keys() or _direct_bytes(package, 64*1024*1024) != raw or
            any(_identity(path.stat()) != identity for path, identity in identities.items())):
        raise ValueError("Native shader package inventory changed")
    return dict(profile=AOT_PROFILE, files=files, module_count=len(rows),
                package=dict(path=AOT_PACKAGE_NAME, sha256=package_pin))


def shader_capability(directory, executable):
    """Describe a new capability without inlining its large finite AOT bank."""
    directory = Path(directory)
    package = directory/AOT_PACKAGE_NAME
    if package.exists() or package.is_symlink():
        result = validate_native_package(directory, executable=executable)
        return dict(profile=AOT_PROFILE, package=result['package'])
    return dict(profile=PROFILE, files=inventory(directory))
REQUIRED_FILES = frozenset("""
bloom.frag.spv
bloom.vert.spv
bsp_patchy_composite.frag.spv
bsp_patchy_noise.frag.spv
cinematic_crossfade.frag.spv
crt_oled.frag.spv
display.frag.spv
fog.frag.spv
fog.vert.spv
hud.frag.spv
light_volume.frag.spv
light_volume.vert.spv
menu_world.frag.spv
menu_world.geom.spv
overlay.frag.spv
overlay.vert.spv
particle.frag.spv
particle.geom.spv
particle.vert.spv
particle_directional.vert.spv
particle_directional_program.glsl
particle_directional_program.hpp
particle_directional_program.json
particle_distortion.vert.spv
particle_distortion_program.glsl
particle_distortion_program.hpp
particle_distortion_program.json
particle_model.geom.spv
particle_model.vert.spv
particle_model_program.glsl
particle_model_program.hpp
particle_model_program.json
particle_perpendicular.vert.spv
particle_perpendicular_program.glsl
particle_perpendicular_program.hpp
particle_perpendicular_program.json
particle_program.glsl
particle_program.hpp
particle_program.json
patchy_composite.frag.spv
patchy_noise.frag.spv
patchy_noise.vert.spv
pstencil/dependent_light.frag.spv
pstencil/light_utility.frag.spv
pstencil/lightmap.frag.spv
pstencil/lightmap.vert.spv
pstencil/normal_accumulation.frag.spv
pstencil/normal_accumulation.vert.spv
pstencil/normal_light_utility.frag.spv
pstencil/normal_light_utility.vert.spv
pstencil/texture_accumulation.frag.spv
pstencil/volume.frag.spv
pstencil/volume.vert.spv
scene_copy.frag.spv
scope_copy.frag.spv
source_contrail.frag.spv
source_contrail.geom.spv
source_contrail.vert.spv
source_contrail_program.glsl
source_contrail_program.hpp
source_contrail_program.json
source_decal.frag.spv
source_decal.geom.spv
source_decal.vert.spv
source_decal_program.glsl
source_decal_program.hpp
source_decal_program.json
source_depth_color_writeback.comp.spv
source_lens_draw.frag.spv
source_lens_draw.vert.spv
source_lens_query.frag.spv
source_lens_query.vert.spv
source_shadow_filter.frag.spv
source_shadow_filter.vert.spv
source_weather_depth_alias.comp.spv
source_weather_line.frag.spv
source_weather_line.geom.spv
source_weather_line.vert.spv
source_weather_plate.frag.spv
source_weather_plate.vert.spv
source_weather_point.frag.spv
source_weather_point.vert.spv
ui.frag.spv
ui.vert.spv
ui_frame.frag.spv
upscale_motion.frag.spv
upscale_motion.vert.spv
upscale_prepare.comp.spv
vista_loading.frag.spv
vista_loading.vert.spv
water_copy.frag.spv
world.frag.spv
world.vert.spv
world_emblem.frag.spv
world_emblem.geom.spv
world_prt.vert.spv
""".split())


def inventory(directory):
    directory = Path(directory)
    if directory.is_symlink() or directory.resolve(strict=True) != directory:
        raise ValueError("Shader bank requires its canonical directory owner")
    files = {}
    for path in sorted(directory.rglob("*")):
        if path.is_symlink():
            raise ValueError("Shader inventory cannot contain aliases")
        if path.is_file():
            files[path.relative_to(directory).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    if not REQUIRED_FILES <= files.keys():
        raise ValueError("Native renderer shader family is incomplete")
    return files


def main():
    parser = argparse.ArgumentParser(description='Read-only verification of a complete native Vulkan shader package')
    parser.add_argument('--verify-aot-package', '--verify-AOT-package', action='store_true', required=True)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--quiet', action='store_true', help='Return admission status without a success record')
    args = parser.parse_args()
    try:
        proof = validate_native_package(args.directory, executable=args.executable)
        if not args.quiet:
            print(json.dumps(dict(passed=True, profile=proof['profile'], module_count=proof['module_count'])))
        return 0
    except (OSError, ValueError, KeyError, TypeError, AttributeError, RuntimeError) as error:
        print(json.dumps(dict(passed=False, profile=AOT_PROFILE, error=str(error))))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
