# Original weather caller capture

The actual original Lockout36B50 entry and complete return are independently
frozen under `analysis/multiplayer-shading/original-weather-20261009/session1/`.
The unmodified title runs in xemu0.8.136 with OpenGL llvmpipe. Hardware guest
breakpoints and read-only RAM/register exports provide source-state evidence;
they do not establish physical Xbox rendering or numerical precision.

The entry is at36B50 with ESPD00B37BC and real stack return2C16E. The actual
return has ESPD00B37C0 and AL=0. Each export retains64MiB RAM, register/status
before and after, and confirms nonrunning state with identical registers while
exporting. All breakpoints were removed afterward. Guest debugger detach can
advance the machine; the subsequently paused current state is not substituted
for either immutable export.

`tools/audit_source_weather_full_original.py` restores actual GPR/SSE state,
empty x87 stack, PC53/nearest controls, source segments and lazy actual RAM
pages. It compares every executed title instruction with the pinned original
XBE. No kernel return, readiness, admission, elapsed time, random value or GPU
result is substituted.

Replay executes552,742 instructions through5,290 distinct addresses. The
observed chain includes intensity398D0, wind4B3D0, plate simulation37260,
particle simulation38450, plate draw37A60/program3A5C0, resident bitmap
requests143C0/bindings137550, particle draw391F0, camera constants12FA0 and
particle program39780. Both actual12DE70 resource-ready services execute.
The admitted first palette carries distanceBF800000 (-1).

The first uncaptured read is at3FA886, addressFD800044: the NV2A queue GET
register. Replay stops there. It has written58,284 distinct nonstack bytes;
these are not asserted equal to the full return because the SDK tail has not
executed. Seven specifically named producer banks are already complete and
match the independent actual return byte for byte:

| Bank | Bytes | Differing bytes |
| --- | ---: | ---: |
| Particle system | 140 | 0 |
| Owning weather/wind | 176 | 0 |
| Particle render rows | 20,480 | 0 |
| Particle state rows | 32,768 | 0 |
| Auxiliary random state | 4 | 0 |
| Shared elapsed clock | 8 | 0 |
| Intensity state | 8 | 0 |

`analysis/multiplayer-shading/weather-full-original/report.json` retains the
source call order, boundary and entry/replay/actual bank hashes. The audit
rejects a different boundary, exhausted instruction budget or producer
disagreement. It does not complete the GPU queue service, remaining SDK command
tail, cleanup, return registers or GPU consumption. A value observed in
another frame cannot resolve the missing same-call service history.

Reproduce with the project's Unicorn environment:

```
.tools/python/bin/python tools/audit_source_weather_full_original.py
```

The three retained source disk/EEPROM hashes and mtimes remain unchanged in
`retained-source-preservation-post-full-weather.json`. All new operations use
the disposable copied session. No guest instruction/data patches, native
runtime activation or reference-session operations are involved.
