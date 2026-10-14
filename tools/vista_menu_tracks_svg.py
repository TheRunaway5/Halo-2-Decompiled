"""Authored linear Vista timing rails and grouped index marks.

The rail/tick spacings and group breaks were measured from the native pages.
Alien lettering in the other track banks is not replaced with invented bars.
"""
from vista_prompt_svg import element


PREFIX='ui\\global_bitmaps\\'


def tracks(d,name):
    if not name.startswith(PREFIX):return False
    key=name[len(PREFIX):]
    if key not in ('track7a','track7b','track7c','track7d','track_timeline2a','track_timeline2b'):
        return False
    d.family='menu-timing-rail'
    blue='#314d9c'
    if key=='track7a':
        for y in (0,7):element(d.root,'rect',y=y,width=2048,height=3,fill=blue,fill_opacity=45/255)
    elif key=='track7b':
        # Paired timing marks repeat after 1024 units. Preserve each pause
        # within the group rather than replacing the cadence with a uniform
        # dashed stroke. The blue baseline has two paint levels.
        group=element(d.root,'g',fill=blue)
        for repeat in (0,1024):
            for x in (0,18,216,293,310,424,440,696,713,750,827,844,958):
                element(group,'path',d=f'M{x+repeat} 0 H{x+repeat+12} V3 H{x+repeat} Z',
                        fill_opacity=21/255)
    elif key=='track7c':
        group=element(d.root,'g',fill=blue,fill_opacity=46/255)
        # Seven marks per 160-unit phrase, then a short phase adjustment in
        # the last four phrases. The final phrase is not stretched to fit.
        phrases=((8,7),(168,7),(328,7),(488,7),(648,7),(808,7),
                 (968,7),(1128,7),(1288,6),(1428,7),(1588,7),(1750,5),(1870,7))
        for start,count in phrases:
            for i in range(count):element(group,'rect',x=start+i*20,width=2,height=14)
    elif key=='track7d':
        group=element(d.root,'g',fill=blue,fill_opacity=34/255)
        for start,end in ((298,982),(1028,1216),(1248,1492),(1574,1658),
                          (1702,1714),(1750,1762),(1808,2004),(2032,2044)):
            for x in range(start,end,8):element(group,'rect',x=x,width=4,height=10)
    elif key=='track_timeline2a':
        # A continuous upper rail and a regular lower step, with a wider
        # fourth mark every 512 units.
        element(d.root,'rect',y=6,width=2048,height=3,fill=blue,fill_opacity=34/255)
        element(d.root,'rect',y=9,width=2048,height=5,fill=blue,fill_opacity=3/255)
        for x in range(128,2049,128):
            width=18 if x%512==0 else 6
            element(d.root,'rect',x=x-width,y=8,width=width,height=6,fill=blue,fill_opacity=41/255)
    else:
        group=element(d.root,'g',fill=blue)
        element(group,'rect',y=11,width=2048,height=3,fill_opacity=32/255)
        for x in range(15,2048,16):
            element(group,'rect',x=x-.5,y=5,width=1,height=5,fill_opacity=5/255)
    return True
