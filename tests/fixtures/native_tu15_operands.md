# Native immutable TU1.5 operand reference

Metadata only; no reference executable bytes are included. The generator and
native operand tests reconstruct and verify the typed field projection.

```json
{
  "schema": "H2_NATIVE_TU15_OPERAND_REFERENCE_V1",
  "reference_xbe_sha256": "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
  "reference_role": "Offline recovery and comparison only; never a native-factory runtime input",
  "typed_records": 61,
  "named_record_fields_excluding_source_path": 377,
  "scalar_operands": 12261,
  "field_ownership_extent_records": 12638,
  "projection_fnv64": "2a84073bd6ab09b7",
  "excluded": [
    "Executable headers and sections",
    "Host pointers and object padding",
    "Mutable world, pool, driver and platform state",
    "Reference input filesystem path"
  ]
}
```
