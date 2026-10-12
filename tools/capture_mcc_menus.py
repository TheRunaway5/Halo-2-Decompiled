#!/usr/bin/env python3
"""Capture selected MCC menu pack through both installed native launchers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,default=ROOT/"analysis/multiplayer/mcc-menus/captures")
    parser.add_argument("--resume",action="store_true",help="Reuse completed captures after verifying their source pins and image hashes")
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    info=subprocess.check_output([str(ROOT/"Halo2-Title-Launcher"),"--launcher-info"],text=True)
    fields=dict(line.split(": ",1) for line in info.splitlines() if ": " in line)
    runtime=Path(fields["Runtime"]);pack=Path(fields["Selected menus/fonts"])
    assert (pack/"menu-source.tsv").read_text()=="H2_MCC_CLASSIC_MENUS_V1\n"
    pins={str(pack/"manifest.json"):sha(pack/"manifest.json")}
    # Optional Vista prompt redraws are independent of the original MCC
    # import manifest and must be pinned separately in rendered evidence.
    prompt_binding=pack/"prompt-source.tsv"
    if prompt_binding.is_file():
        prompt_pack=Path(prompt_binding.read_text().splitlines()[1])
        for path in (prompt_binding,prompt_pack/"prompts.tsv",prompt_pack/"reconstruction.json"):
            pins[str(path)]=sha(path)
    env=os.environ.copy();env.update(SDL_VIDEODRIVER="x11",HALO2_MENU_FOG_SEED="305419896",
        XDG_DATA_HOME=str(out/"capture-profiles"),
        VK_LAYER_PATH=str(ROOT/".tools/validation/usr/share/vulkan/explicit_layer.d"),
        LD_LIBRARY_PATH=str(ROOT/".tools/validation/usr/lib")+(":"+env["LD_LIBRARY_PATH"] if env.get("LD_LIBRARY_PATH") else ""))
    cases=[("title",9), ("main",6), ("settings",19), ("appearance",47), ("lobby",14),
        ("pc-settings",65536+258), ("video",65536+256), ("keyboard",167)]
    report=dict(schema="mcc-menu-captures-v2",passed=False,launcher_info=info,pins=pins,runs=[])
    if args.resume and (out/"captures.json").is_file():
        report=json.loads((out/"captures.json").read_text())
        previous_pins=report["pins"]
        assert all(previous_pins.get(p)==value for p,value in pins.items()),"Completed captures use different asset inputs"
        if report.get("schema")!="mcc-menu-captures-v2":
            original=dict(line.split(": ",1) for line in report["launcher_info"].splitlines() if ": " in line)
            for row in report["runs"]:
                row["runtime"]=original["Runtime"]
                row["runtime_sha256"]=previous_pins[row["runtime"]]
                row["installed_launcher"]=row["command"][4]
                row["launcher_sha256"]=previous_pins[row["installed_launcher"]]
            report["schema"]="mcc-menu-captures-v2"
            report["original_capture_pins"]=previous_pins
        report["pins"]=pins
        expected=dict(cases)
        report["runs"]=[row for row in report["runs"] if expected.get(row["name"])==row["screen"]]
        for row in report["runs"]:
            assert sha(Path(row["runtime"]))==row["runtime_sha256"],"Completed capture runtime changed"
            assert sha(Path(row["image"]))==row["sha256"] and not row["validation_errors"] and row["returncode"]==0
            with Image.open(row["image"]) as rendered:assert rendered.size==(3840,2160)
    completed={row["name"] for row in report["runs"]}
    for index,(name,screen) in enumerate(cases):
        if name in completed:continue
        installed=ROOT/("Halo2-Title-Launcher" if index%2==0 else "Halo2-Launcher")
        # Concurrent chats may install another runtime. Retain the exact
        # installed launcher bytes for each capture and record its own runtime.
        snapshots=out/"launcher-snapshots"/name;snapshots.mkdir(parents=True,exist_ok=True)
        launcher=snapshots/installed.name
        launcher.write_bytes(installed.read_bytes());launcher.chmod(0o755)
        launcher_sha=sha(launcher)
        selected=subprocess.check_output([str(launcher),"--launcher-info"],cwd=ROOT,text=True)
        selected=dict(line.split(": ",1) for line in selected.splitlines() if ": " in line)
        assert Path(selected["Selected menus/fonts"])==pack,"Installed launcher does not select the verified menu pack"
        runtime=Path(selected["Runtime"]);runtime_sha=sha(runtime)
        image=out/(name+"-4k.ppm")
        command=["xvfb-run","-a","-s","-screen 0 3840x2160x24",str(launcher),
            "--skip-intro","--freeze-menu","--menu-tick","181","--menu-screen",str(screen),
            "--frames","3","--menu-ui-ms","5000","--viewport","3840","2160",
            "--hidden","--mute","--validation","--screenshot",str(image)]
        with (out/(name+".log")).open("w") as log:
            result=subprocess.run(command,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=180)
        text=(out/(name+".log")).read_text()
        errors=re.findall(r"VUID-|Validation Error|Fatal|cannot initialize",text)
        assert result.returncode==0 and not errors,(name,result.returncode,errors)
        with Image.open(image) as rendered:
            assert rendered.size==(3840,2160)
            rendered.save(image.with_suffix(".png"))
        image.unlink()
        assert {p:sha(Path(p)) for p in pins}==pins,"Capture inputs changed"
        assert sha(runtime)==runtime_sha and sha(launcher)==launcher_sha,"Capture binary changed"
        report["runs"].append(dict(name=name,screen=screen,command=command,returncode=result.returncode,
            validation_errors=errors,image=str(image.with_suffix(".png")),sha256=sha(image.with_suffix(".png")),
            installed_launcher=str(installed),launcher_snapshot=str(launcher),launcher_sha256=launcher_sha,
            runtime=str(runtime),runtime_sha256=runtime_sha))
        (out/"captures.json").write_text(json.dumps(report,indent=2)+"\n")
        print(name+": 3840x2160; zero validation errors",flush=True)
    report["runs"].sort(key=lambda row:list(dict(cases)).index(row["name"]))
    sheet=Image.new("RGB",(1920,620),(8,14,24));draw=ImageDraw.Draw(sheet)
    for index,row in enumerate(report["runs"]):
        x=(index%4)*480;y=(index//4)*310
        with Image.open(row["image"]) as image:image.thumbnail((480,270));sheet.paste(image,(x,y+35))
        draw.text((x+12,y+12),row["name"]+" (4K)",fill="white")
    sheet.save(out/"menu-overview.png")
    report["passed"]=True
    report["claim"]="Eight 3840x2160 native menu captures with source-verified MCC samples; no retail framebuffer parity claim"
    (out/"captures.json").write_text(json.dumps(report,indent=2)+"\n")


if __name__=="__main__":
    main()
