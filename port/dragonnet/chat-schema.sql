-- Optional community chat. Independent from rooms, presence and Ranked scores.
CREATE TABLE IF NOT EXISTS community_chat (
 seq INTEGER PRIMARY KEY AUTOINCREMENT,
 author TEXT NOT NULL,
 nonce TEXT NOT NULL,
 name TEXT NOT NULL,
 text TEXT NOT NULL,
 made INTEGER NOT NULL,
 UNIQUE(author,nonce)
);
CREATE INDEX IF NOT EXISTS chat_made ON community_chat(made);
