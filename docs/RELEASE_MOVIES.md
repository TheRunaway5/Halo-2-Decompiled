# MCC movie delivery and preserved reference intro and credits

Policy reaffirmed, 2026-10-20: the user's latest instruction makes MCC imports
the sole runtime asset source, with Vista and OG TU1.5 used only as references.
This supersedes the earlier supplied-intro/credits exception for delivered
builds. Preserve those supplied files and their provenance as reference inputs.
Source-policy changes do not establish installed-runtime or release acceptance.

MCC imports are the only runtime asset source for the delivered port. The
separate upscaled intro/credits bank below is preserved as an optional reference
input, not an MCC-derived bank. Normal MCC import, reuse and packaging must not
require, copy or select it. An existing cache's `port_bundled_movies` metadata
does not authenticate MCC ownership.

Actual installed MCC Anniversary cutscenes and collection media have been
inventoried, but none has a verified assignment to the Classic frontend's
intro, credits or attract roles. Those roles remain unavailable for MCC until
their source assignment and decoder path are qualified. An unrelated cutscene
or credits background must not be substituted by filename similarity. This
keeps the remaining movie-delivery requirement explicit; it does not establish
a complete MCC movie replacement. Existing user files and playback copies
remain intact.

Optional reference builds can bundle the user-supplied AI upscaled `intro_60.mp4` and
`credits_60.mp4` without transcoding. Both are 3840x2160 at 60000/1001 fps;
the intro is H.264, the credits are AV1, and both contain six-channel AAC.
FFmpeg must provide H.264, AV1 and AAC decoders. Playback preserves the video
aspect ratio with black bars when needed and downmixes audio into the existing
48 kHz stereo mixer.

The original files total 2,538,778,741 bytes (about 2.36 GiB).
`resources/movie-manifest.json` and `resources/movies/manifest.json` pin their
sizes and SHA-256 checksums. The two MP4 files are tracked with Git LFS; the
manifests remain ordinary Git files. Install Git LFS before cloning or pulling,
then fetch and verify the bundled files:

```sh
git lfs install --local
git lfs pull --include="resources/movies/intro_60.mp4,resources/movies/credits_60.mp4"
python3 tools/bundle_movies.py --verify
```

Git stores small pointers for the movies. Each local checkout also retains
2,538,778,741 bytes in its Git LFS object store, in addition to the working
copies. When publishing, use a normal Git push with the LFS pre-push hook
installed so it uploads both movie objects. Other files in `resources/movies`
remain ignored. These reference movies are port-supplied assets; they do not need
to be obtained from an MCC installation or an external Videos directory.

To replace the bundled files from the supplied originals, import them once:

```sh
python3 tools/bundle_movies.py --source "/home/eric/Videos/Halo 2 Movies"
python3 tools/bundle_movies.py --verify
```

Fresh builds leave `HALO2_BUNDLE_MOVIES=OFF`, including Release and
multi-configuration builds. An explicit reference build can select
`-DHALO2_BUNDLE_MOVIES=ON`; it then stops if either input is missing. Existing
CMake cache selections are retained. `HALO2_MOVIE_SOURCE_DIR` can point at
another directory containing the same verified files. Enabling this reference
target does not assign its movies to an MCC frontend role.

The `halo2_movies` target verifies and copies both files and `manifest.json`
into `<build>/resources/movies` (or `<build>/<config>/resources/movies`). It is
an automatic dependency of `halo2_native` and both runtime launchers. Changed
inputs are verified again; an absent or altered movie fails the build.

```sh
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DHALO2_BUNDLE_MOVIES=ON
cmake --build build-release --target halo2_launcher halo2_title_launcher -j
python3 tools/bundle_movies.py --verify --source build-release/resources/movies
```

When assembling explicit reference folders, include `resources/movies` alongside
`halo2_native`. For a separately assembled destination, stage the movie
component after building, or use the bundling tool directly:

```sh
cmake --install build-release --component movies --prefix /path/to/release
python3 tools/bundle_movies.py --output /path/to/release/resources/movies
```

