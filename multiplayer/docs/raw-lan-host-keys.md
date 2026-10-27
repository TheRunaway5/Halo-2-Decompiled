# Raw LAN host authentication

Native protocol107 requires the host's complete ephemeral public key before a
raw UDP configuration query or join. The host prints a `LAN join token` when
started with `--host PORT` or from the System Link hosting menu. Replace
`HOST_ADDRESS` in that token with the host's reachable IPv4 address and pass the
whole value, quoted, to `--join 'ADDRESS:PORT#HOST_KEY'`. Existing frontend raw
System Link selection hosts a session; remote raw joining uses this CLI.

Exchange the full token through an independent trusted channel, such as showing
the host console in person or an already authenticated conversation. The key is
public, but its authenticity matters: a key obtained from UDP discovery or the
first host response is insufficient. The key changes whenever the host starts a
new process. Address-only joins fail before sending their first packet. Older
native peers cannot join protocol107 sessions.

The Rust authentication owner binds the independently selected host key and the
client key before opening any frame. Its key derivation includes both role
labels, both ephemeral keys, protocol version, client nonce, content fingerprint,
and the client's resolved destination IPv4 address and port. The destination is
a client-declared context authenticated by the selected host key, allowing an
explicit UDP proxy without mistaking its socket for the host identity.
Configuration queries use a separate nonce/context with a zero content
fingerprint, since the query retrieves that value. Configuration fragments and
pre-join rejections are authenticated too. Cookies still prove return
routability before any large response or player allocation; they do not select
the host key. Established frames retain replay protection and packet limits.

Authenticated ONLINE transports already bind admitted room peers. They use their
existing admission and transport identity, and never derive identity from this
raw UDP token. Raw LAN authentication does not encrypt game traffic or establish
a verified player account identity for a local participant.

Local embeddings may read `LanHost::join_address()` or receive the runtime's
`session_lan_host_ready` callback and exchange that value through a trusted local
channel. Tests use that explicit ownership handoff; they do not learn a key from
an untrusted first packet.
