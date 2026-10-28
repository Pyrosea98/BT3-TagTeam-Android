-- Anonymous leases only. Never join these identifiers to Ranked profiles.
CREATE TABLE IF NOT EXISTS online_presence (
 id TEXT PRIMARY KEY,
 network TEXT NOT NULL,
 seen INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS presence_seen ON online_presence(seen);
CREATE INDEX IF NOT EXISTS presence_network ON online_presence(network,seen);
