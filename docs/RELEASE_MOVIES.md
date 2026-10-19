# Release intro and credits

Releases bundle the user-supplied AI upscaled `intro_60.mp4` and
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
remain ignored. The bundled movies are port-supplied assets; they do not need
to be obtained from an MCC installation or an external Videos directory.

To replace the bundled files from the supplied originals, import them once:

```sh
python3 tools/bundle_movies.py --source "/home/eric/Videos/Halo 2 Movies"
python3 tools/bundle_movies.py --verify
```

Normal builds enable `HALO2_BUNDLE_MOVIES` when both local movies exist. Fresh
Release and multi-configuration builds enable it by default and stop if either
input is missing. A code-only build can explicitly select
`-DHALO2_BUNDLE_MOVIES=OFF`. Existing CMake cache selections are retained;
pass `-DHALO2_BUNDLE_MOVIES=ON` when changing an existing build to a release
with movies. `HALO2_MOVIE_SOURCE_DIR` can point at another directory containing
the same verified files.

The `halo2_movies` target verifies and copies both files and `manifest.json`
into `<build>/resources/movies` (or `<build>/<config>/resources/movies`). It is
an automatic dependency of `halo2_native` and both runtime launchers. Changed
inputs are verified again; an absent or altered movie fails the build.

```sh
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DHALO2_BUNDLE_MOVIES=ON
cmake --build build-release --target halo2_launcher halo2_title_launcher -j
python3 tools/bundle_movies.py --verify --source build-release/resources/movies
```

When assembling release folders, include `resources/movies` alongside
`halo2_native`. For a separately assembled destination, stage the movie
component after building, or use the bundling tool directly:

```sh
cmake --install build-release --component movies --prefix /path/to/release
python3 tools/bundle_movies.py --output /path/to/release/resources/movies
```

The runtime first checks `resources/movies` beside its executable, then the
project's `resources/movies` under the launcher's working directory. It uses
the supplied MP4 for intro/credits when present and the extracted disc Bink
otherwise. Attract playback continues to use the disc movie. A present but
undecodable MP4 reports the decoding error. There is no dependency on the
original Videos folder after importing and staging.

To preview either bundled movie, use `--menu-movie intro` or
`--menu-movie credits`; `--movie-frame N` selects an encoded frame. The
title-first launcher and `--skip-intro` retain their startup behavior.
