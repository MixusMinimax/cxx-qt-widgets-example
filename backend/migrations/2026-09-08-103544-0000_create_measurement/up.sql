-- This seems to be on by default now in my case, but doesn't hurt to be specific:
PRAGMA journal_mode = WAL;

CREATE TABLE measurements
(
    id        BLOB   NOT NULL PRIMARY KEY,
    systolic  DOUBLE NOT NULL,
    diastolic DOUBLE NOT NULL,
    map       DOUBLE NOT NULL DEFAULT 0,
    pulse     DOUBLE NOT NULL,
    timestamp DOUBLE NOT NULL UNIQUE
) WITHOUT ROWID;
