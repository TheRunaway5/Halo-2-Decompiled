"""Manually drawn Vista menu illustrations in their native design space.

Contours are traced as editable curves from the original illustration pages.
Soft folds and anatomical shading are separate vector paint layers. No image,
font, automatic raster contour extraction, or sampled texture grid is used.
"""
from vista_prompt_svg import element
from vista_menu_surface_svg import material_grain


VARIANT='ui\\screens\\game_shell\\settings_screen\\variant_settings\\variant'

CROWN=('M190 135 C186 124 184 111 181 99 L177 87 C174 85 171 81 171 77 '
 'C171 73 174 72 177 72 C184 71 187 81 190 91 C194 101 198 106 203 105 '
 'C206 102 204 94 204 82 L203 63 C198 62 196 59 198 55 C199 51 203 48 206 49 '
 'C211 50 212 54 212 62 V78 C214 89 225 96 231 94 C244 89 240 56 241 44 '
 'C237 42 234 40 236 37 C238 33 242 31 246 31 C250 31 255 34 256 38 '
 'C258 41 254 44 252 45 C252 56 252 79 256 88 C259 97 267 96 273 90 '
 'C282 82 282 72 282 61 C281 51 285 46 289 49 C293 50 296 53 295 57 '
 'C295 60 292 61 290 62 L289 83 C288 95 288 104 294 105 C300 105 305 94 307 84 '
 'C309 76 313 70 317 73 C323 76 323 80 319 83 L315 87 L312 104 '
 'C309 118 307 129 304 135 C294 119 275 110 247 111 C220 110 201 117 190 135 Z')

SKULL=('M177 155 C176 149 180 144 188 136 '
 'C204 124 226 118 246 119 C270 121 292 132 304 146 C313 154 317 161 317 167 '
 'C319 180 317 196 315 207 L313 234 Q313 239 318 240 L323 241 '
 'Q329 244 324 249 L319 253 L317 264 Q311 269 303 271 '
 'L295 278 L292 277 L287 279 V292 Q287 300 284 303 '
 'L277 307 L270 307 L263 311 H239 L235 307 L231 309 '
 'L223 306 L217 305 Q211 303 211 297 L210 278 L207 276 '
 'Q191 272 178 261 L177 249 L170 242 Q167 238 172 236 '
 'L177 236 Q182 235 181 231 L179 220 L175 210 Q172 207 174 201 '
 'L176 190 L177 186 Z')
LEFT_EYE=('M188 211 C192 205 199 201 205 200 C211 200 216 199 220 200 '
 'C226 201 226 208 230 213 C233 219 234 226 232 231 L229 237 '
 'C225 242 224 246 216 248 C207 252 198 250 192 246 '
 'Q187 243 187 239 L185 221 C184 217 184 214 188 211 Z')
RIGHT_EYE=('M265 203 C273 200 286 202 296 203 C301 204 306 206 307 211 '
 'C311 218 306 231 306 240 C306 245 297 250 288 252 Q283 253 280 252 '
 'C275 252 272 245 267 241 C262 238 261 232 261 225 L260 214 C260 209 262 205 265 203 Z')
NOSE_LEFT='M247 239 C244 240 241 245 237 248 L232 261 C230 268 231 272 237 272 H243 L244 245 Z'
NOSE_RIGHT='M249 242 L255 255 C259 263 261 272 255 272 H248 L247 247 Z'

LEFT_FLAG=('M125 190 C130 208 140 228 153 253 L201 321 C199 329 195 338 191 343 '
 'L189 344 L190 348 L186 350 L183 353 L185 358 L181 362 L178 367 L178 372 '
 'L174 374 L177 378 L172 389 C170 402 175 418 166 432 L162 431 L160 436 '
 'L160 445 L157 447 L160 451 L154 465 L147 476 L144 474 '
 'Q140 470 141 462 C139 459 136 459 134 461 L132 464 L132 459 '
 'L129 453 L126 450 L123 451 L120 444 L119 436 L118 426 '
 'Q116 424 115 428 L114 432 L110 432 L108 428 L105 423 '
 'Q100 418 102 411 L104 402 C105 399 108 396 107 394 '
 'C105 391 102 392 101 396 C100 401 97 403 93 402 '
 'C88 401 87 396 88 390 L86 380 L86 376 L85 382 L83 382 Q82 374 83 367 L83 355 '
 'C84 343 87 333 89 330 C94 319 100 308 100 300 '
 'C101 288 96 280 95 271 C93 263 94 254 98 240 '
 'L109 220 C115 211 122 201 125 190 Z '
 'M96 347 C95 353 97 361 100 365 C101 370 99 373 97 370 '
 'C94 364 93 357 93 351 Z')