For non-MCC reference inputs, the runtime checks `resources/movies` beside its executable, then the
project's `resources/movies` under the launcher's working directory. It uses
the supplied MP4 for intro/credits when present and the extracted disc Bink
otherwise. Attract playback continues to use the disc movie. A present but
undecodable MP4 reports the decoding error. There is no dependency on the
original Videos folder after importing and staging.

To preview either bundled movie, use `--menu-movie intro` or
`--menu-movie credits`; `--movie-frame N` selects an encoded frame. The
title-first launcher and `--skip-intro` retain their startup behavior.

## Hardware decoding and lossless intro playback

Automatic playback tries FFmpeg hardware decoding through CUDA, D3D11VA,
VideoToolbox or VAAPI, according to the available codec/device configuration.
VAAPI covers AMD and Intel on Linux; D3D11VA covers supported Windows GPUs.
Unavailable or failed hardware decoding falls back to the original file in
software; the HEVC compatibility copy is used only with hardware. FFmpeg and
the installed driver must expose the chosen backend and codec. Software
decoding uses FFmpeg's automatic CPU thread count. Initialization and decoding
stay on one worker thread, with two frames of bounded prefetch. The authored
clock, audio source, duration, frame count, aspect ratio and RGBA conversion
are preserved. GPU NV12 downloads are deinterleaved before the original planar
color conversion to avoid rounding changes.

The original intro has an H.264 High 4:4:4 Predictive profile identifier despite
containing eight-bit 4:2:0 pixels. Some AMD/Intel hardware cannot admit that
profile. An optional **lossless HEVC** playback copy makes the same pixels
available to supported decoders without replacing either original MP4.

```sh
python3 tools/prepare_movie_playback.py --report analysis/movie-preparation.json
python3 tools/prepare_movie_playback.py --verify
```

Preparation tries NVENC, then portable libx265; `--encoder libx265` explicitly
uses the CPU. It compares every decoded YUV frame against the pinned original,
copies audio packets, and publishes only after all frames match. Temporary
encoding/verification files use an owned temporary directory, under
`HALO2_TEST_SCRATCH` when supplied. Encoding is a preparation step; ordinary
builds only verify and stage an existing bank.

Keep `resources/movie-playback/manifest.json` with the separately generated
`intro.hevc.mp4`. The video is ignored by Git and can be regenerated. When this
bank exists at configure time, `halo2_movie_playback` stages it next to the
runtime in `movie-playback/`; the `movies` install component includes it too.
For another runtime directory:

```sh
python3 tools/prepare_movie_playback.py --stage resources/movie-playback \
  --output /path/to/runtime/movie-playback
```

The runtime checks the exact original and derived payload hashes before using
the optional copy. A stale, missing or invalid bank falls back to the original.
The prepared 2026-10-20 payload is 1,658,954,183 bytes (1.55 GiB). It retains
all 1,079 original intro frames, 3840x2160 resolution, 60000/1001 frame rate
and the original 18.139833-second duration. Credits continue to use the original
AV1 file; movie audio continues to decode from the original files.

Diagnostic controls:

- `HALO2_MOVIE_HWACCEL=auto|cuda|vaapi|d3d11va|videotoolbox|software` selects a
  backend. `HALO2_MOVIE_HW_DEVICE=/dev/dri/renderD129` can select a VAAPI node;
  automatic VAAPI initialization also probes available render nodes.
- `HALO2_REFERENCE_SOFTWARE_MOVIES=1` selects the original software path,
  bypassing the optional playback bank for comparisons.
- `HALO2_MOVIE_TIMINGS=/path/to/output.tsv` records advance time, authored
  clock, displayed frame and the decoder actually used.
- `movie_tests --compare-decoders resources/movies --deep` checks GPU versus
  original software RGBA, catch-up, metadata and middle/end samples. Unsupported
  hardware returns the CTest skip code; software decoding remains required.

Physical NVIDIA and AMD decoder checks pass on the development machine.
Intel, Windows and macOS hardware were not available for physical validation.
