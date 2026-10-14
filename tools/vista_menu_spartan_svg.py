"""Experimental hand tracing of Vista's player-profile Spartan illustration.

The 512x1024 canvas, including its empty right and lower margins, is retained.
Armor boundaries and weapon contours use native reference coordinates. This
recipe is deliberately not admitted by the menu reconstruction dispatcher yet.
"""
from vista_prompt_svg import element
from vista_menu_materials_svg import linear
from vista_menu_surface_svg import material_grain


PROFILE='ui\\screens\\game_shell\\settings_screen\\player_profile\\profile'

SILHOUETTE=(
    'M88 64 V58 L96 49 L97 38 Q98 34 107 28 L116 25 L131 22 '
    'L145 24 L151 28 L157 39 L164 46 Q166 58 169 70 L176 72 L184 68 '
    'L206 75 Q213 78 217 85 L229 108 L225 126 L230 136 '
    'Q233 144 233 154 V172 L226 182 L213 205 L213 210 L220 215 '
    'L223 220 L222 230 L220 235 L225 240 '
    'L233 241 L238 245 L243 265 L250 299 L253 321 L247 329 '
    'L249 353 L251 363 L257 374 L265 387 L272 404 L275 427 '
    'L284 456 L280 463 L284 473 L286 493 L288 506 L297 514 '
    'L299 518 V527 L296 530 V537 Q296 541 291 542 L280 543 '
    'H273 L269 546 L263 542 L255 538 L254 534 L247 532 '
    'Q242 530 244 523 L249 513 V509 L243 505 L242 497 L244 480 '
    'L247 469 L248 464 L242 453 L230 430 L222 417 '
    'L216 398 L214 381 L205 363 L197 344 L190 303 L183 296 '
    'L178 286 H169 L162 289 L156 298 L144 315 L128 338 L115 354 '
    'L110 361 L105 366 L107 369 L118 375 L121 380 L125 396 '
    'L125 403 L118 421 L124 429 L123 433 L119 442 '
    'L124 456 L126 469 L128 477 L119 483 L106 485 L94 490 L93 497 '
    'L84 500 H77 L70 503 H56 L52 500 L53 495 H45 V489 '
    'Q47 482 57 476 L65 472 L71 467 L79 461 L79 448 L85 441 L88 441 '
    'L87 435 L82 432 L82 421 L81 411 L78 403 L70 396 L62 384 '
    'L56 375 L58 370 L62 361 L67 351 L72 341 L76 330 L78 320 '
    'L79 307 L95 276 L106 252 L115 239 L121 237 L131 241 '
    'L128 219 L108 202 L102 194 L93 189 L92 184 L83 183 L76 181 '
    'L68 185 L58 190 L48 192 L37 190 L22 188 L18 184 L17 178 '
    'L28 159 L38 151 L47 146 L57 142 L67 135 L73 127 L77 117 '
    'L78 107 L75 100 L70 99 '
    'V90 L74 87 L75 80 H84 L85 88 L90 92 L92 99 L91 104 '
    'L99 105 L96 101 L94 96 V88 L97 83 H100 L97 65 Z')


