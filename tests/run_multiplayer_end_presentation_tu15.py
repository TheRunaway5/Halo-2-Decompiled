"""Actual source-world end/round GPU proof, using only scratch-owned raw frames."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    binary,map_path,preset=sys.argv[1:4]
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    output=scratch/'end-presentation';output.mkdir()
    layout_path=ROOT/'tests/fixtures/team_full_scoreboard_original.json'
    status_path=ROOT/'tests/fixtures/player_status_tu15_original.json'
    original=json.loads(layout_path.read_text());status=json.loads(status_path.read_text())
    assert original['source_xbe_sha256']==status['source_sha256']==PIN
    layouts={}
    for case in original['cases']:
        raw=case['input'];key=(raw['team_mode'],raw['game_over'],bool(raw['round_limit']),raw['score_limit']==1)
        if key in layouts:continue
        headers={name:{k:(bool(v) if k in ('valid','right_justified') else v)
                       for k,v in widget.items()} for name,widget in case['output']['headers'].items()}
        layouts[key]=dict(team_mode=key[0],game_over=key[1],rounds=key[2],score_one=key[3],
            columns=list(map(bool,case['output']['columns'])),title=case['output']['title'],headers=headers,
            original_case=case['name'])
    controls=dict(source_sha256=PIN,labels=status['lifecycle_labels'],layouts=list(layouts.values()),
        original_fixtures={str(p.relative_to(ROOT)):sha(p) for p in (layout_path,status_path)})
    compact=output/'original-controls.json';compact.write_text(json.dumps(controls)+'\n')
    del original,status,layouts
    report=output/'report.json'
    command=[binary,map_path,str(ROOT/'tests/fixtures/multiplayer_engines_original_tu15.json'),str(compact),str(report),str(output),preset]
    result=subprocess.run(command,timeout=600)
    if result.returncode==77:return 77
    value=json.loads(report.read_text()) if report.is_file() else dict(passed=False)
    value.update(returncode=result.returncode,command=command,original_controls=controls['original_fixtures'],
        input_sha256={str(p):sha(p) for p in (Path(binary),Path(map_path),ROOT/'tests/fixtures/multiplayer_engines_original_tu15.json')},
        raw_captures=[dict(file=p.name,bytes=p.stat().st_size,sha256=sha(p)) for p in sorted(output.glob('*.ppm'))])
    destination=os.environ.get('HALO2_ONLINE_FLOW_EVIDENCE')
    if destination:
        target=Path(destination)
        if target.suffix:target=target.with_name(target.stem+'-end-'+preset.lower().replace(' ','-')+'.json')
        else:target=target/('end-'+preset.lower().replace(' ','-')+'.json')
        target.parent.mkdir(parents=True,exist_ok=True)
        from PIL import Image,ImageDraw
        retained=[]
        for p in sorted(output.glob('*.ppm')):
            image=Image.open(p);image.load();png=target.parent/(target.stem+'-'+p.stem+'.png');image.save(png)
            size=png.stat().st_size
            total=sum(q.stat().st_size for q in target.parent.glob('*.png'))
            if size>1024*1024 or total>8*1024*1024:png.unlink();continue
            retained.append(dict(path=str(png),bytes=size,sha256=sha(png),raw_ppm_sha256=sha(p)))
        value['retained_pngs']=retained;value['retained_large_artifacts']=[]
        target.write_text(json.dumps(value,indent=2)+'\n')
        print(json.dumps(dict(evidence=str(target),passed=value['passed'],retained_pngs=len(retained))))
    return result.returncode
if __name__=='__main__':sys.exit(main())
