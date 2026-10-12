# Imported original sound-startup operands

`OriginalData::sound_startup_constants` now imports only the authenticated
immutable126B20 startup operands for1.1 and the user-owned1.5 image. The provider
type is the separately implemented `SoundStartupConstants`. Unknown base-image
operands remain absent. Arena allocation, mutable source pools, clocks/CRT RNG,
cache initialization, device creation and live map activation are not supplied.

The pinned source packet is
`analysis/multiplayer/integration/sound-startup-native/paired/providers.json`
(SHA256dcfdb4ae7172b4ef3f746f4b186d0bbd363158235cb202a32ab2be8a93765aec).
The actual ambient binary32 word is read at4457BC in1.1 and44923C in1.5;
the policy's complete16 bytes are read at451220 and455B20. The packet, rather
than a provisional investigation message, is address authority. The provider
does not turn these source policy bytes into a native platform cap.

`tools/export_mp_sound_startup_constants.py` authenticates the packet and both
owned executable hashes, independently resolves their XBE sections, and compares
each actual byte. Measured and independent repeated exports have identical
fixtures and reports. The fixture SHA256 is
1a310f612849179eccfaabcd1cb167b32ea14c6ef364e315de162dd5791127cb;
the report SHA256 is
71c82b761ad280294ccdfefe9e670d8b3c5fb53a9b0c52df686158d8253d2bc6.

Production `original_sound_startup_constants` passes in normal and GCC
ASan/UBSan builds: both selected images supply all40 literal bytes, and the
unverified base supplies no optional packet. This is an operand/provider test,
not original startup acceptance, active-map readiness or audible gameplay.

The separately frozen `sound_startup.cpp` is also registered in the production
Cache library. `original_sound_startup` passes in both production configurations:
the complete retained source prefix/arena and2188 ordered represented stores
match, and the separately bounded complete original post-device commit fragment
matches741 stores. The test retains the actual platform stop; its fixture cache
callback is not a live cache implementation. Actual cache/storage/clock/CRT RNG
and real native platform construction remain mandatory services. Source map
activation is a later owner; startup itself leaves the live-map flag zero.
