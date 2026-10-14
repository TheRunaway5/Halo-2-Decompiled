#!/usr/bin/env python3
"""Check reflection of authored menu contours, independently of raster rounding."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import re
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image


def paths(data):
    tokens=re.findall(r'[A-Za-z]|[-+]?(?:\d*\.\d+|\d+\.?\d*)(?:[eE][-+]?\d+)?',data)
    at=0;point=(0.,0.);start=None;segments=[];result=[]
    while at<len(tokens):
        command=tokens[at];at+=1
        if command=='Z':
            if point!=start:segments.append(('L',(point,start)))
            result.append(segments);segments=[];point=start;continue
        count={'M':2,'L':2,'H':1,'V':1,'Q':4,'C':6}[command]
        values=list(map(float,tokens[at:at+count]));at+=count
        if command=='M':
            assert not segments,'Open contour in symmetry check'
            point=tuple(values);start=point;continue
        if command=='H':points=[(values[0],point[1])];command='L'
        elif command=='V':points=[(point[0],values[0])];command='L'
        else:points=list(zip(values[::2],values[1::2]))
        segments.append((command,tuple([point]+points)));point=points[-1]
    assert not segments,'Only closed contours can be checked'
    return result


def canonical(segments):
    segments=tuple((kind,tuple((round(x,7),round(y,7)) for x,y in points)) for kind,points in segments)
    reverse=tuple((kind,tuple(reversed(points))) for kind,points in reversed(segments))
    return min(sequence[i:]+sequence[:i] for sequence in (segments,reverse) for i in range(len(sequence)))


def check_contour(data,origin,direction):
    dx,dy=direction;length=math.hypot(dx,dy);dx/=length;dy/=length
    def reflect(point):
        x,y=point[0]-origin[0],point[1]-origin[1];along=x*dx+y*dy
        return origin[0]+2*along*dx-x,origin[1]+2*along*dy-y
    source=paths(data)
    reflected=[[(kind,tuple(reflect(p) for p in points)) for kind,points in contour] for contour in source]
    assert Counter(map(canonical,source))==Counter(map(canonical,reflected)),'Contour is not an exact reflection'


def verify(pack):
    report=json.loads((pack/'reconstruction.json').read_text());checks=[];rasters=[]
    for row in report['records']:
        if row['cache']!='mainmenu.map':continue
        name=row['name'];frame=row['frame'];art=row['reconstruction'];targets=[]
        if name=='ui\\screens\\game_shell\\pregame_lobby\\download_progress' and 1<=frame<=12:
            targets=[('download-outline',(21,0),(0,1))]
            if frame>1:targets.append(('download-white-phase',(21,0),(0,1)))
        elif name=='ui\\screens\\game_shell\\network_squad_browser_screen\\arrows' and frame in (1,2):
            targets=[('browser-scroll-arrow-foreground',(26,0),(0,1))]
        elif name in ('ui\\global_bitmaps\\live_icons','ui\\global_bitmaps\\live_icons_sm'):
            if frame in (7,18,31) or 20<=frame<=28:targets=[('duke-controller-body',(28,0),(0,1))]
            elif frame==29:targets=[('party-crown',(21,0),(0,1))]
            if frame in (2,7,31):targets.append(('orbit-arrowhead',None,None))
        if not targets:continue
        root=ET.parse(pack/art['vector']).getroot()
        for identity,origin,direction in targets:
            node=next(n for n in root.iter() if n.get('id')==identity)
            if origin is None:
                edges=paths(node.get('d'))[0]
                origin=edges[0][1][0];left=edges[0][1][-1];right=edges[2][1][-1]
                direction=((left[0]+right[0])/2-origin[0],(left[1]+right[1])/2-origin[1])
            check_contour(node.get('d'),origin,direction)
            checks.append(dict(name=name,frame=frame,contour=identity,reflection='exact'))
        if name.endswith('download_progress') or name.endswith('\\arrows') or (name.endswith('live_icons') and frame==18):
            image=np.asarray(Image.open(pack/art['dds']).convert('RGBA')).astype(float)
            paint=np.dstack((image[:,:,:3]*image[:,:,3,None]/255,image[:,:,3]))
            difference=abs(paint-paint[:,::-1])
            rasters.append(dict(name=name,frame=frame,max_premultiplied_channel_difference=float(difference.max()),
                mean_premultiplied_channel_difference=float(difference.mean())))
    assert len(checks)==57,'Missing expected normal/small reflection checks'
    return dict(passed=True,vector_contours_checked=len(checks),checks=checks,raster_measurements=rasters,
        scope='Authored contours reflect exactly. Raster measurements separately retain Cairo filter/coverage rounding; no bitwise pixel-symmetry claim.')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pack',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();result=verify(args.pack)
    args.report.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('checks','raster_measurements')},indent=2))


if __name__=='__main__':main()
