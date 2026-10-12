# Original particle view constant fixtures

These records come from executing unchanged `default.xbe` routine `0023E460`.
Only device upload `003F7330` is intercepted. Original FSIN/FCOS, SSE table
subtractions, the orientation transpose, both shadow-copy branches, and dirty
flag clearing execute unchanged. The native test checks the pure 22-vector
constructor; it does not claim to implement the dirty flag or upload scheduling.

Two input orientations are recovered from original captured uniforms at draws
5879 and 6463. Every resulting original register word matches that capture.
The remaining 254 cases include signed zero, subnormals, finite extremes,
infinities, a NaN payload, and reproducible arbitrary orientation entries.

Each input record is nine float words after a little-endian count. Each expected
record is 22 float4 vectors, physical registers 148..169 (game registers 52..73).
Original bytes are produced independently of the native constructor. Live camera
state generation and particle scheduling remain unverified.

XBE SHA-256: `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

Input SHA-256: `ac83265fcc79bd41059e369ed28c77d4ef6cf504a36a18657e6db94ae4c00173`.

Expected SHA-256: `9eecbbc31655aec690a9055730e499ff943cdff7aecd7358bfd823ea266bb353`.
