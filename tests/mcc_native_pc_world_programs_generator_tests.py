"""Reject changed source bodies and authentic but incompatible namespace roots."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import sys
from unittest import mock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / "tools"))
    location = root / "tools/generate_mcc_native_pc_world_programs.py"
    spec = importlib.util.spec_from_file_location("pc_world_programs", location)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    cache = args.cache.resolve(strict=True)
    policy = module.generate(cache)
    assert [(r["map"], len(r["records"])) for r in policy["maps"]] == [
        ("needle", 34), ("street_sweeper", 34)]
    cases = 1

    def rejected(action):
        nonlocal cases
        try:
            action()
        except ValueError:
            cases += 1
        else:
            raise AssertionError("unproved or changed original source was accepted")

    original_read = Path.read_bytes
    source = cache / "native-shaders/01b_spacestation/library.json"

    def changed_library(path):
        raw = original_read(path)
        if path == source:
            value = json.loads(raw)
            value["shader_tags"][0]["bytes"] += 1
            return json.dumps(value).encode()
        return raw

    with mock.patch.object(Path, "read_bytes", changed_library):
        rejected(lambda: module.generate(cache))
    row = policy["maps"][0]
    for item in (row["records"][0], row["equivalent_existing_identities"][0]):
        filename = item["record"]["payload"] if "record" in item else item["candidate_payload"]
        source = cache / "native-shaders" / item["source_map"] / "payloads" / filename

        def changed_payload(path):
            raw = original_read(path)
            return bytes([raw[0] ^ 1]) + raw[1:] if path == source else raw

        with mock.patch.object(Path, "read_bytes", changed_payload):
            rejected(lambda: module.generate(cache))
    # The complete typed graph proves this authentic Dune program equivalent;
    # its different allocation order/padding must not create a false conflict.
    roots = copy.deepcopy(module.ROOTS)
    roots["street_sweeper"][9] = ("dune", "transparent", "one_alpha_env")
    with mock.patch.object(module, "ROOTS", roots):
        alternate = module.generate(cache)
        assert alternate["maps"][1]["roots"][9]["source_map"] == "dune"
        cases += 1
    print(f"{cases} complete-source/payload mutation and exact typed alternate-allocation controls passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
