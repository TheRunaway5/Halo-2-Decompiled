#!/usr/bin/env python3
"""Replay unchanged-PE loading draw arguments with actual original D3D9 shaders.

Only the declared geometry/clock/progress fixtures are replayed. This does not
run the protected loading thread, establish actual progress, or claim physical
Windows equivalence. The deterministic input framebuffer is black, RGB only.
"""
import argparse
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "analysis/frontend/vista-session-flow-audit/native-loading-presentation-original"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="new evidence directory")
    parser.add_argument("--wine", type=Path, default=Path.home()/".local/share/lutris/runners/wine/wine-ge-8-26-x86_64/bin/wine")
    parser.add_argument("--prefix", type=Path, default=SOURCE.parent/"wine-ge-reference")
    parser.add_argument("--case", type=int, action="append", help="optional original case index")
    parser.add_argument("--draw-prefix", type=int, action="append", help="optional cumulative original draw count")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    sys.path.insert(0, str(SOURCE))
    spec = importlib.util.spec_from_file_location("loading_original_commands", SOURCE/"commands.py")
    commands = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(commands)
    pe = commands.PE(commands.EXE, commands.EXE_SHA)
    resources = output/"resources"
    resources.mkdir()
    shader = pe.data[pe.file(0x7e6b10-pe.base):pe.file(0x7e6b10-pe.base)+432]
    if hashlib.sha256(shader).hexdigest() != "0004bc2c5bbf32b053d68531d635d637bd52f43f2942ab033761009934828e4f":
        raise RuntimeError("Original loading vertex shader differs")
    (resources/"loading-vertex.vs20").write_bytes(shader)
    for source in (SOURCE/"resources").glob("*.ps20"):
        shutil.copy2(source, resources/source.name)
    cases = []
    for width,height in ((640,480),(1280,720),(1280,800),(3440,1440),(3840,1080)):
        for elapsed in (0,1000,15000):
            cases.append(commands.Original(pe).run(elapsed=elapsed,width=width,height=height,progress=0.375))
    cases.append(commands.Original(pe).run(elapsed=15000,width=1280,height=720,progress=0.375,active=True,start=14.0))
    selected=[]
    for index,case in enumerate(cases):
        if args.case is not None and index not in args.case:
            continue
        for prefix in args.draw_prefix or [len(case["draws"])]:
            if prefix<1 or prefix>len(case["draws"]):
                raise RuntimeError("Draw prefix outside original case")
            row=copy.deepcopy(case)
            row["original_case_index"]=index
            row["draw_prefix"]=prefix
            row["draws"]=row["draws"][:prefix]
            selected.append(row)
    cases=selected
    (output/"original-commands.json").write_text(json.dumps(cases,indent=2)+"\n")
    data = bytearray(b"VLP1" + struct.pack("<I",len(cases)))
    def words(*values):
        data.extend(struct.pack("<"+"I"*len(values),*values))
    for case in cases:
        words(case["width"],case["height"],len(case["draws"]))
        for draw in case["draws"]:
            handle=draw["texture_handles"][0]
            words(draw["kind"],draw["shader"],handle-0x7000 if handle else 0xffffffff)
            words(len(draw["states"]))
            for state,value in draw["states"].items():
                words(int(state),value)
            words(len(draw["sampler_states"]))
            for key,value in draw["sampler_states"].items():
                words(*map(int,key.split("/")),value)
            data.extend(bytes.fromhex(draw["constants"]["vertex:177"]["raw_hex"]))
            constant=draw["constants"].get("pixel:0")
            words(int(constant is not None))
            if constant:
                data.extend(bytes.fromhex(constant["raw_hex"]))
            data.extend(bytes.fromhex(draw["raw_vertex_hex"]))
    stream = output/"original-commands.bin"
    stream.write_bytes(data)
    temporary = ROOT/"build-frontend/compiler-temp"
    temporary.mkdir(parents=True,exist_ok=True)
    executable = output/"d3d9_frame_probe.exe"
    compile_command=["i686-w64-mingw32-g++","-std=c++20","-O2","-Wall","-Wextra","-Werror","-static",
        str(SOURCE/"d3d9_frame_probe.cpp"),"-ld3d9","-o",str(executable)]
    environment=dict(os.environ,TMPDIR=str(temporary))
    subprocess.run(compile_command,env=environment,check=True)
    def windows(path):
        return "Z:"+str(path.resolve())
    environment.update(WINEPREFIX=str(args.prefix.resolve()),WINEDEBUG="-all",
        WINEDLLOVERRIDES="d3dx9_31,d3d9=n",DXVK_LOG_LEVEL="error")
    run=["xvfb-run","-a","-s","-screen 0 1280x720x24",str(args.wine),str(executable),
         windows(ROOT/"Halo 2 Vista installer/loading.bin"),windows(resources),windows(stream),windows(output)]
    with (output/"probe.jsonl").open("w") as stdout,(output/"probe.stderr").open("w") as stderr:
        process=subprocess.run(run,cwd=ROOT,env=environment,stdout=stdout,stderr=stderr,timeout=180)
    files=[commands.EXE,ROOT/"Halo 2 Vista installer/loading.bin",SOURCE/"commands.py",SOURCE/"d3d9_frame_probe.cpp",
        Path(__file__).resolve(),executable,stream,*resources.glob("*")]
    for name in ("d3dx9_31.dll","d3d9.dll"):
        files.append((args.prefix/"drive_c/windows/syswow64"/name).resolve())
    images=[]
    if process.returncode==0:
        for index,case in enumerate(cases):
            raw=output/f"frame-{index:02d}.bgra"
            png=raw.with_suffix(".png")
            pixels=raw.read_bytes()
            if len(pixels)!=case["width"]*case["height"]*4:
                raise RuntimeError("Incomplete actual D3D9 frame")
            Image.frombytes("RGBA",(case["width"],case["height"]),pixels,"raw","BGRA").convert("RGB").save(png)
            images.append(dict(index=index,width=case["width"],height=case["height"],elapsed_ms=case["elapsed_ms"],
                active=case["active"],raw_sha256=digest(raw),png_sha256=digest(png)))
    report=dict(schema="H2V_ORIGINAL_LOADING_D3D9_REPLAY_V1",scope=__doc__,returncode=process.returncode,
        compile_argv=compile_command,run_argv=run,pins={str(p):digest(p) for p in files},images=images)
    (output/"report.json").write_text(json.dumps(report,indent=2)+"\n")
    print(f"Original D3D9 replay return={process.returncode}, frames={len(images)}: {output}")
    return process.returncode


if __name__=="__main__":
    raise SystemExit(main())