RIGHT_FLAG=('M368 190 C377 205 381 227 384 245 C384 258 392 267 400 280 '
 'L414 307 C419 315 421 324 428 334 C433 343 432 350 430 355 L425 356 '
 'C424 360 424 365 427 368 L422 377 L421 382 L417 379 L415 382 L410 381 L407 379 '
 'C407 388 414 394 412 401 C410 410 407 414 403 418 L400 415 L396 417 '
 'C395 424 397 427 399 430 L396 433 L395 436 L392 438 L391 442 '
 'L387 443 L387 446 L384 446 L383 448 L385 451 L385 455 L383 455 '
 'L381 460 H373 L373 462 L376 467 L377 469 L374 471 L368 473 '
 'Q365 472 364 465 L360 451 '
 'C358 444 357 438 357 432 L356 428 L352 422 L351 418 L346 414 '
 'L341 408 L338 404 L334 396 L330 391 L326 386 L326 382 L327 381 '
 'L324 375 L320 372 L316 370 L314 367 L314 361 L312 355 L308 350 '
 'L306 346 L304 345 L303 339 L299 338 L298 332 L294 325 '
 'L367 199 Q367 194 368 190 Z '
 'M411 334 C419 337 415 346 412 350 C408 348 408 341 410 336 Z')

LEFT_FINIAL=('M91 153 C94 153 95 160 97 164 C102 170 108 173 116 171 '
 'C123 168 124 163 124 157 C124 150 121 146 118 143 '
 'C115 141 115 138 118 138 C123 139 129 146 132 153 '
 'C134 160 133 168 128 173 L123 178 L121 179 '
 'C111 182 101 180 94 174 C89 170 88 166 88 162 '
 'C88 158 89 154 91 153 Z')
RIGHT_FINIAL=('M374 144 C378 143 379 147 376 150 C369 156 366 161 367 167 '
 'C368 174 374 178 381 178 C389 179 395 175 398 170 L400 164 '
 'C402 158 405 159 405 164 C407 170 404 178 398 182 '
 'C389 188 376 187 368 182 C361 178 359 171 360 162 '
 'C360 155 365 150 370 146 Z')
LEFT_FINIAL_FACE=('M99 151 C100 147 105 145 110 146 C114 146 118 149 118 154 '
 'L121 156 Q123 159 119 161 L118 166 L119 169 L114 169 L110 166 '
 'L106 166 L104 162 L101 160 L99 157 Z')
RIGHT_FINIAL_FACE=('M378 154 C380 151 385 150 389 152 C395 155 396 162 392 166 '
 'L389 169 L389 172 L384 173 L380 176 L377 173 L375 170 '
 'L374 165 L375 162 L377 160 Z')

LEFT_STAFF='M125.8 178.8 L312.4 501.8 L308.2 504.2 L121.6 181.2 Z'
RIGHT_STAFF='M371 185.2 L183.5 508.2 L179.5 505.8 L367 182.8 Z'
BROWS=(
    'M187 198 Q191 197 194 192 C197 188 204 191 209 193 L218 193 '
    'C229 195 236 199 241 204 L243 208 C243 213 239 215 237 218',
    'M253 209 V203 L260 196 C269 187 283 188 294 191 C301 193 307 197 309 200 '
    'L314 207 L315 213')
BROW_FACES=(
    BROWS[0]+' L237 212 C232 201 225 199 220 199 H205 C199 199 193 204 187 210 Z',
    BROWS[1]+' L307 210 C303 203 299 201 293 200 L272 199 '
    'C268 198 265 203 262 209 Z')


def clip(d,key,path):
    result=element(d.defs,'clipPath',id=key)
    element(result,'path',d=path,fill_rule='evenodd',clip_rule='evenodd')
    return f'url(#{key})'


def field(d,key,stops,y0,y1):
    return d.linear(key,stops,y1=y0,y2=y1,calibrate=False)


def soft_path(d,parent,key,path,ink,alpha=1,radius=2):
    element(parent,'path',id=key,d=path,fill=ink,opacity=alpha,
            filter=d.glow(key+'-softness',radius))


