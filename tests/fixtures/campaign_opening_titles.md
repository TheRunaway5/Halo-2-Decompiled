# Opening title source records

The two TSV fixtures contain the complete consecutive 36-byte `scnr+0x1f0`
title records, in file order. They are little endian. The first line names
the source domain, map, and full SHA256 of the actual source file.

- Original Xbox reference: `assets/disc/maps/00a_introduction.map`, build
  `02.09.27.09809`, SHA256
  `5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28`.
- Tested MCC input: the private cache selected in `assets/mcc-source.tsv`,
  `halo2/h2_maps_win64_dx11/00a_introduction.map`, version13, SHA256
  `f6916833eedc5efba6250ff2bfd0d0547318572901776069ca043c0c8b4329b8`.

Both sources name `scenarios\solo\00a_introduction\00a_introduction`.
Their title text is identical: the High Charity location/age card and the
Cairo location/date card. String IDs differ by domain and are preserved.
The authored font selector, alignment, colors, and timing bytes are identical.
Original bounds are `[350,0,480,595]`; the MCC bytes decode to
`[225,1024,40,80]`. Direct Xbox rectangle subtraction produces width `-944`
and the reported one-character-per-line failure.

`read_campaign_titles` projects the complete matched MCC bank into the
original logical bounds for both production consumers. It does not edit source
cache metadata or font artwork. A different scenario, original input, or any
changed donor byte receives no retail projection. This is a finite source
profile, not a claimed general conversion of unknown MCC title coordinates.

Extraction/comparison receipt:
`analysis/opening-cinematic-20261015/mcc-title-layout-proof-v771/report.json`.
The original cache is only an offline metadata authority. Playback tests use
the selected MCC preparation.
