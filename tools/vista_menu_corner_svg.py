"""Manually outlined Vista corner frames, with separate metal and blue light.

Each entry retains the native corner cuts, detached rails and empty canvas.
Face normals describe the direction of the translucent polished metal; these
are semantic surfaces, not source pixels converted into filled regions.
"""
import numpy as np
from scipy.optimize import lsq_linear

from vista_prompt_svg import element,rgb,render
from vista_menu_materials_svg import linear


# Outlines, local face-paint direction, edge glints, and inward blue fields.
CORNERS={
 'ul_02':dict(faces=[
    ('M10 58 L55 10 H210 L207 14 H82 L36 60 V80 Z',(35,38,51,54)),
    ('M40 63 L86 18 H90 L43 65 V120 L56 133 V205 L40 220 Z',(40,0,56,0))],
    glints=['M10 58 L55 10 H210','M40 64 V220','M43 121 L56 134 V205'],
    fields=[(67,70,170,190,.28)]),
 'ul_03':dict(faces=[
    ('M35 10 H178 L174 14 H46 L45 16 H36 Z',(0,10,0,16)),
    ('M34 16 H72 L69 20 H40 L28 32 V168 L10 150 V112 L21 122 V32 Z',(10,0,28,0)),
    ('M10 58 L17 64 V104 L10 98 Z',(10,0,17,0))],
    glints=['M35 10 H178','M34 16 H72','M28 33 V168'],fields=[(38,38,185,180,.23)]),
 'ul_05':dict(faces=[
    ('M25 62 L70 10 H134 L132 13 H83 L29 68 V169 L10 188 V144 L25 129 Z',(10,0,29,0)),
    ('M18 58 L20 56 V82 L12 90 V80 L18 74 Z',(12,0,20,0))],
    glints=['M25 62 L70 10 H134','M29 69 V169 L10 188'],fields=[(42,72,130,170,.28)]),
 'ul_06':dict(faces=[
    ('M22 55 L55 24 V10 H166 L180 24 H89 L54 57 V155 L22 123 Z '
     'M40 97 H42 V113 H40 Z',(22,0,54,0))],
    glints=['M22 55 L55 24 V10 H166 L180 24','M54 58 V155'],
    fields=[(100,28,150,80,.23),(56,90,70,145,.22)]),
 'ul_07':dict(faces=[
    ('M32 10 H82 L80 13 H220 V16 H80 L78 19 H29 L14 35 V70 L10 66 V30 Z',(0,10,0,19)),
    ('M16 108 L19 110 V141 L16 147 V222 L10 229 V146 L16 140 Z',(10,0,19,0)),
    ('M14 119 V139 L10 143 V127 Z',(10,0,14,0))],
    glints=['M32 10 H82','M81 13 H220','M16 35 V119','M19 111 V141'],fields=[]),
 'ul_09':dict(faces=[
    ('M17 10 H165 L170 15 H87 L85 18 H20 L12 26 V217 L17 222 V226 '
     'L10 221 V18 Z',(10,0,20,0)),
    ('M169 10 H211 L207 16 H178 Z',(0,10,0,16)),
    ('M10 227 L17 233 V345 L12 350 H10 Z',(10,0,17,0))],
    glints=['M17 10 H165','M170 10 H211','M12 26 V217'],
    fields=[(20,170,300,400,.12),(150,20,140,90,.08)]),
 'ul_10':dict(faces=[
    ('M10 35 L35 10 H128 L126 12 H46 L20 39 V139 L10 130 Z',(10,0,20,0)),
    ('M16 62 H18 V104 L16 102 Z',(16,0,18,0))],
    glints=['M10 35 L35 10 H128','M20 40 V139'],fields=[(50,76,180,160,.24)]),
 'bl_01':dict(faces=[
    ('M10 78 L27 96 V264 L59 296 H45 L23 278 V138 L10 122 Z',(10,0,27,0)),
    ('M10 129 L17 135 V165 L14 160 V141 L10 138 Z',(10,0,17,0))],
    glints=['M10 78 L27 96 V264 L59 296','M23 139 V278'],fields=[(38,262,120,95,.24)]),
 'br_03':dict(faces=[
    ('M306 110 L300 116 V144 L306 147 Z',(300,0,306,0)),
    ('M306 147 V218 L293 232 V228 L302 218 V147 Z',(302,0,306,0)),
    ('M116 250 L132 233 L138 238 H253 L271 218 H298 L276 241 '
     'H220 L213 250 Z',(0,238,0,250))],
    glints=['M306 110 V218 L293 232','M116 250 H213','M138 238 H253 L271 218 H298'],
    fields=[(266,202,230,180,.24)]),
 'br_04':dict(faces=[
    ('M238 82 V169 L228 178 H219 L161 229 H157 L222 165 V98 Z '
     'M231 115 H233 V125 H231 Z',(222,0,238,0)),
    ('M117 225 L122 220 H173 L168 225 Z',(0,220,0,225))],
    glints=['M238 82 V169 L228 178 H219 L161 229','M117 225 H168'],
    fields=[(220,140,75,130,.22),(160,219,100,70,.22)]),
 'br_05':dict(faces=[
    ('M232 78 L238 72 V186 L232 180 Z',(232,0,238,0)),
    ('M222 128 L226 124 V176 L237 188 V214 L193 250 H106 L111 245 '
     'H190 L218 217 V199 L202 183 L222 163 Z',(218,0,237,0))],
    glints=['M238 72 V186','M226 124 V176 L237 188 V214 L193 250 H106',
            'M111 245 H190 L218 217'],
    fields=[(232,180,140,190,.28),(170,240,110,80,.23)]),
 'br_06':dict(faces=[
    ('M236 60 L239 63 V199 L262 225 V268 L252 282 H135 V279 H240 '
     'L248 269 V224 L236 208 Z',(236,0,262,0)),
    ('M246 84 L257 96 V140 L246 128 Z',(246,0,257,0))],
    glints=['M236 60 V208 L248 224 V269 L240 279 H135','M262 225 V268 L252 282'],
    fields=[(220,181,180,190,.23)]),
 'br_07':dict(faces=[
    ('M120 10 L130 20 V46 L120 36 Z',(120,0,130,0)),
    ('M128 46 H130 V114 L101 140 H10 L12 136 H95 L128 103 Z',(0,136,0,140)),
    ('M122 44 L124 46 V87 H122 Z',(122,0,124,0))],
    glints=['M130 20 V114 L101 140 H10','M122 44 V87'],fields=[]),
 'br_09':dict(faces=[
    ('M458 318 L462 321 V340 L458 338 Z',(458,0,462,0)),
    ('M278 400 H414 L424 411 H457 L461 407 V342 H462 V408 L453 416 '
     'H443 V413 H411 L400 403 H337 L334 408 H284 Z',(0,400,0,416))],
    glints=['M458 318 V338','M462 342 V408 L453 416 H443','M278 400 H414'],
    fields=[(420,380,270,250,.15)]),
 'tr_02':dict(faces=[
    ('M42 10 H151 V16 L213 73 V156 L195 178 V73 L139 16 H42 Z '
     'M205 117 H208 V131 H205 Z',(195,0,213,0)),
    ('M237 173 V204 L218 224 V497 L214 493 V198 Z',(214,0,237,0))],
    glints=['M42 10 H151 V16 L213 73 V156','M218 224 V497'],
    fields=[(193,151,140,320,.25),(120,22,110,80,.18),(198,260,70,300,.08)]),
}


