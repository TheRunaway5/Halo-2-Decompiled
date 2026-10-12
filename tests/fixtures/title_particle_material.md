These six records cover the two explicitly selected pixel implementations of
original mainmenu particle tags E21A00A5, E21F00AA, and E22100AC at layer 15,
pass zero. They do not establish which implementation a live draw selects.

`tools/export_title_particle_material_oracle.py` obtains program words from the
original pass definition and executes unchanged XBE 17170 constant remapping,
17000 vertex remapping, and 189A0 sampler decoding. The expected state records
come directly from original cache metadata. Every expected texture mip is the
RGBA8 resource data extracted from the original Xemu Vulkan capture, rather
than from the native decoder.

Original executable SHA-256:
03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d

Original capture SHA-256:
077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431

Input SHA-256:
31a3f93bafaaa6ccd943417e7fd046e5b8bc6ce9b14d7b30084a20d6cf048cd6

Expected SHA-256:
39b379c36af1de9ff9b98183c254ce68fa538d178fb7590a4f6664118baee4b4

The input starts with a uint32 record count, followed by four uint32 fields per
record: particle datum, layer, pass in layer, and implementation. Each expected
record has a uint32 byte size, then 60 program words, 128 vertex constant floats,
a uint32 state count with five-byte states, a uint32 sampler count with 14-word
decoded samplers, and four texture stage records. A stage has a uint32 presence
flag. Present stages contain datum/index/log-dimension, width/height/mip-count,
then width/height/byte-size and RGBA8 bytes for each mip. All words are little
endian. Complete simulation, scheduling, native rasterization and title-frame
parity remain separate requirements.
