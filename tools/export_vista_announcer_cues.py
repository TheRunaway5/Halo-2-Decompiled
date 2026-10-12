#!/usr/bin/env python3
"""Export actual shared-mulg announcer references; optional offline WAV decoding.

The bounded source family is the supplied 11081 PC map/executable, with three
proven response selections (general19/general31/Slayer0, audience4). Payloads
are found through snd!/ugh! metadata, never naked retained resource offsets.
No installer, game, audio device, queue or live import is activated.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import mmap
from pathlib import Path
import struct
import subprocess

SOURCE_SHA="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109"
SHARED_SHA="6c0afbcf46ecd3a70ef5106e4f51d68414d5a389c1e0b1d14c47a2766abddfe0"
BUILD="11081.07.04.30.0934.main"
LANGUAGES=("english","japanese","german","french","spanish","italian","korean","chinese","portuguese")
CUES=(("game_over",0x98,0,19,"sound\\dialog\\multiplayer\\general\\misc\\game_over"),
      ("round_over",0x98,0,31,"sound\\dialog\\multiplayer\\general\\misc\\round_over"),
      ("slayer",0xA8,2,0,"sound\\dialog\\multiplayer\\games\\names\\slayer"))

def sha(data):return hashlib.sha256(data).hexdigest()
def file_sha(path):
    h=hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda:f.read(1024*1024),b""):h.update(block)
    return h.hexdigest()
def require(value,reason):
    if not value:raise ValueError(reason)
def stable_write(path,data):
    path.parent.mkdir(parents=True,exist_ok=True)
    if path.exists():require(path.read_bytes()==data,f"refusing different existing output: {path}")
    else:path.write_bytes(data)
    require(path.read_bytes()==data,"export readback mismatch")

class PcMap:
    def __init__(self,path):
        self.path=path.resolve();self.file=self.path.open("rb");self.data=None
        try:
            self.data=mmap.mmap(self.file.fileno(),0,access=mmap.ACCESS_READ)
            self.require(0,2048)
            require((self.u32(0),self.u32(4),self.u32(2044))==(0x68656164,8,0x666f6f74),"not a version8 PC map")
            require(self.u32(8)==len(self.data) and self.string(0x12c,32)==BUILD,"unsupported PC build/extent")
            self.index=self.u32(16);self.require(self.index,32)
            count=self.u32(self.index+24)
            require(0<count<=65536 and count==self.u32(0x2cc),"invalid PC tag count")
            table=self.pointer(self.u32(self.index+8),count*16)
            names,size,offsets=self.u32(0x2d0),self.u32(0x2d4),self.u32(0x2d8)
            self.require(names,size);self.require(offsets,count*4);self.tags={}
            for i in range(count):
                at=table+i*16;group=self.u32(at)
                if group==0xffffffff:continue # shared.map contains null slots
                name=self.u32(offsets+i*4);require(name<size,"tag name outside string bank")
                datum,address,length=self.u32(at+4),self.u32(at+8),self.u32(at+12)
                require(datum not in self.tags,"duplicate actual tag datum")
                if address:self.pointer(address,length)
                self.tags[datum]={"datum":f"{datum:08x}","group":group.to_bytes(4,"big").decode("ascii"),
                    "name":self.string(names+name,size-name),"address":address,"bytes":length,
                    "tag_table_offset":at,"file_offset":self.index+address if address else None}
        except Exception:
            self.close();raise
    def close(self):
        if self.data is not None:self.data.close();self.data=None
        self.file.close()
    def require(self,p,n):require(0<=p<=len(self.data) and 0<=n<=len(self.data)-p,"PC map bounds")
    def u8(self,p):self.require(p,1);return self.data[p]
    def u16(self,p):self.require(p,2);return struct.unpack_from("<H",self.data,p)[0]
    def u32(self,p):self.require(p,4);return struct.unpack_from("<I",self.data,p)[0]
    def string(self,p,n):
        self.require(p,n);raw=self.data[p:p+n];end=raw.find(b"\0")
        require(end>=0,"unterminated PC map string");return raw[:end].decode("utf-8")
    def pointer(self,p,n):at=self.index+p;self.require(at,n);return at
    def block(self,p,stride):
        count=self.u32(p);require(count<=1000000,"PC block count cap")
        if not count:return []
        at=self.pointer(self.u32(p+4),count*stride)
        return [at+i*stride for i in range(count)]
    def one(self,group,name=None):
        rows=[t for t in self.tags.values() if t["group"]==group and t["address"] and (name is None or t["name"]==name)]
        require(len(rows)==1,f"expected one actual {group} tag: {name}");return rows[0]
    def sound(self,datum):
        require(datum in self.tags,"sound reference is not in actual tag bank")
        tag=self.tags[datum];require(tag["group"]=="snd!" and tag["address"] and tag["bytes"]>=20,"invalid sound tag")
        return tag

def encoded_chunks(cache,tag,maps):
    s=tag["file_offset"];g=cache.one("ugh!")["file_offset"]
    cache.require(s,20);cache.require(g,72)
    rate,encoding,compression=cache.u8(s+3),cache.u8(s+4),cache.u8(s+5)
    require(rate<=2 and encoding==2 and compression==4,"selected cue outside proved PC serialized WMA family")
    cls=cache.u8(s+2);require(cls<54,"invalid source sound class")
    ranges=cache.block(g+32,12);perms=cache.block(g+40,16);chunks=cache.block(g+64,12)
    first,pitch_count=cache.u16(s+8),cache.u8(s+10)
    require(pitch_count>0 and first+pitch_count<=len(ranges),"source sound pitch range extent")
    meta={"tag":tag,"snd_record_hex":cache.data[s:s+20].hex(),"sound_flags":cache.u16(s),
          "sound_class":cls,"rate_index":rate,"sample_rate":(22050,44100,32000)[rate],
          "encoding":encoding,"compression":compression,"channels":2,"pitch_ranges":[]}
    for pitch in range(pitch_count):
        r=ranges[first+pitch];pi,pc=cache.u16(r+8),cache.u16(r+10)
        require(pc>0 and pi+pc<=len(perms),"source permutation extent")
        row={"pitch":pitch,"gestalt_index":first+pitch,"file_offset":r,
             "record_hex":cache.data[r:r+12].hex(),"permutations":[]}
        for perm in range(pc):
            p=perms[pi+perm];ci,cc=cache.u16(p+12),cache.u16(p+14)
            require(cc>0 and ci+cc<=len(chunks),"source chunk extent")
            out={"permutation":perm,"gestalt_index":pi+perm,"file_offset":p,
                 "record_hex":cache.data[p:p+16].hex(),"source_sample_count":cache.u32(p+8),"chunks":[]}
            for n in range(cc):
                at=chunks[ci+n];pointer,sizeword=cache.u32(at),cache.u32(at+4)
                selector,offset=pointer>>30,pointer&0x3fffffff;length=sizeword&0x3fffffff
                resource=cache if selector==0 else maps.get(selector)
                require(resource is not None,"unprovided external sound resource map")
                require(0<length<=128*1024*1024,"sound resource size cap");resource.require(offset,length)
                payload=resource.data[offset:offset+length]
                out["chunks"].append({"chunk":n,"gestalt_index":ci+n,"file_offset":at,
                    "record_hex":cache.data[at:at+12].hex(),"resource_pointer":f"{pointer:08x}",
                    "resource_selector":selector,"resource_map":str(resource.path),"resource_offset":offset,
                    "size_word":f"{sizeword:08x}","bytes":length,"sha256":sha(payload),"_payload":payload})
            row["permutations"].append(out)
        meta["pitch_ranges"].append(row)
    return meta

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--maps",type=Path,required=True)
    p.add_argument("--exe",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--languages",choices=("english","all"),default="english")
    mode=p.add_mutually_exclusive_group(required=True)
    mode.add_argument("--metadata-only",action="store_true")
    mode.add_argument("--decoder",type=Path)
    args=p.parse_args();require(file_sha(args.exe)==SOURCE_SHA,"unexpected supplied PC executable")
    require(file_sha(args.maps/"shared.map")==SHARED_SHA,"source-qualified export requires pinned shared.map")
    require(not (args.output/"manifest.json").exists(),"use a fresh export directory; manifest already exists")
    maps={};manifest={"version":"H2V_ANNOUNCER_CUE_BUNDLE_V1","complete":False,
        "scope":"actual general19/general31/Slayer0 audience4 base responses; no event/queue/mixer activation",
        "exe":{"path":str(args.exe.resolve()),"sha256":SOURCE_SHA},"source_maps":[],
        "tool_sha256":file_sha(Path(__file__)),"language_mode":args.languages,"decoded":bool(args.decoder),
        "boundaries":["Serialized chunk files retain every original byte",
          "All pitch/permutation/chunk choices are exported, never selected by a guessed RNG",
          "Chunks are decoded independently; no unproved cross-chunk concatenation/stream timing",
          "WAV48k stereo PCM is native FFmpeg output, not a Microsoft DMO or mixer/device equality claim"],"cues":[]}
    if args.decoder:
        args.decoder=args.decoder.resolve();require(args.decoder.is_file(),"decoder CLI missing")
        manifest["decoder"]={"path":str(args.decoder),"sha256":file_sha(args.decoder)}
    try:
        maps[1]=PcMap(args.maps/"mainmenu.map");maps[2]=PcMap(args.maps/"shared.map")
        for selector,cache in maps.items():manifest["source_maps"].append({"selector":selector,"path":str(cache.path),
            "bytes":len(cache.data),"sha256":file_sha(cache.path),"build":BUILD})
        shared=maps[2];mulg=shared.one("mulg","multiplayer\\multiplayer_globals")
        runtime=shared.block(mulg["file_offset"]+8,0x568)
        require(len(runtime)==1,"actual shared multiplayer runtime bank is not unique")
        for cue,bank,event_type,subtype,expected_name in CUES:
            candidates=[(i,at) for i,at in enumerate(shared.block(runtime[0]+bank,0xA8))
                        if shared.u16(at+2)==event_type and shared.u16(at+4)==subtype and
                           shared.u16(at+6)==4 and shared.u16(at+0x10)==0 and shared.u16(at+0x12)==0]
            require(len(candidates)==1,f"source response selection is not unique: {cue}")
            ordinal,at=candidates[0]
            require(shared.u16(at+0x40)&1 and not shared.block(at+0xA0,0x50),"response is not the observed base announcer family")
            refs=[]
            for lang,name in enumerate(LANGUAGES):
                group,datum=shared.u32(at+0x44+lang*8),shared.u32(at+0x48+lang*8)
                if datum==0xffffffff:refs.append({"language":name,"datum":"ffffffff","sound":None});continue
                require(group==0x736e6421,"response does not reference snd!")
                sound=shared.sound(datum)
                refs.append({"language":name,"datum":f"{datum:08x}","sound":sound["name"]})
            require(refs[0]["sound"]==expected_name,"actual English response cue identity changed")
            entry={"cue":cue,"mulg":mulg,"runtime_offset":runtime[0],"response_bank_offset":bank,
                "response_ordinal":ordinal,"response_offset":at,"event_type":event_type,"subtype":subtype,
                "audience":4,"delay_float_word":f"{shared.u32(at+0x3c):08x}","sound_flags":shared.u16(at+0x40),
                "response_record_hex":shared.data[at:at+0xA8].hex(),"language_references":refs,"exports":[]}
            manifest["cues"].append(entry)
            for lang,ref in enumerate(refs):
                if ref["sound"] is None or (args.languages=="english" and lang!=0):continue
                sound=encoded_chunks(shared,shared.sound(int(ref["datum"],16)),maps)
                sound["language"]=ref["language"];entry["exports"].append(sound)
                for pitch in sound["pitch_ranges"]:
                    for perm in pitch["permutations"]:
                        for chunk in perm["chunks"]:
                            relative=Path(cue)/ref["language"]/f"pitch-{pitch['pitch']}"/f"permutation-{perm['permutation']}"/f"chunk-{chunk['chunk']}.auds"
                            payload=chunk.pop("_payload");stable_write(args.output/relative,payload)
                            chunk["serialized_file"]=relative.as_posix()
                            if not args.decoder:continue
                            wav=relative.with_suffix(".wav")
                            command=[str(args.decoder),"--input",str((args.output/relative).resolve()),
                                "--output",str((args.output/wav).resolve()),"--rate",str(sound["sample_rate"]),
                                "--channels",str(sound["channels"]),"--class",str(sound["sound_class"]),
                                "--name",sound["tag"]["name"]]
                            run=subprocess.run(command,check=True,capture_output=True,text=True)
                            chunk["decoded_stats"]=json.loads(run.stdout)
                            chunk["wav"]={"file":wav.as_posix(),"bytes":(args.output/wav).stat().st_size,
                                          "sha256":file_sha(args.output/wav)}
        manifest["complete"]=True
        stable_write(args.output/"manifest.json",(json.dumps(manifest,indent=2)+"\n").encode())
        print(json.dumps({"complete":True,"cues":len(manifest["cues"]),
              "sounds":sum(len(c["exports"]) for c in manifest["cues"]),"decoded":bool(args.decoder),
              "manifest_sha256":file_sha(args.output/"manifest.json")},indent=2))
    except Exception as e:
        args.output.mkdir(parents=True,exist_ok=True)
        (args.output/"failure.json").write_text(json.dumps({"complete":False,"error":str(e)},indent=2)+"\n")
        raise
    finally:
        for cache in maps.values():cache.close()
if __name__=="__main__":main()