def wash(d,parent,key,cx,cy,rx,ry,colour,alpha=1,angle=0):
    # Each wash is a named anatomical plane or cloth highlight. It is not a
    # texture tile or a field of sampled source pixels.
    gradient=element(d.defs,'radialGradient',id=key,gradientUnits='userSpaceOnUse',
                     cx=0,cy=0,r=1,gradientTransform=
                     f'translate({cx} {cy}) rotate({angle}) scale({rx} {ry})')
    for offset,a in ((0,alpha),(.4,alpha*.9),(1,0)):
        element(gradient,'stop',offset=offset,stop_color=colour,stop_opacity=a)
    element(parent,'rect',width=d.width,height=d.height,fill=f'url(#{key})')


def cloth_surface(d,parent,key,path):
    paint=field(d,key+'-base',[(0,(137,158,180)),(.35,(122,147,174)),
                             (.72,(112,137,165)),(1,(93,117,145))],190,476)
    group=body(d,parent,key,path,paint)
    group.set('filter',material_grain(d,key+'-cloth-grain',frequency=(.7,.35),contrast=.05,seed=43))
    return element(group,'g',clip_path=clip(d,key+'-fold-bounds',path))


def orbital_material(d,parent,side,path):
    material=element(parent,'g',clip_path=clip(d,side+'-orbital-material-bounds',path))
    if side=='left':
        low=('M196 211 C201 207 207 209 211 207 L217 209 L213 214 '
             'L209 214 L205 218 L202 217 L199 221 L194 222 Z',
             'M202 222 L208 218 L211 219 L210 222 L214 224 L211 227 '
             'L207 227 L206 230 L202 231 L199 229 L202 226 Z',
             'M193 233 L197 230 L200 232 L203 230 L206 233 L204 236 '
             'L200 235 L199 239 L195 241 L192 238 Z')
        high=('M195 216 L199 214 L203 216 L206 213 L210 214 L212 212',
              'M193 225 L196 222 L199 223 L202 221 L205 223',
              'M195 234 L199 231 L201 233 L205 231 L208 232',
              'M204 240 L208 237 L212 238 L215 235 L219 234')
    else:
        low=('M270 212 L274 209 L279 210 L281 213 L287 213 L291 217 '
             'L287 220 L282 218 L279 221 L275 219 L271 220 Z',
             'M292 219 L296 217 L301 219 L303 223 L299 226 L295 225 '
             'L292 228 L289 225 Z',
             'M271 230 L275 228 L278 231 L282 230 L285 234 L282 237 '
             'L278 235 L275 238 L271 237 L269 234 Z')
        high=('M272 215 L276 212 L280 214 L284 213 L288 216',
              'M287 221 L291 219 L294 221 L298 220 L301 222',
              'M273 234 L277 232 L280 234 L283 232',
              'M282 245 L286 242 L290 243 L293 240 L297 239')
    for i,contour in enumerate(low):
        soft_path(d,material,f'{side}-orbit-pocket-{i}',contour,'#142332',.2,.65)
    light=element(material,'g',fill='none',stroke='#829cb5',stroke_opacity=.3,
                  stroke_width=.55,stroke_linecap='round',stroke_linejoin='round',
                  filter=d.glow(side+'-orbit-ridge-softness',.55))
    for contour in high:element(light,'path',d=contour)