ARMOR_PANELS=(
    # Helmet shell and visor surround, retraced directly in native space.
    ('helmet-side-cap','M97 38 Q101 33 110 28 L107 43 L102 48 H97 Z','metal-dark'),
    ('helmet-top','M111 26 L130 24 L129 42 L107 46 L108 36 Z','metal'),
    ('helmet-cap','M131 24 L145 25 L151 28 L157 38 L162 43 L137 41 L129 43 Z','metal-light'),
    ('helmet-side-face','M107 46 L130 43 L138 42 L162 43 L164 47 '
     'L140 50 L127 52 L105 55 Z','metal'),
    ('helmet-brow','M96 49 L107 46 L105 55 L128 52 L130 51 L150 49 '
     'L163 47 L161 53 L149 56 L129 59 L127 61 L106 64 H88 V58 L93 52 Z','metal'),
    ('helmet-side-rail','M110 28 L112 27 L108 45 L106 55 L105 63 L102 64 '
     'L104 52 L106 39 Z','edge'),
    ('helmet-front-rail','M130 24 H132 L130 42 L128 52 L127 59 H125 L126 50 L129 42 Z','metal-light'),
    ('helmet-brow-slot','M110 57 L126 54 V59 L110 62 Z','rubber'),
    ('visor','M100 65 L119 62 L139 59 L155 56 Q157 58 157 63 V73 '
     'Q157 78 151 83 L144 89 L135 96 L132 98 H121 Q106 96 101 84 L99 65 Z','visor'),
    ('helmet-left-cheek','M94 65 H99 L101 80 L107 91 L112 98 L108 103 '
     'L100 98 L97 89 Z','metal'),
    ('helmet-right-cheek','M159 54 L164 58 L166 70 L169 74 L165 77 '
     'L163 85 L158 95 L151 103 L144 108 L140 104 L144 99 L154 88 '
     'L160 77 L161 65 Z','metal'),
    ('helmet-chin','M111 100 L121 105 H128 L134 102 L137 105 V113 L139 119 '
     'L136 122 H123 L119 118 L116 109 Z','metal'),
    ('helmet-chin-inset','M121 109 H132 L134 113 V120 H124 L122 117 Z','rubber'),
    ('helmet-jaw-face','M136 99 L145 92 L154 83 L160 78 L163 77 L161 88 '
     'L153 99 L147 105 L143 107 L139 105 Z','metal-light'),
    ('helmet-left-temple','M96 84 Q100 80 104 85 L107 91 L106 99 L103 102 '
     'L97 100 L95 96 V89 Z','metal'),
    ('helmet-ear','M160 76 L166 73 L171 75 L173 82 V89 L170 94 '
     'L166 96 L162 93 L161 86 Z','edge'),
    # Rear shoulder, chest, and neck.
    ('rear-shoulder-top','M173 69 L182 68 L202 74 L209 77 L227 104 '
     'L229 109 L225 127 L218 125 L211 116 L199 105 L190 92 L172 87 Z','metal'),
    ('rear-shoulder-face','M173 86 L191 90 L174 114 L156 111 L157 105 Z','metal'),
    ('rear-shoulder-rim','M191 90 L207 106 L194 109 L175 124 L169 125 '
     'L164 115 L174 103 Z','metal'),
    ('rear-shoulder-outer','M198 75 L207 78 L221 95 L223 98 L211 99 '
     'L208 104 L192 91 Z','metal-light'),
    ('rear-shoulder-tip','M223 98 L229 108 L225 127 L211 113 L208 104 L211 99 Z','metal-dark'),
    ('rear-shoulder-opening','M192 109 L207 108 L225 126 L218 125 '
     'Q208 126 202 135 L194 141 Q188 145 184 146 L177 150 L171 158 '
     'V139 L173 125 Z','rubber'),
    ('rear-shoulder-vent-upper','M183 70 L185 69 L194 72 L189 77 L183 78 Z','rubber'),
    ('rear-shoulder-vent-lower','M184 80 L191 76 L197 78 V86 H190 L183 85 Z','rubber'),
    ('neck','M118 119 L159 114 L158 126 Q152 133 138 138 H129 '
     'Q122 137 119 132 Z','rubber'),
    ('neck-collar-upper','M119 121 Q122 128 129 129 H137 Q150 125 157 118 '
     'L157 121 Q151 127 138 131 H129 Q122 130 119 124 Z','metal'),
    ('neck-collar-lower','M120 128 Q123 135 130 135 H137 Q149 131 157 123 '
     'L156 126 Q149 133 138 137 H130 Q124 136 121 132 Z','metal'),
    ('left-chest-rim','M94 105 H100 L111 116 L115 119 L109 144 V152 '
     'L116 156 V161 L111 159 L104 154 L100 146 L94 141 L92 130 Z','metal'),
    ('left-chest-inset','M97 109 H101 L108 116 L106 129 H101 V116 L97 114 Z','rubber'),
    ('centre-chest','M116 121 L157 120 L151 135 L141 148 L133 159 '
     'L125 164 L117 160 L116 156 L110 153 L109 150 L112 141 L114 132 Z','metal-dark'),
    ('right-chest-edge','M157 106 L169 109 L157 120 L171 123 L169 128 L156 125 Z','edge'),
    ('right-chest-plate','M155 121 L168 125 L155 140 L145 136 L148 129 Z','metal'),
    ('right-chest','M168 128 L173 130 L170 142 L166 157 L156 164 '
     'L142 163 L143 146 L147 137 L155 141 Z','metal-dark'),
    # Near arm: shoulder and upper arm, behind the horizontal forearm.
    ('near-shoulder','M195 142 L199 130 Q202 127 207 126 L214 124 '
     'Q220 124 225 131 L230 140 Q233 145 233 153 V172 L225 176 '
     'L218 174 L214 166 L208 164 L202 167 L196 167 Z','metal-light'),
    ('near-shoulder-lower','M194 168 L203 164 L208 165 L218 173 L219 177 '
     'V189 L211 209 L205 219 H201 V209 L193 203 L186 198 L181 192 '
     'L180 188 L183 181 H190 L193 176 Z','metal'),
    ('near-shoulder-band','M194 167 L207 163 L220 174 V188 L212 209 '
     'L205 219 L201 218 L202 210 L207 204 L214 188 V177 L206 169 '
     'L197 172 L195 178 L191 183 L182 184 L180 189 L178 186 L181 179 H190 Z','metal-dark'),
    ('near-upper-arm','M171 158 L178 153 L182 146 L195 141 V165 '
     'L191 173 L183 176 L179 182 L177 186 L168 178 L164 173 L165 166 Z','metal'),
    ('arm-joint','M196 202 L202 207 V218 L210 207 L213 211 L207 225 '
     'L197 237 L191 232 L190 214 Z','rubber'),
    # Far arm and hands around the rifle grip.
    ('far-upper-arm','M70 123 L76 117 L77 133 L73 140 L67 145 L62 147 '
     'L53 145 L59 139 Z','metal-dark'),
    ('far-upper-arm-edge','M51 145 L59 139 L69 135 L74 132 L76 136 '
     'L68 140 L61 145 L59 148 Z','metal'),
    ('far-forearm','M42 149 L50 146 L60 149 L64 153 V163 L66 174 L72 180 '
     'L66 187 L56 191 L46 192 L32 189 L22 188 L18 183 L17 179 '
     'L26 165 L35 155 Z','metal-dark'),
    ('far-bracer','M25 168 L34 170 L41 174 Q44 179 45 185 V189 L31 188 '
     'L22 187 L18 182 V178 Z','metal'),
    ('far-thumb','M62 148 L67 146 L68 153 L67 158 V163 L66 170 L67 174 '
     'L69 177 L63 176 L62 164 L61 157 Z','metal-dark'),
    ('front-glove','M71 140 L83 138 L91 140 H95 Q99 142 100 148 V151 '
     'Q103 158 109 160 L114 161 L115 168 L114 179 L110 187 L108 190 '
     'L103 188 L92 187 L82 186 H76 L71 182 L67 179 L63 176 L62 169 '
     'L64 162 L65 159 L64 152 L65 147 Z','rubber'),
    ('front-hand-rim','M65 146 L70 143 V153 L68 158 V165 L66 169 L67 175 '
     'L71 178 L76 182 L81 184 H85 V187 H80 L75 185 L70 181 L62 177 '
     'L61 174 V159 L63 152 Z','metal'),
    ('front-forearm','M112 164 L159 174 Q178 181 184 188 '
     'Q192 198 193 212 L194 232 L199 235 Q204 240 202 245 '
     'Q199 249 193 249 L184 246 L168 237 L149 226 L128 216 '
     'L107 197 L103 190 Z','metal'),
    ('front-bracer','M112 165 L146 175 Q158 179 160 186 V195 L150 202 '
     'L132 207 L106 189 L111 181 Z','metal-light'),
    ('front-bracer-face','M115 170 L142 178 Q153 181 157 186 L157 191 '
     'L135 189 L113 182 Z','metal'),
    ('front-wrist','M109 154 L114 157 L118 161 L116 171 L114 181 L110 189 '
     'L105 192 L101 189 L109 183 L110 173 V162 L108 159 Z','rubber'),
    # Abdomen and belt, visible beneath the near forearm.
    ('abdomen','M128 218 L144 229 L163 238 L179 243 L192 241 L199 247 '
     'L193 257 L178 262 L159 259 L143 248 L130 239 Z','metal-dark'),
    ('front-elbow-band','M153 229 L159 225 H174 L180 231 L184 232 '
     'L188 230 L194 233 L199 235 Q203 240 202 244 L199 247 '
     'Q195 250 190 248 L180 244 L170 240 L160 236 Z','metal'),
    ('right-waist','M204 218 L216 216 L221 220 V232 L211 239 L197 252 '
     'L192 250 L195 238 Z','metal'),
    ('belt-centre','M155 242 L179 247 V262 H155 Z','metal'),
    ('pelvis-centre','M155 262 H180 Q178 275 175 284 L173 287 H163 '
     'Q158 283 157 277 Z','metal-dark'),
    ('pelvis-left','M130 240 L151 259 L154 276 L158 286 L163 288 L147 291 '
     'Q133 284 129 276 L126 254 Z','rubber'),
    ('pelvis-right','M216 237 L220 240 L217 247 Q201 259 199 266 '
     'L195 279 L189 292 L179 289 L177 287 L183 274 L187 254 L198 247 Z','rubber'),
    # The left leg is retraced in native space. Paint regions can close behind
    # another surface; visible open contours are authored separately below.
    ('left-thigh','M118 243 L128 242 L135 251 L142 260 L149 278 L154 292 '
     'L144 306 L121 341 Q103 337 84 326 L77 319 L95 281 L108 254 Z','metal-dark'),
    ('left-thigh-outer','M116 242 Q112 246 111 254 L112 267 L108 276 '
     'L102 282 L97 300 L91 318 Q93 324 100 328 L116 334 L123 335 '
     'L120 339 Q106 337 97 332 L80 321 L89 300 L104 268 Z','metal'),
    ('left-thigh-face','M106 286 L111 289 L112 307 Q122 315 135 318 '
     'L129 327 L105 317 L103 300 Z','metal-dark'),
    ('left-thigh-upper-bevel','M112 275 L119 281 L120 295 Q124 300 132 303 '
     'L143 307 L142 310 Q130 307 119 298 L118 283 L110 278 Z','metal'),
    ('left-thigh-middle-bevel','M105 285 L111 289 L112 307 Q120 313 129 316 '
     'L135 318 L134 320 Q121 317 110 309 L109 290 L104 288 Z','metal'),
    ('left-thigh-lower-bevel','M100 299 L103 302 L103 319 Q112 325 124 329 '
     'L127 330 L125 333 Q111 329 101 321 L101 304 L99 302 Z','metal'),
    ('left-knee-upper','M77 322 L87 330 Q103 339 121 341 L110 359 '
     'Q103 361 96 359 L82 354 L68 345 Z','rubber'),
    ('left-knee-rim','M78 322 Q92 337 115 340 L119 341 L117 345 '
     'Q99 343 85 333 L77 325 Z','edge'),
    ('left-knee','M68 352 L85 362 L76 376 Q66 379 57 372 L61 363 Z','metal-dark'),
    ('left-knee-cap','M76 377 Q85 375 96 374 L107 377 L100 389 '
     'L95 398 L94 405 L87 403 L75 393 L69 385 Z','metal'),
    ('left-shin','M107 376 L118 377 Q124 386 125 399 L123 407 L118 420 '
     'L112 425 L105 422 L98 412 L94 405 L98 400 Z','metal'),
    ('left-calf','M78 403 L89 404 L97 410 L102 420 V432 L99 443 H89 '
     'Q88 440 87 437 V434 L81 430 L81 413 Z','metal-dark'),
    ('left-calf-side','M109 423 L113 425 L118 421 L124 428 L124 431 '
     'L120 439 L111 442 Z','metal'),
    ('left-ankle','M86 440 L92 441 H102 L111 441 L119 439 L121 446 '
     'L121 455 L124 460 L122 470 L106 469 L93 466 L79 461 V448 Z','metal'),
    ('left-boot-top','M79 461 L92 465 L110 470 L124 470 L127 477 '
     'L119 482 L106 485 L93 490 L89 484 L86 479 L77 473 L70 469 Z','metal-dark'),
    ('left-boot-toe','M70 469 L81 474 L89 479 Q94 482 94 488 L91 498 '
     'L84 499 H77 L71 502 H55 L51 499 L53 495 H45 V489 L48 482 '
     'L57 476 L64 473 Z','metal'),
    ('left-boot-sole','M45 490 H49 L52 492 L53 496 H50 L49 494 H45 Z','rubber'),
    # Native right leg: curved plate faces and the rounded toe with its recess.
    ('right-thigh','M222 242 Q229 239 234 245 L238 262 L244 285 L247 303 '
     'L250 327 L246 331 L244 333 L248 357 H235 L229 353 L226 349 '
     'L209 352 V361 L202 349 L199 339 L198 317 L195 297 L195 271 L207 257 Z','metal'),
    ('right-thigh-face','M209 259 L218 251 L221 246 Q224 251 230 253 '
     'L233 260 L231 265 V269 L235 278 L239 288 V302 L226 303 '
     'L216 295 L214 286 Z','metal-light'),
    ('right-thigh-edge','M195 272 L196 290 L198 300 L202 315 L204 332 '
     'L202 338 L198 341 L195 337 L193 323 L191 312 V300 L194 297 Z','rubber'),
    ('right-knee-upper','M214 351 L235 348 L241 356 H250 V370 L257 376 '
     'L253 382 L245 374 L242 369 Q230 372 220 374 L215 378 Z','rubber'),
    ('right-knee','M216 378 L220 375 L228 373 L239 370 L243 369 '
     'L249 376 L253 383 L251 387 L244 388 L239 394 L229 396 '
     'L221 393 L218 394 L216 389 Z','metal-dark'),
    ('right-knee-lower','M218 392 L226 396 L235 395 L242 391 L247 387 '
     'L254 385 V397 L256 406 L258 415 L256 421 L250 426 L241 429 '
     'L231 426 L223 421 L219 414 L217 404 Z','metal'),
    ('right-shin','M258 378 L265 383 Q273 393 274 404 V429 L270 432 '
     'L267 423 L265 413 H261 L260 401 V386 Z','metal'),
    ('right-shin-face','M263 414 L268 413 L271 416 L274 428 L276 438 '
     'L282 457 L278 461 L272 460 L269 453 L265 438 L261 429 L257 426 Z','metal-dark'),
    ('right-calf','M232 426 L242 430 L255 425 L259 414 L266 418 L272 435 '
     'L278 458 L269 463 L263 459 L254 461 V467 L248 468 L244 462 L238 448 Z','metal-dark'),
    ('right-ankle','M247 467 L253 466 V461 L262 459 L269 462 L279 460 '
     'L282 470 L284 475 H245 Z','rubber'),
    ('right-boot-upper','M247 476 Q261 473 284 473 L286 490 V494 H243 L244 484 Z','metal'),
    ('right-boot-band','M243 494 H286 L288 505 L284 508 H259 L250 512 '
     'L246 507 L244 505 Z','rubber'),
    ('right-boot-toe','M253 509 L260 507 H286 L298 516 L299 527 L296 530 '
     'L289 532 L280 528 H269 L259 532 L254 533 L248 532 L244 528 '
     'L245 523 L249 515 Z','metal'),
    ('right-boot-toe-cap','M254 531 Q264 520 273 520 Q286 518 295 531 '
     'V537 Q293 541 286 541 H273 L269 545 L263 541 L257 538 Z','metal'),
    ('right-boot-toe-recess','M268 540 Q270 533 277 534 Q283 533 285 540 Z','rubber'),
    # Forward-facing weapon contours, observed without assuming its identity.
    ('rifle-sight','M76 88 V81 L78 80 H82 L84 83 V87 L81 90 Z','metal'),
    ('rifle-muzzle','M73 89 Q80 86 86 91 Q89 94 88 98 L87 101 '
     'Q83 105 78 103 L74 101 Q70 99 70 95 Z','metal'),
    ('rifle-shaft','M77 102 L87 103 L88 113 L90 126 L89 136 L85 140 '
     'L75 138 L74 132 L75 116 Z','metal-dark'),
    ('rifle-frame','M74 137 Q80 131 88 137 L92 142 L94 150 V161 '
     'Q94 168 86 170 Q80 173 74 168 Q71 165 71 160 V148 Z','metal'),
    ('rifle-front-face','M78 139 Q84 137 89 143 L91 149 L90 155 L86 159 '
     'L77 161 L73 157 L72 149 L75 143 Z','metal-dark'),
    ('rifle-lower-lip','M73 158 Q80 162 89 158 L92 155 V161 '
     'Q92 167 87 170 Q80 173 75 168 Q72 165 72 161 Z','rubber'),
)

