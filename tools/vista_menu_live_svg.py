"""Manually drawn Vista Live-menu glyphs, including both numbered icon banks.

The 56-unit outlines were plotted against the original normal bank. Small and
28-unit list variants keep their separate padding; extra width for 9+ never
stretches the circular border. Numerals are paths, not a substituted font.
"""
from vista_prompt_svg import element


LIVE_DIGITS = {
    '2': 'M0 3 Q0 0 3 0 H5 Q8 0 8 3 V5 Q8 6.5 6.5 8 L2 11 V12 H8 V14 H0 V10.5 L5.5 6 Q6 5.5 6 4 V3 Q6 2 5 2 H3 Q2 2 2 3 V4 H0 Z',
    '3': 'M0 0 H6 Q8 0 8 2 V5 Q8 6.5 6.5 7 Q8 7.5 8 9 V12 Q8 14 6 14 H0 V12 H5 Q6 12 6 11 V9 Q6 8 5 8 H2 V6 H5 Q6 6 6 5 V3 Q6 2 5 2 H0 Z',
    '4': 'M5 0 H8 V8 H9 V10 H8 V14 H6 V10 H0 V7 Z M6 2.5 L2 8 H6 Z',
    '5': 'M0 0 H8 V2 H2 V5 H6 Q8 5 8 7 V12 Q8 14 6 14 H0 V12 H5 Q6 12 6 11 V8 Q6 7 5 7 H0 Z',
    '6': 'M2 0 H7 V2 H3 Q2 2 2 3 V5 H6 Q8 5 8 7 V12 Q8 14 6 14 H2 Q0 14 0 12 V2 Q0 0 2 0 Z M2 7 V12 H6 V7 Z',
    '7': 'M0 0 H8 V3 L4 14 H1 L6 3 V2 H0 Z',
    '8': 'M2 0 H6 Q8 0 8 2 V5 Q8 6.5 6.5 7 Q8 7.5 8 9 V12 Q8 14 6 14 H2 Q0 14 0 12 V9 Q0 7.5 1.5 7 Q0 6.5 0 5 V2 Q0 0 2 0 Z M2 2 V6 H6 V2 Z M2 8 V12 H6 V8 Z',
    '9': 'M2 0 H6 Q8 0 8 2 V12 Q8 14 6 14 H1 V12 H5 Q6 12 6 11 V9 H2 Q0 9 0 7 V2 Q0 0 2 0 Z M2 2 V7 H6 V2 Z',
}


def person(parent, transform=None):
    group=element(parent,'g',**({'transform':transform} if transform else {}))
    element(group,'circle',cx=30,cy=15.5,r=4.5)
    element(group,'path',d='M20.5 21.5 H39.5 L23 43 Z')


def controller(parent):
    # Original Xbox Duke profile. Mirrored shoulders and round grip tips,
    # with the wide shallow inner arch, rather than a notched block/pad.
    element(parent,'path',id='duke-controller-body',d='M28 16.8 C24.5 16.8 23.5 15.5 21.5 17 '
            'C17.5 18.5 15.7 22 14.6 27 C13.9 30.2 13.8 34 14.5 37.5 '
            'C14.9 39.8 17.2 40 18.6 38 L22.8 32.8 Q24 31.2 28 31.3 '
            'Q32 31.2 33.2 32.8 L37.4 38 C38.8 40 41.1 39.8 41.5 37.5 '
            'C42.2 34 42.1 30.2 41.4 27 C40.3 22 38.5 18.5 34.5 17 '
            'C32.5 15.5 31.5 16.8 28 16.8 Z')
    element(parent,'circle',id='duke-controller-badge',cx=28,cy=23,r=5.6,
            fill='#91c4f3',stroke='#c3dff9',stroke_width=.65)
    # Curved, flared Xbox cross, inset within the circular guide badge. The
    # same paths serve the orbit, unnumbered and every numbered controller.
    element(parent,'path',id='duke-controller-x',d='M24.2 19.1 '
            'C25.6 19.4 27 20.5 28 21.5 C29 20.5 30.4 19.4 31.8 19.1 '
            'C30.7 20.3 29.7 21.6 29 23 C29.7 24.4 30.7 25.7 31.8 26.9 '
            'C30.4 26.6 29 25.5 28 24.5 C27 25.5 25.6 26.6 24.2 26.9 '
            'C25.3 25.7 26.3 24.4 27 23 C26.3 21.6 25.3 20.3 24.2 19.1 Z',
            fill='white',stroke='none')


