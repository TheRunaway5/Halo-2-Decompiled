"""Editable, semantic Vista menu geometry; no raster-to-path conversion.

Each admitted family below has deliberately authored geometry. Reference
measurements supply its canvas, placement, and small paint palette only.
Unknown families return None and remain explicitly unfinished.
"""
import math
import xml.etree.ElementTree as ET

import numpy as np
from scipy.optimize import lsq_linear

from vista_prompt_svg import Drawing, element, rgb, bounds
from vista_emblem_svg import foreground
from vista_wordmark_svg import wordmark
from vista_menu_panels_svg import panel
from vista_menu_status_svg import status
from vista_menu_framing_svg import framing
from vista_menu_rank_svg import numbered_rank
from vista_menu_live_svg import live_icon
from vista_menu_materials_svg import materials
from vista_menu_tracks_svg import tracks
from vista_menu_aux_svg import auxiliary
from vista_menu_corner_svg import corners
from vista_menu_lightpanels_svg import lightpanels
from vista_menu_branding_svg import branded_row
from vista_menu_center_svg import centre_frame
from vista_menu_controls_svg import controls

FIXED_PALETTE_FAMILIES={
    'virtual-keyboard-key','emblem-background','emblem-foreground',
    'setting-list-highlight','list-selector','virtual-keyboard-field',
    'striped-list-row','voice-mail-control','voice-mail-timeline-border',
    'postgame-column-panel','lobby-name-field','browser-status-icon',
    'lobby-speaker-icon','postgame-team-panel','postgame-spinner-arrow',
    'lobby-settings-divider','browser-grid-panel','profile-grid-panel',
    'lobby-grid-panel','browser-list-frame','lobby-chat-frame',
    'numbered-rank-badge',
    'celestial-rank-illustration',
}


