# Native direct peer dependencies

The native binary statically links unmodified libdatachannel0.24.6, commit
`6b1e2e620f1e37f0eafeee702eaea0043cb305fd`, and its pinned libjuice, plog and
usrsctp dependencies. Upstream source and submodule revisions are available at
https://github.com/paullouisageneau/libdatachannel/tree/6b1e2e620f1e37f0eafeee702eaea0043cb305fd.
Ship this directory with packaged binaries. OpenSSL remains a system dependency.
The adjacent files retain each upstream license and copyright notice.
