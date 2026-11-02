"""Read authored H2EK template/pass parameter metadata without importing code.

Field roles are cross-checked against the upstream TagTool Gen2 declarations:
https://github.com/TheGuardians/TagTool/blob/master/TagTool/Tags/Definitions/Gen2/ShaderTemplate.cs
https://github.com/TheGuardians/TagTool/blob/master/TagTool/Tags/Definitions/Gen2/ShaderPass.cs
Those describe input structure, not native engine behavior. The installed tag
bytes, fieldset counts/strides, complete traversal and immutable hashes are the
actual evidence. Shader-code data is skipped and hashed, never executed or
retained. Run through run_test.py; retain only the bounded JSON report.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct


def need(ok, why):
    if not ok:
        raise ValueError("H2EK authored shader metadata: " + why)


def word(data, at):
    return struct.unpack_from("<I", data, at)[0]


def short(data, at):
    return struct.unpack_from("<h", data, at)[0]


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Reader:
    def __init__(self, path, group):
        self.path = path
        self.raw = path.read_bytes()
        need(80 <= len(self.raw) <= 8 * 1024 * 1024, "tag extent exceeds metadata budget")
        need(self.raw[60:64] == b"!MLB" and word(self.raw, 44) == 64 and
             word(self.raw, 36) == int.from_bytes(group.encode(), "big"), "tag header/group differs")
        self.at = 64
        self.fieldsets = []
        self.skipped_code = []

    def take(self, size):
        need(0 <= size <= len(self.raw) - self.at, "serialized child escaped its actual file")
        value = self.raw[self.at:self.at + size]
        self.at += size
        return value

    def block(self, count, stride, decode):
        need(0 <= count <= 4096 and 0 < stride <= 1024, "excessive serialized fieldset")
        if not count:
            return []
        at = self.at
        header = self.take(16)
        need(header[:4] == b"dfbt" and struct.unpack_from("<III", header, 4) == (0, count, stride),
             "fieldset version/count/stride differs at " + str(at))
        self.fieldsets.append(dict(offset=at, count=count, stride=stride))
        rows = self.take(count * stride)
        return [decode(rows[i * stride:(i + 1) * stride]) for i in range(count)]

    def sid(self, data, at):
        value = word(data, at)
        return dict(word=value, name=self.take(value >> 24).decode("utf-8"))

    def data(self, data, at, code=False):
        size = word(data, at)
        need(size <= 4 * 1024 * 1024, "excessive tag data field")
        value = self.take(size)
        result = dict(bytes=size, sha256=sha(value))
        if code:
            self.skipped_code.append(result)
        return result

    def reference(self, data, at):
        kind, size = word(data, at), word(data, at + 8)
        need(size <= 4096, "excessive symbolic tag reference")
        raw = self.take(size + 1) if size else b""
        need(not size or raw[-1] == 0, "tag-reference terminator differs")
        return dict(group=kind.to_bytes(4, "big").decode("ascii") if kind != 0xffffffff else "NONE",
                    name=raw[:-1].decode("utf-8") if size else "")

    def template_parameter(self, raw):
        name = self.sid(raw, 0)
        explanation = self.data(raw, 4)
        bitmap = self.reference(raw, 28)
        return dict(name=name, explanation=explanation, type=short(raw, 24), flags=short(raw, 26),
            default_bitmap=bitmap, default_value=struct.unpack_from("<f", raw, 44)[0],
            default_rgb=struct.unpack_from("<3f", raw, 48), bitmap_type=short(raw, 60),
            bitmap_animation_flags=short(raw, 64), bitmap_scale=struct.unpack_from("<f", raw, 68)[0])

    def template_category(self, raw):
        name = self.sid(raw, 0)
        parameters = self.block(word(raw, 4), 72, self.template_parameter)
        return dict(name=name, parameters=parameters)

    def template(self):
        def root(raw):
            documentation = self.data(raw, 0)
            material = self.sid(raw, 20)
            properties = self.block(word(raw, 28), 8, lambda p: dict(property=short(p, 0), parameter=self.sid(p, 4)))
            categories = self.block(word(raw, 40), 16, self.template_category)
            response = self.reference(raw, 52)
            def lod(p):
                return dict(diameter=struct.unpack_from("<f", p)[0], passes=self.block(word(p, 4), 32,
                    lambda q: dict(layer=short(q, 0), reference=self.reference(q, 4))))
            lods = self.block(word(raw, 68), 16, lod)
            unknown = [self.block(word(raw, at), 4, lambda p: p.hex()) for at in (80, 92)]
            auxiliary = [self.reference(raw, at) for at in (104, 124)]
            need(word(raw, 144) == 0, "authored template has a compiled bank outside this reader")
            return dict(documentation=documentation, default_material=material, properties=properties,
                        categories=categories, light_response=response, lods=lods, unknown=unknown, auxiliary=auxiliary)
        result = self.block(1, 156, root)[0]
        return self.finish(result)

    def pass_parameter(self, raw):
        name = self.sid(raw, 0)
        explanation = self.data(raw, 4)
        bitmap = self.reference(raw, 28)
        return dict(name=name, explanation=explanation, type=short(raw, 24), flags=short(raw, 26),
            default_bitmap=bitmap, default_value=struct.unpack_from("<f", raw, 44)[0],
            default_rgb=struct.unpack_from("<3f", raw, 48), source_extern=short(raw, 60))

    def constant(self, raw):
        return dict(parameter=self.sid(raw, 0), constant=short(raw, 6))

    def texture(self, raw):
        parameter = self.sid(raw, 0)
        states = [self.block(word(raw, at), stride, lambda p: p.hex())
                  for at, stride in ((20, 8), (32, 16), (44, 12), (56, 8))]
        constants = self.block(word(raw, 68), 8, self.constant)
        return dict(parameter=parameter, source_extern=short(raw, 4), mode=short(raw, 10),
                    dot_mapping=short(raw, 14), input_stage=short(raw, 16), states=states, constants=constants)

    def vertex_constant(self, raw):
        return dict(parameter=self.sid(raw, 0), scale_by_texture_stage=short(raw, 4), register_bank=short(raw, 6),
                    register_index=short(raw, 8), component_mask=short(raw, 10))

    def implementation(self, raw):
        textures = self.block(word(raw, 4), 80, self.texture)
        vertex = self.reference(raw, 16)
        constants = self.block(word(raw, 32), 12, self.vertex_constant)
        code = self.data(raw, 44, code=True)
        states = [self.block(word(raw, at), stride, lambda p: p.hex())
                  for at, stride in ((72, 4), (84, 16), (96, 8), (108, 16), (120, 4), (132, 12), (144, 8))]
        state_constants = self.block(word(raw, 156), 8, self.constant)
        pixel = self.reference(raw, 168)
        return dict(flags=short(raw, 0), textures=textures, vertex_shader=vertex, vertex_constants=constants,
            unused_shader_code_metadata=code, channels=short(raw, 64), alpha_blend=short(raw, 66), depth=short(raw, 68),
            states=states, state_constants=state_constants, pixel_shader=pixel)

    def shader_pass(self):
        def root(raw):
            doc = self.data(raw, 0)
            parameters = self.block(word(raw, 20), 64, self.pass_parameter)
            implementations = self.block(word(raw, 36), 184, self.implementation)
            need(word(raw, 48) == 0, "authored pass has compiled banks outside this reader")
            return dict(documentation=doc, parameters=parameters, implementations=implementations)
        return self.finish(self.block(1, 60, root)[0])

    def finish(self, result):
        need(self.at == len(self.raw), "serialized tag has unowned trailing children at " + str(self.at))
        need(self.path.read_bytes() == self.raw, "source tag changed while reading")
        return dict(path=str(self.path), bytes=len(self.raw), sha256=sha(self.raw), complete_traversal=True,
                    fieldsets=self.fieldsets, skipped_shader_code_data=self.skipped_code, metadata=result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--h2ek-root", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    tags = (args.h2ek_root / "tags").resolve(strict=True)
    templates, passes, errors = {}, {}, []
    for family in ("tex_bump_env_tiling_specular", "tex_bump_env_illum_tiling_specular"):
        path = tags / "shaders/shader_templates/opaque" / (family + ".shader_template")
        try:
            templates[family] = Reader(path, "stem").template()
        except (ValueError, IndexError, struct.error, UnicodeError) as error:
            errors.append(dict(path=str(path), error=str(error)))
    selected = {r"shaders\shader_passes\lightmap\lightmap_env"}
    for row in templates.values():
        selected.update(p["reference"]["name"] for lod in row["metadata"]["lods"] for p in lod["passes"])
    for name in sorted(selected):
        path = (tags / (name.replace("\\", "/") + ".shader_pass")).resolve(strict=True)
        need(path.is_relative_to(tags), "authored pass escaped its source tag tree")
        try:
            passes[name] = Reader(path, "spas").shader_pass()
        except (ValueError, IndexError, struct.error, UnicodeError) as error:
            errors.append(dict(path=str(path), error=str(error)))
    report = dict(scope=__doc__, passed=not errors, templates=templates, passes=passes, errors=errors,
        input_behavior_authority=False, shader_programs_imported=False, native_role_mapping_admitted=False,
        helper_sha256=sha(Path(__file__).read_bytes()), owned_scratch=str(owner), retained_large_artifacts=[])
    raw = json.dumps(report, indent=2) + "\n"
    need(len(raw.encode()) <= 1024 * 1024, "decoded metadata exceeds bounded report budget")
    args.report.write_text(raw)
    print(json.dumps(dict(passed=not errors, templates=len(templates), passes=len(passes), errors=errors)))
    return 0 if not errors else 1


if __name__ == "__main__":
    raise SystemExit(main())
