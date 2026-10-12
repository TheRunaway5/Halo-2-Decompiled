# Dear ImGui provenance

Unmodified Dear ImGui v1.91.9b, upstream commit `f5befd2d29e66809cd1110a152e375a7f1981f06`:
https://github.com/ocornut/imgui/tree/f5befd2d29e66809cd1110a152e375a7f1981f06

Core files and the SDL2 backend were copied from the local Phase-Distorter
vendor directory after checking its SHA256SUMS. The Vulkan backend was retrieved
from the same pinned upstream commit. LICENSE.txt retains the MIT license;
SHA256SUMS records every imported file. No Phase-Distorter game code is copied.

The optional F1 panel follows Phase-Distorter's input capture and command request
pattern, adapted to Halo's native services. It does not alter authored menu art.
