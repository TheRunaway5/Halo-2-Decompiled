#!/usr/bin/env python3
"""Reproducible headless Ghidra analysis of an extracted original Xbox XBE."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import urllib.request
import zipfile

from disc import inspect_xbe

ROOT = Path(__file__).resolve().parents[1]
GHIDRA_VERSION = "12.0.3"
PACKAGES = {
    "ghidra.zip": "https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.0.3_build/ghidra_12.0.3_PUBLIC_20260210.zip",
    "ghidra-xbe.zip": "https://github.com/XboxDev/ghidra-xbe/releases/download/build-202602250354/ghidra_12.0.3_PUBLIC_20260225_ghidra-xbe.zip",
}
PACKAGE_SHA256 = {
    "ghidra.zip": "90d3fffb20b00030dcef8d2a24dd0f422d3a61e432b3ad43f77233ac6d667981",
    "ghidra-xbe.zip": "fc689e84a3940d840edac94491e28f83a0cfc1f177fffceed835932bdc2d54b1",
}
MATCHING_SHA = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
INVENTORY_URL = "https://raw.githubusercontent.com/kirklandsig/halo2-decompiled/5f08630655e3ddc9ac45909090f11ae799fa0c27/config/functions.csv"
INVENTORY_SHA256 = "76117247be7d24fc3baadfce3ca2ca29d7ce0f0e796db72a2c7ae1dd112218f6"


def bootstrap():
    tools = ROOT / ".tools"
    downloads = tools / "downloads"
    downloads.mkdir(parents=True, exist_ok=True)
    ghidra = tools / "ghidra_12.0.3_PUBLIC"
    for name, url in PACKAGES.items():
        archive = downloads / name
        if not archive.exists():
            temporary = archive.with_suffix(".partial")
            print(f"Downloading {url}", flush=True)
            urllib.request.urlretrieve(url, temporary)
            temporary.replace(archive)
        with archive.open("rb") as source:
            if hashlib.file_digest(source, "sha256").hexdigest() != PACKAGE_SHA256[name]:
                raise ValueError(f"{archive} does not match the pinned upstream release SHA256")
        dest = tools if name == "ghidra.zip" else ghidra / "Ghidra/Extensions"
        marker = ghidra / "support/analyzeHeadless" if name == "ghidra.zip" else dest / "ghidra-xbe/extension.properties"
        if not marker.exists():
            with zipfile.ZipFile(archive) as z:
                for item in z.infolist():
                    if not (dest / item.filename).resolve().is_relative_to(dest.resolve()):
                        raise ValueError("unsafe archive path")
                z.extractall(dest)
                for item in z.infolist():
                    mode = item.external_attr >> 16
                    if mode & 0o111:
                        (dest / item.filename).chmod(0o755)
    return ghidra


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--xbe", type=Path, default=ROOT / "assets/disc/default.xbe")
    parser.add_argument("--ghidra", type=Path)
    parser.add_argument("--bootstrap", action="store_true")
    parser.add_argument("--inventory", type=Path, help="optional matching function inventory CSV")
    parser.add_argument("--matching-inventory", action="store_true", help="download the pinned CC0 retail function boundary inventory")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/retail")
    parser.add_argument("--project", default="halo2-retail")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_-]+", args.project):
        parser.error("invalid project name")
    ghidra = bootstrap() if args.bootstrap else args.ghidra or ROOT / ".tools/ghidra_12.0.3_PUBLIC"
    runner = ghidra.resolve() / "support/analyzeHeadless"
    if not runner.is_file():
        parser.error("Ghidra unavailable; pass --bootstrap or --ghidra")
    metadata = inspect_xbe(args.xbe)
    output = args.output.resolve()
    if (output / "summary.json").exists():
        parser.error("completed output already exists; choose a new --output and --project")
    output.mkdir(parents=True, exist_ok=True)
    projects = ROOT / "analysis/projects"
    projects.mkdir(parents=True, exist_ok=True)
    if (projects / (args.project + ".gpr")).exists():
        parser.error("Ghidra project already exists; choose a new --project to preserve existing analysis")
    command = [str(runner), str(projects), args.project, "-import", str(args.xbe.resolve()),
               "-scriptPath", str(ROOT / "tools/ghidra"), "-max-cpu", "8", "-analysisTimeoutPerFile", "1800"]
    if args.matching_inventory:
        if args.inventory:
            parser.error("choose --inventory or --matching-inventory")
        args.inventory = output / "retail-inventory.csv"
        urllib.request.urlretrieve(INVENTORY_URL, args.inventory)
        if hashlib.sha256(args.inventory.read_bytes()).hexdigest() != INVENTORY_SHA256:
            parser.error("downloaded retail function inventory does not match its pinned SHA256")
        metadata["inventory_url"] = INVENTORY_URL
    if args.inventory:
        if metadata["sha256"] != MATCHING_SHA:
            parser.error("inventory only supported for the exact matching retail XBE SHA256")
        inventory = args.inventory.read_bytes()
        metadata["inventory_sha256"] = hashlib.sha256(inventory).hexdigest()
        seeds = output / "seeds.tsv"
        with args.inventory.open(newline="") as f, seeds.open("w") as out:
            for row in csv.DictReader(f):
                name = re.sub(r"[^a-zA-Z0-9_$]", "_", row.get("name", ""))
                out.write(f"{row['va']}\t{name}\n")
        command += ["-preScript", "SeedFunctions.java", str(seeds)]
    command += ["-postScript", "ExportDecompilation.java", str(output)]
    metadata.update({"ghidra_version": GHIDRA_VERSION, "command": command, "packages": PACKAGES,
                     "package_sha256": PACKAGE_SHA256})
    (output / "provenance.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print("Ghidra project:", projects / args.project, flush=True)
    print("Analysis output:", output, flush=True)
    environment = os.environ.copy()
    # Ghidra 12 requires JDK 21+. The user may override JAVA_HOME.
    if "JAVA_HOME" not in environment and Path("/usr/lib/jvm/java-21-openjdk").is_dir():
        environment["JAVA_HOME"] = "/usr/lib/jvm/java-21-openjdk"
    with (output / "ghidra.log").open("w") as log:
        result = subprocess.run(command, env=environment, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode or not (output / "summary.json").is_file():
        raise SystemExit(f"Ghidra did not complete the export; see {output / 'ghidra.log'}")
    print((output / "summary.json").read_text())


if __name__ == "__main__":
    main()