def cloth_folds(d,parent):
    left=cloth_surface(d,parent,'left-cloth',LEFT_FLAG)
    # The Vista folds are short diagonal crests separated by broad troughs.
    # Their endpoints follow the ragged cloth edge and the attached staff.
    for key,path,ink,alpha,softness in (
        ('left-upper-trough','M113 211 C113 236 125 257 150 278 L171 292 '
         'C151 267 135 243 125 207 Z','#455b73',.8,3.5),
        ('left-upper-crest','M106 227 C111 253 123 270 146 291 L184 316 '
         'C164 288 134 268 120 242 L114 224 Z','#c0cfdc',.25,4),
        ('left-middle-trough','M93 266 C112 288 135 311 166 327 L199 326 '
         'L191 350 C162 339 133 311 110 294 Z','#405670',.75,4),
        ('left-middle-crest','M100 284 C117 304 136 320 156 326 L183 332 '
         'C159 322 139 302 116 285 Z','#c4d2de',.65,3),
        ('left-lower-crest','M90 317 C98 331 120 354 139 369 '
         'C152 380 156 391 161 404 L147 405 C136 385 124 370 112 360 '
         'C96 346 88 335 90 317 Z','#bdcedd',.72,4),
        ('left-tear-trough','M82 368 C94 382 107 389 119 387 '
         'C133 387 143 398 147 415 L153 449 L140 469 '
         'C133 445 119 434 110 421 C113 406 111 399 99 397 '
         'C88 397 80 388 82 368 Z','#3c516a',.83,3.5),
        ('left-bottom-crest','M102 356 C117 373 132 397 143 409 '
         'C150 422 154 434 156 445 L146 463 C147 438 135 423 124 411 '
         'C115 395 108 375 102 356 Z','#b0c5d8',.7,4),
        ('left-trailing-shadow','M190 313 L204 318 C201 334 188 351 180 370 '
         'L170 422 L153 467 C161 431 157 411 164 389 '
         'C168 361 182 338 190 313 Z','#32455b',.9,3),
        ('left-edge-valley','M90 271 C92 284 99 295 97 306 '
         'C94 315 87 329 84 346 L78 355 C79 327 93 311 92 300 Z','#475c75',.65,2),
    ):soft_path(d,left,key,path,ink,alpha,softness)
    wash(d,left,'left-upper-rim',108,240,10,35,'#c8d6e2',.45,-20)
    wash(d,left,'left-hole-rim',96,365,10,17,'#d6e1e9',.6,-18)
    wash(d,left,'left-bottom-tear',133,447,13,21,'#374a60',.65,-25)
    wash(d,left,'left-lower-fold-face',137,400,11,25,'#c5d4e1',.58,-30)
    right=cloth_surface(d,parent,'right-cloth',RIGHT_FLAG)
    for key,path,ink,alpha,softness in (
        ('right-upper-crest','M367 202 C362 238 351 269 324 289 '
         'L302 317 L292 318 C306 294 329 274 341 249 C352 229 360 208 367 202 Z',
         '#d0dce6',.67,4),
        ('right-upper-trough','M374 218 C380 247 372 263 359 278 '
         'C345 294 329 314 300 331 L290 334 L302 349 '
         'C329 333 351 311 365 291 C379 270 385 250 374 218 Z','#405772',.82,4.5),
        ('right-middle-crest','M386 272 C393 292 403 307 403 324 '
         'C398 343 378 363 361 378 L357 395 L367 395 '
         'C385 375 404 351 410 331 C416 307 397 282 386 272 Z','#c7d6e2',.8,4),
        ('right-middle-trough','M410 311 C421 321 428 338 422 349 '
         'C412 359 398 370 398 385 L402 407 L389 424 '
         'C393 396 381 388 387 374 C393 359 409 349 411 335 Z','#3b5069',.9,4),
        ('right-lower-crest','M373 371 C371 391 367 401 362 410 '
         'L359 421 L366 425 C372 414 378 407 380 398 L381 373 Z',
         '#bccddd',.64,4),
        ('right-trailing-shadow','M284 321 C297 336 305 347 313 361 '
         'L332 396 L344 420 L350 445 L358 473 L346 452 C346 438 338 422 327 408 '
         'L309 382 L295 362 Z','#304258',.92,3),
        ('right-torn-edge','M414 361 L433 360 L419 385 L413 405 L401 424 '
         'L389 443 L379 454 L373 466 L365 470 C371 453 371 445 383 432 '
         'C396 417 389 402 397 388 Z','#394d64',.68,3),
    ):soft_path(d,right,key,path,ink,alpha,softness)
    wash(d,right,'right-tip-light',369,217,8,23,'#bdcfdf',.4,17)
    wash(d,right,'right-hole-rim',422,340,8,17,'#dae4ec',.65,20)
    wash(d,right,'right-cross-fold-valley',346,312,15,6,'#40546c',.35,-35)
    wash(d,right,'right-lower-fold-valley',361,432,8,15,'#455c75',.35,-10)
    wash(d,right,'right-bottom-fold-face',369,450,8,15,'#9fb7ce',.3,-10)