# Move the chest casing behind its neck ribs, and foreground arm/weapon
# components in front of the torso and belt. This is source layer ownership,
# not an automatic geometry reconstruction.
_centre=next(part for part in ARMOR_PANELS if part[0]=='centre-chest')
_parts=[part for part in ARMOR_PANELS if part[0]!='centre-chest']
_neck=next(i for i,part in enumerate(_parts) if part[0]=='neck')
_parts.insert(_neck,_centre)
# The upper-arm face sits behind the shoulder casing and its lower rim.
_upper_arm=next(part for part in _parts if part[0]=='near-upper-arm')
_parts.remove(_upper_arm)
_parts.insert(next(i for i,part in enumerate(_parts) if part[0]=='near-shoulder'),_upper_arm)
# The flexible hip insert covers the upper end of the left thigh casing.
_hip_insert=next(part for part in _parts if part[0]=='pelvis-left')
_parts.remove(_hip_insert)
_parts.insert(next(i for i,part in enumerate(_parts) if part[0]=='left-thigh')+1,_hip_insert)
ARMOR_PANELS=tuple(part for part in _parts if not part[0].startswith(('front-','rifle-')))+\
             tuple(part for part in _parts if part[0].startswith(('front-','rifle-')))
RIMLESS={'neck','neck-collar-upper','neck-collar-lower'}
NATIVE_OPEN_CONTOUR_PARTS=('belt','pelvis','left-thigh','left-knee','left-shin',
                           'left-calf','left-ankle','left-boot','right-thigh',
                           'right-knee','right-shin','right-calf','right-ankle','right-boot',
                           'rear-shoulder','near-shoulder','near-upper-arm','arm-joint',
                           'right-waist','right-chest','left-chest','centre-chest','far-',
                           'helmet','visor','front-glove','front-hand','front-wrist','rifle-')

HELMET_CONTOURS=(
    ('helmet-side-cap-seam','M110 28 L106 44 L102 63'),
    ('helmet-front-cap-seam','M130 24 L129 42 L127 52 L126 59'),
    ('helmet-cap-lower-seam','M97 48 L108 46 L130 43 L138 42 L162 43'),
    ('helmet-brow-upper-edge','M88 57 H102 L106 55 L127 52 L164 48'),
    ('helmet-brow-lower-edge','M88 64 H103 L107 62 L126 59 L157 55'),
    ('helmet-visor-perimeter','M100 65 L119 62 L139 59 L155 56 Q157 58 157 63 '
     'V73 Q157 78 151 83 L144 89 L135 96 L132 98 H121 Q106 96 101 84 L99 65'),
    ('helmet-left-temple-frame','M96 84 Q100 80 104 85 L107 91 L106 99 L103 102 L97 100 L95 96 V89 Z'),
    ('helmet-right-temple-frame','M160 76 L166 73 L171 75 L173 82 V89 L170 94 L166 96 L162 93 L161 86 Z'),
    ('helmet-jaw-ridge','M108 97 L119 103 L122 106 H128 L134 102 L140 104 L149 111'),
    ('helmet-chin-inset-rim','M120 108 L121 117 L124 121 H136 L134 109 L133 103'),
    ('helmet-lower-jaw','M110 103 L117 117 L123 122 H137 Q146 117 151 111 L159 103'),
    ('helmet-right-cheek-fold','M139 99 L146 92 L155 83 L161 78'),
)

HAND_CONTOURS=(
    ('grip-thumb-upper-edge','M94 140 Q99 141 100 149 Q100 153 106 158 L114 161'),
    ('grip-left-fold','M65 147 L63 157 L64 161 L62 166 V175 L69 178'),
    ('grip-lower-fold','M66 176 Q70 180 76 183 L83 186 H90 L103 188 L108 190 L112 181 L114 166'),
    ('grip-heel-fold','M73 182 L78 186 L85 188 H91'),
    ('wrist-upper-step','M111 142 L113 150 L111 154 L116 158 V161'),
)

WEAPON_CONTOURS=(
    ('weapon-sight','M76 88 V81 L78 80 H82 L84 83 V87'),
    ('weapon-upper-shell','M73 89 Q80 86 86 91 Q89 94 88 98 L87 101 '
     'Q83 105 78 103 L74 101 Q70 99 70 95 Z'),
    ('weapon-shaft-left-edge','M75 104 V116 L74 134'),
    ('weapon-shaft-right-edge','M88 103 L89 115 L92 133'),
    ('weapon-forward-shell','M75 135 H85 L90 138 L94 142 V160 Q94 168 86 170 '
     'Q80 173 74 168 Q71 165 71 160 V148'),
    ('weapon-forward-face-rim','M76 139 Q82 135 88 141 L90 145 Q94 155 87 159 '
     'Q82 161 77 159 L74 156 V147 L75 142'),
)

