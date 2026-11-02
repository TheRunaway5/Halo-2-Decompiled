//! Typed conversion and synchronization boundary only; measurements live in Rust.
use crate::ffi;
use halo2_online_protocol::peer_mesh_quality as core;
pub struct PeerMeshQuality(core::Mesh);
fn protect<T>(call: impl FnOnce() -> Result<T, core::Error>) -> Result<T, String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(call))
        .map_err(|_| "Rust direct mesh quality failed".to_owned())?
        .map_err(|e| e.to_string())
}
fn context(c: &ffi::MeshQualityContext) -> Result<core::Context, core::Error> {
    Ok(core::Context {
        session: c.session,
        epoch: c.epoch,
        membership_revision: c.membership_revision,
        peers: c.peers,
        host: c.host.try_into().map_err(|_| core::Error)?,
    })
}
fn to_context(c: core::Context) -> ffi::MeshQualityContext {
    ffi::MeshQualityContext {
        session: c.session,
        epoch: c.epoch,
        membership_revision: c.membership_revision,
        peers: c.peers,
        host: c.host as u32,
    }
}
fn row(row: Option<core::Row>) -> ffi::MeshQualityRow {
    let present = row.is_some();
    let row = row.unwrap_or(core::Row {
        links: [core::Quality::default(); 16],
        received_ms: 0,
    });
    ffi::MeshQualityRow {
        present,
        received_ms: row.received_ms,
        links: row.links.map(|v| ffi::MeshQualityLink {
            rtt_ms: v.rtt_ms,
            upload_bytes_per_second: v.upload_bytes_per_second,
            loss_per_mille: v.loss_per_mille,
            measured: v.measured,
        }),
    }
}
pub fn new_peer_mesh_quality(
    self_peer: u32,
    c: &ffi::MeshQualityContext,
) -> Result<Box<PeerMeshQuality>, String> {
    protect(|| {
        core::Mesh::new(self_peer.try_into().map_err(|_| core::Error)?, context(c)?)
            .map(|v| Box::new(PeerMeshQuality(v)))
    })
}
pub fn peer_mesh_quality_context(
    mesh: &PeerMeshQuality,
) -> Result<ffi::MeshQualityContext, String> {
    protect(|| Ok(to_context(mesh.0.context())))
}
pub fn peer_mesh_quality_poll(
    mesh: &mut PeerMeshQuality,
    ready: u16,
    now: u64,
    budget: u32,
) -> Result<Vec<ffi::HostAgreementPacket>, String> {
    protect(|| {
        Ok(mesh
            .0
            .poll(ready, now, budget)
            .into_iter()
            .map(|v| ffi::HostAgreementPacket {
                peer: v.peer,
                bytes: v.bytes,
            })
            .collect())
    })
}
pub fn peer_mesh_quality_receive(
    mesh: &mut PeerMeshQuality,
    peer: u32,
    bytes: &[u8],
    now: u64,
) -> Result<bool, String> {
    protect(|| Ok(mesh.0.receive(peer.try_into().unwrap_or(16), bytes, now)))
}
pub fn peer_mesh_quality_matrix(
    mesh: &PeerMeshQuality,
    now: u64,
) -> Result<ffi::MeshQualityMatrix, String> {
    protect(|| {
        Ok(ffi::MeshQualityMatrix {
            rows: mesh.0.matrix(now).map(row),
        })
    })
}
pub fn peer_mesh_quality_local(
    mesh: &PeerMeshQuality,
    now: u64,
) -> Result<ffi::MeshQualityRow, String> {
    protect(|| Ok(row(mesh.0.local(now))))
}
pub fn peer_mesh_quality_candidates(
    mesh: &PeerMeshQuality,
    now: u64,
    migration: bool,
) -> Result<Vec<ffi::MeshQualityHost>, String> {
    protect(|| {
        Ok(if migration {
            mesh.0.migration_candidates(now)
        } else {
            mesh.0.candidates(now)
        }
        .into_iter()
        .map(|v| ffi::MeshQualityHost {
            peer: v.peer as u32,
            mean_rtt_ms: v.mean_rtt_ms,
            minimum_upload_bytes_per_second: v.minimum_upload_bytes_per_second,
            worst_loss_per_mille: v.worst_loss_per_mille,
        })
        .collect())
    })
}
pub fn peer_mesh_quality_is_packet(bytes: &[u8]) -> bool {
    std::panic::catch_unwind(|| core::is_packet(bytes)).unwrap_or(false)
}

#[cfg(test)]
mod tests {
    use super::*;
    fn config() -> ffi::MeshQualityContext {
        ffi::MeshQualityContext {
            session: [1; 32],
            epoch: 3,
            membership_revision: 5,
            peers: 0x8011,
            host: 16,
        }
    }
    #[test]
    fn unsigned_native_indices_cannot_alias_physical_zero_or_the_pregame_sentinel() {
        for bad in [16, 17, 255, 256, u32::MAX] {
            assert!(new_peer_mesh_quality(bad, &config()).is_err());
        }
        for bad in [17, 255, 256, u32::MAX] {
            let mut c = config();
            c.host = bad;
            assert!(new_peer_mesh_quality(0, &c).is_err());
        }
        let mut c = config();
        c.host = 0;
        let mut mesh = new_peer_mesh_quality(15, &c).unwrap();
        let returned = peer_mesh_quality_context(&mesh).unwrap();
        assert_eq!(returned.host, 0);
        assert_eq!(returned.peers, 0x8011);
        let ping = peer_mesh_quality_poll(&mut mesh, 1, 1000, 1)
            .unwrap()
            .remove(0)
            .bytes;
        let mut receiver = new_peer_mesh_quality(4, &c).unwrap();
        assert!(peer_mesh_quality_receive(&mut receiver, 256, &ping, 1001).unwrap());
        assert!(
            peer_mesh_quality_poll(&mut receiver, 0, 1002, 1)
                .unwrap()
                .is_empty()
        );
        assert!(!peer_mesh_quality_local(&receiver, 1002).unwrap().links[0].measured);
    }
}