INSETS={
 'ul_02':'M43 65 L92 16 H256 V512 H43 Z',
 'ul_03':'M28 33 L45 16 H512 V256 H28 Z',
 'ul_05':'M29 68 L85 13 H256 V512 H29 Z',
 'ul_06':'M54 57 L90 24 H512 V512 H54 Z',
 'ul_09':'M20 25 L32 18 H512 V512 H20 Z',
 'ul_10':'M20 39 L47 12 H512 V256 H20 Z',
 'bl_01':'M27 0 H256 V296 H59 L27 264 Z',
 'br_03':'M0 0 H300 V216 L270 238 H0 Z',
 'br_04':'M0 0 H222 V165 L160 222 H0 Z',
 'br_05':'M0 0 H232 V178 L220 200 V218 L190 245 H0 Z',
 'br_06':'M0 0 H236 V207 L248 223 V269 L240 279 H0 Z',
 'br_09':'M0 0 H458 V399 H0 Z',
 'tr_02':'M0 16 H139 L195 73 V178 L214 198 V1024 H0 Z',
}


def blue_field(d,parent,index,colour,cx,cy,rx,ry,opacity):
    gradient=element(d.defs,'radialGradient',id='corner-blue-'+str(index),gradientUnits='userSpaceOnUse',
                     cx=0,cy=0,r=1,gradientTransform=f'translate({cx} {cy}) scale({rx} {ry})')
    for offset,weight in ((0,1),(.3,.7),(.6,.25),(.85,.05),(1,0)):
        element(gradient,'stop',offset=offset,stop_color=colour,stop_opacity=min(1,opacity*2*weight))
    element(parent,'ellipse',cx=cx,cy=cy,rx=rx,ry=ry,fill=f'url(#corner-blue-{index})')


