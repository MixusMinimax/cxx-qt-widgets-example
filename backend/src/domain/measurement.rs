// Copyright (C) 2026  Maxi Barmetler <maxi@barmetler.com>
//
// Use of this source code is governed by either the MIT or Apache-2.0 license, at your choice.
// A copy of each can be found in the corresponding LICENSE-* file, or online (respectively):
// https://opensource.org/licenses/MIT.
// http://www.apache.org/licenses/LICENSE-2.0

use crate::domain::measurement::io_impl::read_csv;
use diesel::connection::SimpleConnection;
use diesel::r2d2::{ConnectionManager, CustomizeConnection, Pool};
use diesel::{AsChangeset, Insertable, Queryable, Selectable, SqliteConnection};
use diesel_migrations::{EmbeddedMigrations, MigrationHarness, embed_migrations};
use sql_uuid::Uuid;
use std::borrow::Cow;
use std::convert::Infallible;
use std::io;
use std::num::{NonZeroU8, ParseFloatError};
use tokio::task::spawn_blocking;

#[derive(Queryable, Selectable, Insertable, Clone, PartialEq, Debug, Default)]
#[diesel(table_name = crate::schema::measurements)]
#[diesel(primary_key(id))]
#[diesel(check_for_backend(diesel::sqlite::Sqlite))]
#[repr(C)]
pub struct Measurement {
    pub id: Uuid,
    pub systolic: f64,
    pub diastolic: f64,
    pub map: f64,
    pub pulse: f64,
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
    Io(#[from] io::Error),
    #[error("export error: {0}")]
    Export(#[from] ExportError),
    #[error("import error: {0}")]
    Import(#[from] ImportError),
}

#[derive(Debug, thiserror::Error)]
pub enum ExportError {
    #[error("csv error: {0}")]
    Csv(#[from] csv::Error),
    #[error("io error: {0}")]
    Io(#[from] io::Error),
    #[error("invalid timestamp")]
    InvalidTimestamp(f64),
}

#[derive(Debug, thiserror::Error)]
pub enum ImportError {
    #[error("diesel error: {0}")]
    Diesel(#[from] diesel::result::Error),
    #[error("csv error: {0}")]
    Csv(#[from] csv::Error),
    #[error("io error: {0}")]
    Io(#[from] io::Error),
    #[error("unknown header: {0}")]
    UnknownHeader(String),
    #[error("invalid uuid: {0}")]
    InvalidUuid(#[from] sql_uuid::uuid::Error),
    #[error("invalid number: {0}")]
    InvalidNumber(#[from] ParseFloatError),
    #[error("invalid datetime: {0}")]
    InvalidDateTime(#[from] chrono::ParseError),
}

impl From<Infallible> for ExportError {
    fn from(_: Infallible) -> Self {
        unreachable!()
    }
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
                // write WAL changes back every 1000 pages, for an on average
                // 1MB WAL file. May affect readers if number is
                // increased
                conn.batch_execute("PRAGMA wal_autocheckpoint = 1000;")?;
                // free some space by truncating possibly massive WAL files from
                // the last run
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

#[derive(Clone, Eq, PartialEq, Debug, Default)]
pub struct ExportOptions {
    pub delimiter: Option<NonZeroU8>,
    pub date_time_format: DateTimeFormat,
    pub columns: Columns,
    pub export_id: bool,
    pub export_map: bool,
}

#[derive(Clone, Eq, PartialEq, Debug, Default)]
pub struct ImportOptions {
    pub delimiter: Option<NonZeroU8>,
    pub columns: Columns,
    pub date_time_format: DateTimeFormat,
    pub import_collision_strategy: ImportCollisionStrategy,
    pub ignore_unknown_headers: bool,
}

#[derive(Clone, Eq, PartialEq, Debug, Default)]
pub enum DateTimeFormat {
    #[default]
    Rfc3339,
    Rfc2822,
    Format(Cow<'static, str>),
}

#[derive(Copy, Clone, Eq, PartialEq, Debug, Default)]
pub enum ImportCollisionStrategy {
    #[default]
    Skip,
    Replace,
}

#[derive(Clone, Eq, PartialEq, Debug, Default)]
pub struct ImportResult<R> {
    reader: R,
    inserted_rows: usize,
}

#[derive(Clone, Eq, PartialEq, Debug, Default)]
pub struct Columns {
    pub id: Option<String>,
    pub systolic: Option<String>,
    pub diastolic: Option<String>,
    pub map: Option<String>,
    pub pulse: Option<String>,
    pub date_time: Option<String>,
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

    pub async fn export_measurements<W: io::Write + Send + 'static>(
        &self,
        wtr: W,
        opts: ExportOptions,
    ) -> MSResult<W> {
        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        let pool = self.pool.clone();
        spawn_blocking(move || {
            let m = pool.get().unwrap().transaction(|connection| {
                measurements.order_by(timestamp.asc()).load(connection)
            })?;
            Ok(io_impl::write_csv(m, wtr, opts)?)
        })
        .await
        .expect("join failed")
    }

    pub async fn import_measurements<R: io::Read + Send + 'static>(
        &self,
        rdr: R,
        opts @ ImportOptions {
            import_collision_strategy,
            ..
        }: ImportOptions,
    ) -> MSResult<ImportResult<R>> {
        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        let pool = self.pool.clone();
        spawn_blocking(move || {
            let (mut parsed, rdr) = read_csv(rdr, opts)?;
            parsed.iter_mut().for_each(|m| {
                if m.id.0.is_nil() {
                    m.id = Uuid::new_v4();
                }
                // we purposefully do not fix MAP on insert, so that the formula
                // can change
            });
            pool.get().unwrap().transaction(|connection| {
                if let ImportCollisionStrategy::Replace = import_collision_strategy {
                    let timestamps = parsed.iter().map(|m| m.timestamp).collect::<Vec<_>>();
                    diesel::delete(measurements.filter(timestamp.eq_any(timestamps)))
                        .execute(connection)?;
                }
                let count = diesel::insert_into(measurements)
                    .values(parsed)
                    .execute(connection)?;
                Ok(ImportResult {
                    inserted_rows: count,
                    reader: rdr,
                })
            })
        })
        .await
        .expect("join failed")
    }
}

mod io_impl {
    use super::{
        DateTimeFormat, ExportError, ExportOptions, ImportError, ImportOptions, Measurement,
    };
    use chrono::{DateTime, SecondsFormat};
    use csv::StringRecord;
    use std::io::{Read, Write};

    pub fn write_csv<I: IntoIterator<Item = Measurement>, W: Write>(
        measurements: I,
        wtr: W,
        ExportOptions {
            delimiter,
            columns,
            date_time_format,
            export_id,
            export_map,
        }: ExportOptions,
    ) -> Result<W, ExportError> {
        trait FormatDate {
            fn format_date(&self, ts: f64) -> Result<String, ExportError>;
        }

        impl FormatDate for DateTimeFormat {
            #[inline]
            fn format_date(&self, ts: f64) -> Result<String, ExportError> {
                let dt = DateTime::from_timestamp_secs(ts as i64)
                    .ok_or(ExportError::InvalidTimestamp(ts))?;
                Ok(match self {
                    DateTimeFormat::Rfc3339 => dt.to_rfc3339_opts(SecondsFormat::Secs, true),
                    DateTimeFormat::Rfc2822 => dt.to_rfc2822(),
                    DateTimeFormat::Format(f) => format!("{}", dt.format(f)),
                })
            }
        }

        let mut wtr = csv::WriterBuilder::new()
            .delimiter(delimiter.map(Into::into).unwrap_or(b','))
            .from_writer(wtr);

        // write header
        if export_id {
            wtr.write_field(columns.id.as_deref().unwrap_or(columns::ID))?;
        }
        wtr.write_field(columns.systolic.as_deref().unwrap_or(columns::SYSTOLIC))?;
        wtr.write_field(columns.diastolic.as_deref().unwrap_or(columns::DIASTOLIC))?;
        if export_map {
            wtr.write_field(columns.map.as_deref().unwrap_or(columns::MAP))?;
        }
        wtr.write_field(columns.pulse.as_deref().unwrap_or(columns::PULSE))?;
        wtr.write_field(columns.date_time.as_deref().unwrap_or(columns::DATE_TIME))?;
        wtr.write_record(None::<&[u8]>)?;

        let mut buf = Vec::<u8>::new();

        for Measurement {
            id,
            systolic,
            diastolic,
            map,
            pulse,
            timestamp,
        } in measurements
        {
            if export_id {
                buf.clear();
                write!(&mut buf, "{}", id.0.as_hyphenated())?;
                wtr.write_field(&buf)?;
            }
            buf.clear();
            write!(&mut buf, "{}", systolic)?;
            wtr.write_field(&buf)?;
            buf.clear();
            write!(&mut buf, "{}", diastolic)?;
            wtr.write_field(&buf)?;
            if export_map {
                buf.clear();
                write!(&mut buf, "{}", map)?;
                wtr.write_field(&buf)?;
            }
            buf.clear();
            write!(&mut buf, "{}", pulse)?;
            wtr.write_field(&buf)?;

            wtr.write_field(date_time_format.format_date(timestamp)?)?;

            wtr.write_record(None::<&[u8]>)?;
        }

        wtr.flush()?;

        Ok(wtr.into_inner().unwrap())
    }

    pub fn read_csv<R: Read>(
        rdr: R,
        ImportOptions {
            delimiter,
            columns,
            date_time_format,
            ignore_unknown_headers,
            ..
        }: ImportOptions,
    ) -> Result<(Vec<Measurement>, R), ImportError> {
        trait ParseDate {
            fn parse_date(&self, s: &str) -> Result<f64, ImportError>;
        }

        impl ParseDate for DateTimeFormat {
            #[inline]
            fn parse_date(&self, s: &str) -> Result<f64, ImportError> {
                let dt = match self {
                    DateTimeFormat::Rfc3339 => DateTime::parse_from_rfc3339(s)?,
                    DateTimeFormat::Rfc2822 => DateTime::parse_from_rfc2822(s)?,
                    DateTimeFormat::Format(f) => DateTime::parse_from_str(s, f)?,
                };
                Ok(dt.timestamp() as f64 + dt.timestamp_subsec_millis() as f64 / 1000.)
            }
        }

        let mut rdr = csv::ReaderBuilder::new()
            .delimiter(delimiter.map(Into::into).unwrap_or(b','))
            .has_headers(true)
            .from_reader(rdr);

        let id_col = columns.id.as_deref().unwrap_or(columns::ID);
        let systolic_col = columns.systolic.as_deref().unwrap_or(columns::SYSTOLIC);
        let diastolic_col = columns.diastolic.as_deref().unwrap_or(columns::DIASTOLIC);
        let map_col = columns.map.as_deref().unwrap_or(columns::MAP);
        let pulse_col = columns.pulse.as_deref().unwrap_or(columns::PULSE);
        let date_time_col = columns.date_time.as_deref().unwrap_or(columns::DATE_TIME);

        let headers = rdr.headers()?;
        let mut id_idx = None;
        let mut systolic_idx = None;
        let mut diastolic_idx = None;
        let mut map_idx = None;
        let mut pulse_idx = None;
        let mut date_time_idx = None;
        for (i, h) in headers.iter().enumerate() {
            match () {
                () if h.eq_ignore_ascii_case(id_col) => id_idx = Some(i),
                () if h.eq_ignore_ascii_case(systolic_col) => systolic_idx = Some(i),
                () if h.eq_ignore_ascii_case(diastolic_col) => diastolic_idx = Some(i),
                () if h.eq_ignore_ascii_case(map_col) => map_idx = Some(i),
                () if h.eq_ignore_ascii_case(pulse_col) => pulse_idx = Some(i),
                () if h.eq_ignore_ascii_case(date_time_col) => date_time_idx = Some(i),
                () if ignore_unknown_headers => {}
                () => return Err(ImportError::UnknownHeader(h.to_string())),
            }
        }

        let mut res = Vec::new();
        // reuse StringRecord, as .iter() would do new allocations every time
        let mut rec = StringRecord::with_capacity(0, headers.len());
        while rdr.read_record(&mut rec)? {
            let mut measurement = Measurement::default();
            if let Some(id_idx) = id_idx {
                measurement.id = rec[id_idx].parse()?;
            }
            if let Some(systolic_idx) = systolic_idx {
                measurement.systolic = rec[systolic_idx].parse()?;
            }
            if let Some(diastolic_idx) = diastolic_idx {
                measurement.diastolic = rec[diastolic_idx].parse()?;
            }
            if let Some(map_idx) = map_idx {
                measurement.map = rec[map_idx].parse()?;
            }
            if let Some(pulse_idx) = pulse_idx {
                measurement.pulse = rec[pulse_idx].parse()?;
            }
            if let Some(date_time_idx) = date_time_idx {
                measurement.timestamp = date_time_format.parse_date(&rec[date_time_idx])?;
            }
            res.push(measurement);
        }

        Ok((res, rdr.into_inner()))
    }

    mod columns {
        pub const ID: &str = "ID";
        pub const SYSTOLIC: &str = "Systolic";
        pub const DIASTOLIC: &str = "Diastolic";
        pub const MAP: &str = "Map";
        pub const PULSE: &str = "Pulse";
        pub const DATE_TIME: &str = "Time";
    }

    #[cfg(test)]
    mod tests {
        use super::*;
        use crate::domain::measurement::Columns;
        use std::num::NonZeroU8;

        #[test]
        fn test_write_csv() {
            let mut buf = Vec::<u8>::new();

            let it = write_csv(
                [Measurement {
                    systolic: 123.,
                    diastolic: 88.,
                    timestamp: (5 * 24 * 60 * 60) as f64,
                    ..Measurement::default()
                }],
                &mut buf,
                ExportOptions {
                    export_id: false,
                    export_map: false,
                    delimiter: NonZeroU8::new(b';'),
                    ..ExportOptions::default()
                },
            );

            assert!(it.is_ok());
            let it = it.unwrap();

            // This would be a place to use std::bstr::ByteStr::new() once it's
            // stable
            assert_eq!(
                unsafe { str::from_utf8_unchecked(it) },
                "Systolic;Diastolic;Pulse;Time\n\
                 123;88;0;1970-01-06T00:00:00Z\n",
            );
        }

        #[test]
        fn test_read_csv() {
            let s = b"SYSTOLIC,DATETIME,MAP,DIASTOLIC\n\
                               130,1970-01-05T00:00:00Z,90,85\n";

            let columns = Columns {
                systolic: Some("SYSTOLIC".to_string()),
                diastolic: Some("DIASTOLIC".to_string()),
                map: Some("MAP".to_string()),
                date_time: Some("DATETIME".to_string()),
                ..Columns::default()
            };

            let (measurements, _) = read_csv(
                &s[..],
                ImportOptions {
                    delimiter: NonZeroU8::new(b','),
                    columns,
                    ..ImportOptions::default()
                },
            )
            .expect("parsing should succeed");

            assert_eq!(
                measurements,
                [Measurement {
                    systolic: 130.,
                    diastolic: 85.,
                    map: 90.,
                    timestamp: (4 * 24 * 60 * 60) as f64,
                    ..Measurement::default()
                }],
            );
        }
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
        // in-memory database gets deleted if last connection is dropped, so we
        // keep one alive during the test.
        let (sut, conn) = setup();
        let v = sut.load_measurements().await.unwrap();
        assert!(v.contains(&M0));
        drop(conn);
    }
}
