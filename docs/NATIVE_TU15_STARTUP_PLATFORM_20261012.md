# Native desktop startup without BIOS or EEPROM dependencies

The user's BIOS is a local reference for understanding platform behavior.
It is not a distributable game asset or a required runtime dependency.
`NativeTu15StartupPlatform` now connects native capability and application
configuration owners to the original English TU1.5 startup and settings caller.
Its only image input is the user's owned title executable. It reads no BIOS,
EEPROM, captured kernel text or status tables.

## Native platform contract

The original startup tests hardware-info bit8, independently named
`XBOX_HW_FLAG_ARCADE` in the pinned nxdk declarations. The native desktop is
not an Xbox arcade machine. Its immutable ABI record describes that native
capability; it does not copy the observed Xbox record or claim an Xbox GPU/MCP
revision. The remaining bytes are unconsumed storage in these supported
callers. The original caller takes its ordinary settings-query branch. It is
not bypassed using an arcade flag.

`NativeTu15StartupSettings::automatic_power_down` is explicit application
configuration supplied by its owner. No EEPROM image or EEPROM defaults are
constructed. The original settings wrapper queries XC_MISC17 and consumes
bit0 of the returned DWORD. The native query returns that real preference,
typeREG_DWORD and length4, with three logical DWORD output stores. This is a
declared native configuration/query ABI; it does not claim original kernel
store granularity, hardware discovery or mutable EEPROM behavior.

The native query's returning domain is STATUS_SUCCESS. Original reference301
independently establishes its conversion to ERROR_SUCCESS. The adapter owns
that domain without loading the BIOS's tables. Other status inputs reject
explicitly. Query and conversion authenticate the original full frames and
preserve other CPU state under the declared native stdcall ABI. All output
extents are checked before publication. Existing BIOS/EEPROM-based comparison
helpers remain separate reference paths; this new path does not invoke them.

The complete original startup and settings bodies execute native compiled
control flow. The original timer149 call remains an unresolved real operation,
with its actual timer, DPC and signed six-hour request. No timer, heap-global
installation, world constructor or physics result is manufactured.

## Verification

The independent original-only procedure uses unchanged TU1.5 instructions,
fresh patterned stack/storage, independently compiled primary record/list
owners, and the explicitly declared native query/status ABI. It reads no BIOS
or EEPROM.48 cases cover both incoming entry points, both application setting
values, flags2/CD7, three stack alignments and empty x87 TOP0/3. They execute
2,832 original instructions,1,800 source stores,816 primary record/list stores
and144 native configuration writes. Native comparisons check instruction
order, all stores, final banks, complete pending timer frame, GP/XMM,
applicable flags and FP state. Invalid frames/statuses, unknown callers and
read-only outputs are also checked. Independent original repeats are identical.

Fresh Release and ASAN/UBSAN/leak builds each pass the native startup comparison
and the authenticated TU1.5 image-loader check. The actual production CMake
modules are compiled. Unchanged world arithmetic uses its previously validated
archive only after checking every compiler-discovered source/header input.
A successful Release file-access trace confirms no BIOS, EEPROM or captured
kernel-table file is opened.

Evidence:
`analysis/multiplayer/sandbox-source/native-tu15-startup-platform-20261012/`.
Validation seal SHA-256:
`f35bb2cc038efc59ef89cfa5b5bebe1057ef1cf9e73c5581e4c1af3b4fafa8cd`.

The supplied local Complex4627 reference has SHA-256
`34f1c8ded59116436065783f8ad2ef0939df3cbfc76277ec9e5c41bf9ccb93cd`.
That hash records reference identity only. Its bytes are not copied into this
adapter or its code snapshot. Earlier modified-BIOS/xemu observations do not
establish physical Xbox equivalence.

## Remaining integration

The adapter is registered in production and connects through the original
startup to its required timer frontier. It does not yet replace live match
physics. Native timer lifecycle, complete process-heap installation, renderer
completion, occupied world/body/controller setup and tick/readback integration
remain required. The visible Lockout lift and Coagulation vehicle acceptance
failures remain open. The full multiplayer goal is active and unfinished.