def fit_material_opacity(d,light,metal):
    # Two global paint weights only. The authored paths, fields, fade radii,
    # bevel stops and corner topology remain fixed. Rendering the finished
    # SVG never needs the reference or this fitting step.
    def sample():
        d.save();pixels=np.asarray(render(d.path,1)).astype(float)/255
        return np.dstack((pixels[:,:,:3]*pixels[:,:,3,None],pixels[:,:,3]))
    columns=[]
    for selected in (light,metal):
        light.set('opacity','1' if selected is light else '0')
        metal.set('opacity','1' if selected is metal else '0')
        columns.append(sample())
    source=d.source.astype(float)/255
    target=np.dstack((source[:,:,:3]*source[:,:,3,None],source[:,:,3]))
    model=np.stack(columns,axis=-1).reshape(-1,2)
    weights=lsq_linear(model,target.reshape(-1),bounds=(0,1),tol=1e-8).x
    for group,weight in zip((light,metal),weights):group.set('opacity',str(float(weight)))


def corners(d,name):
    prefix='ui\\global_bitmaps\\'
    if not name.startswith(prefix) or name[len(prefix):] not in CORNERS:return False
    key=name[len(prefix):];spec=CORNERS[key];d.family='corner-frame-metal'
    reserve=element(d.defs,'mask',id='metal-reserved',maskUnits='userSpaceOnUse',
                    x=0,y=0,width=d.width,height=d.height)
    element(reserve,'rect',width=d.width,height=d.height,fill='white')
    for path,normal in spec['faces']:
        element(reserve,'path',d=path,fill='black',fill_rule='evenodd')
    light=element(d.root,'g',id='inward-blue-light',mask='url(#metal-reserved)')
    if spec['fields']:
        clip=element(d.defs,'clipPath',id='corner-content');element(clip,'path',d=INSETS[key])
        field_group=element(light,'g',clip_path='url(#corner-content)')
        pixels=d.source.astype(float)
        blue=(pixels[:,:,3]>8)&(pixels[:,:,2]-pixels[:,:,0]>70)&(pixels[:,:,1]-pixels[:,:,0]>30)
        colour=rgb(np.average(pixels[blue,:3],axis=0,weights=pixels[blue,3]))
        for i,field in enumerate(spec['fields']):blue_field(d,field_group,i,colour,*field)
    halo=d.glow('corner-edge-halo',6)
    metal=element(d.root,'g',id='corner-metal',filter=d.glow('corner-metal-edge',.25))
    for i,(path,normal) in enumerate(spec['faces']):
        element(light,'path',d=path,fill='none',stroke='#82bcf4',stroke_width=6,
                opacity=.20,filter=halo)
        x1,y1,x2,y2=normal
        paint=linear(d,'corner-face-'+str(i),[(0,'white',.81),(.12,'white',.72),
            (.5,'white',.54),(.88,'white',.705),(1,'white',.825)],x1=x1,y1=y1,x2=x2,y2=y2)
        element(metal,'path',d=path,fill=paint,fill_rule='evenodd')
        clip=element(d.defs,'clipPath',id='metal-face-'+str(i))
        element(clip,'path',d=path,clip_rule='evenodd')
        element(metal,'path',d=path,fill='none',stroke='white',stroke_width=5,
                opacity=.34,clip_path=f'url(#metal-face-{i})')
        element(metal,'path',d=path,fill='none',stroke='white',stroke_width=1,
                opacity=.36,clip_path=f'url(#metal-face-{i})')
    for path in spec['glints']:
        element(metal,'path',d=path,fill='none',stroke='white',stroke_width=.7,opacity=.45)
    fit_material_opacity(d,light,metal)
    return True
