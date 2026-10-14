"""Authored Vista list materials, column panels and voice-mail controls.

The horizontal engraving is a repeating vector material. Canvas extents,
corner cuts and state fades retain their separate original definitions.
"""
from vista_prompt_svg import element, rgb


LIST_ROWS={
 'ui\\list_skins\\color_picker\\item_background':('upper-right',10,8,0,(201,219,249),52,36),
 'ui\\list_skins\\default\\item_background':('rectangle',0,8,0,(197,227,254),55,34),
 'ui\\list_skins\\game_browser\\item_background':('lower-right',10,4,1,(201,219,249),68,52),
 'ui\\list_skins\\playlist_entries\\item_background':('upper-left',6,4,0,(179,218,255),92,65),
 'ui\\list_skins\\playlist_listing\\item_background':('upper-right',10,8,0,(201,219,249),52,36),
 'ui\\list_skins\\profile_picker\\item_background':('rectangle',0,8,2,(197,227,254),52,36),
 'ui\\list_skins\\profile_picker2\\item_background':('rectangle',0,8,2,(197,227,254),52,36),
}


def stripe_material(d,name,period,phase,color,high,low):
    pattern=element(d.defs,'pattern',id=name,patternUnits='userSpaceOnUse',
                    width=1,height=period,x=0,y=phase)
    element(pattern,'rect',width=1,height=period/2,fill=rgb(color),fill_opacity=high/255)
    element(pattern,'rect',y=period/2,width=1,height=period/2,fill=rgb(color),fill_opacity=low/255)
    return 'url(#'+name+')'


def list_row(d,name):
    d.family='striped-list-row'
    corner,cut,period,phase,color,high,low=LIST_ROWS[name]
    w,h=d.width,d.height
    if 'playlist_entries' in name and d.frame:color=(127,193,255)
    if corner=='upper-right':outline=f'M0 0 H{w-cut} L{w} {cut} V{h} H0 Z'
    elif corner=='lower-right':outline=f'M0 1 H{w} V{h-cut} L{w-cut} {h} H0 Z'
    elif corner=='upper-left':outline=f'M0 {cut} L{cut} 0 H{w} V{h} H0 Z'
    else:outline=f'M0 0 H{w} V{h} H0 Z'
    clip=element(d.defs,'clipPath',id='row-shape');element(clip,'path',d=outline)
    group=element(d.root,'g',id='row-material',clip_path='url(#row-shape)')
    if d.frame in (1,2):
        mask=element(d.defs,'mask',id='row-state-fade',maskUnits='userSpaceOnUse',
                     x=0,y=0,width=w,height=h)
        gradient=element(d.defs,'linearGradient',id='row-fade',gradientUnits='userSpaceOnUse',
                         x1=0,x2=0,y1=0,y2=h)
        extent=min(16,h)
        stops=((0,1),((h-extent)/h,1),(1,.04)) if d.frame==1 else ((0,.08),(extent/h,1),(1,1))
        for offset,alpha in stops:element(gradient,'stop',offset=offset,stop_color='white',stop_opacity=alpha)
        element(mask,'rect',width=w,height=h,fill='url(#row-fade)')
        group.set('mask','url(#row-state-fade)')
    paint=stripe_material(d,'horizontal-engraving',period,phase,color,high,low)
    attrs={}
    if 'default\\item' in name:attrs['filter']=d.glow('soft-engraving',.7)
    element(group,'rect',x=-2,y=2 if 'profile_picker' in name else -4,
            width=w+4,height=h+8,fill=paint,**attrs)
    if 'profile_picker' in name:
        # The first band meets the page boundary before the regular repeat.
        element(group,'rect',width=w,height=2,fill=rgb(color),fill_opacity=high/255)


def voice_control(d,name):
    d.family='voice-mail-control'
    w,h=d.width,d.height
    outer=(f'M12 3 H{w-10} L{w-7} 6 V11 L{w-10} 14.5 L{w-7} 18 '
           f'V24 L{w-10} 27 H12 L7 22 V18 L10 14.5 L7 11 V7 Z')
    inner=f'M15 4 H{w-11} V23 L{w-14} 26 H11 V8 Z'
    group=element(d.root,'g',id='voice-control')
    if d.frame:
        element(group,'path',d=outer,fill='none',stroke='#2e9aff',stroke_width=5,
                filter=d.glow('voice-control-glow',1.8))
    clip=element(d.defs,'clipPath',id='control-inset');element(clip,'path',d=inner)
    high,low=(94,67) if d.frame else (28,19)
    material=stripe_material(d,'control-engraving',4,0,(201,219,249),high,low)
    element(group,'path',d=inner,fill=material)
    if d.frame:
        element(group,'path',d=inner,fill='none',stroke='#2e9aff',stroke_width=3.5,
                filter=d.glow('voice-inset-light',1.5),clip_path='url(#control-inset)')
    element(group,'path',d=outer+' '+inner,fill='#ffffff' if d.frame else '#adc3d8',fill_rule='evenodd')
    if d.frame:
        element(group,'path',d=f'M2 10 L8 14.5 L2 19 Z M{w-2} 10 L{w-8} 14.5 L{w-2} 19 Z',fill='white')
    if name.endswith('\\play'):
        paint=d.linear('play-polished-face',[(0,(145,225,141)),(.14,(53,206,37)),(1,(53,206,37))],
                       x1=24,x2=36,y1=0,y2=0,calibrate=False)
        element(group,'path',d='M24 8 L36 14 L24 20 Z',fill=paint)
    elif name.endswith('\\record'):
        element(group,'circle',cx=30.5,cy=14.5,r=6.5,fill='#bf2026',stroke='#de686c',
                stroke_width=.5,stroke_opacity=.7)
    elif name.endswith('\\stop'):
        element(group,'rect',x=24,y=8,width=13,height=13,fill='#2e9aff',stroke='#9bdcff',
                stroke_width=.5,stroke_opacity=.8)


def panel(d,name):
    if name in LIST_ROWS:list_row(d,name)
    elif name.startswith('ui\\screens\\game_shell\\xbox_live\\voice_mail_record\\') and name.rsplit('\\',1)[1] in ('play','record','stop','done'):
        voice_control(d,name)
    elif name=='ui\\screens\\game_shell\\xbox_live\\voice_mail_record\\border':
        d.family='voice-mail-timeline-border'
        element(d.root,'path',d='M6 .5 H236 L241.5 6.5 L236 12.5 H6 L.5 6.5 Z',
                fill='none',stroke='#adc3d8',stroke_width=1)
    elif name in ('ui\\screens\\game_shell\\postgame_statistics\\3_column','ui\\screens\\game_shell\\postgame_statistics\\5_column'):
        d.family='postgame-column-panel'
        columns=((0,384),(386,324),(712,322)) if name.endswith('3_column') else ((0,384),(386,162),(550,160),(712,160),(874,160))
        for x,width in columns:element(d.root,'rect',x=x,y=0,width=width,height=d.height,fill='black',fill_opacity=99/255)
    elif name=='ui\\screens\\game_shell\\pregame_lobby\\name_field':
        d.family='lobby-name-field'
        element(d.root,'path',d='M16 0 H624 L676 54 H16 L0 70 V16 Z',fill='#061532')
    else:return False
    return True