TEMPLE_PORTS=(
    ('left-upper','helmet-left-temple',99.5,87,2.5,3),
    ('left-lower','helmet-left-temple',100,97,2.5,2.3),
    ('right-upper','helmet-ear',165.6,80.5,2.7,3.3),
    ('right-lower','helmet-ear',166.8,87.5,2.4,3.2),
)

UPPER_CONTOURS=(
    ('shoulder-vent-frame','M172 71 H180 V86 L175 88'),
    ('shoulder-upper-vent','M183 70 L185 69 L194 72 L189 77 L183 78 Z'),
    ('shoulder-lower-vent','M184 80 L191 76 L197 78 V86 H190 L183 85 Z'),
    ('shoulder-chest-strap','M171 86 L191 90 L172 113 L156 111 V108'),
    ('shoulder-strap-base','M155 111 L154 119 L168 123 H171 L174 126'),
    ('shoulder-diagonal-joint','M191 90 L207 106 L194 109 L171 126'),
    ('shoulder-outer-facets','M198 75 L207 78 L223 98 L211 99 L208 104'),
    ('shoulder-outside-lower-edge','M208 104 L225 126'),
    ('shoulder-arm-opening','M171 159 V143 L174 128 L192 110 Q201 106 207 109 L223 124'),
    ('shoulder-front-turn','M195 141 V165 L198 168'),
    ('near-arm-upper-rim','M198 168 L206 164 L210 165 L219 174 L220 178 V187'),
    ('near-arm-inner-rim','M198 171 L206 168 L216 176 V190'),
    ('upper-arm-side','M167 161 Q178 154 183 146 L195 141'),
    ('near-arm-elbow-indent','M169 178 L177 185 L179 188 L182 181 H190 L194 173'),
    ('near-arm-face-left-edge','M178 189 L184 196 L196 205 L202 208 V215 L204 218'),
    ('near-arm-slanted-recess','M196 192 L206 201 L207 205 L204 207 L194 197 L193 192 Z'),
    ('near-arm-face-tip','M208 208 L205 218'),
    ('waist-face-inside','M203 219 H215 L216 224 V232 Q214 240 209 244'),
    ('left-chest-pocket','M96 107 H102 L110 115 L107 130 H100 V116 L96 114'),
    ('left-chest-outer-face','M93 112 V127 L95 135 L94 140 L100 142 L106 153 L111 158 H118'),
    ('chest-left-inner-edge','M115 119 L109 145 V152 L115 156 V161 L121 163 L128 164'),
    ('chest-lower-groove','M113 142 Q119 149 125 149 H129 L136 147 L142 142'),
    ('chest-notch','M117 153 V156 L119 160 L124 157 L126 154'),
    ('chest-right-inset','M155 121 L167 125 L155 140 L145 135 L147 129 L155 122'),
    ('chest-right-trim','M157 107 L155 119 L169 123 V128 L174 127 L173 133'),
    ('chest-right-lower-edge','M168 128 L170 140 L166 157'),
    ('far-arm-upper-facet','M51 145 L59 148 L66 143 L69 137 L74 134'),
    ('far-forearm-pocket','M47 150 L51 153 L52 158 H62 V153 L65 149'),
    ('far-arm-inner-turn','M66 146 V159 L68 164 L65 168 V176 L69 178'),
    ('far-forearm-tip','M25 169 Q38 172 42 177 L45 184 V189'),
)

# Visible source boundaries are open curves, independent of the hidden
# closures needed by paint and clips. Coordinates are native, without width
# transforms or contour extraction from the source image.
LOWER_CONTOURS=(
    ('belt-face','M155 242 L179 247 V262 H155 V242'),
    ('pelvis-centre','M155 262 Q156 280 163 287 H173 Q178 276 180 262'),
    ('pelvis-left','M130 241 L151 259 Q152 280 161 288'),
    ('pelvis-left-inset','M134 249 L136 269 Q136 280 142 285 L148 289'),
    ('pelvis-right','M216 237 L219 240 L217 246 L200 263 Q196 280 189 292'),
    ('thigh-left-inset','M119 243 Q114 246 114 251 L115 262 Q115 273 109 278 '
     'L104 283 Q96 298 92 316 Q90 322 96 326 Q108 335 121 338'),
    ('thigh-upper-ridge','M112 275 L119 281 L120 295 Q124 301 132 303 L143 307'),
    ('thigh-middle-ridge','M105 285 L111 289 L112 307 Q122 315 135 318'),
    ('thigh-lower-ridge','M100 299 L103 302 L103 319 Q114 327 127 330'),
    ('knee-upper-rib','M78 320 Q95 337 120 341'),
    ('knee-lower-rib','M78 324 Q96 341 117 344'),
    ('knee-outside-grooves','M78 328 L84 334 M76 333 L83 339'),
    ('knee-joint-top','M71 339 Q89 358 106 359 H111'),
    ('knee-side','M65 352 L85 362 L76 376 Q66 379 58 371'),
    ('knee-face-top','M73 377 Q87 376 96 374 L118 377'),
    ('knee-face-divider','M86 376 Q89 393 94 403'),
    ('knee-shin-divider','M109 378 L99 401 L98 406 L111 423 L114 425 L119 421'),
    ('knee-inner-plate','M67 385 Q75 398 92 406 L99 404'),
    ('calf-front-plate','M79 405 L89 404 Q101 408 102 421 V431 L99 443 H89 '
     'L87 436 V434 L81 431 V418 L79 405'),
    ('calf-side-plate','M109 422 L111 442 Q120 440 124 432'),
    ('ankle-top','M85 440 L92 441 H101'),
    ('ankle-face-divider','M84 445 L97 450 L108 454 L123 460 L122 469'),
    ('boot-upper-rib','M79 461 Q98 472 122 471'),
    ('boot-toe-upper','M70 468 L87 478 Q93 481 94 487 L93 490'),
    ('boot-toe-face','M56 479 L64 482 L68 489 L74 492 H80 L82 487 L93 484'),
    ('boot-toe-side','M82 487 V499'),
    ('boot-sole-notch','M46 489 L51 491 L53 495'),
    ('right-thigh-plate','M209 259 L214 286 Q215 293 218 296 L226 303 H239 '
     'V290 L233 273 Q230 266 232 264 L233 260 L230 254 L226 253 L221 247'),
    ('right-thigh-inside','M195 271 L196 292 L200 307 L203 321 L204 334 '
     'Q201 340 197 342'),
    ('right-thigh-side-ribs','M198 307 L191 315 M203 320 L195 325'),
    ('right-thigh-lower-edge','M209 361 V351 L226 347 L233 354 L237 357 H249'),
    ('right-thigh-outer-groove','M248 300 L249 321 L247 326 L243 330'),
    ('right-thigh-small-ribs','M244 299 L246 300 M246 307 L248 308 '
     'M247 312 L249 313 M248 317 L250 318'),
    ('right-thigh-upper-slot','M208 338 Q207 336 210 335 L227 332 '
     'Q231 331 231 335 Q231 337 228 338 L211 341 Q208 342 208 338 Z'),
    ('right-thigh-lower-slot','M210 346 Q209 343 213 343 L228 340 '
     'Q232 339 232 343 Q232 345 229 347 L213 350 Q210 351 210 346 Z'),
    ('right-knee-top','M215 378 L219 375 L231 372 L242 369 L249 379 L252 384'),
    ('right-knee-lower-edge','M218 392 L224 396 L234 394 L240 391 L243 386 L253 384'),
    ('right-knee-side','M257 378 Q268 385 273 399 L274 412 V431'),
    ('right-knee-front-turn','M254 385 V399 L257 411 L257 417'),
    ('right-knee-bottom','M221 415 Q224 424 235 427 L241 429 L249 427 '
     'L254 424 L258 417'),
    ('right-knee-outer-recess','M263 412 H268 L270 419 L268 422'),
    ('right-calf-lower-rim','M247 466 H253 V461 L263 459 L269 463 L278 460'),
    ('right-ankle-lower-edge','M247 473 H283'),
    ('right-boot-upper-rim','M244 479 Q261 473 284 474'),
    ('right-boot-lower-rim','M243 494 H286'),
    ('right-boot-side-recess','M255 493 L249 503 L246 506'),
    ('right-boot-instep','M252 511 L261 507 H287'),
    ('right-boot-toe-divider','M249 513 L254 517 L255 531'),
    ('right-boot-toe-arch','M254 532 Q264 520 273 520 Q286 518 295 531'),
    ('right-boot-toe-recess','M268 541 Q270 534 276 534 H280 Q284 535 285 541'),
)

SEAMS=(
    'M119 121 Q122 129 130 129 H137 Q150 125 157 118',
    'M120 128 Q123 136 131 136 H137 Q150 131 158 123',
    'M89 430 L94 432 L95 437 L93 440',
)

