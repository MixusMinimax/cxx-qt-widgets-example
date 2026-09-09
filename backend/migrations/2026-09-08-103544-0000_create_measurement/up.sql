CREATE TABLE measurements
(
    id        BLOB     NOT NULL PRIMARY KEY,
    systolic  DOUBLE   NOT NULL,
    diastolic DOUBLE   NOT NULL,
    map       DOUBLE   NOT NULL DEFAULT 0,
    pulse     DOUBLE   NOT NULL,
    date_time DATETIME not null
) WITHOUT ROWID
