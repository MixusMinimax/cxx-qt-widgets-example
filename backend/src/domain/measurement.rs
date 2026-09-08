use crate::schema::measurements::dsl::measurements;
use diesel::internal::derives::multiconnection::chrono::NaiveDateTime;
use diesel::{Connection, Insertable, Queryable, Selectable, SqliteConnection};
use dotenvy::dotenv;
use sql_uuid::Uuid;
use std::env;
use tokio::task::spawn_blocking;
use tokio_util::future::FutureExt;
use tokio_util::sync::CancellationToken;

#[derive(Queryable, Selectable, Insertable, Clone, Debug, Default)]
#[diesel(table_name = crate::schema::measurements)]
#[diesel(check_for_backend(diesel::sqlite::Sqlite))]
pub struct Measurement {
    pub id: Uuid,
    pub systolic: f64,
    pub diastolic: f64,
    pub map: f64,
    pub pulse: f64,
    pub date_time: NaiveDateTime,
}

#[derive(Debug, PartialEq, thiserror::Error)]
pub enum MeasurementServiceError {
    #[error("diesel error")]
    Diesel(#[from] diesel::result::Error),
    #[error("canceled")]
    Canceled,
}

#[derive(Debug, Default)]
pub struct MeasurementService {}

impl MeasurementService {
    pub async fn load_measurements(
        &self,
        ct: CancellationToken,
    ) -> Result<Vec<Measurement>, MeasurementServiceError> {
        use crate::schema::measurements::dsl::*;
        use diesel::prelude::*;

        let results = spawn_blocking(|| {
            let connection = &mut establish_connection();
            connection.transaction(|connection| {
                measurements
                    .select(Measurement::as_select())
                    .load(connection)
            })
        })
        .with_cancellation_token(&ct)
        .await
        .ok_or_else(|| MeasurementServiceError::Canceled)?
        .expect("join failed")?;

        Ok(results)
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
    use diesel::prelude::*;
    use dotenvy::dotenv;
    use std::env;

    #[test]
    fn test_query() {
        use crate::schema::measurements::dsl::*;

        let connection = &mut establish_connection();

        // diesel::insert_into(crate::schema::measurements::table)
        //     .values(Measurement {
        //         id: uuid::Uuid::new_v4(),
        //         ..std::default::Default::default()
        //     })
        //     .returning(Measurement::as_returning())
        //     .get_result(connection)
        //     .expect("Error saving measurement");

        let results = measurements
            .find(Uuid::parse_str("c746b21f-2839-4160-a8eb-70b2c9a7b23c").unwrap())
            .select(Measurement::as_select())
            .load(connection)
            .expect("Error loading measurements");

        println!("{:?}", results);
    }
}