MATERIAL_REGIONS=(
    (('helmet',),(89,21,174,121)),
    (('visor',),(100,54,160,99)),
    (('rear-shoulder',),(152,68,229,128)),
    (('near-shoulder',),(195,124,234,219)),
    (('near-upper-arm','arm-joint'),(164,141,223,237)),
    (('neck',),(117,106,159,143)),
    (('left-chest','centre-chest','chest-lower','right-chest'),(91,106,194,168)),
    (('far-',),(17,119,77,190)),
    (('front-glove','front-hand','front-wrist'),(61,138,118,192)),
    (('front-forearm','front-bracer','front-elbow-band'),(103,164,204,249)),
    (('abdomen','right-waist','belt','pelvis'),(119,219,232,296)),
    (('left-thigh',),(82,242,151,343)),
    (('left-knee',),(58,322,120,403)),
    (('left-shin','left-calf'),(79,378,128,446)),
    (('left-ankle','left-boot'),(43,439,134,504)),
    (('right-thigh',),(194,242,267,356)),
    (('right-knee',),(214,348,267,430)),
    (('right-shin','right-calf'),(233,381,298,469)),
    (('right-ankle','right-boot'),(244,462,316,546)),
    (('rifle',),(68,81,93,173)),
)


def component_transform(key):
    # Every part has now been retraced in native canvas coordinates.
    return 'translate(0 0)'


def component_clip(d,key):
    if any(node.get('id')==key+'-surface-bounds' for node in d.defs):
        return f'url(#{key}-surface-bounds)'
    node=element(d.defs,'clipPath',id=key+'-surface-bounds')
    path=next(path for name,path,_ in ARMOR_PANELS if name==key)
    element(node,'path',d=path,transform=component_transform(key))
    return f'url(#{key}-surface-bounds)'


def surface_region(d,body,key,**attrs):
    """Keep material paint on the visible portion of its authored component."""
    name=key+'-surface-visibility'
    if not any(node.get('id')==name for node in d.defs):
        mask=element(d.defs,'mask',id=name,maskUnits='userSpaceOnUse',
                     x=0,y=0,width=512,height=1024)
        index=next(i for i,part in enumerate(ARMOR_PANELS) if part[0]==key)
        element(mask,'path',d=ARMOR_PANELS[index][1],fill='white')
        for _,occluder,_ in ARMOR_PANELS[index+1:]:
            element(mask,'path',d=occluder,fill='black')
    return element(body,'g',clip_path=component_clip(d,key),mask=f'url(#{name})',**attrs)


def lower_armor_surfaces(d,body):
    """Native-reference reflections and recesses, distinct from contour ink."""
    def region(key):
        return surface_region(d,body,key)
    # Broad light fields describe the turn of each plate. Their bounds and
    # peaks come from native material references, not the procedural grain.
    for key,cx,cy,rx,ry,colour,alpha in (
        ('left-thigh-face',119,300,17,29,'#a8bec6',.46),
        ('right-thigh',235,309,23,55,'#acc9d3',.48),
        ('right-thigh-face',229,278,17,40,'#c4dbe2',.4),
        ('left-shin',114,396,14,25,'#b5ced7',.7),
        ('left-calf',90,411,10,24,'#a2becb',.22),
        ('left-calf-side',118,430,5,10,'#a4c0cd',.25),
        ('right-knee-lower',249,397,19,22,'#c6e1e9',.72),
        ('right-calf',258,445,8,26,'#bfdce6',.65),
        ('right-shin-face',267,441,10,25,'#bfdce6',.6),
        ('left-ankle',101,451,6,12,'#c0d9e1',.4),
        ('left-boot-toe',72,485,21,12,'#9dbbc7',.4),
        ('right-boot-toe',281,533,18,17,'#a8c5d0',.55),
    ):
        group=region(key)
        gradient=element(d.defs,'radialGradient',id=key+'-surface-light',
                         gradientUnits='userSpaceOnUse',cx=0,cy=0,r=1,
                         gradientTransform=f'translate({cx} {cy}) scale({rx} {ry})')
        for offset,weight in ((0,alpha),(.4,alpha*.8),(1,0)):
            element(gradient,'stop',offset=offset,stop_color=colour,stop_opacity=weight)
        element(group,'rect',width=512,height=1024,fill=f'url(#{key}-surface-light)')
    # These are coherent paint features: joint ribs, plate bevels, recesses,
    # wear patches and vent cavities. They are not samples on a source grid.
    for component,key,path,colour,alpha,softness in (
        ('left-thigh','thigh-upper-rib',
         'M118 249 Q126 248 131 254 L131 258 Q125 254 119 257 L115 260 L115 253 Z',
         '#829fa9',.4,1.1),
        ('left-thigh','thigh-middle-rib',
         'M114 261 L123 259 L130 261 L130 264 L125 263 L119 265 L114 264 Z',
         '#95b1bc',.27,1),
        ('left-thigh','thigh-lower-rib',
         'M111 269 Q120 265 128 269 L129 273 L123 271 L113 273 Z',
         '#8da7b1',.28,.8),
        ('left-thigh-face','thigh-inset-depth',
         'M111 282 L117 285 L116 291 L112 293 L109 289 Z',
         '#101c22',.85,.6),
        ('left-thigh-face','thigh-inset-lower-wear',
         'M116 294 Q121 293 126 298 L128 303 L124 304 L120 299 L116 299 Z',
         '#182b33',.45,.9),
        ('left-thigh-outer','thigh-outer-reflection',
         'M92 294 L97 287 L97 302 L95 312 L97 318 L105 322 L104 325 '
         'L94 321 L91 313 Z','#a8c4cd',.35,1),
        ('right-thigh','thigh-right-inner-shadow',
         'M212 264 L217 263 L221 291 L224 316 L223 326 L218 326 '
         'L216 308 Z','#10232c',.65,1.4),
        ('right-thigh-face','thigh-right-face-wear',
         'M226 250 L230 249 L234 253 L233 257 L229 256 L228 253 Z',
         '#28424c',.5,.8),
        ('right-thigh-face','thigh-central-patina',
         'M221 269 H223 L224 272 L223 275 H221 L220 272 Z','#547581',.4,1),
        ('right-thigh-face','thigh-lower-patina',
         'M228 286 H232 L234 289 L232 292 L229 291 L227 289 Z','#3e616e',.25,1.4),
        ('right-thigh-face','thigh-right-face-edge',
         'M237 267 L239 268 L241 275 L239 279 L237 274 Z',
         '#f1fbff',.9,.9),
        ('right-thigh','thigh-right-lower-wear',
         'M235 306 L239 304 L243 306 L241 309 L237 310 L234 308 Z',
         '#284651',.6,.8),
        ('right-thigh','thigh-right-lower-bevel',
         'M241 327 L245 326 L246 329 L242 332 L239 332 Z',
         '#e5f5f8',.65,.8),
        ('right-thigh','thigh-upper-slot-depth',
         'M209 337 L228 333 L230 334 L230 336 L210 340 Z',
         '#102733',.75,.4),
        ('right-thigh','thigh-lower-slot-depth',
         'M211 345 L229 341 L231 342 L230 345 L213 349 L211 348 Z',
         '#102733',.75,.4),
        ('left-knee-cap','left-knee-recess',
         'M90 382 Q94 380 97 382 L97 385 L94 386 H90 Z',
         '#0b1a20',.95,.65),
        ('left-knee-cap','left-knee-recess-bevel',
         'M90 385 Q94 387 97 384 L97 386 L94 388 L90 387 Z',
         '#c1d9e0',.55,.5),
        ('right-knee','right-knee-upper-recess',
         'M228 385 L231 384 L234 387 L233 392 L229 393 L227 390 Z',
         '#0c1b22',.9,.65),
        ('left-shin','left-shin-lower-depth',
         'M102 411 Q108 419 112 422 L117 418 L116 422 L112 426 L106 423 Z',
         '#152d37',.5,1),
        ('left-shin','left-shin-outer-reflection',
         'M121 390 Q125 398 122 406 L119 412 V406 L122 400 Z',
         '#e4f4fa',.35,1.1),
        ('left-calf','left-calf-depth',
         'M89 410 C95 410 98 413 99 421 L98 438 L95 441 H91 L89 435 Z',
         '#081a23',.65,1.1),
        ('left-calf','left-calf-fastener',
         'M89 432 L92 433 L93 436 L91 437 L89 435 Z',
         '#102733',.9,.45),
        ('right-knee-lower','right-knee-bottom-shadow',
         'M227 407 Q236 412 247 412 L251 418 L246 423 L234 424 L228 419 Z',
         '#0a1920',.8,1.1),
        ('right-knee-lower','right-knee-right-cavity',
         'M248 413 L251 412 L254 414 L254 418 L250 419 L247 416 Z',
         '#0a1720',.9,.5),
        ('right-calf','right-calf-inner-shadow',
         'M237 428 L251 431 L252 451 L256 459 L247 465 L242 455 Z',
         '#0e202a',.8,1.4),
        ('left-ankle','left-ankle-upper-wear',
         'M89 445 L92 445 L93 451 L90 454 L88 451 Z',
         '#122d36',.75,.7),
        ('left-ankle','left-ankle-centre-wear',
         'M96 451 L99 451 L99 457 L96 461 L94 458 Z',
         '#1b3640',.7,.7),
        ('left-ankle','left-ankle-centre-reflection',
         'M102 451 L106 453 L104 462 L106 468 L102 468 L100 463 Z',
         '#d9eef5',.4,.8),
        ('left-ankle','left-ankle-inset-depth',
         'M104 456 L112 458 L120 462 L120 466 L108 463 L104 461 Z',
         '#17333e',.6,1),
        ('left-ankle','left-ankle-right-wear',
         'M118 461 L122 462 L123 466 L119 467 L116 465 Z',
         '#17333e',.7,.65),
        ('left-boot-toe','left-toe-turn-reflection',
         'M81 484 L85 484 L86 491 L83 498 L79 498 L80 491 Z',
         '#effbff',.9,.8),
        ('right-boot-upper','right-boot-vent-upper-left',
         'M259 476 L261 476 L262 478 L261 481 L258 480 Z',
         '#162f38',.8,.7),
        ('right-boot-upper','right-boot-vent-upper-centre',
         'M271 480 L274 480 L275 483 L274 485 L271 485 L270 482 Z',
         '#17323b',.85,.65),
        ('right-boot-upper','right-boot-vent-upper-right',
         'M282 476 L284 477 L284 480 L281 481 L280 479 Z',
         '#16303a',.8,.65),
        ('right-boot-upper','right-boot-vent-lower-left',
         'M259 485 Q261 484 262 487 L261 491 L258 489 L258 487 Z',
         '#18333e',.8,.6),
        ('right-boot-upper','right-boot-vent-lower-right',
         'M282 485 L285 486 L286 489 L285 491 L282 490 L281 487 Z',
         '#183540',.8,.65),
        ('right-boot-upper','right-boot-raised-rib',
         'M266 480 L268 480 L268 484 L267 486 L265 485 Z',
         '#eefbff',.85,.6),
        ('right-boot-toe','right-toe-left-scrape',
         'M258 511 L260 512 L264 519 L267 521 L266 523 L262 520 Z',
         '#cee5ec',.65,.6),
        ('right-boot-toe','right-toe-tip-recess',
         'M272 537 Q278 533 283 537 L285 540 L271 541 Z',
         '#0b1f29',.8,.6),
    ):
        element(region(component),'path',id=key,d=path,fill=colour,opacity=alpha,
                filter=d.glow(key+'-softness',softness))


