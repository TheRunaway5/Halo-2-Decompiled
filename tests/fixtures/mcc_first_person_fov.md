# MCC first-person FOV camera override

Generate the ignored binary-derived TSV and provenance report from an owned
installation of MCC:

```sh
python tools/oracle_mcc_first_person_fov.py \
  --dll '/path/to/Halo The Master Chief Collection/halo2/halo2.dll' \
  --fixture tests/fixtures/mcc_first_person_fov.tsv \
  --report tests/fixtures/mcc_first_person_fov.provenance.json
```

The pinned DLL hash is
`de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946`.
The oracle executes unchanged instructions at RVAs `7E0DE7` through `7E0F69`,
without hooks, against 20 declared incoming camera/aspect combinations. The
first-person pass saves the world camera, then overwrites its vertical angle
with `0.8655884265899658`, near with `0.015625`, and far with `1` before building
its own projection. Position, orientation and safe/outer rectangles survive.
This establishes the lens override independently of the world slider.

Native gameplay adopts the recovered lens for hands, weapon geometry and muzzle
particles. The native NV2A W-depth representation keeps its existing scaling;
this fixture does not establish MCC depth-buffer or live framebuffer parity.