def sound(parent,waves=False):
    # The round rear cap shares the horn's straight edge. A semicircular arc
    # in the same contour avoids a detached dot or an internal seam.
    if waves:
        path=('M7 17 H15 V39 H7 Z M17 17 L27.5 8 Q30 6 30 9 '
              'V24 A4 4 0 0 1 30 32 V47 '
              'Q30 50 27.5 48 L17 39 Z')
    else:
        path=('M12 17 H20 V40 H12 Z M22 17 L31.5 8 Q34 6 34 9 '
              'V24 A4 4 0 0 1 34 32 V47 '
              'Q34 50 31.5 48 L22 40 Z')
    element(parent,'path',id='speaker-horn',d=path)
    sound_divider(parent,30 if waves else 34,9,47)
    if waves:
        for p in ('M38 19 C45 24 45 34 38 39','M42 13 C53 21 53 37 42 45'):
            element(parent,'path',d=p,fill='none',stroke='white',stroke_width=2.5)


def sound_divider(parent,x,top,bottom):
    # Vista keeps the horn's mouth edge visible through its round rear cap.
    element(parent,'path',id='speaker-mouth-divider',d=f'M{x} {top} V{bottom}',
            fill='none',stroke='#639ac5',stroke_width=.7,stroke_linecap='round')


def orbit(parent):
    element(parent,'path',d='M11 33 C9 38 7 44 8 49 Q10 54 26 48 '
            'C35 44 42 38 46 30 L42 29.5 C32 40 17 49 12 46 '
            'Q11 41 14 36 Z')
    # A symmetric chevron around the measured diagonal direction. Its base
    # corners are reflections across the tip-to-base-centre axis.
    import math
    tip=(50,24);base=(38.5,37);length=math.hypot(base[0]-tip[0],base[1]-tip[1])
    dx=(base[0]-tip[0])/length;dy=(base[1]-tip[1])/length
    left=(base[0]+dy*6.02,base[1]-dx*6.02)
    right=(base[0]-dy*6.02,base[1]+dx*6.02)
    notch=(tip[0]+dx*12.8,tip[1]+dy*12.8)
    element(parent,'path',id='orbit-arrowhead',d=f'M{tip[0]} {tip[1]} L{left[0]} {left[1]} '
            f'L{notch[0]} {notch[1]} L{right[0]} {right[1]} Z')