def head_and_grip_surfaces(d,body):
    """Observed helmet wear, temple fittings and glove folds."""
    for component,key,path,ink,alpha,softness in (
        ('helmet-cap','helmet-cap-dark-wear',
         'M142 32 H145 L147 34 V37 L145 38 L144 35 H142 Z','#243a44',.7,.75),
        ('helmet-top','helmet-top-wear',
         'M116 28 L125 27 L127 29 L126 32 H121 L117 31 Z','#21353e',.55,1),
        ('helmet-side-face','helmet-front-shadow',
         'M130 44 H155 L162 46 L154 48 L137 49 L128 50 Z','#122831',.5,.8),
        ('helmet-chin-inset','helmet-chin-cavity',
         'M121 109 H132 L134 113 V120 H124 L122 117 Z','#061217',.85,.6),
        ('helmet-jaw-face','helmet-jaw-reflection',
         'M138 99 L143 98 L147 100 V104 L143 106 L139 103 Z','#b9d1d9',.45,.9),
        ('front-glove','glove-lower-fold-depth',
         'M75 178 Q86 181 95 181 L108 178 L111 173 V180 L108 186 '
         'H99 L86 184 H79 Z','#0a181f',.7,1.2),
        ('front-glove','glove-thumb-reflection',
         'M96 144 Q99 148 99 152 L102 157 L108 160 V162 L101 159 L97 154 Z',
         '#8ea4ae',.45,.7),
        ('front-hand-rim','glove-left-knuckle-reflection',
         'M64 168 L68 169 V173 L72 176 L71 178 L66 175 L64 172 Z',
         '#9ab3bf',.35,.7),
        ('front-hand-rim','glove-heel-ridge',
         'M74 182 L83 184 L85 185 L84 187 H80 L77 185 Z','#98b3c1',.4,.5),
    ):
        element(surface_region(d,body,component),'path',id=key,d=path,fill=ink,
                opacity=alpha,filter=d.glow(key+'-softness',softness))
    for key,component,cx,cy,rx,ry in TEMPLE_PORTS:
        group=surface_region(d,body,component)
        element(group,'ellipse',id='temple-'+key+'-cavity',cx=cx,cy=cy,rx=rx,ry=ry,
                fill='#12252e',stroke='#8da7b4',stroke_width=.7,
                filter=d.glow('temple-'+key+'-softness',.3))
        element(group,'path',d=f'M{cx-rx*.65} {cy+ry*.45} Q{cx} {cy+ry*.9} {cx+rx*.65} {cy+ry*.4}',
                fill='none',stroke='#c3d7df',stroke_width=.6,stroke_opacity=.5)


def upper_armor_surfaces(d,body):
    """Source-observed broad reflections, worn planes and arm recesses."""
    group=surface_region(d,body,'far-forearm')
    gradient=element(d.defs,'radialGradient',id='far-forearm-surface-light',
                     gradientUnits='userSpaceOnUse',cx=0,cy=0,r=1,
                     gradientTransform='translate(38 158) scale(14 12)')
    for offset,alpha in ((0,.18),(.4,.12),(1,0)):
        element(gradient,'stop',offset=offset,stop_color='#9db2bb',stop_opacity=alpha)
    element(group,'rect',width=512,height=1024,fill='url(#far-forearm-surface-light)')
    for component,key,path,ink,alpha,softness in (
        ('near-shoulder-lower','near-arm-upper-reflection',
         'M202 170 L205 169 L208 170 L211 172 L214 175 V179 L211 181 '
         'L207 180 L205 178 L204 175 L201 174 Z','#f4fcff',.88,.9),
        ('near-shoulder-lower','near-arm-face-depth',
         'M214 182 L216 184 L215 190 L212 193 L211 190 L212 186 Z',
         '#35515d',.55,.7),
        ('near-shoulder-lower','near-arm-lower-scrape',
         'M190 195 L194 196 L197 199 L202 201 L204 204 L201 205 '
         'L196 203 L193 201 L190 198 Z','#e8f6fa',.7,1),
        ('near-shoulder-lower','near-arm-recess-depth',
         'M195 194 L204 202 V204 L195 197 Z','#304b58',.7,.35),
        ('near-shoulder-band','near-arm-rim-depth',
         'M195 167 L206 164 L218 174 V179 L215 180 V176 L205 169 '
         'L198 172 Z','#233944',.7,.65),
        ('near-upper-arm','upper-arm-turn-reflection',
         'M178 170 L181 169 L184 170 L183 175 L181 179 L177 182 '
         'L175 180 L176 175 Z','#e3f3f7',.8,1.3),
        ('far-forearm','far-forearm-pocket-depth',
         'M48 151 L52 154 V157 H60 V151 L56 150 L53 152 Z',
         '#16282e',.6,.65),
        ('far-forearm','far-forearm-diagonal-depth',
         'M37 153 H40 L42 157 L43 161 L47 164 V168 L44 167 L41 163 L39 159 Z',
         '#0b1e25',.65,1.1),
        ('far-forearm','far-forearm-worn-ridge',
         'M34 157 H36 L37 160 L38 164 L36 165 L33 162 Z','#76939e',.35,1),
        ('far-forearm','far-forearm-lower-depth',
         'M39 171 L45 170 L48 173 V179 H45 L44 176 L41 174 Z','#0b1b23',.75,.7),
        ('far-thumb','far-thumb-depth',
         'M64 151 L66 150 V158 L65 163 L66 168 L64 171 L63 164 Z',
         '#0e1b20',.8,.6),
    ):
        element(surface_region(d,body,component),'path',id=key,d=path,fill=ink,
                opacity=alpha,filter=d.glow(key+'-softness',softness))


