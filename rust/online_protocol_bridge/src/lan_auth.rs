//! CXX ownership only. Authentication and replay decisions live in safe Rust.
use halo2_online_protocol::lan_auth as policy;
pub struct LanEndpoint(policy::Endpoint);
pub struct LanSession(policy::Session);
pub fn new_lan_endpoint() -> Result<Box<LanEndpoint>, String> {
    policy::Endpoint::new().map(|v| Box::new(LanEndpoint(v))).map_err(|e| e.to_string())
}
pub fn new_lan_session() -> Box<LanSession> { Box::new(LanSession(policy::Session::default())) }
pub fn lan_public(endpoint: &LanEndpoint) -> [u8; 32] { endpoint.0.public() }
pub fn lan_bind(endpoint: &LanEndpoint, session: &mut LanSession, peer: &[u8;32]) -> Result<(), String> {
    endpoint.0.bind(&mut session.0, *peer).map_err(|e| e.to_string())
}
pub fn lan_seal(endpoint: &LanEndpoint, session: &mut LanSession, host: bool, body: &[u8]) -> Result<Vec<u8>, String> {
    endpoint.0.seal(&mut session.0, host, body).map_err(|e| e.to_string())
}
pub fn lan_open(endpoint: &LanEndpoint, session: &mut LanSession, host: bool, packet: &[u8]) -> Result<Vec<u8>, String> {
    endpoint.0.open(&mut session.0, host, packet).map_err(|e| e.to_string())
}
pub fn lan_is_packet(packet: &[u8]) -> bool { policy::is_packet(packet) }
