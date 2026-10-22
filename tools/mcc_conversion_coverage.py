"""Validate the owner set of prepared stock MCC maps before cache admission.

This authenticates conversion coverage, not complete Xbox gameplay equivalence.
Nested/mod maps and loose tags/data retain their source-relative identities;
they are reported separately until their own conversion producers are qualified.
"""
from pathlib import PurePosixPath
import re

from audit_rendering import FormatError

PROFILE = "H2_MCC_TU15_PREPARATION_V1"
SCHEMA = "H2_MCC_CONVERSION_COVERAGE_V1"
MAP_DIRECTORY = PurePosixPath("h2_maps_win64_dx11")
SHA256 = re.compile(r"[0-9a-f]{64}\Z")


def _require(condition, message):
    if not condition:
        raise FormatError("MCC conversion coverage: " + message)


def _relative(name):
    _require(isinstance(name, str) and name and "\\" not in name and "\0" not in name,
             "invalid source-relative owner")
    path = PurePosixPath(name)
    _require(path.parts and not path.is_absolute() and name == path.as_posix() and
             all(part not in ("", ".", "..") for part in path.parts),
             "invalid source-relative owner: " + name)
    return path


def _sha(value):
    return isinstance(value, str) and SHA256.fullmatch(value) is not None


def validate(records, conversion):
    """Return exact converted output identities and an explicit coverage report.

    Existing prepared caches can be checked without migration or access to the
    installation. Each stock map must have exactly one conversion result bound
    to the same original source hash as the private-copy inventory. File payload
    hashes remain the caller's responsibility, including later graphics deltas.
    """
    _require(isinstance(records, dict) and records, "missing copied-source inventory")
    paths = {}
    for name, record in records.items():
        path = _relative(name)
        _require(isinstance(record, dict) and _sha(record.get("sha256")) and
                 type(record.get("bytes")) is int and record["bytes"] >= 0,
                 "invalid copied-source identity: " + name)
        paths[name] = path
    required = {name for name, path in paths.items()
                if path.parent == MAP_DIRECTORY and path.suffix == ".map"}
    _require(required, "no stock MCC maps in the copied-source inventory")
    _require(isinstance(conversion, dict) and conversion.get("profile") == PROFILE and
             isinstance(conversion.get("maps"), list), "invalid conversion report")
    outputs = {}
    for row in conversion["maps"]:
        _require(isinstance(row, dict), "invalid converted map record")
        leaf = _relative(row.get("map"))
        _require(len(leaf.parts) == 1 and leaf.suffix == ".map",
                 "converted map must retain its stock relative owner")
        name = (MAP_DIRECTORY / leaf).as_posix()
        _require(name not in outputs, "duplicate converted map owner: " + name)
        _require(name in required, "converted map has no copied stock owner: " + name)
        _require(_sha(row.get("source_sha256")) and
                 row["source_sha256"] == records[name]["sha256"],
                 "converted map source differs from the copied donor: " + name)
        _require(_sha(row.get("output_sha256")), "invalid converted map output: " + name)
        outputs[name] = row["output_sha256"]
    _require(set(outputs) == required,
             "missing converted map owners: " + ", ".join(sorted(required - set(outputs))))
    other_maps = sorted(name for name, path in paths.items()
                        if path.suffix.lower() == ".map" and name not in required)
    loose_tags = sorted(name for name, path in paths.items()
                        if any(part.lower() == "tags" for part in path.parts[:-1]))
    loose_data = sorted(name for name, path in paths.items()
                        if any(part.lower() == "data" for part in path.parts[:-1]))
    coverage = dict(schema=SCHEMA, stock_map_owners=sorted(required),
                    stock_conversion_results_complete=True,
                    preserved_additional_map_owners=other_maps,
                    preserved_loose_tag_owners=loose_tags,
                    preserved_loose_data_owners=loose_data,
                    additional_map_or_loose_tag_conversion_qualified=False,
                    full_tu15_semantic_conversion=False)
    return outputs, coverage