def skull_surface(d):
    group=element(d.root,'g',id='skull-surface-paint',opacity=91/255,
                  filter=material_grain(d,'skull-bone-grain',frequency=(.44,.55),contrast=.25,seed=53))
    bone=field(d,'skull-base',[(0,(183,199,216)),(.2,(109,135,164)),
        (.45,(123,148,176)),(.67,(151,171,193)),(.85,(132,155,180)),
        (1,(123,146,172))],117,311)
    element(group,'path',d=SKULL,fill=bone)
    face=element(group,'g',clip_path=clip(d,'skull-paint-bounds',SKULL))
    soft_path(d,face,'cranial-dark-band',
        'M175 174 C199 148 219 142 246 139 C278 137 307 147 328 167 '
        'L328 181 C308 163 281 155 255 155 C222 154 200 165 176 188 Z',
        '#43556c',.9,3.2)
    soft_path(d,face,'cranial-light-cap',
        'M177 152 C199 127 230 115 252 119 C286 122 312 141 326 160 '
        'C299 139 274 131 253 130 C224 129 201 139 177 159 Z','#d0dce6',.45,3)
    for key,cx,cy,rx,ry,ink,alpha,angle in (
        ('left-temple',183,181,17,32,'#35485e',.9,18),
        ('right-temple',314,186,12,27,'#546c86',.7,-18),
        ('forehead-centre',253,184,22,25,'#c9d7e3',.5,0),
        ('left-frontal-plane',216,177,14,23,'#90aac3',.6,12),
        ('right-frontal-plane',280,182,15,23,'#c3d2df',.5,-15),
        ('glabella-shadow',246,204,8,15,'#344a63',.85,-10),
        ('nose-bridge-light',253,228,7,22,'#c4d5e4',.65,-10),
        ('left-zygoma',193,261,20,12,'#c4d3e0',.5,-10),
        ('right-zygoma',307,260,20,11,'#c8d6e2',.55,-20),
        ('maxilla-light',241,275,21,7,'#bdcedd',.6,4),
        ('upper-jaw-shadow',247,286,29,8,'#435a74',.85,0),
        ('left-alveolar-plane',220,282,7,13,'#536c86',.7,-15),
        ('right-alveolar-plane',280,282,7,13,'#526c88',.7,15),
        ('left-cheek-foramen',206,258,4,4,'#202e3e',.95,0),
        ('right-cheek-foramen',283,258,4,5,'#1d2d40',.95,20),
    ):wash(d,face,key,cx,cy,rx,ry,ink,alpha,angle)
    for key,path,ink,alpha,softness in (
        ('left-nasal-shadow','M235 212 C230 231 227 247 217 258 L216 279 '
         'L228 287 C224 264 232 249 242 241 L244 220 Z','#354b64',.86,2.5),
        ('right-nasal-rim','M257 212 C259 227 265 240 278 249 L288 254 '
         'L281 264 C267 253 260 247 255 235 Z','#b9cddd',.76,2),
        ('left-orbital-rim','M185 216 C187 230 186 241 194 246 '
         'C206 251 220 251 227 241 L236 227 C237 245 228 255 212 256 '
         'C193 258 181 247 185 216 Z','#d0dce6',.75,2),
        ('right-orbital-rim','M261 227 C262 241 273 250 286 252 '
         'C297 254 305 248 307 238 L311 222 C313 246 308 257 291 258 '
         'C271 258 260 246 261 227 Z','#cfdae4',.77,2),
        ('nasal-sill-shadow','M231 267 C239 272 250 272 260 267 '
         'L261 274 C251 279 237 279 228 272 Z','#26394e',.9,1.6),
    ):soft_path(d,face,key,path,ink,alpha,softness)
    wash(d,face,'nasal-plate-light',242,276,14,4,'#bed0e0',.6,0)
    cavities=element(face,'g',id='orbital-cavities')
    cavity=field(d,'orbital-depth',[(0,(23,32,43)),(.4,(53,69,89)),
                                 (.68,(39,51,67)),(1,(57,75,96))],202,253)
    for side,path in (('left',LEFT_EYE),('right',RIGHT_EYE)):
        element(cavities,'path',d=path,fill=cavity)
        orbit=element(cavities,'g',clip_path=clip(d,side+'-orbit',path))
        if side=='left':
            soft_path(d,orbit,'left-orbital-wall','M185 213 C199 210 211 215 221 210 '
                'L219 219 L202 226 L204 239 L195 245 L187 242 Z','#607b96',.78,2)
            soft_path(d,orbit,'left-orbital-ledge','M191 231 C201 226 206 229 214 224 '
                'L222 225 C213 232 201 236 192 238 Z','#94abc0',.25,1.6)
            wash(d,orbit,'left-orbit-inner-depth',221,230,9,17,'#101823',.82,12)
        else:
            soft_path(d,orbit,'right-orbital-wall','M271 213 C281 219 290 214 306 216 '
                'L308 226 L289 231 L277 225 L272 230 Z','#92a8bd',.72,2)
            soft_path(d,orbit,'right-orbital-septum','M278 218 L283 218 L282 230 '
                'L277 232 Z','#2a3d53',.85,1)
            wash(d,orbit,'right-orbit-lower-depth',282,237,17,12,'#141e2a',.2,0)
        orbital_material(d,orbit,side,path)
    nasal=field(d,'nasal-depth',[(0,(22,28,36)),(1,(20,26,34))],239,270)
    for path in (NOSE_LEFT,NOSE_RIGHT):element(face,'path',d=path,fill=nasal)
    for i,path in enumerate(BROW_FACES):
        element(face,'path',d=path,fill=bone)
        brow=element(face,'g',clip_path=clip(d,'brow-paint-'+str(i),path))
        wash(d,brow,'brow-ridge-'+str(i),231 if i==0 else 280,202 if i==0 else 197,
             12 if i==0 else 20,6 if i==0 else 7,'#e0e7ee' if i==0 else '#ffffff',
             .8 if i==0 else 1,30 if i==0 else 6)
        wash(d,brow,'brow-trough-'+str(i),215 if i==0 else 307,195 if i==0 else 207,
             14,5,'#455d78',.7,12 if i==0 else 30)
    # Vertical tooth-root shadows remain separate from the outlined enamel.
    for key,path in (
        ('left-canine-root','M222 272 C225 278 226 285 224 292 L219 289 L218 277 Z'),
        ('left-incisor-root','M235 277 L241 278 L240 292 L235 294 Z'),
        ('right-incisor-root','M251 280 L257 278 L259 294 L253 296 Z'),
        ('right-canine-root','M270 274 L275 274 L278 287 L272 292 Z')):
        soft_path(d,face,key,path,'#40556e',.75,1.5)
    # Broad, irregular frontal planes visible between the cranial shadow and
    # the brows. The fine mottled bone texture still needs separate review.
    for key,path,ink,alpha,softness in (
        ('left-frontal-valley','M204 158 C202 165 206 171 202 178 L201 190 '
         'L209 191 C210 180 207 169 213 158 Z','#425b76',.4,1.7),
        ('centre-frontal-plane','M234 156 C239 162 234 168 237 174 '
         'L233 185 L240 190 C246 180 242 175 246 169 L244 157 Z','#b5c8da',.4,1.8),
        ('right-frontal-valley','M267 162 C264 167 269 172 265 178 '
         'L265 188 L270 188 L274 179 L271 172 L274 165 Z','#4f6883',.4,1.6),
        ('outer-frontal-plane','M294 173 C300 179 293 184 296 190 '
         'L306 196 L307 187 L301 181 L302 175 Z','#a2bbcf',.3,1.6),
    ):soft_path(d,face,key,path,ink,alpha,softness)
    return group


