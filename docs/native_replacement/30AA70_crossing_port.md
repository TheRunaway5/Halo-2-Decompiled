# Remaining crossing continuation of `30AA70`

This is an implementation handoff for the still-unbound native ray parent,
based on the unchanged `30AE75..30B17B` continuation. It is not qualification
evidence. The shared native plane/branch controllers stop immediately before
this stage; the public value traversal's existing `split()` is algorithmic
guidance, not a qualified raw caller adapter.

## DATA and numerical phases

The first endpoint projection remains live on x87 at entry. Source order is:

1. Push the low plane, load original bounds start into XMM1, subtract the low
   plane from the retained first projection, then load bounds end into XMM2.
2. Store both captured vectors to caller child-bounds storage at SP+80/SP+90.
   These writes can alias the original bounds; preserve the two prior loads.
3. Spill start-low to SP+34, subtract high from the remaining first projection
   and spill start-high to SP+2C. Read the rounded end from SP+18 independently
   for each subtraction. Spill end-low to SP+30 and end-high back to SP+18.
4. Compare start-low/end-low. Ordered less selects the increasing branch;
   equal/unordered selects the other branch. Preserve the actual FNSTSW/TEST
   publication and complete FP status, not just the decision.
5. Increasing: test end-low * start-low against the original epsilon. When
   required, interpolate the saved child end with
   start-low / (start-low - end-low). Recurse into the left child.
6. Decreasing: test end-high * start-high and interpolate the saved child end
   using start-high / (start-high - end-high). Recurse into the right child.
7. After the genuine child returns, increasing uses end-high/start-high for
   the original bounds-start update; decreasing uses end-low/start-low.
   These source fields and original bound vectors must be reread lazily.
8. Reread saved cache identity at SP+28 and its current fraction. If it has
   shortened the ray, rebuild the end from current cache/code information,
   variable scale and variable translation in the source SSE order. The
   source stores and rereads intermediate end vectors rather than retaining
   one final expression.
9. Reread the axis slot at SP+3C. Its comparison with 3 is signed in the raw
   source; aliases/children may have changed the slot. The axis endpoint test
   can finish the parent early. Otherwise increasing continues at the current
   right program; decreasing advances by the current left-minus-right offset.

## Ownership constraints

The recursive call has real caller PUSHes and a return address. Perform these
stores through the parent FP owner before `Scope::child`; parking the source
stack before these parent stores would publish an empty stack through the
parent I/O policy. `Scope::child` is the actual native child handoff, not a
provider for captured/declared outcomes.

After return, use the real retained GP/SP/BP and lazy storage. Do not restore a
captured entry tuple or assume a child preserved source storage through aliases.
The existing caller frame already uses live registers for its return.

The stage's x87 spills need the qualified staged conversion protocol: observe
pre-instruction state, stage conversion/pop, write successfully, commit FP,
then invoke the completed-store observer. SSE interpolation must preserve
operand order, status and escaped XMM values, including each original lazy
constant/vector read. Ordinary provider atomicity is not a physical page/MMIO
fault proof.

The decreasing child fraction overwrites SP+2C; do not assume all four original
delta values retain one shared lifetime. Source cache, variable and axis reads
after children are separate DATA accesses. The final full controller should
publish the plane/branch entry at 30ADE4 only once; the independent stage probes
currently use separate terminal/entry observations there.

## Required next proof

Share the complete crossing algorithm between value and raw views, implement
the real recursive native controller, and compare unchanged-i386 returns,
child entries/returns, full ABI/live80, banks, ordered I/O and retained failure
prefixes. Include both directions, clipping/no-clipping, source private/original
bounds aliases, saved-register aliases and post-child cache/axis changes.
Slabs, complete leaf services, diagnostics, MapPack and whole-parent admission
remain separate required work.