def live_icon(d,name):
    live=name in ('ui\\global_bitmaps\\live_icons','ui\\global_bitmaps\\live_icons_sm')
    compact=name=='ui\\list_skins\\y_menu_list\\hacky_beta_hack_dont_use'
    top=name=='ui\\list_skins\\y_menu_list\\top_item'
    if not (live or compact or top):return False
    frame=d.frame
    if compact:
        frame={0:0,1:1,2:2,3:3,4:30,5:5,6:19}.get(frame,-1)
    if top:
        if frame not in (0,1):return False
        d.family='live-list-category-icon'
        element(d.root,'rect',width=d.width,height=d.height,fill='#4197d6')
        g=element(d.root,'g',fill='white',filter=d.glow('category-edge-softness',.24))
        element(g,'circle',cx=19.5,cy=19.5,r=15,fill='none',stroke='white',stroke_width=1.7)
        if frame==0:
            element(g,'circle',cx=20,cy=11.5,r=2.7)
            element(g,'path',d='M13 17 H26 L15 31.5 Z')
        else:
            # Vista's white shoulder band continues around the upper-right
            # quarter. Its blue counter narrows at the waist; the lower-left
            # counter starts at the same horizontal quarter boundary.
            element(g,'path',id='category-shield',d='M10.8 11.5 L12.7 9.25 '
                'C14.5 10.6 16.7 10.6 19 9.35 C21.3 10.6 23.5 10.6 25.3 9.25 '
                'L28.3 11.45 C26.65 13.2 26.05 14.35 26.4 16 '
                'C26.55 17.1 27.4 18.4 28 19.95 C29.05 22.6 28.15 24.6 26.15 26.15 '
                'C24.5 27.65 21.3 29.75 19.35 30.5 C17.4 29.2 14.65 27.7 12.65 25.75 '
                'C10.35 23.8 10.3 22.1 11.25 19.6 C12.3 17.15 13.15 15.65 12.3 13.65 Z '
                'M19 12 H25.4 C24 13.5 23.7 15.1 24.3 16.6 L25.2 19 H19 Z '
                'M12.9 19 H19 V27.85 C16.8 26.7 14.65 25.4 13.3 23.9 '
                'Q11.75 22.4 12.9 19 Z',fill_rule='evenodd')
        return True
    if frame not in (*range(0,32),):return False
    d.family='live-menu-status-icon'
    if frame in (7,18,31) or 20<=frame<=28:
        d.art_references=[dict(name='User-provided original Xbox Duke controller photograph',
            sha256='80b51966523efc5552429d37633bdf13ccd4172f4497df0d13fc02da3cf142e0',
            purpose='Rounded shoulder and grip profile reference; Vista governs glyph placement and palette')]
    # A 9+ glyph has additional canvas width. The base symbol remains round.
    unit=.5 if compact else .75 if name.endswith('_sm') else 1
    g=element(d.root,'g',transform=f'scale({unit})',fill='white',stroke_linejoin='round')
    blue=d.glow('live-symbol-blue-glow',.9)
    glint=d.glow('live-symbol-edge-softness',.29)
    art=element(g,'g',id='live-symbol',filter=glint,stroke='#96bee2',
                stroke_width=.55,paint_order='stroke fill')
    if frame in (1,3,6,8,18) or 9<=frame<=17 or 20<=frame<=28:
        ring=element(art,'g',id='live-symbol-circle')
        if 9<=frame<=17 or 20<=frame<=28:
            clip=element(d.defs,'mask',id='number-space',maskUnits='userSpaceOnUse',x=0,y=0,width=64,height=60)
            element(clip,'rect',width=64,height=60,fill='white')
            element(clip,'rect',x=39,y=38,width=24,height=22,fill='black')
            ring.set('mask','url(#number-space)')
        element(ring,'circle',cx=28,cy=28,r=23,fill='none',stroke='#87b9df',stroke_width=4,opacity=.5)
        element(ring,'circle',cx=28,cy=28,r=23,fill='none',stroke='white',stroke_width=2.5)
    if frame==0:person(art)
    elif frame==1:person(art,'translate(-2 0)')
    elif frame in (2,7,31):
        element(art,'circle',cx=24,cy=22,r=17,fill='none',stroke='#87b9df',stroke_width=4,opacity=.5)
        element(art,'circle',cx=24,cy=22,r=17,fill='none',stroke='white',stroke_width=2.5)
        if frame==2:
            element(art,'circle',cx=26,cy=12.5,r=3.6)
            element(art,'path',d='M17.8 18 H32 L20 32.5 Z')
        else:
            inner=element(art,'g',transform='translate(1.5 .5) scale(.8)');controller(inner)
        orbit(art)
    elif frame in (3,4,30):
        if frame==3:
            element(art,'path',id='speaker-horn',d='M16 20 H22 V36 H16 Z M24 20 L33.5 14 '
                    'Q36 12.5 36 15 V25 A3 3 0 0 1 36 31 V41 '
                    'Q36 43.5 33.5 42 L24 36 Z')
            sound_divider(art,36,15,41)
        else:sound(art,frame==30)
        if frame==3:
            element(art,'path',d='M12 44 L43 13',fill='none',stroke='#266fa9',stroke_width=7)
            element(art,'path',d='M12 44 L43 13',fill='none',stroke='white',stroke_width=3)
    elif frame==5:
        element(art,'path',d='M2.5 10.5 H50.5 V45.5 H2.5 Z M6 14 H38 V42 H6 Z',fill='none',stroke='white',stroke_width=1.8)
        element(art,'path',id='speaker-horn',d='M12 22 H16 V34 H12 Z M18 22 L24.5 17 '
                'Q26 16 26 18 V25 A3 3 0 0 1 26 31 V38 '
                'Q26 40 24.5 39 L18 34 Z')
        sound_divider(art,26,18,38)
        for y in (28,38):element(art,'rect',x=42,y=y,width=6,height=2)
    elif frame==6 or 9<=frame<=17:
        element(art,'path',d='M24 14 L10.5 28 L26 43 L42 31 H27 Q24 31 24 28 Z '
                'M28 13 L44 27 H28 Z')
    elif frame==8:
        element(art,'path',d='M27 12 C23 15 21 20 21 25 V29 H14 L28 43 '
                'L42 29 H31 V22 C31 15 36 13 43 13 Z')
    elif frame==18 or 20<=frame<=28:controller(art)
    elif frame in (19,29):
        shift=0 if frame==29 else -2
        people=element(art,'g',transform=f'translate(0 {shift})')
        element(people,'circle',cx=21,cy=21.5,r=3.8)
        element(people,'path',d='M13 27 H30.5 L15 47 Z')
        element(people,'circle',cx=37,cy=29.5,r=3.8)
        element(people,'path',d='M29 35 H46.5 L31 55 Z')
        if frame==29:
            crown=('M10.8 6.8 L16.5 10.3 L18.7 9.6 L21 1.5 L23.3 9.6 L25.5 10.3 '
                   'L31.2 6.8 L29.8 12.5 C29.8 14.8 28.8 15.3 27.2 14.4 '
                   'C25.2 13.2 23.3 12.7 21 12.7 C18.7 12.7 16.8 13.2 14.8 14.4 '
                   'C13.2 15.3 12.2 14.8 12.2 12.5 Z')
            element(art,'path',id='party-crown',d=crown,fill='white')
    if 9<=frame<=17 or 20<=frame<=28:
        digit=str(min(9,frame-7 if frame<=17 else frame-18))
        glyph=element(art,'g',id='live-count',transform=f'translate({38 if frame in (17,28) else 40} 39)')
        element(glyph,'path',d=LIVE_DIGITS[digit],fill='white',fill_rule='evenodd',
                stroke='#347fb7',stroke_width=1.8,paint_order='stroke fill')
        if frame in (17,28):element(glyph,'path',d='M13 4 H15 V8 H19 V10 H15 V14 H13 V10 H9 V8 H13 Z')
    # Draw a blue halo behind the entire semantic icon, including its counter
    # edges, rather than replacing the original with a sharper flat pictogram.
    import copy
    halo=copy.deepcopy(art)
    halo.attrib.pop('id',None);halo.set('filter',blue);halo.set('opacity','.8')
    for node in halo.iter():
        node.attrib.pop('id',None)
        if node.get('fill','white')!='none':node.set('fill','#2e9aff')
        if node.get('stroke')=='white':node.set('stroke','#2e9aff')
        elif node.tag.rsplit('}',1)[-1] in ('path','circle','rect') and node.get('fill')!='none':
            node.set('stroke','#2e9aff');node.set('stroke-width','1.2')
    g.insert(0,halo)
    return True
