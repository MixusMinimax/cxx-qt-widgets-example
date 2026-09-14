use diesel::{AsChangeset, Connection, Insertable, Queryable, Selectable, SqliteConnection};
use dotenvy::dotenv;
use sql_uuid::Uuid;
use std::{env, mem};
use tokio::task::spawn_blocking;

#[derive(Queryable, Selectable, Insertable, Clone, Debug, Default)]
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

#[derive(Debug, PartialEq, thiserror::Error)]
pub enum MeasurementServiceError {
    #[error("diesel error")]
    Diesel(#[from] diesel::result::Error),
}

#[derive(Debug, Default)]
pub struct MeasurementService {}

fn fix_map(m: &mut Measurement) {
    if m.map == 0.0 {
        // Mean Arterial Pressure = 1/3*(SBP) + 2/3*(DBP)
        // DOI: 10.1097/CCM.0000000000000324
        m.map = 1.0 / 3.0 * m.systolic + 2.0 / 3.0 * m.diastolic;
    }
}

impl MeasurementService {
    pub async fn load_measurements(&self) -> Result<Vec<Measurement>, MeasurementServiceError> {
        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        let mut results = spawn_blocking(|| {
            establish_connection().transaction(|connection| {
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
    ) -> Result<Measurement, MeasurementServiceError> {
        fix_map(&mut measurement);
        let measurement = Measurement {
            id: Uuid::new_v4(),
            ..measurement
        };

        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        spawn_blocking(|| {
            establish_connection().transaction(|connection| {
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
    ) -> Result<Measurement, MeasurementServiceError> {
        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        spawn_blocking(|| {
            establish_connection().transaction(|connection| {
                diesel::update(measurements)
                    .set(measurement_changeset)
                    .get_result(connection)
            })
        })
        .await
        .expect("join failed")
        .map_err(Into::into)
    }
}

fn establish_connection() -> SqliteConnection {
    dotenv().ok();

    let database_url = env::var("DATABASE_URL").expect("DATABASE_URL must be set");
    SqliteConnection::establish(&database_url)
        .unwrap_or_else(|_| panic!("Error connecting to {}", database_url))
}

#[cfg(test)]
mod tests {
    use super::*;
    use diesel::debug_query;
    use diesel::internal::derives::multiconnection::chrono::{NaiveDate, NaiveDateTime, NaiveTime};
    use diesel::prelude::*;
    use diesel::sqlite::Sqlite;

    #[test]
    fn test_query() {
        use crate::schema::measurements::dsl::*;

        let connection = &mut establish_connection();

        println!(
            "{}",
            debug_query::<Sqlite, _>(
                &diesel::insert_into(crate::schema::measurements::table).values(Measurement {
                    id: Uuid::new_v4(),
                    timestamp: NaiveDateTime::new(
                        NaiveDate::from_ymd_opt(2026, 9, 9).unwrap(),
                        NaiveTime::from_hms_opt(18, 45, 0).unwrap(),
                    )
                    .and_utc()
                    .timestamp() as f64
                        / 1000.0f64,
                    ..std::default::Default::default()
                }),
            )
        );

        let results = measurements
            .find(Uuid::parse_str("c746b21f-2839-4160-a8eb-70b2c9a7b23c").unwrap())
            .select(Measurement::as_select())
            .load(connection)
            .expect("Error loading measurements");

        println!("{:?}", results);
    }
}
