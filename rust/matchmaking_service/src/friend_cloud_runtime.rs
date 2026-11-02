//! Directory transactions for private file claims; never hold SQL during IO.
use super::*;
impl Runtime {
    pub fn configure_friend_cloud(&mut self, enabled: bool) {
        self.friend_cloud_enabled = enabled;
    }
    pub fn friend_cloud_wake(&self) -> std::sync::Arc<tokio::sync::Notify> {
        self.friend_cloud_wake.clone()
    }
    pub fn friend_cloud_next(&mut self) -> Result<Value> {
        self.directory(
            "friends",
            "cloud_next",
            None,
            None,
            Value::Null,
            Value::Null,
        )
    }
    pub fn friend_cloud_finish(&mut self, claim: &str, outcome: &str) -> Result<()> {
        self.directory(
            "friends",
            "cloud_finish",
            None,
            None,
            json!({"claim":claim,"outcome":outcome}),
            Value::Null,
        )?;
        Ok(())
    }
}
