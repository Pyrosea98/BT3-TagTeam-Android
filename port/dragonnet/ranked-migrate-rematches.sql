-- Additive migration. Preserve players, credentials, ratings and past results.
ALTER TABLE ranked_matches ADD COLUMN rematch_caps INTEGER NOT NULL DEFAULT 0;
CREATE TABLE IF NOT EXISTS ranked_rounds (
 match_id TEXT NOT NULL, round_no INTEGER NOT NULL CHECK(round_no>=2 AND round_no<=256),
 state TEXT NOT NULL DEFAULT 'paired', made INTEGER NOT NULL,
 host_score INTEGER, guest_score INTEGER, host_digest TEXT, guest_digest TEXT,
 host_rating INTEGER NOT NULL, guest_rating INTEGER NOT NULL, delta INTEGER, settlement TEXT,
 PRIMARY KEY(match_id,round_no)
);
