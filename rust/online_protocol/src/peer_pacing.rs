//! Fixed physical-peer scan order under a shared packet/byte budget.
//!
//! Only a packet that was actually emitted advances the cursor. Inspecting an
//! empty, unaffordable or excluded peer leaves the next scan unchanged, so a
//! credit-starved scan cannot repeatedly favor the same endpoint. Membership,
//! admission, token credit, retries and acknowledgements remain session-owned.

const PEERS: u8 = 16;

#[derive(Debug, Default)]
pub struct PeerPollCursor {
    next: u8,
}

impl PeerPollCursor {
    pub fn order(&self) -> [u8; 16] {
        std::array::from_fn(|offset| (usize::from(self.next) + offset) as u8 % PEERS)
    }

    /// Record the last successfully emitted physical peer. Invalid input
    /// fails without changing the next order or panicking across the bridge.
    pub fn record_served(&mut self, peer: u8) -> bool {
        if peer >= PEERS {
            return false;
        }
        self.next = (peer + 1) % PEERS;
        true
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn every_order_is_unique_bounded_and_wraps_after_the_served_peer() {
        let mut cursor = PeerPollCursor::default();
        for served in 0..PEERS {
            assert!(cursor.record_served(served));
            let order = cursor.order();
            assert_eq!(order[0], (served + 1) % PEERS);
            assert_eq!(order[15], served);
            let mut sorted = order;
            sorted.sort_unstable();
            assert_eq!(sorted, std::array::from_fn(|peer| peer as u8));
        }
        assert_eq!(cursor.order()[0], 0);
    }

    #[test]
    fn invalid_physical_peers_leave_the_cursor_unchanged() {
        let mut cursor = PeerPollCursor::default();
        assert!(cursor.record_served(8));
        let before = cursor.order();
        for invalid in [16, u8::MAX] {
            assert!(!cursor.record_served(invalid));
            assert_eq!(cursor.order(), before);
        }
    }

    #[test]
    fn empty_or_unaffordable_scans_do_not_advance() {
        let mut cursor = PeerPollCursor::default();
        assert!(cursor.record_served(4));
        let before = cursor.order();
        for _ in 0..64 {
            assert_eq!(cursor.order(), before);
        }
        assert_eq!(cursor.order()[0], 5);
    }

    #[test]
    fn truncated_scans_resume_after_the_last_actual_emission() {
        let mut cursor = PeerPollCursor::default();
        let scan = cursor.order();
        // Fixed membership contains PCs 0, 4 and 8. Peer 0 is the sender;
        // one affordable packet goes to 4 before the shared credit empties.
        assert!(cursor.record_served(scan[4]));
        let next = cursor.order();
        assert_eq!(next[0], 5);
        assert!(
            next.iter().position(|&peer| peer == 8) < next.iter().position(|&peer| peer == 4)
        );
        // A second actual packet in the same scan, including a control
        // packet, is the last served owner and determines the next order.
        assert!(cursor.record_served(8));
        assert_eq!(cursor.order()[0], 9);
    }
}
