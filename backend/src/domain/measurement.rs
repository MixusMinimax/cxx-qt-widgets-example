use diesel::connection::SimpleConnection;
use diesel::r2d2::{ConnectionManager, CustomizeConnection, Pool};
use diesel::{AsChangeset, Insertable, Queryable, Selectable, SqliteConnection};
use diesel_migrations::{EmbeddedMigrations, MigrationHarness, embed_migrations};
use serde::{Deserialize, Serialize};
use sql_uuid::Uuid;
use tokio::task::spawn_blocking;

#[derive(
    Queryable, Selectable, Insertable, Clone, PartialEq, Debug, Default, Serialize, Deserialize,
)]
#[diesel(table_name = crate::schema::measurements)]
#[diesel(primary_key(id))]
#[diesel(check_for_backend(diesel::sqlite::Sqlite))]
#[repr(C)]
pub struct Measurement {
    #[serde(serialize_with = "ser::ser_uuid", deserialize_with = "ser::de_uuid")]
    pub id: Uuid,
    pub systolic: f64,
    pub diastolic: f64,
    pub map: f64,
    pub pulse: f64,
    #[serde(serialize_with = "ser::ser_ts", deserialize_with = "ser::de_ts")]
    pub timestamp: f64,
}

#[derive(AsChangeset, Clone, Debug, Default)]
#[diesel(table_name = crate::schema::measurements)]
#[diesel(primary_key(id))]
#[diesel(check_for_backend(diesel::sqlite::Sqlite))]
pub struct MeasurementChangeset {
    pub id: Uuid,
    pub systolic: Option<f64>,
    pub diastolic: Option<f64>,
    pub map: Option<f64>,
    pub pulse: Option<f64>,
    pub timestamp: Option<f64>,
}