def reflected_surfaces(d,body):
    # Reflection contours are traced in native canvas coordinates after the
    # component-width correction. They are intentionally separate from the
    # armor seams; a reflection is paint, not an extra blue outlined plate.
    collar=surface_region(d,body,'neck',id='collar-channel-depth')
    element(collar,'path',d='M121 129 Q126 132 132 131 H138 Q147 127 153 124 '
            'L153 126 Q146 131 138 134 H130 Q124 133 121 131 Z',
            fill='#0b1113',opacity=.93,filter=d.glow('collar-channel-softness',.3))
    chest=surface_region(d,body,'centre-chest')
    element(chest,'path',id='chest-inset-depth',d='M115 133 Q123 141 133 139 '
            'L148 130 L143 145 L134 151 L125 153 L115 146 Z',
            fill='#172a30',opacity=.45,filter=d.glow('chest-inset-softness',1.2))
    element(chest,'path',id='chest-central-depth',d='M131 136 L141 137 V142 '
            'L135 145 L131 143 Z',fill='#0d191d',opacity=.35,
            filter=d.glow('chest-central-softness',1.8))
    visor=surface_region(d,body,'visor',id='visor-reflections')
    light=linear(d,'visor-upper-reflection',[(0,'#d0e0e4',1),(1,'#6d8f97',1)],
                 x1=0,x2=0,y1=61,y2=85)
    for key,path,ink in (
        ('visor-top-light','M106 64 L114 62 L127 63 L137 60 L144 59 L149 57 '
         'L152 59 L149 67 L145 70 L136 70 L130 68 L115 67 L110 70 L106 69 Z',light),
        ('visor-central-reflection','M116 68 L123 68 L129 70 L133 74 '
         'L132 78 L128 80 L129 84 L125 85 L121 83 L118 86 L115 84 '
         'L116 79 L112 76 L113 72 Z',light),
        ('visor-right-light','M150 63 L154 63 L155 71 L151 82 L149 88 '
         'L146 89 L146 81 L147 74 Z','#a1bac1'),
        ('visor-left-depth','M102 66 L107 68 L108 73 L112 76 L113 82 '
         'L112 88 L109 91 L105 84 L102 79 Z','#081418'),
        ('visor-right-depth','M138 72 L145 73 L147 78 L146 87 L143 92 '
         'L138 94 L135 90 L136 83 L134 79 Z','#101e23'),
        ('visor-left-reflected-edge','M114 71 L117 73 L116 79 L119 84 '
         'L119 89 L115 92 L112 88 L113 81 L111 77 Z','#1b3039'),
        ('visor-upper-dark-pocket','M117 70 Q121 72 121 76 L117 77 '
         'L115 74 Z','#0e1c22'),
        ('visor-centre-dark-pocket','M128 73 L131 73 L133 76 L129 77 '
         'L127 76 Z','#13252b'),
        ('visor-lower-dark-pocket','M126 78 L129 79 L130 82 L127 83 '
         'L124 81 Z','#102027'),
        ('visor-lower-reflection','M121 94 L131 92 L140 94 L140 97 '
         'L131 96 L124 97 L119 96 Z','#aac5cf'),
    ):
        element(visor,'path',id=key,d=path,fill=ink,filter=d.glow(key+'-softness',.65))
    for name,cx,cy,rx,ry,colour,strength in (
        ('near-shoulder',218,148,19,23,'#dce9ed',.65),
        ('near-shoulder-lower',212,179,14,27,'#d5e4e9',.83),
        ('near-upper-arm',192,163,12,22,'#afc7ce',.83),
        ('front-forearm',179,201,14,27,'#a4bfc8',.83)):
        group=surface_region(d,body,name)
        gradient=element(d.defs,'radialGradient',id=name+'-highlight',gradientUnits='userSpaceOnUse',
                         cx=0,cy=0,r=1,gradientTransform=f'translate({cx} {cy}) scale({rx} {ry})')
        for offset,alpha in ((0,strength),(.35,strength*.84),(1,0)):
            element(gradient,'stop',offset=offset,stop_color=colour,stop_opacity=alpha)
        element(group,'rect',width=512,height=1024,fill=f'url(#{name}-highlight)')
    shoulder=surface_region(d,body,'near-shoulder')
    for key,path,colour,alpha in (
        ('shoulder-dark-seam','M216 134 L219 133 L220 140 L218 145 L215 145 Z','#304a54',.7),
        ('shoulder-light-facet','M216 144 L220 145 L223 149 L220 153 '
         'L215 151 L213 147 Z','#eff6f7',.45),
        ('shoulder-lower-facet','M210 157 L216 156 L221 159 L217 162 '
         'L211 161 L208 163 L206 161 Z','#dcebee',.55),
    ):
        element(shoulder,'path',id=key,d=path,fill=colour,opacity=alpha,
                filter=d.glow(key+'-softness',.7))
    bracer=surface_region(d,body,'front-bracer-face',id='bracer-wear')
    element(bracer,'ellipse',id='bracer-fastener-recess',cx=125,cy=175,rx=2.1,ry=2.5,
            fill='#344a51',stroke='#8faab4',stroke_width=.6,
            filter=d.glow('bracer-fastener-softness',.35))
    element(bracer,'path',d='M123.6 176.2 Q125 178.1 126.6 176.3',fill='none',
            stroke='#deebef',stroke_width=.6,stroke_opacity=.55)
    for key,path,ink,alpha in (
        ('bracer-dark-wear','M136 179 C139 179 142 181 143 183 '
         'L141 186 L137 185 L134 182 Z','#526e7b',.32),
        ('bracer-edge-wear','M147 180 L152 182 L155 185 L155 188 '
         'L151 186 L149 184 Z','#eff4f6',.5),
    ):
        element(bracer,'path',id=key,d=path,fill=ink,opacity=alpha,
                filter=d.glow(key+'-softness',.6))
    bracer_depth=surface_region(d,body,'front-bracer')
    element(bracer_depth,'path',id='bracer-underlip-shadow',
            d='M111 186 L127 192 Q145 198 159 194 L159 199 L135 204 L118 194 Z',
            fill='#1b2f37',opacity=.55,filter=d.glow('bracer-underlip-softness',1))
    element(bracer_depth,'path',d='M109 191 L132 205 L156 200',fill='none',
            stroke='#c6d9df',stroke_width=1.1,stroke_opacity=.25,
            filter=d.glow('bracer-bevel-softness',.7))
    forearm=surface_region(d,body,'front-forearm',id='forearm-wear')
    for key,path,ink,alpha,softness in (
        ('forearm-turn-highlight','M168 183 C181 185 188 198 191 208 L191 220 '
         'L186 216 C186 201 181 193 172 190 Z','#c0d4dd',.25,1.2),
        ('forearm-under-bracer-depth','M142 208 L148 211 Q156 214 163 211 '
         'L166 210 L168 214 L164 219 L155 224 L149 223 L142 217 L134 210 Z',
         '#0a1921',.75,1.3),
        ('forearm-raised-wear-plane','M143 208 L148 207 L151 209 L156 207 '
         'H161 L163 210 L160 214 L155 217 H149 L145 214 Z',
         '#93aeba',.55,.8),
        ('forearm-upper-reflection','M172 189 L175 191 L176 194 H181 '
         'L184 199 V203 L181 205 L177 204 L174 202 L173 199 L171 197 Z',
         '#f2fcff',.82,1.5),
        ('forearm-left-sheen','M171 190 H175 L176 196 L175 202 L177 205 '
         'L176 208 L172 206 L171 201 L170 196 Z','#effaff',.7,1.2),
        ('forearm-lower-wear-shadow','M178 210 L182 211 L185 214 V218 '
         'L182 220 L179 217 L177 214 Z','#325361',.4,1.7),
        ('forearm-specular-facet','M177 196 L180 196 L183 199 L182 202 '
         'L179 205 L176 203 L175 200 Z','#f2fcff',.9,.65),
        ('forearm-left-wear','M150 209 L154 208 L156 211 L155 215 L152 215 Z',
         '#23333c',.6,.5),
        ('forearm-right-wear','M158 210 L162 210 L163 213 L160 215 L158 213 Z',
         '#23333c',.6,.5),
        ('forearm-outer-turn-reflection','M187 207 L190 209 L192 219 L193 228 '
         'L190 229 L188 219 Z','#bad1db',.35,1),
    ):
        element(forearm,'path',id=key,d=path,fill=ink,opacity=alpha,
                filter=d.glow(key+'-softness',softness))
    weapon=element(body,'g',id='weapon-surface-detail')
    for key,path,ink,alpha,softness in (
        ('sight-depth','M78 82 H81 L80 86 L77 87 Z','#14252b',.9,.45),
        ('muzzle-recess','M77 93 Q81 91 85 94 Q87 99 85 101 L82 102 '
         'L78 100 Q74 98 76 95 Z','#071115',.95,.5),
        ('muzzle-left-reflection','M75 94 Q78 92 80 95 L79 98 '
         'Q78 101 76 99 L74 97 Z','#e0eff1',.9,.55),
        ('shaft-upper-recess','M79 106 L82 106 L82 110 L80 113 L78 113 Z',
         '#091519',.9,.45),
        ('shaft-lower-recess','M79 115 L81 115 L82 123 L85 132 H78 '
         'L77 126 Z','#061114',.95,.45),
        ('shaft-left-rail','M75 115 L77 115 L77 133 L79 134 L78 136 '
         'L75 134 Z','#d7ebef',.9,.55),
        ('shaft-right-rail','M85 118 L87 118 L87 125 L89 133 L87 136 '
         'L85 133 Z','#c3dce3',.85,.55),
        ('face-upper-recess','M79 140 Q82 138 85 140 L87 143 L88 146 '
         'Q84 146 77 146 L76 144 Z','#12262d',.9,.5),
        ('face-left-reflection','M75 145 Q77 147 76 151 L77 156 V159 '
         'L75 158 L73 153 Z','#d4e8ee',.8,.7),
        ('face-centre-channel','M78 147 Q82 145 86 148 Q89 152 86 156 '
         'L82 158 Q77 157 76 153 L77 150 Z','#7594a5',.75,.35),
        ('face-centre-reflection','M79 149 Q81 147 82 150 L81 152 L80 151 Z',
         '#bfd9e9',.8,.4),
        ('face-left-cavity','M78 150 L80 148 L82 150 L80 152 L77 153 Z',
         '#1b3542',.85,.3),
        ('face-right-cavity','M84 149 L86 150 L87 153 L84 154 L83 152 Z',
         '#1b3542',.85,.3),
        ('face-bottom-cavity','M81 153 L83 154 V156 L81 157 L79 155 Z',
         '#1b3542',.85,.3),
        ('face-right-reflection','M88 145 Q90 149 88 154 L86 157 H84 '
         'L86 153 V149 Z','#d9eef3',.85,.55),
        ('face-lower-depth','M76 158 Q81 160 86 158 V160 Q81 162 76 160 Z',
         '#12262e',.8,.45),
        ('frame-lower-depth','M74 162 Q80 165 89 161 L88 166 Q82 170 77 167 Z',
         '#070f13',.9,.5),
    ):
        element(weapon,'path',id=key,d=path,fill=ink,opacity=alpha,
                filter=d.glow(key+'-softness',softness))
    head_and_grip_surfaces(d,body)
    upper_armor_surfaces(d,body)
    lower_armor_surfaces(d,body)