def body(d,parent,key,path,paint,folds=(),opacity=91/255):
    # Opaque paint inside a single translucent group keeps the alpha of
    # overlapping folds from accumulating through the cloth or bone face.
    group=element(parent,'g',id=key,opacity=opacity)
    element(group,'path',d=path,fill=paint,fill_rule='evenodd')
    shading=element(group,'g',clip_path=clip(d,key+'-surface',path),
                    filter=d.glow(key+'-fold-softness',3.3))
    for contour,ink,alpha in folds:
        element(shading,'path',d=contour,fill=ink,opacity=alpha)
    return group


def variant_illustration(d):
    d.family='variant-skull-flags-illustration'
    bone=field(d,'bone-grey',[(0,(172,190,207)),(.38,(130,153,176)),(.7,(108,132,157)),(1,(137,158,180))],117,311)
    crown=field(d,'crown-metal',[(0,(139,157,178)),(.65,(97,121,147)),(1,(137,155,176))],0,136)
    cloth_folds(d,element(d.root,'g',id='flag-cloth'))
    ornament=element(d.root,'g',id='staff-ornaments')
    for key,path in (('left-finial',LEFT_FINIAL),('right-finial',RIGHT_FINIAL),
                     ('left-finial-face',LEFT_FINIAL_FACE),('right-finial-face',RIGHT_FINIAL_FACE)):
        body(d,ornament,key,path,bone)
    outside=element(d.defs,'mask',id='staff-outside-skull',maskUnits='userSpaceOnUse',
                    x=0,y=0,width=512,height=512)
    element(outside,'rect',width=512,height=512,fill='white')
    element(outside,'path',d=SKULL,fill='black')
    poles=element(d.root,'g',id='crossed-staves',mask='url(#staff-outside-skull)')
    staff=field(d,'staff-grey',[(0,(134,155,180)),(1,(59,77,99))],180,508)
    for key,path in (('right-staff',RIGHT_STAFF),('left-staff',LEFT_STAFF)):
        body(d,poles,key,path,staff)
        element(poles,'path',d=path,fill='none',stroke='#84c1fb',stroke_width=1.7,
                stroke_linejoin='round')
    body(d,d.root,'crown',CROWN,crown,(
        ('M207 57 C214 63 208 89 217 101 C224 111 238 108 246 108 '
         'V120 C218 117 203 125 190 135 C192 116 203 112 202 99 Z','#c1d0df',.3),
        ('M247 44 C254 48 252 80 260 95 C267 110 280 106 287 99 '
         'C285 113 296 117 304 135 C291 119 270 114 253 113 Z','#29435c',.42)))
    for key,cx,cy,radius in (('crown-centre-jewel',246.5,10.5,9.2),
                             ('crown-left-jewel',173.5,57.2,5.5),('crown-right-jewel',320,57.2,5.5)):
        group=element(d.root,'g',id=key,opacity=91/255)
        element(group,'circle',cx=cx,cy=cy,r=radius,fill=crown)
    head=skull_surface(d)
    teeth=(
        'M212 282 L216 291 L216 301 L213 298 Z',
        'M218 290 L222 294 L221 304 L217 302 Z',
        'M224 294 Q227 293 229 296 L228 306 L223 304 Z',
        'M230 295 L235 298 L234 307 L229 305 Z',
        'M235 297 Q240 294 245 298 L247 303 V311 H238 L235 302 Z',
        'M247 300 Q251 295 257 297 L263 295 L259 306 L256 311 H247 Z',
        'M265 295 L270 294 L270 306 L262 310 Z',
        'M272 293 H277 L277 305 L271 307 Z',
        'M279 291 L283 287 L282 301 L278 304 Z',
        'M284 287 L287 280 V293 L283 300 Z')
    enamel=field(d,'tooth-enamel',[(0,(98,123,151)),(.55,(139,160,184)),
                                  (1,(140,160,182))],290,311)
    for i,path in enumerate(teeth):body(d,head,'tooth-'+str(i),path,enamel,opacity=1)
    tooth_edges=(
        'M212 282 L216 291 V301',
        'M218 290 Q220 292 222 294 L221 304',
        'M224 294 Q227 292 230 296 L229 305',
        'M230 295 Q233 295 235 298 L234 307',
        'M235 297 Q240 294 245 298 L247 303 V311',
        'M247 300 Q251 295 257 297 L263 295 L259 306 L256 311',
        'M265 295 L270 294 V306',
        'M272 293 H277 V305',
        'M279 291 L283 287 L282 301',
        'M284 287 L287 280')
    contours=element(d.root,'g',id='illustration-contours',fill='none',stroke='#84c1fb',
                     stroke_opacity=1,stroke_width=1.5,stroke_linejoin='round',stroke_linecap='round')
    for path in (LEFT_FLAG,RIGHT_FLAG,LEFT_FINIAL,RIGHT_FINIAL,LEFT_FINIAL_FACE,RIGHT_FINIAL_FACE,
                 CROWN,SKULL,LEFT_EYE,RIGHT_EYE,NOSE_LEFT,NOSE_RIGHT,*BROWS,*tooth_edges):
        element(contours,'path',d=path)
    for cx,cy,radius in ((246.5,10.5,9.2),(173.5,57.2,5.5),(320,57.2,5.5)):
        element(contours,'circle',cx=cx,cy=cy,r=radius)
    for path in ('M179 181 C183 195 178 203 180 214 L183 229 Q185 235 180 239',
                 'M312 207 L310 232 C309 238 315 241 321 242'):
        element(contours,'path',d=path)


def illustrations(d,name):
    if name!=VARIANT:return False
    variant_illustration(d)
    return True