class MenuDrawing(Drawing):
    def __init__(self,image,path,scale,name,frame):
        super().__init__(image,frame,path,scale,name)
        self.root.find('{http://www.w3.org/2000/svg}title').text=f'Vista {name} page {frame}: authored menu vectors'
        self.root.find('{http://www.w3.org/2000/svg}desc').text=(
            'Editable paths, strokes and gradients reconstructed by named design family. '
            'No embedded raster, autotraced regions, pixel grids, or font substitution.')
        self.frame=frame

    def glow(self,name,radius):
        f=element(self.defs,'filter',id=name,x='-100%',y='-100%',width='300%',height='300%',
                  color_interpolation_filters='sRGB')
        element(f,'feGaussianBlur',stdDeviation=radius)
        return f'url(#{name})'

    def painted_path(self,parent,path,color,**attrs):
        n=element(parent,'path',d=path,**attrs)
        self.solid(n,color)
        return n

    def finish(self):
        # Only paint anchors are fitted. The path topology/vertices, blur
        # radii, stroke widths, and every state remain the authored values.
        density=self.scale
        self.scale=1
        if self.family not in FIXED_PALETTE_FAMILIES:self.calibrate()
        self.scale=density
        self.save()

    def calibrate(self):
        """Fit a few paint anchors using premultiplied source channels.

        Menu fills are often almost transparent white. Matching straight RGB
        while discarding their low alpha would incorrectly make them opaque.
        Neither this fit nor its small color parameter set changes geometry.
        """
        from vista_prompt_svg import render
        if not self.slots:return
        def sample():
            a=np.asarray(render(self.path,1)).astype(float)/255
            return a[:,:,:3]*a[:,:,3,None]
        for slot in self.slots:self.set_slot(slot,(0,0,0))
        self.save();base=sample();columns=[]
        for slot in self.slots:
            self.set_slot(slot,(255,255,255));self.save()
            columns.append(sample()[:,:,0]-base[:,:,0])
            self.set_slot(slot,(0,0,0))
        model=np.stack(columns,axis=-1).reshape(-1,len(self.slots))
        source=self.source.astype(float)/255
        target=(source[:,:,:3]*source[:,:,3,None]-base).reshape(-1,3)
        # Portraits can opt into stable material interiors. This prevents
        # differences in an outlined edge from steering a plate's paint.
        # No reference mask or pixels are serialized into replacement art.
        alpha_range=getattr(self,'paint_fit_alpha_range',None)
        if alpha_range is not None:
            from scipy.ndimage import binary_erosion
            low,high=alpha_range
            native_alpha=np.asarray(render(self.path,1))[:,:,3]
            original_alpha=self.source[:,:,3]
            interior=(original_alpha>=low)&(original_alpha<=high)&\
                     (native_alpha>=low)&(native_alpha<=high)
            interior=binary_erosion(interior,iterations=2).reshape(-1)
            if not interior.any():raise ValueError('No stable material interiors for paint fitting')
            model=model[interior]
            target=target[interior]
        step=max(1,len(model)//20000)
        ridge=.005
        model=np.vstack((model[::step],np.eye(len(self.slots))*ridge))
        initial=np.array([s[2] for s in self.slots])/255
        values=[]
        for channel in range(3):
            y=np.r_[target[::step,channel],initial[:,channel]*ridge]
            values.append(lsq_linear(model,y,bounds=(0,1),tol=1e-7).x*255)
        for slot,color in zip(self.slots,np.array(values).T):self.set_slot(slot,color)


def keyboard(d,name):
    d.family='virtual-keyboard-key'
    w,h=d.width,d.height
    inset=7
    corner=8
    p=f'M{inset} {inset+corner} L{inset+corner} {inset} H{w-inset} V{h-inset-corner} L{w-inset-corner} {h-inset} H{inset} Z'
    glow=d.glow('key-edge-glow',3.2 if d.frame else 3.4)
    g=element(d.root,'g',id='key')
    outside=element(d.defs,'mask',id='outside-key',maskUnits='userSpaceOnUse',x=0,y=0,width=w,height=h)
    element(outside,'rect',width=w,height=h,fill='white')
    element(outside,'path',d=p,fill='black')
    element(g,'path',d=p,fill='none',stroke='#58aaf8' if d.frame else '#3fa2ff',
        stroke_opacity=(.55 if d.frame==1 else .24) if d.frame else .95,
        stroke_width=9 if d.frame else 2,filter=glow,mask='url(#outside-key)')
    # Fit semantic paint anchors directly to stable interior/edge samples;
    # no global color fit is allowed to dim the thin opaque rim.
    center=d.source[h//2,w//2]
    element(g,'path',d=p,fill=rgb(center[:3]),fill_opacity=float(center[3])/255)
    # The selected/hovered states have an interior light band, distinct from
    # the ordinary outline. It is a smooth vector stroke, not a bitmap mask.
    if d.frame:
        clip=element(d.defs,'clipPath',id='key-interior')
        element(clip,'path',d=p)
        element(g,'path',d=p,fill='none',stroke='#58aaf8',stroke_width=10,
                stroke_opacity=.65 if d.frame==1 else .4,filter=d.glow('interior-light',3.5),
                clip_path='url(#key-interior)')
    element(g,'path',d=p,fill='none',stroke_width=2,stroke_linejoin='miter',stroke='#93d2fe')


def lobby_button(d):
    d.family='lobby-button'
    w=d.width
    p=f'M21 8 H{w-21} L{w-15} 14 V24 L{w-20} 29 V35 L{w-15} 40 V50 L{w-21} 56 H21 L15 50 V40 L20 35 V29 L15 24 V14 Z'
    inner=f'M28 10 H{w-23} V49 L{w-29} 54 H23 V16 Z'
    group=element(d.root,'g',id='lobby-button')
    element(group,'path',d=p,fill='none',stroke='#3eacff',stroke_width=7,
        stroke_opacity=.7 if d.frame else .12,filter=d.glow('edge-light',3))
    d.painted_path(group,p+' '+inner,(255,255,255) if d.frame==1 else (176,199,215),fill_rule='evenodd')
    d.painted_path(group,inner,(201,219,249),fill_opacity=.085)
    # Repeated horizontal engraving is part of the source button material.
    stripe=d.linear('engraving-paint',[(0,(201,219,249)),(1,(201,219,249))],y1=10,y2=54)
    for y in range(13,54,6):
        element(group,'path',d=f'M25 {y} H{w-25}',fill='none',stroke=stripe,stroke_width=1.7,stroke_opacity=.05)
    if d.frame==1:
        for p in ('M6 23 L15 32 L6 41 Z',f'M{w-4} 23 L{w-13} 32 L{w-4} 41 Z'):
            element(group,'path',d=p,fill='#ffffff')


def arrows(d,name):
    d.family='list-direction-arrow'
    settings='setting_lists' in name or 'squad_lobby_player_list' in name or 'game_setup_list' in name
    if settings:
        points=[(37,8),(63,34),(44,34),(37,27),(30,34),(11,34)]
    elif 'y_menu_list' in name:
        points=[(13.5,3),(23,12),(16,12),(13.5,9.5),(11,12),(4,12)]
    elif 'network_squad_browser' in name and d.frame==2:
        return False
    else:
        points=[(26,7),(45,26),(31,26),(26,21),(21,26),(7,26)]
    # Native arrow pages use distinct up/down padding. Do not rotate the
    # bitmap canvas or interchange source frame ownership.
    basew,baseh=(74,46) if settings else (27,17) if 'y_menu_list' in name else (52,34)
    if d.frame==1:points=[(x,baseh-y) for x,y in points]
    g=element(d.root,'g',id='arrow',transform=f'scale({d.width/basew} {d.height/baseh})')
    path='M'+' L'.join(f'{x} {y}' for x,y in points)+' Z'
    element(g,'path',d=path,fill='#2186d0',fill_opacity=.7,filter=d.glow('arrow-glow',2.2))
    n=element(g,'path',d=path,fill_opacity=1 if settings else .7)
    d.solid(n,(255,255,255) if settings else (230,242,255))
    return True


def end_cap(d):
    d.family='default-list-end-cap'
    p='M17 7 H30 L33 11 H50 V14 H20 V54 H52 V57 H33 L29 61 H17 L8 51 V39 H13 V35 H8 V30 L14 23 V11 Z'
    g=element(d.root,'g',id='list-end-cap')
    if d.frame:
        mask=element(d.defs,'mask',id='cap-fade',maskUnits='userSpaceOnUse',x=0,y=0,width=d.width,height=d.height)
        grad=element(d.defs,'linearGradient',id='fade',x1=0,y1=0,x2=0,y2=1)
        for offset,value in ((0,255),(.35,255),(.83,0),(1,0)):
            if d.frame==2:offset=1-offset
            element(grad,'stop',offset=offset,stop_color=rgb([value]*3))
        # SVG stop order is ascending in both native fade directions.
        grad[:]=sorted(grad,key=lambda n:float(n.attrib['offset']))
        element(mask,'rect',width=d.width,height=d.height,fill='url(#fade)')
        g.set('mask','url(#cap-fade)')
    element(g,'path',d=p,fill='#599ad6',filter=d.glow('cap-glow',2.4))
    paint=d.linear('cap-metal',[(0,(255,255,255)),(.4,(194,217,239)),(1,(255,255,255))],y1=7,y2=61)
    element(g,'path',d=p,fill=paint)


def bracket(d):
    d.family='setting-list-bracket'
    w=d.width
    # The two widths are separately authored Vista variants, not a stretched
    # raster; both retain the same end bevel and horizontal stroke thickness.
    x0,x1=(12,w-12) if w<200 else (13,w-18)
    p=f'M{x0} 24 L{x0+12} 10 H{x1-13} L{x1} 24 H{x1-11} L{x1-20} 12.5 H{x0+21} L{x0+12} 24 Z'
    g=element(d.root,'g',id='bracket',transform=f'translate(0 {d.height-1}) scale(1 -1)' if d.frame%2 else 'translate(0 0)')
    background=d.linear('bracket-inner',[(0,(36,50,72)),(1,(44,58,79))],y1=12,y2=24)
    element(g,'path',d=f'M{x0+12} 12.5 H{x1-13} L{x1} 24 H{x0} Z',fill=background)
    element(g,'path',d=p,fill='#349ce4',filter=d.glow('bracket-glow',2.5))
    d.painted_path(g,p,(255,255,255))


def background_emblem(d):
    d.family='emblem-background'
    opaque=d.source[d.source[:,:,3]>=250,:3]
    ink=rgb(np.median(opaque,axis=0)) if len(opaque) else '#0000fe'
    g=element(d.root,'g',id='background',transform=f'scale({d.width/256} {d.height/256})',fill=ink)
    # Emblem channels are authored shader masks: literal blue, not the final
    # player-selected display color. All three native sizes share geometry.
    i=d.frame
    rectangles={1:(0,0,128,256),2:(0,128,256,128),3:(0,0,256,128),6:(86,0,84,256),
        7:(0,86,256,84),17:(40,40,176,176),26:(0,0,170,256),27:(0,0,86,256),
        28:(0,0,128,128),29:(0,46,256,164),31:(44,0,168,256)}
    cross='M0 0 H58 L128 70 L198 0 H256 V58 L186 128 L256 198 V256 H198 L128 186 L58 256 H0 V198 L70 128 L0 58 Z'
    paths={8:'M128 0 H256 V128 H128 V256 H0 V128 H128 Z',
        9:'M0 0 H128 V128 H256 V256 H128 V128 H0 Z',
        10:'M0 0 L256 256 H0 Z',11:'M0 0 H256 L128 256 Z',
        12:cross,
        13:'M0 0 H256 V256 H0 Z '+cross,
        15:'M128 8 L248 128 L128 248 L8 128 Z',
        16:'M88 0 H168 V88 H256 V168 H168 V256 H88 V168 H0 V88 H88 Z',
        # Two circular cutouts are centered 20 units outside the canvas.
        # This preserves the native hourglass profile, unlike an ellipse.
        18:'M0 0 H256 V256 H0 Z M-20 0 A128 128 0 0 1 -20 256 A128 128 0 0 1 -20 0 Z '
           'M276 0 A128 128 0 0 1 276 256 A128 128 0 0 1 276 0 Z',
        19:'M128 18 L249 228 H7 Z',20:'M0 0 H256 L128 128 Z M0 256 H256 L128 128 Z',
        21:'M0 0 L128 128 L0 256 Z',22:'M0 0 H256 L128 128 L256 256 H0 Z',
        23:'M0 0 H256 V64 H0 Z M0 128 H256 V192 H0 Z',
        24:'M0 64 H256 V128 H0 Z M0 192 H256 V256 H0 Z',
        30:'M0 0 H256 V46 H0 Z M0 210 H256 V256 H0 Z'}
    if i in rectangles:
        x,y,w,h=rectangles[i];element(g,'rect',x=x,y=y,width=w,height=h)
    elif i in paths:element(g,'path',d=paths[i],fill_rule='evenodd')
    elif i==14:element(g,'circle',cx=128,cy=128,r=120)
    elif i==25:
        element(g,'path',d='M0 0 H256 V128 H248 A120 120 0 0 0 8 128 H0 Z')
        element(g,'path',d='M8 128 H248 A120 120 0 0 1 8 128 Z')
    elif i in (4,5):
        grad=element(d.defs,'linearGradient',id='emblem-ramp',x1=0,y1=0,x2=1 if i==5 else 0,y2=0 if i==5 else 1)
        for offset,value in ((0,3),(1/8,23),(1/4,53),(1/2,129),(3/4,206),(1,254)):
            if i==5:offset=1-offset
            element(grad,'stop',offset=offset,stop_color=rgb((255-value,255-value,value)),stop_opacity=value/255)
        grad[:]=sorted(grad,key=lambda n:float(n.attrib['offset']))
        element(g,'rect',width=256,height=256,fill='url(#emblem-ramp)')
    elif i!=0:return False
    return True


def information_illustration(d):
    """The perspective metal information symbol in Vista Edit Profile."""
    d.family='profile-information-illustration'
    outer='M138 7 A130 145 0 0 1 138 297 A130 145 0 0 1 138 7 Z'
    inner='M135 26 A107 126 0 0 1 135 278 A107 126 0 0 1 135 26 Z'
    group=element(d.root,'g',id='information-symbol')
    metal=d.linear('ring-metal',[(0,(62,89,132)),(.28,(113,139,178)),(.64,(108,134,174)),(1,(40,67,107))],
                   x1=58,x2=230,y1=30,y2=279)
    element(group,'path',d=outer+' '+inner,fill=metal,fill_rule='evenodd',fill_opacity=111/255)
    edge=element(group,'g',id='rim-light',fill='none',stroke='#9cd8ff',stroke_width=1.8)
    element(edge,'path',d=outer)
    element(edge,'path',d=inner)
    element(group,'path',d=outer+' '+inner,fill='none',stroke='#5aabed',stroke_width=2.5,
            stroke_opacity=.3,filter=d.glow('information-glow',2.2))
    # A separate inner bevel gives the ring its perspective depth.
    element(group,'path',d='M126 26 C65 31 20 92 22 157 C25 219 67 272 129 279',
            fill='none',stroke='#b4d9f5',stroke_width=1.2,stroke_opacity=.8)
    head=element(d.defs,'radialGradient',id='head-metal',gradientUnits='userSpaceOnUse',
                 cx=144,cy=73,r=41,gradientTransform='translate(0 -14) scale(1 1.2)')
    for offset,color in ((0,(128,152,190)),(.5,(89,114,155)),(1,(27,54,93))):
        stop=element(head,'stop',offset=offset);d.slot(stop,'stop-color',color)
    element(group,'ellipse',cx=142,cy=87.5,rx=25.5,ry=30.5,fill='url(#head-metal)',fill_opacity=111/255,
            stroke='#a5d7fd',stroke_width=1.8)
    top=d.linear('triangle-top',[(0,(115,141,181)),(1,(77,104,147))],y1=126,y2=160)
    face=d.linear('triangle-face',[(0,(85,112,154)),(1,(20,47,85))],y1=140,y2=254)
    side=d.linear('triangle-side',[(0,(20,47,83)),(1,(2,23,54))],x1=92,x2=119,y1=160,y2=194)
    element(group,'path',d='M90 127 L192 126 L119 160 Z',fill=top,fill_opacity=111/255)
    element(group,'path',d='M119 160 L192 126 L111 254 Z',fill=face,fill_opacity=111/255)
    element(group,'path',d='M90 127 L119 160 L111 254 Z',fill=side,fill_opacity=111/255)
    element(group,'path',d='M90 127 L192 126 L111 254 Z M90 127 L119 160 L192 126 M119 160 L111 254',
            fill='none',stroke='#a2d8ff',stroke_width=1.2)


def setting_highlight(d):
    d.family='setting-list-highlight'
    w=d.width
    outer=f'M24 9 H{w-24} L{w-17} 16 V27 L{w-23} 33 V35 L{w-17} 41 V52 L{w-24} 59 H24 L17 52 V41 L23 35 V33 L17 27 V16 Z'
    inner=f'M30 11 H{w-25} V51 L{w-31} 57 H25 V17 Z'
    g=element(d.root,'g',id='setting-highlight')
    element(g,'path',d=outer,fill='none',stroke='#2e9aff',stroke_width=7,stroke_opacity=.8,
            filter=d.glow('setting-highlight-glow',3.3))
    if d.frame==1:
        element(g,'path',d=inner,fill='#293446',fill_opacity=.8)
        clip=element(d.defs,'clipPath',id='stripe-window');element(clip,'path',d=inner)
        stripes=element(g,'g',id='engraved-interior',clip_path='url(#stripe-window)')
        for y in range(14,58,8):
            element(stripes,'rect',x=25,y=y,width=w-50,height=4,fill='#212e42',fill_opacity=.8)
    element(g,'path',d=outer+' '+inner,fill='#ffffff',fill_rule='evenodd')
    for p in ('M7 24 L17 34 L7 44 Z',f'M{w-9} 24 L{w-19} 34 L{w-9} 44 Z'):
        element(g,'path',d=p,fill='#ffffff')


def selector(d,name):
    d.family='list-selector'
    selected=d.frame==0
    profile='profile_picker' in name
    color='color_picker' in name
    if profile:
        body='M17 4 V42 L11 36 V28 L15 24 V20 L11 16 V10 Z' if selected else \
             ('M16 5 V42 L10 36 V28 L14 24 V20 L10 16 V11 Z' if d.frame==2 else
              'M16 5 V39 L10 33 V27 L14 23 V19 L10 15 V10 Z')
        arrow='M3 18 L10 23 L3 28 Z'
    elif color:
        body='M22 9 V76 L15 69 V49 L18 46 V40 L15 37 V16 Z' if selected else \
             'M18 5 V71 L11 64 V46 L15 42 V34 L11 30 V12 Z'
        arrow='M8 36 L18 43 L8 50 Z'
    else:
        body='M23 8 V60 L16 53 V42 L20 38 V30 L16 26 V15 Z' if selected else \
             'M20 4 V56 L13 49 V39 L18 34 V26 L13 21 V11 Z'
        arrow='M8 27 L18 34 L8 41 Z'
    group=element(d.root,'g',id='selector')
    if selected:
        for p in (body,arrow):
            element(group,'path',d=p,fill='#319cff',stroke='#319cff',stroke_width=3,
                filter=d.glow('selector-glow-'+str(len(group)),2.1))
        element(group,'path',d=body,fill='#cdddeb')
        element(group,'path',d=arrow,fill='#cdddeb')
    else:
        # The inactive top/bottom caps have different fades and native page
        # sizes; their source ordinals are preserved rather than mirrored.
        grad=element(d.defs,'linearGradient',id='inactive-opacity',x1=0,y1=0,x2=0,y2=1)
        stops=((0,.5),(.65,.5),(1,0)) if d.frame==1 else ((0,0),(.35,.5),(1,.5))
        if profile:stops=((0,.7),(1,.7))
        for offset,opacity in stops:element(grad,'stop',offset=offset,stop_color='#ffffff',stop_opacity=opacity)
        element(group,'path',d=body,fill='url(#inactive-opacity)')


def keyboard_field(d,name):
    d.family='virtual-keyboard-field'
    w,h=d.width,d.height
    if name.endswith('button_disabled'):
        p=f'M6 14 L14 6 H{w-7} V{h-14} L{w-15} {h-6} H6 Z'
        element(d.root,'path',d=p,fill='#114272',fill_opacity=130/255)
        return
    if name.endswith('field_2'):
        # Same bevel/corner design as keys, with a separate native height.
        keyboard(d,name)
        return
    p=f'M3 7 L7 3 H{w-3} V{h-7} L{w-7} {h-3} H3 Z'
    element(d.root,'path',d=p,fill='#c7d1dc',fill_opacity=130/255)
    element(d.root,'path',d=p,fill='none',stroke='#006db0',stroke_width=1,
            filter=d.glow('field-outline-glow',1.6))
    element(d.root,'path',d=p,fill='none',stroke='#006db0',stroke_width=1)


def draw(image,path,name,frame,scale=4):
    d=MenuDrawing(image,path,scale,name,frame)
    source=np.asarray(image)
    # A single rectangle is the exact design of a uniform source page. Empty
    # native placeholders remain empty SVGs and preserve all bank ordinals.
    if np.all(source==source[0,0]):
        d.family='uniform-page'
        color=source[0,0]
        if color[3]:element(d.root,'rect',width=d.width,height=d.height,fill=rgb(color[:3]),fill_opacity=color[3]/255)
    elif name.startswith('ui\\screens\\game_shell\\virtual_keyboard\\button_') and not name.endswith('disabled'):
        keyboard(d,name)
    elif name=='ui\\screens\\game_shell\\pregame_lobby\\button':lobby_button(d)
    elif name=='ui\\list_skins\\default\\end_cap':end_cap(d)
    elif name in ('ui\\list_skins\\default\\arrows','ui\\list_skins\\setting_lists\\arrows',
            'ui\\list_skins\\squad_lobby_player_list\\arrows','ui\\list_skins\\game_setup_list\\arrows',
            'ui\\list_skins\\y_menu_list\\arrows'):
        if not arrows(d,name):return None
    elif name=='ui\\list_skins\\setting_lists\\hilite_bracket':bracket(d)
    elif name in ('ui\\global_bitmaps\\emblems\\background','ui\\global_bitmaps\\emblems\\background_medium',
            'ui\\global_bitmaps\\emblems\\background_small'):
        if not background_emblem(d):return None
    elif name in ('ui\\global_bitmaps\\emblems\\foreground','ui\\global_bitmaps\\emblems\\foreground_medium',
            'ui\\global_bitmaps\\emblems\\foreground_small'):
        if not foreground(d):return None
    elif name=='ui\\screens\\game_shell\\settings_screen\\player_profile\\edit_profile' and frame==4:
        information_illustration(d)
    elif name in ('ui\\list_skins\\setting_lists\\hilite','ui\\list_skins\\setting_lists\\advanced_hilite'):
        setting_highlight(d)
    elif name in ('ui\\list_skins\\profile_picker\\end_cap','ui\\list_skins\\color_picker\\end_cap',
                  'ui\\list_skins\\playlist_listing\\select'):
        selector(d,name)
    elif name in ('ui\\screens\\game_shell\\virtual_keyboard\\field_1','ui\\screens\\game_shell\\virtual_keyboard\\field_2',
                  'ui\\screens\\game_shell\\virtual_keyboard\\button_disabled'):
        keyboard_field(d,name)
    elif name=='ui\\screens\\game_shell\\start_screen\\start_screen':wordmark(d)
    elif name in ('ui\\global_bitmaps\\rank_icons','ui\\global_bitmaps\\rank_icons_sm'):
        if not numbered_rank(d):return None
    elif not (panel(d,name) or status(d,name) or framing(d,name) or live_icon(d,name)
              or controls(d,name) or materials(d,name) or tracks(d,name)
          or auxiliary(d,name) or corners(d,name) or lightpanels(d,name)
          or branded_row(d,name) or centre_frame(d,name)):return None
    d.finish()
    return d
