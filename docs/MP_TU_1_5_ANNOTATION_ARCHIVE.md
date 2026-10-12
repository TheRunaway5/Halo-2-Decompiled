# Purported update 1.5 annotation archive

The user supplied `/home/eric/Downloads/decomp1.5TU.zip`, SHA-256
`555ce875748e0921d79ca72eeabf15a98a63ab4b3c6fb7c55b578ef83388ab54`.
It is an IDA annotation export, not an executable or a decompiled code export.
The six members were extracted into ignored reference storage under
`analysis/multiplayer/title-update-1.5/`; the included scripts were read, not
executed. Original source and existing frozen builds were not modified.

The JSON metadata names an input file under a Windows directory named
`Halo 2 1.5TU`, ending in `default.xbe`. This is a provenance claim, not a
verified executable version. It contains no hash, certificate, executable
bytes, disassembled instructions or pseudocode bodies with which to verify
update 1.5 behavior.

The archive contains:

| Member | Content |
| --- | --- |
| `functions.json` | 16,094 address/name/comment/end/segment/flag records |
| `items.json` | 120,593 address/name/comment/pre/post-comment records |
| `annotations.json` | The same function/item records, metadata, and empty structure/enum/stack-variable lists |
| `function_inventory.md` | Human-readable names, ranges and comments |
| `Import_IDA_Annotations_Jython.py` | Applies labels/comments to an already loaded Ghidra program |
| `Verify_IDA_Annotations_Jython.py` | Checks those labels/comments in an already loaded program |

The separate JSON function and item lists are exactly equal to their combined
`annotations.json` copies. All function records have precisely seven metadata
fields; all item records have seven annotation fields. Neither schema contains
code bytes or function bodies. Of the function records, 1,063 have names other
than ordinary `sub_`/`nullsub_` names and 679 have function comments. Those
human-authored names and interpretations may aid navigation once the matching
code is available, but are not behavioral authority by themselves.

[Archive/member size and hash manifest](../analysis/multiplayer/title-update-1.5/import-manifest.json).
At the initial inspection this archive did not supply a matching executable.
The subsequent user-supplied `Halo2DLC.zip` does contain an authenticated retail
Halo 2 executable under `content/u5English/default.xbe`. All 16,094 annotation
ranges fit its named, raw-backed sections, and its decoded entry point matches
the exported `start` record. This permits using the inventory for navigation;
it does not verify the semantic labels/comments or make them executable code.
The separately attached `/home/eric/Downloads/functions.json` is byte-identical
to this archive's member.

See [the executable import and validation](MP_TU_1_5_EXECUTABLE_IMPORT.md).
Existing update 1.1 fixtures and the previously delivered native build retain
their original authority. No 1.5 gameplay change is integrated or claimed from
annotations alone.
