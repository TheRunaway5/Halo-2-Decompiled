//! Accepted clan membership and native service storage policy.
//! Client role/lifecycle rules: online_teams,2393AE,1A334A,23955E.
//! HTTP, quotas, name folding, message retention and invitation expiry are
//! explicit modern policies, not a recovered Xbox Live server implementation.
use super::*;

const SCHEMA: &str = r#"
CREATE TABLE IF NOT EXISTS clans(id TEXT PRIMARY KEY,name TEXT NOT NULL,folded_name TEXT NOT NULL UNIQUE);
CREATE TABLE IF NOT EXISTS clan_members(player TEXT PRIMARY KEY REFERENCES players(id),clan TEXT NOT NULL REFERENCES clans(id) ON DELETE CASCADE,role INTEGER NOT NULL CHECK(role BETWEEN 0 AND 3),joined INTEGER NOT NULL);
CREATE INDEX IF NOT EXISTS clan_roster ON clan_members(clan);
CREATE TABLE IF NOT EXISTS clan_invites(id TEXT PRIMARY KEY,clan TEXT NOT NULL REFERENCES clans(id) ON DELETE CASCADE,target TEXT NOT NULL REFERENCES players(id),inviter TEXT NOT NULL REFERENCES players(id),expires INTEGER NOT NULL,UNIQUE(clan,target));
CREATE TABLE IF NOT EXISTS clan_blocks(player TEXT NOT NULL REFERENCES players(id),clan TEXT NOT NULL REFERENCES clans(id) ON DELETE CASCADE,PRIMARY KEY(player,clan));
CREATE TABLE IF NOT EXISTS clan_messages(id TEXT PRIMARY KEY,clan TEXT NOT NULL REFERENCES clans(id) ON DELETE CASCADE,author TEXT NOT NULL REFERENCES players(id),name TEXT NOT NULL,body TEXT NOT NULL,created INTEGER NOT NULL);
CREATE TABLE IF NOT EXISTS clan_requests(player TEXT NOT NULL REFERENCES players(id),request TEXT NOT NULL,payload TEXT NOT NULL,expires INTEGER NOT NULL,PRIMARY KEY(player,request));
"#;
fn member(c: &mut Cursor<'_>, who: &str) -> Flow<Option<Value>> {
    c.one("SELECT m.clan,m.role,c.name FROM clan_members m JOIN clans c ON c.id=m.clan WHERE m.player=?",json!([who]))
}
fn cleanup(c: &mut Cursor<'_>, now: i64) -> Flow<()> {
    c.batch(vec![
        statement(
            "DELETE FROM clan_invites WHERE expires<=?",
            json!([now]),
            None,
        ),
        statement(
            "DELETE FROM clan_requests WHERE expires<=?",
            json!([now]),
            None,
        ),
        statement(
            "DELETE FROM clan_messages WHERE created<?",
            json!([now - 30 * 86400]),
            None,
        ),
    ])?;
    Ok(())
}
fn count(c: &mut Cursor<'_>, sql: &str, params: Value) -> Flow<i64> {
    let row = c
        .one(sql, params)?
        .ok_or(Error(503, "Clan count unavailable"))?;
    Ok(m::integer(&row[0], 0, i64::MAX)?)
}
fn membership_changing(c: &mut Cursor<'_>, who: &str) -> Flow<()> {
    if c.source {
        let rooms = c.provider("rooms", Value::Null)?;
        let result =
            c.core(json!({"op":"party_clan_membership_check","player":who,"rooms":rooms}))?;
        for pid in result
            .as_array()
            .ok_or(Error(503, "Native clan invalidation required"))?
        {
            c.changed(txt(pid)?, false)?;
        }
    }
    Ok(())
}
/// Original2393AE: established actor >=Staff and strictly above the target;
/// the requested role may equal the actor, but never exceed it.
fn can_change(actor: i64, target: i64, requested: i64) -> bool {
    (0..=3).contains(&actor)
        && (0..=3).contains(&target)
        && (0..=3).contains(&requested)
        && actor >= 2
        && actor > target
        && requested <= actor
}
/// Original1A334A/23955E: a last member may leave; a sole Overlord
/// with remaining members must first promote another Overlord.
fn can_leave(role: i64, members: i64, overlords: i64) -> bool {
    members == 1 || role != 3 || overlords > 1
}
fn command_body(body: &Value) -> Result<&str> {
    let action = m::text(&body["action"], 64)?;
    uuid(&body["request_id"])?;
    let additional: &[&str] = match action {
        "create" => &["name"],
        "invite" => &["username"],
        "accept" | "decline" | "block" | "cancel" | "remove" => &["target"],
        "role" => &["target", "role", "confirmed"],
        "leave" => &[],
        "broadcast" => &["message"],
        _ => return Err(Error(400, "Unknown clan action")),
    };
    let mut fields = vec!["request_id", "action"];
    fields.extend(additional);
    m::fields(body, &fields)?;
    Ok(action)
}
fn apply(c: &mut Cursor<'_>, who: &str, body: &Value, now: i64) -> Flow<()> {
    let action = command_body(body)?;
    let current = member(c, who)?;
    if action == "create" {
        need(current.is_none(), 409, "Leave your current clan first")?;
        let name = m::text(&body["name"], 15)?.trim();
        need(
            !name.is_empty()
                && name.bytes().all(|b| b.is_ascii_alphanumeric() || b == b' ')
                && name
                    .as_bytes()
                    .first()
                    .is_some_and(u8::is_ascii_alphanumeric)
                && name
                    .as_bytes()
                    .last()
                    .is_some_and(u8::is_ascii_alphanumeric),
            400,
            "Clan names use letters, numbers and spaces",
        )?;
        let folded = name.to_ascii_lowercase();
        need(
            c.one("SELECT 1 FROM clans WHERE folded_name=?", json!([folded]))?
                .is_none(),
            409,
            "Clan name already exists",
        )?;
        need(
            count(c, "SELECT count(*) FROM clans", json!([]))? < 1000,
            503,
            "Clan capacity reached",
        )?;
        let cid = c.random("clan", false)?;
        c.batch(vec![
            statement(
                "INSERT INTO clans VALUES(?,?,?)",
                json!([cid, name, folded]),
                Some(1),
            ),
            statement(
                "INSERT INTO clan_members VALUES(?,?,3,?)",
                json!([who, cid, now]),
                Some(1),
            ),
            statement(
                "DELETE FROM clan_invites WHERE target=?",
                json!([who]),
                None,
            ),
        ])?;
        return Ok(());
    }
    if ["accept", "decline", "block"].contains(&action) {
        let iid = uuid(&body["target"])?;
        let invite = c
            .one(
                "SELECT clan FROM clan_invites WHERE id=? AND target=? AND expires>?",
                json!([iid, who, now]),
            )?
            .ok_or(Error(404, "Clan invitation no longer available"))?;
        let cid = txt(&invite[0])?;
        if action == "accept" {
            need(current.is_none(), 409, "Leave your current clan first")?;
            need(
                count(
                    c,
                    "SELECT count(*) FROM clan_members WHERE clan=?",
                    json!([cid]),
                )? < 100,
                409,
                "Clan is full",
            )?;
            c.batch(vec![
                statement(
                    "INSERT INTO clan_members VALUES(?,?,1,?)",
                    json!([who, cid, now]),
                    Some(1),
                ),
                statement(
                    "DELETE FROM clan_invites WHERE target=?",
                    json!([who]),
                    None,
                ),
            ])?;
        } else {
            c.write("DELETE FROM clan_invites WHERE id=?", json!([iid]), Some(1))?;
            if action == "block" {
                c.write(
                    "INSERT OR IGNORE INTO clan_blocks VALUES(?,?)",
                    json!([who, cid]),
                    None,
                )?;
            }
        }
        return Ok(());
    }
    let current = current.ok_or(Error(409, "You are not in a clan"))?;
    let cid = txt(&current[0])?;
    let role = m::integer(&current[1], 0, 3)?;
    match action {
        "invite" => {
            need(role >= 1, 403, "Member role required to recruit")?;
            let username = m::text(&body["username"], 73)?;
            let target = if username.contains('#') {
                friends::resolve(c, &json!(username))?
            } else {
                let name = username.to_lowercase();
                need(
                    (2..=32).contains(&name.len())
                        && name.bytes().all(|b| {
                            b.is_ascii_lowercase() || b.is_ascii_digit() || b"_.".contains(&b)
                        }),
                    400,
                    "Enter a friend tag",
                )?;
                let row = c
                    .one(
                        "SELECT player FROM player_profiles WHERE discord_username=?",
                        json!([name]),
                    )?
                    .ok_or(Error(
                        404,
                        "Player must sign into ONLINE before being invited",
                    ))?;
                txt(&row[0])?.to_owned()
            };
            need(
                !c.blocked(who, &target)?,
                403,
                "Clan invitation unavailable",
            )?;
            need(
                member(c, &target)?.is_none(),
                409,
                "Player already belongs to a clan",
            )?;
            need(
                c.one(
                    "SELECT 1 FROM clan_blocks WHERE player=? AND clan=?",
                    json!([target, cid]),
                )?
                .is_none(),
                403,
                "Player is not accepting invitations",
            )?;
            need(
                count(
                    c,
                    "SELECT count(*) FROM clan_members WHERE clan=?",
                    json!([cid]),
                )? < 100,
                409,
                "Clan is full",
            )?;
            need(
                count(
                    c,
                    "SELECT count(*) FROM clan_invites WHERE clan=?",
                    json!([cid]),
                )? < 100,
                429,
                "Clan invitation limit reached",
            )?;
            need(
                count(
                    c,
                    "SELECT count(*) FROM clan_invites WHERE target=?",
                    json!([target]),
                )? < 100,
                429,
                "Player invitation limit reached",
            )?;
            let iid = c.random("invitation", false)?;
            c.write(
                "INSERT INTO clan_invites VALUES(?,?,?,?,?) ON CONFLICT(clan,target) DO NOTHING",
                json!([iid, cid, target, who, now + 7 * 86400]),
                None,
            )?;
        }
        "cancel" => {
            let iid = uuid(&body["target"])?;
            let row = c
                .one(
                    "SELECT inviter FROM clan_invites WHERE id=? AND clan=?",
                    json!([iid, cid]),
                )?
                .ok_or(Error(404, "Invitation no longer available"))?;
            need(
                row[0] == who || role >= 2,
                403,
                "Cannot cancel this invitation",
            )?;
            c.write("DELETE FROM clan_invites WHERE id=?", json!([iid]), Some(1))?;
        }
        "role" | "remove" => {
            let target = uuid(&body["target"])?;
            let row = c
                .one(
                    "SELECT role FROM clan_members WHERE clan=? AND player=?",
                    json!([cid, target]),
                )?
                .ok_or(Error(404, "Member no longer available"))?;
            let old = m::integer(&row[0], 0, 3)?;
            need(
                role >= 2 && role > old,
                403,
                "Cannot change a member of equal or higher role",
            )?;
            if action == "role" {
                let rank = m::integer(&body["role"], 0, 3)?;
                need(
                    can_change(role, old, rank),
                    403,
                    "Cannot grant a higher role than your own",
                )?;
                need(
                    body["confirmed"].is_boolean() && (rank != 3 || body["confirmed"] == true),
                    409,
                    "Confirm irreversible Overlord promotion",
                )?;
                c.write(
                    "UPDATE clan_members SET role=? WHERE player=? AND clan=? AND role=?",
                    json!([rank, target, cid, old]),
                    Some(1),
                )?;
            } else {
                membership_changing(c, target)?;
                c.write(
                    "DELETE FROM clan_members WHERE player=? AND clan=? AND role=?",
                    json!([target, cid, old]),
                    Some(1),
                )?;
            }
        }
        "leave" => {
            let members = count(
                c,
                "SELECT count(*) FROM clan_members WHERE clan=?",
                json!([cid]),
            )?;
            let overlords = count(
                c,
                "SELECT count(*) FROM clan_members WHERE clan=? AND role=3",
                json!([cid]),
            )?;
            need(
                can_leave(role, members, overlords),
                409,
                "Promote another member to Overlord before leaving",
            )?;
            membership_changing(c, who)?;
            if members == 1 {
                c.write("DELETE FROM clans WHERE id=?", json!([cid]), Some(1))?;
            } else {
                c.write(
                    "DELETE FROM clan_members WHERE player=? AND clan=?",
                    json!([who, cid]),
                    Some(1),
                )?;
            }
        }
        "broadcast" => {
            need(role >= 1, 403, "Member role required to broadcast")?;
            let message = m::text(&body["message"], 256)?;
            let name = c
                .one(
                    "SELECT screen_name FROM player_profiles WHERE player=?",
                    json!([who]),
                )?
                .ok_or(Error(404, "Player profile unavailable"))?;
            let mid = c.random("message", false)?;
            c.batch(vec![statement("INSERT INTO clan_messages VALUES(?,?,?,?,?,?)",json!([mid,cid,who,name[0],message,now]),Some(1)),
                statement("DELETE FROM clan_messages WHERE clan=? AND id NOT IN (SELECT id FROM clan_messages WHERE clan=? ORDER BY created DESC,rowid DESC LIMIT 100)",json!([cid,cid]),None)])?;
        }
        _ => return Err(Error(400, "Unknown clan action").into()),
    }
    Ok(())
}
fn view(c: &mut Cursor<'_>, who: &str, presence: bool) -> Flow<Value> {
    let member = member(c, who)?;
    let mut clan = Value::Null;
    if let Some(m) = member {
        let cid = txt(&m[0])?;
        let role = m::integer(&m[1], 0, 3)?;
        let roster=c.query("SELECT m.player,p.screen_name,m.role FROM clan_members m JOIN player_profiles p ON p.player=m.player WHERE m.clan=? ORDER BY m.role DESC,m.joined,m.player LIMIT 101",json!([cid]))?;
        need(roster.len() <= 100, 409, "Clan capacity exceeds bounds")?;
        let states = if presence {
            c.provider(
                "presence_states",
                json!(roster.iter().map(|r| r[0].clone()).collect::<Vec<_>>()),
            )?
        } else {
            Value::Null
        };
        if presence {
            need(
                states.as_array().is_some_and(|v| {
                    v.len() == roster.len()
                        && v.iter()
                            .all(|v| ["online", "playing", "offline"].iter().any(|s| v == *s))
                }),
                503,
                "Invalid clan presence observations",
            )?;
        }
        let mut members = Vec::new();
        for (i, r) in roster.iter().enumerate() {
            let mut member = json!({"id":r[0],"name":r[1],"role":r[2],"self":r[0]==who});
            if presence {
                member["presence"] = states[i].clone();
            }
            members.push(member);
        }
        let outgoing=c.query("SELECT i.id,p.screen_name FROM clan_invites i JOIN player_profiles p ON p.player=i.target WHERE i.clan=? AND (i.inviter=? OR ?>=2) ORDER BY i.expires,i.id LIMIT 101",json!([cid,who,role]))?;
        need(
            outgoing.len() <= 100,
            409,
            "Clan invitation capacity exceeds bounds",
        )?;
        let messages=c.query("SELECT id,name,body FROM clan_messages WHERE clan=? ORDER BY created DESC,rowid DESC LIMIT 20",json!([cid]))?;
        clan = json!({"id":cid,"name":m[2],"role":role,"members":members,
            "outgoing":outgoing.iter().map(|r|json!({"id":r[0],"name":r[1]})).collect::<Vec<_>>(),
            "messages":messages.iter().map(|r|json!({"id":r[0],"name":r[1],"body":r[2]})).collect::<Vec<_>>()});
    }
    let incoming=c.query("SELECT i.id,c.name FROM clan_invites i JOIN clans c ON c.id=i.clan WHERE i.target=? ORDER BY i.expires,i.id LIMIT 100",json!([who]))?;
    Ok(
        json!({"clan":clan,"invitations":incoming.iter().map(|r|json!({"id":r[0],"name":r[1]})).collect::<Vec<_>>()}),
    )
}
pub(super) fn operation(r: &Value, c: &mut Cursor<'_>) -> Flow<Value> {
    let now = m::integer(&r["wall"], 0, 9007199254650000)?;
    let who = r["player"].as_str().unwrap_or("");
    match txt(&r["mode"])? {
        "schema" => {
            c.batch(
                SCHEMA
                    .split(';')
                    .map(str::trim)
                    .filter(|s| !s.is_empty())
                    .map(|s| statement(s, json!([]), None))
                    .collect(),
            )?;
            Ok(Value::Null)
        }
        "cleanup" => {
            cleanup(c, now)?;
            Ok(Value::Null)
        }
        "member" => Ok(member(c, who)?.unwrap_or(Value::Null)),
        "membership_changing" => {
            membership_changing(c, who)?;
            Ok(Value::Null)
        }
        "view" => view(
            c,
            who,
            r["features"]["presence"] == true || r["features"]["presence"] == "1",
        ),
        "get" => {
            cleanup(c, now)?;
            view(
                c,
                who,
                r["features"]["presence"] == true || r["features"]["presence"] == "1",
            )
        }
        "command" => {
            let body = &r["body"];
            command_body(body)?;
            cleanup(c, now)?;
            let rid = uuid(&body["request_id"])?;
            let payload = ascii_json(body);
            if let Some(row) = c.one(
                "SELECT payload FROM clan_requests WHERE player=? AND request=?",
                json!([who, rid]),
            )? {
                need(
                    row[0] == payload,
                    409,
                    "Command ID reused with different fields",
                )?;
                return Ok(Value::Null);
            }
            need(
                count(
                    c,
                    "SELECT count(*) FROM clan_requests WHERE player=?",
                    json!([who]),
                )? < 256,
                429,
                "Clan command quota reached; retry tomorrow",
            )?;
            apply(c, who, body, now)?;
            c.write(
                "INSERT INTO clan_requests VALUES(?,?,?,?)",
                json!([who, rid, payload, now + 86400]),
                Some(1),
            )?;
            Ok(Value::Null)
        }
        _ => Err(Error(400, "Unknown clan directory operation").into()),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn original_client_roles_and_last_overlord_leave_contracts() {
        // Client source and authored roles, not an unchanged-XBE/server replay.
        for actor in 0..=3 {
            for target in 0..=3 {
                for requested in 0..=3 {
                    let admitted = match actor {
                        2 => target < 2 && requested <= 2,
                        3 => target < 3,
                        _ => false,
                    };
                    assert_eq!(can_change(actor, target, requested), admitted);
                }
            }
        }
        for role in 0..=3 {
            for members in [1, 2, 100] {
                for overlords in [1, 2] {
                    assert_eq!(
                        can_leave(role, members, overlords),
                        members == 1 || role != 3 || overlords > 1
                    );
                }
            }
        }
        assert!(!can_change(3, 3, 0));
        assert!(can_change(2, 1, 2));
        assert!(!can_change(2, 2, 0));
        assert!(!can_change(3, 1, 4));
    }
}