#[derive(Debug, thiserror::Error)]
pub enum MeasurementServiceError {
    #[error("diesel error: {0}")]
    Diesel(#[from] diesel::result::Error),
    #[error("csv error: {0}")]
    Csv(#[from] csv::Error),
    #[error("io error: {0}")]
    Io(#[from] std::io::Error),
}

#[derive(Debug)]
pub struct MeasurementService {
    pool: Pool<ConnectionManager<SqliteConnection>>,
}

impl MeasurementService {
    const MIGRATIONS: EmbeddedMigrations = embed_migrations!("migrations");

    pub fn new(connection_string: impl Into<String>) -> Self {
        #[derive(Debug)]
        struct MyCustomizer;
        impl CustomizeConnection<SqliteConnection, diesel::r2d2::Error> for MyCustomizer {
            fn on_acquire(&self, conn: &mut SqliteConnection) -> Result<(), diesel::r2d2::Error> {
                conn.batch_execute("PRAGMA busy_timeout = 2000;")?;
                // better write-concurrency
                conn.batch_execute("PRAGMA journal_mode = WAL;")?;
                // fsync only in critical moments
                conn.batch_execute("PRAGMA synchronous = NORMAL;")?;
                // write WAL changes back every 1000 pages, for an in average 1MB WAL file.
                // May affect readers if number is increased
                conn.batch_execute("PRAGMA wal_autocheckpoint = 1000;")?;
                // free some space by truncating possibly massive WAL files from the last run
                conn.batch_execute("PRAGMA wal_checkpoint(TRUNCATE);")?;
                Ok(())
            }
        }

        let manager = ConnectionManager::<SqliteConnection>::new(connection_string);
        let pool = Pool::builder()
            .test_on_check_out(true)
            .connection_customizer(Box::new(MyCustomizer))
            .build(manager)
            .unwrap();

        let mut conn = pool.get().unwrap();
        conn.run_pending_migrations(Self::MIGRATIONS).unwrap();

        Self { pool }
    }
}

fn fix_map(m: &mut Measurement) {
    if m.map == 0.0 {
        // Mean Arterial Pressure = 1/3*(SBP) + 2/3*(DBP)
        // DOI: 10.1097/CCM.0000000000000324
        m.map = 1.0 / 3.0 * m.systolic + 2.0 / 3.0 * m.diastolic;
    }
}

#[derive(Clone, PartialEq, Debug, Default)]
pub struct MeasurementUpdated {
    pub new: Measurement,
    pub old_ts: f64,
}

type MSResult<T> = Result<T, MeasurementServiceError>;

impl MeasurementService {
    pub async fn load_measurements(&self) -> MSResult<Vec<Measurement>> {
        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        let pool = self.pool.clone();
        let mut results = spawn_blocking(move || {
            pool.get().unwrap().transaction(|connection| {
                measurements
                    .select(Measurement::as_select())
                    .order_by(timestamp.asc()) // assuming formatted as iso
                    .load(connection)
            })
        })
        .await
        .expect("join failed")?;

        results.iter_mut().for_each(fix_map);

        Ok(results)
    }

    pub async fn save_measurement_new(
        &self,
        mut measurement: Measurement,
    ) -> MSResult<Measurement> {
        fix_map(&mut measurement);
        let measurement = Measurement {
            id: Uuid::new_v4(),
            ..measurement
        };

        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        let pool = self.pool.clone();
        spawn_blocking(move || {
            pool.get().unwrap().transaction(|connection| {
                diesel::insert_into(measurements)
                    .values(measurement)
                    .get_result(connection)
            })
        })
        .await
        .expect("join failed")
        .map_err(Into::into)
    }

    pub async fn update_measurement(
        &self,
        measurement_changeset: MeasurementChangeset,
    ) -> MSResult<MeasurementUpdated> {
        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        let pool = self.pool.clone();
        spawn_blocking(move || {
            pool.get()
                .unwrap()
                .transaction::<_, MeasurementServiceError, _>(|connection| {
                    let old_ts = measurements
                        .find(measurement_changeset.id)
                        .select(timestamp)
                        .get_result(connection)?;

                    let new = diesel::update(measurements.find(measurement_changeset.id))
                        .set(measurement_changeset)
                        .get_result(connection)?;

                    Ok(MeasurementUpdated { new, old_ts })
                })
        })
        .await
        .expect("join failed")
    }

    pub async fn delete_measurement(&self, measurement_id: Uuid) -> MSResult<Measurement> {
        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        let pool = self.pool.clone();
        spawn_blocking(move || {
            pool.get().unwrap().transaction(|connection| {
                diesel::delete(measurements.find(measurement_id)).get_result(connection)
            })
        })
        .await
        .expect("join failed")
        .map_err(Into::into)
    }

    pub async fn export_measurements<W: std::io::Write + Send + 'static>(
        &self,
        wtr: W,
    ) -> MSResult<W> {
        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        let pool = self.pool.clone();
        spawn_blocking(move || {
            pool.get().unwrap().transaction(|connection| {
                let mut wtr = csv::WriterBuilder::new()
                    .has_headers(true)
                    .delimiter(b';')
                    .from_writer(wtr);

                for m @ Measurement { .. } in measurements.load(connection)? {
                    wtr.serialize(m)?;
                }

                wtr.flush()?;

                Ok(wtr.into_inner().unwrap())
            })
        })
        .await
        .expect("join failed")
    }
}

mod ser {
    use chrono::{DateTime, SecondsFormat, Utc};
    use serde::de::{Unexpected, Visitor};
    use serde::{Deserializer, Serializer};
    use sql_uuid::Uuid;
    use std::fmt::Formatter;
    use std::str::FromStr;

    pub fn ser_uuid<S: Serializer>(uuid: &Uuid, s: S) -> Result<S::Ok, S::Error> {
        s.collect_str(uuid.0.as_hyphenated())
    }

    pub fn de_uuid<'de, D: Deserializer<'de>>(d: D) -> Result<Uuid, D::Error> {
        struct V;
        impl<'de2> Visitor<'de2> for V {
            type Value = Uuid;

            fn expecting(&self, f: &mut Formatter) -> std::fmt::Result {
                write!(f, "a uuid string")
            }

            fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
            where
                E: serde::de::Error,
            {
                Uuid::parse_str(v).map_err(|_| E::invalid_value(Unexpected::Str(v), &self))
            }
        }

        d.deserialize_str(V)
    }

    pub fn ser_ts<S: Serializer>(timestamp: &f64, s: S) -> Result<S::Ok, S::Error> {
        let x = DateTime::from_timestamp_millis(*timestamp as i64)
            .ok_or_else(|| <S::Error as serde::ser::Error>::custom("invalid timestamp"))?;
        s.collect_str(&x.to_rfc3339_opts(SecondsFormat::Secs, true))
    }

    pub fn de_ts<'de, D: Deserializer<'de>>(d: D) -> Result<f64, D::Error> {
        struct V;

        impl<'de2> Visitor<'de2> for V {
            type Value = f64;

            fn expecting(&self, f: &mut Formatter) -> std::fmt::Result {
                write!(f, "an rfc 3339 datetime")
            }

            fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
            where
                E: serde::de::Error,
            {
                DateTime::<Utc>::from_str(v)
                    .map(|dt| dt.timestamp_millis() as f64 / 1000.)
                    .map_err(|_| E::invalid_value(Unexpected::Str(v), &self))
            }
        }

        d.deserialize_str(V)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const M0: Measurement = Measurement {
        id: Uuid::from_bytes(*b"\x12\x34\x56\x78\x9a\xbc\xde\xf0\x12\x34\x56\x78\x9a\xbc\xde\xf0"),
        systolic: 123.,
        diastolic: 85.,
        map: 99.,
        pulse: 65.,
        timestamp: 1789653244.,
    };

    const CONN_STR: &str = "file:testdb?mode=memory&cache=shared";

    fn setup() -> (MeasurementService, SqliteConnection) {
        let mut conn = SqliteConnection::establish(CONN_STR).unwrap();

        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        let ms = MeasurementService::new(CONN_STR);

        diesel::insert_into(measurements)
            .values(M0)
            .execute(&mut conn)
            .unwrap();

        (ms, conn)
    }

    #[tokio::test]
    async fn test_list() {
        // in-memory database gets deleted if last connection is dropped, so we keep one alive during the test.
        let (sut, conn) = setup();
        let v = sut.load_measurements().await.unwrap();
        assert!(v.contains(&M0));
        drop(conn);
    }
}
