# Mainmenu portal mask admission

The literal mainmenu BSP has151 portals, all with flag word0. Its indirect
association header at BSP+220 has no owner records and no portal-index list.
Consequently original210DF0 produces16 open mask words through its actual
210F00/210E80/210E20 callees without requiring a fabricated machine owner.
The separate acoustic bank is retained. These facts close the portal-owner
input boundary only; they do not establish visibility, a containing cluster,
shaderLOD, raw-world submission, fog coverage or framebuffer parity.

`initialize_mainmenu_portal_state(state,cache,bsp)` requires the actual source
scenario type2 and loads the real BSP definition through
`Cache::load_source_portal_mask_definition`. Its definition overload rejects
flag8, any association record or any associated index before changing state.
The existing source constructor retains its bank-size validation. Neither
function chooses a camera, marks owners visible, resets acoustics or mutates a
separate live machine pool. Caller-owned persistent state is explicit.

The independent CPU oracle is
`tests/fixtures/generate_mainmenu_portal_state_original.py`. It pins the retail
XBE SHA256`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`
and mainmenu cache SHA256
`1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
It executes unchanged210DF0 and all four actual callees on the literal BSP
bytes, with an explicitly empty scratch machine pool. Four different prior
mask/acoustic banks all produce open words while preserving all128 acoustic
words. A private flag8 mutation closes portal0 in the original execution,
demonstrating that the guard's input distinction affects the source result.
The scratch pool is a controlled input, not a claim about full title object
lifetimes; empty associations make owner reset irrelevant to these mask words.

The retained original fixture is
`tests/fixtures/mainmenu_portal_state_original.tsv`, SHA256
`8a57163857f4889e6371f85705b33aa51464b201f97db805d902b74752ffcedc`.
`analysis/frontend/title-fog-mainmenu-portal-original.json` records actual file
offsets, portal flags, routine entry counts and the private original control.
The production-linked test compares both helper seams with those original
outputs, repeats initialization, and requires unchanged state on nine rejected
banks. Four controls modify disposable cache copies: flag8, a nonempty owner
bank, a nonempty portal-index bank and non-mainmenu scenario type. The supplied
cache is never modified.

The production-linked `retail_mainmenu_portal_state` test now passes against
the retained original fixture, including the rejected-input controls. The
launcher was rebuilt with the helper. It is not yet called by live title
admission. The title caller still needs an actual per-frame camera, aggregate
fog/cluster, source portal visibility and `SourceBspAdmission` before its
resident plan can select parts. The verified original view/selector helpers
should consume those inputs without substituting the retained capture IDs.