def profile_illustration(d,regional_palette=True,layered_grain=True):
    d.family='player-profile-spartan-illustration'
    if regional_palette:d.paint_fit_alpha_range=(88,95)
    palette={
        'metal':[(0,'#94b2ba',1),(.4,'#657f89',1),(1,'#23363c',1)],
        'metal-light':[(0,'#c0d7df',1),(.45,'#94b2bc',1),(1,'#526c75',1)],
        'metal-dark':[(0,'#526e78',1),(.45,'#263e47',1),(1,'#101c20',1)],
        'rubber':[(0,'#1c2b2f',1),(.6,'#101a1d',1),(1,'#30454b',1)],
        'visor':[(0,'#40565b',1),(.55,'#17282d',1),(1,'#263c43',1)],
        'edge':[(0,'#b4cdd5',1),(.5,'#526f7b',1),(1,'#334d59',1)]}
    clip=element(d.defs,'clipPath',id='profile-silhouette')
    element(clip,'path',d=SILHOUETTE)
    grain={'contrast':.55,'secondary_frequency':(.14,.19)} if layered_grain else {}
    body=element(d.root,'g',id='profile-material',opacity=91/255,
                 clip_path='url(#profile-silhouette)',
                 filter=material_grain(d,'profile-armor-grain',**grain))
    element(body,'path',d=SILHOUETTE,fill='#263a40')
    shared_stops={}
    for key,path,kind in ARMOR_PANELS:
        # Explicit anatomical paint anchors, independent of path syntax.
        prefixes,(x0,y0,x1,y1)=next((prefixes,bounds) for prefixes,bounds in MATERIAL_REGIONS
                                   if key.startswith(prefixes))
        if key=='front-bracer-face':
            # Three stable material anchors from the native face. The right
            # end carries a bright metallic reflection, not a dark gradient.
            paint=linear(d,key+'-paint',[(0,'#89afbc',1),(.55,'#80a7b4',1),(1,'#d3e8ef',1)],
                         x1=117,x2=153,y1=174,y2=186)
        elif key=='right-boot-upper':
            paint=linear(d,key+'-paint',[(0,'#5c7a83',1),(.58,'#e0f2f8',1),(1,'#6e929f',1)],
                         x1=250,x2=286,y1=482,y2=490)
        else:paint=linear(d,key+'-paint',palette[kind],x1=x0,x2=x1,y1=y0,y2=y1)
        # Share three paint anchors by authored anatomical material group.
        # No contour, transform, grain field, alpha, or blur is fitted.
        paint_kind=key if key in ('front-bracer-face','right-boot-upper') else kind
        if regional_palette and paint_kind not in ('front-bracer-face','right-boot-upper'):
            paint_kind=(prefixes[0],kind)
        for i,stop in enumerate(d.defs[-1]):
            colour=stop.get('stop-color')
            shared_stops.setdefault((paint_kind,i),(colour,[]))[1].append(stop)
        element(body,'path',id=key,d=path,fill=paint,transform=component_transform(key))
    for colour,stops in shared_stops.values():
        rgb=[int(colour[i:i+2],16) for i in (1,3,5)]
        d.slots.append((tuple(stops),'stop-color',rgb))
    reflected_surfaces(d,body)
    line=element(d.root,'g',id='profile-contours',fill='none',stroke='#85c3fe',
                 stroke_opacity=.9,stroke_width=1.05,stroke_linejoin='round',stroke_linecap='round')
    element(line,'path',d=SILHOUETTE)
    panels=element(line,'g',clip_path='url(#profile-silhouette)')
    for i,(key,path,_) in enumerate(ARMOR_PANELS):
        if key in RIMLESS or key.startswith(NATIVE_OPEN_CONTOUR_PARTS):continue
        # A later foreground plate covers both the paint and the hidden rim
        # of plates behind it. Keeping every closed contour visible would
        # turn the collar and elbows into intersecting construction lines.
        mask=element(d.defs,'mask',id=key+'-visible-rim',maskUnits='userSpaceOnUse',
                     x=0,y=0,width=512,height=1024)
        element(mask,'rect',width=512,height=1024,fill='white')
        for later,occluder,_ in ARMOR_PANELS[i+1:]:
            element(mask,'path',d=occluder,fill='black',transform=component_transform(later))
        # Put the transform inside its mask so the mask and all foreground
        # components share the native canvas coordinate system.
        visible=element(panels,'g',mask=f'url(#{key}-visible-rim)')
        element(visible,'path',id=key+'-outline',d=path,transform=component_transform(key))
    detail=element(line,'g',stroke_width=.8,stroke_opacity=.6)
    native_contours=element(line,'g',id='native-lower-contours',stroke_width=1,
                            stroke_opacity=.85)
    for key,path in LOWER_CONTOURS+UPPER_CONTOURS+HELMET_CONTOURS+HAND_CONTOURS+WEAPON_CONTOURS:
        element(native_contours,'path',id=key+'-source-contour',d=path)
    for key,_,cx,cy,rx,ry in TEMPLE_PORTS:
        element(native_contours,'ellipse',id='temple-'+key+'-port-contour',
                cx=cx,cy=cy,rx=rx,ry=ry,stroke_width=.75,stroke_opacity=.8)
    for i,path in enumerate(SEAMS):
        transform=component_transform('helmet') if i<5 else 'translate(0 0)'
        element(detail,'path',d=path,transform=transform)
    # The face is not a set of regular concentric rings. Open source grooves
    # follow the recessed upper facet and central mechanical channel.
    weapon_detail=element(line,'g',stroke_width=.65,stroke_opacity=.65)
    for path in ('M76 94 Q80 90 84 93 Q87 97 84 101 Q81 104 77 100 Q74 97 76 94 Z',
                 'M76 105 L80 104 L85 105',
                 'M75 114 H83 L86 117',
                 'M78 141 Q82 138 86 141 L88 146',
                 'M77 149 Q78 146 82 147 Q86 147 87 151 L86 154 '
                 'Q83 157 80 156 Q77 155 77 152 Z',
                 'M74 159 Q80 162 87 159'):
        element(weapon_detail,'path',d=path)
