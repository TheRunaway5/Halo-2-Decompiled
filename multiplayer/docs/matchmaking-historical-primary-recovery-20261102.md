# Historical matchmaking primary input recovery

Research date: 2026-11-02 (America/New_York). The complete Excel attachment
linked by Bungie's April 2006 announcement is recovered and statically read.
The final-service hopper and network downloads remain unrecovered. The
[parity audit](matchmaking-rust-parity-audit-20261028.md) and its GAP and
acceptance records are unchanged.

## April 2006 workbook

The [28 April 2006 Bungie Weekly Update preserved by HBO](https://halo.bungie.org/bwu/index.html?item=72)
describes an Excel snapshot of variant settings for the April release. Its
attachment points to [HBO's hosted gamevariants.xls](https://halo.bungie.org/bwu/images/gamevariants.xls),
the same target already recorded as R02 in the
[master PDF](reference/halo2-original-matchmaking-master-v5.pdf), §15.6 and
§15.13. The PDF recorded a damaged extraction and no complete binary. This pass
retrieved the full HTTP response using a binary-capable client. The text web
reader refused the Excel MIME type, while direct HTTP HEAD and GET returned
200. The [original Bungie announcement link](https://www.bungie.net/News/content.aspx?link=april28update&type=topnews)
returned 404 in the web reader.

The exact response is retained as
[halo2-gamevariants-20060428.xls](reference/halo2-gamevariants-20060428.xls),
245,248 bytes, SHA-256
`a5efd4c183744811dea3a9356b559595b9cc22dafd1e2dd99e20e73455fb7998`.
The [provenance receipt](reference/halo2-gamevariants-20060428.provenance.json)
records the identical requested and final URLs, UTC retrieval time, HTTP 200,
`Content-Type: application/vnd.ms-excel`, `Content-Length: 245248`,
`Last-Modified: Fri, 28 Apr 2006 22:57:19 GMT`, and ETag
`"3be00-41285a077d1c0"`. Two complete GETs yielded the same digest, and the
retained file was independently rehashed. The compound-file properties name
Jeremiah Pieschl as author and last saver and Microsoft Corporation as company;
they report creation on 18 April and save on 27 April 2006. The document
property title is unset; the sheet's title is at `Variants!A1`. The dated
announcement link and metadata support provenance. Neither the HTTP dates nor
the hash of today's copy supply a Bungie-issued digest or independently prove
that no byte ever changed after publication.

The retained bytes have CFB magic `d0cf11e0a1b11ae1`. Static olefile 0.47 and
xlrd 2.0.2 reads, plus a BIFF8 record-boundary walk, found one visible worksheet,
`Variants`, and no hidden worksheets. The global substream ends at the
worksheet BOF, and the worksheet EOF ends at byte 231,514, the end of the
`Workbook` stream. The rectangular dimensions are `A1:FP480`; nonempty
cells span `A1:FM480`. All 82,560 positions were traversed: 921 text,
6,797 numeric, 56,886 formatted blanks and 17,956 empty. There are 296 merged
ranges. Columns `CC` and `DI` are hidden and contain no named variant or
settings values. There are no hidden rows, cell formula records, cell errors,
or worksheet hyperlinks. The only SUPBOOK record is internal. The CFB
directory lists two property streams and `Workbook`, with no macro storage.
No spreadsheet application opened the file; no formulas, macros, embedded
content or external links were run. Counts, exact headings, sheet state and
the grid digest are in the [static receipt](reference/halo2-gamevariants-20060428.provenance.json).

### What the cells establish

Row 1 groups engine families. Row 2 contains 162 distinct named columns,
including six explicitly labeled code defaults. Seven of the 169 matrix
columns have neither a name nor settings values. A named column is a workbook
entry, not evidence of how often or in which playlist it ran.

| Family heading | Column span | Named columns including default |
| --- | --- | ---: |
| Slayer | `D:CC` | 77 |
| Oddball | `CD:CP` | 13 |
| King of the Hill | `CQ:DG` | 17 |
| Capture the Flag | `DH:EM` | 30 |
| Assault | `EN:FF` | 18 |
| Territories | `FG:FP` | 7 |

The 92 labeled option groups cover match, player, team, game, vehicle and
equipment settings. Columns B and C supply option and choice labels; the
variant columns supply their populated cells. Examples include a literal
`0` at `H5` for the one-round choice and a literal `NA` at `CD13`.
The matrix includes 295 text cells. Preserve literal values separately from
empty and formatted blank cells. A label for
Juggernaut score choices exists at `B62`, but there is no Juggernaut family
heading. Neither this label nor the absence of a family establishes a deployed
Juggernaut pool. [Workbook and exact headings](reference/halo2-gamevariants-20060428.provenance.json).

For a concrete mapping candidate, `AP2` names Team SWAT. `AP130=1` selects
the no-shields choice under `B128`, `AP134=0` the off choice under motion
sensor at `B132`, and `AP146=1` the on choice under team play at `B145`.
Those settings can be checked against original parameters 19, 20 and 91.
Keep the source cells and original descriptor meaning joined; verify each
choice against the original serializer before claiming a buffer. The workbook's
choice values, including the defaults, do not by themselves prove an entire
304-byte variant, its version, or any hidden/default bytes.
[Retained workbook](reference/halo2-gamevariants-20060428.xls),
[original option descriptor evidence](playlist-hopper-sources-20261028.md).

The sheet contains no playlist-membership or map-assignment table, numeric
rotation weight, hopper header, network policy, or serialized variant payload.
Equipment settings for items placed on a map are not map assignments.
The [14 April Bungie update](https://halo.bungie.org/bwu/index.html?item=70)
points to the original Optimatch thread, which returns 404 at Bungie.net.
The workbook supplies April 2006 settings evidence independently of the
changelist below.

A [separate option replay](evidence/matchmaking-rust-parity-20261028/april2006-workbook-option-replay.json)
now checks those three cells directly against the unchanged TU1.5 descriptors,
setter, getter and read-only validator. The original and Rust agree on ten
diagnostic Slayer buffers. Exact workbook labels and written byte masks are in
the receipt. The rest of each buffer comes from the TU1.5 default initializer
and the diagnostic controls, so this does not recover a deployed SWAT buffer.

## April and June 2006 deployment changes

Wayback's availability API resolved Optimatch post **6642602** to a November
11, 2007 capture. Both the [replay](https://web.archive.org/web/20071111201933/http://www.bungie.net:80/Forums/posts.aspx?postID=6642602)
and the [raw capture](https://web.archive.org/web/20071111201933id_/http://www.bungie.net:80/Forums/posts.aspx?postID=6642602)
returned the first post by Ninja 0n Fire. The header shows April 14, 2006;
the body is marked edited April 25 and explicitly announces deployment. Use
the edited text as the dated change set. It is separate from the draft
described in the April 14 weekly update. Capture and first-post hashes are in
the [changelist receipt](evidence/matchmaking-rust-parity-20261028/april-june-2006-playlist-changelog-join.json).

The posted changes remove Containment, Waterworks and Elongation from Team
Snipers. The current reconstructed `h2f-final-era-v1` Team Snipers row includes
all three. This is a documented difference from the April profile. The April
change does not establish whether any of those maps later returned, so do not
apply it as a final-era rotation edit.

For ActionSack, the post's Pit of Joy win target is five minutes. The workbook
distinguishes the King score to win (`Variants!DF50=300`) from the ten-minute
round limit (`DF85=600`). The corresponding Team Brutality entry sets damage
resistance off (`R143=0`), consistent with the post. Neither source supplies
the numeric weight increases/decreases announced for Pit of Joy, shotgun modes
or Rumble Training objectives. These joins corroborate documentary settings;
they do not execute a serialized historical variant.

The [June 2 update](https://halo.bungie.org/bwu/index.html?item=77) records a
different set of changes, including the addition of unranked Team SWAT and
an increase in Team Slayer's Lockout weight. [KP's contemporary reply](https://forums.bungie.org/halo/archive24.pl?read=717097)
and [HBO's deployment report](https://halo.bungie.org/news.html?item=15893)
place the rollout on June 6. These are dated deltas. The complete base
rotations, numeric weights and final 2010 title-storage downloads are open.

## February and August 2007 Team SWAT

The February 9 Optimatch post by Jeremiah on the Bungie Online Team lists
22 map and variant pairs across nine maps for the unranked Team SWAT test
rotation. Five pairs use CTF. Bungie's February 13 announcement confirms the
return to the live playlist list. The [captured Optimatch post](https://web.archive.org/web/20070212051255id_/http://www.bungie.net:80/Forums/posts.aspx?postID=9997598),
[weekly update](https://halo.bungie.org/bwu/index.html?item=113) and
[Bungie deployment announcement](https://bnetarchive.haloman30.com/news/Blog7d2b-2.html?dfilter=&mode=tags&page=1&tag=22)
support those two dated claims.

August 30 match records establish later play of SWAT Classic on Desolation,
SWAT Carbines on Midship and SWAT Magnums on Sanctuary. The same records
show Team Snipers on Tombstone, Relic and Terminal. The current reconstructed
Team SWAT catalog has six rows, all using a single Slayer variant. The
February CTF settings, deployment payload and numeric weights remain open.
The [August service record](https://bnetarchive.haloman30.com/Stats/playerstatshalo2a5c3.html?player=Xerosus)
and the [membership receipt](evidence/matchmaking-rust-parity-20261028/february-2007-team-swat-membership.json)
record the observations and the exact comparison.

## Final original Xbox Live title storage

The [prior configuration follow-up](online-historical-config-followup-20261013.md)
pins the 1,453,248-byte constructed retail fallback and distinguishes it from
a downloaded hopper. No new title-storage payload was found in this pass.

| Route | Observed response and boundary |
| --- | --- |
| [Internet Archive item metadata for multiplayer_hopper.dat](https://archive.org/advancedsearch.php?q=%22multiplayer_hopper.dat%22&output=json) | HTTP 200, `numFound: 0`. This index query does not inspect embedded files or all archived HTTP responses. |
| [Internet Archive item metadata for network_configuration.dat](https://archive.org/advancedsearch.php?q=%22network_configuration.dat%22&output=json) | HTTP 200, `numFound: 0`, with the same scope limit. |
| [Wayback availability for Bungie's public catalog on 14 April 2010](https://archive.org/wayback/available?url=http%3A%2F%2Fwww.bungie.net%2FStats%2Fpage.aspx%3Fsection%3DFAQInfo%26subsection%3Dplaylists%26page%3D1&timestamp=20100414) | HTTP 200; closest returned capture `20070314234226`. This corroborates the earlier receipt. A public 2007 catalog does not supply a 2010 download. |

The source-derived next route is an authorized read-only image of a dated
original Xbox installation/cache or a preserved original service transfer.
The retail hopper file owner opens `n:\multiplayer_hopper.dat`; resolve the
title's actual volume mapping on the particular evidence image before relying
on this path. The retail network owner requests title storage through
`XOnlineStorageCreateServerPath` and downloads to memory. Its observed retail
version prefix is `9809`; this does not identify a public Bungie web URL or
prove a persistent network file. An authorized dated original transfer or
service archive is the direct route for that memory payload. For any candidate,
record the original title/build and acquisition date, exact source path,
read-only image or capture identity, payload SHA-256 and original size,
version and CRC checks where applicable. Valid structure is insufficient to
date a file or distinguish a reconstructed fallback from the Live download.
[Hopper file owner](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp#L404),
[title-storage path owner](../../tools/reference/halo2-decompiled/src/unknown_0b3d30.cpp#L90),
[network receive and validation evidence](halo2-candidate-scoring-evidence.md).

## August 2006 Team Carnage / HBO

The live [original Bungie story URL](https://www.bungie.net/News/content.aspx?link=wonga&type=news)
returned 404. The preserved [KPaul deployment story, 22 August 2006](https://bnetarchive.haloman30.com/news/content3afc.html?link=wonga&type=news)
is reachable by the permalink in the master's N15 blog capture. It identifies
HBO as author and records the 6v6 offering plus selected settings. It names
Ascension for Tower of Power; this is a concrete map association, not a map
table. It presents Chamb0 CTF's time field as a first-to count; it supplies no
round duration for that field. The [1 September Bungie update](https://halo.bungie.org/bwu/index.html?item=90)
confirms the August deployment and rotating community configuration.

The [HBO news archive](https://halo.bungie.org/newsarchive.html?month=August&year=2006)
links its contemporary feedback thread. A direct HTTP client followed its
redirects to [the preserved original thread](https://forums.bungie.org/halo/archive25.pl?read=737819)
and returned 200 after the text web reader refused the redirected route.
[KP's contemporary reply](https://forums.bungie.org/halo/archive25.pl?read=737858)
explains why FAE was adapted to team Slayer and identifies the Rocketball
antecedent for Wheeeeeee!. [Louis Wu's reply](https://forums.bungie.org/halo/archive25.pl?read=737861)
also distinguishes the matchmaking adaptations from the customary games and
attributes the Chamb0 name to c0ld. These author-side posts are useful leads
for an original HBO submission/export; they do not link a complete variant
file. An original saved custom antecedent would still need a dated join to
the actual Bungie submission and deployed revision. The April workbook is
earlier than this checkpoint; no heading matched the checked names Sun King,
Capture Madness, Chamb0 CTF, FAE or Tower of Power. No numeric weights, full
map membership or full HBO buffers were recovered in this pass.

Only the required 245,248-byte workbook, its 39,409-byte JSON receipt and this
note were retained. Owned extraction and download scratch, including parser
dependencies, was removed. No GAP, algorithm ID or acceptance gate is closed
by this recovery.
