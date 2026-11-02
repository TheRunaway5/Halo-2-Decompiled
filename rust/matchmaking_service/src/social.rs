//! HTTP-facing social reads and commands use the shared native directory owners.
//! Population is an anonymous observation of admitted accounts, seats and rooms.
use super::*;

impl Runtime {
    pub fn friends(&mut self, header: &str, body: Option<Value>, features: Value) -> Result<Value> {
        let auth = self.session(header)?;
        self.directory(
            "friends",
            if body.is_some() { "command" } else { "get" },
            Some(
                auth["player"]
                    .as_str()
                    .ok_or(Error(503, "Native account player missing"))?,
            ),
            Some(
                auth["key"]
                    .as_str()
                    .ok_or(Error(503, "Native account key missing"))?,
            ),
            body.unwrap_or(Value::Null),
            features,
        )
    }
    pub fn population(&mut self, features: Value) -> Result<Value> {
        if !features.as_array().is_some_and(Vec::is_empty) {
            return Err(Error(400, "Online counts do not accept filters"));
        }
        let clock = self.clock()?;
        let rooms = self.room_sockets.observations(self.started);
        self.owners.request(
            self.accounts,
            &json!({
                "op":"presence","view":"population","source":self.queue,
                "now":clock.monotonic,"queue_now":clock.monotonic,"wall":clock.wall,
                "rooms":rooms.facts()
            }),
        )
    }
}
