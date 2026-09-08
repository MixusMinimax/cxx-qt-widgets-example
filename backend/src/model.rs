use diesel::internal::derives::multiconnection::chrono::NaiveDateTime;
use diesel::{Insertable, Queryable, Selectable};

#[cxx::bridge]
mod ffi {
    pub struct Measurement {
        pub id: [u8; 16],
        pub systolic: f64,
        pub diastolic: f64,
        pub map: f64,
        pub pulse: f64,
    }
}

#[derive(Queryable, Selectable, Insertable, Clone, Debug, Default)]
#[diesel(table_name = crate::schema::measurements)]
#[diesel(check_for_backend(diesel::sqlite::Sqlite))]
pub struct Measurement {
    #[diesel(serialize_as = my_uuid::Uuid, deserialize_as = my_uuid::Uuid)]
    pub id: uuid::Uuid,
    pub systolic: f64,
    pub diastolic: f64,
    pub map: f64,
    pub pulse: f64,
    pub date_time: NaiveDateTime,
}

mod my_uuid {
    use diesel::deserialize::FromSql;
    use diesel::{
        AsExpression, FromSqlRow,
        backend::Backend,
        deserialize,
        serialize::{self, Output, ToSql},
        sql_types::Binary,
    };

    #[derive(Clone, Copy, Hash, PartialEq, Eq, Debug, Default, FromSqlRow, AsExpression)]
    #[diesel(sql_type = Binary)]
    pub struct Uuid(pub uuid::Uuid);

    impl From<uuid::Uuid> for Uuid {
        fn from(value: uuid::Uuid) -> Self {
            Uuid(value)
        }
    }

    impl From<Uuid> for uuid::Uuid {
        fn from(Uuid(value): Uuid) -> Self {
            value
        }
    }

    impl<B: Backend> FromSql<Binary, B> for Uuid
    where
        *const [u8]: FromSql<Binary, B>,
    {
        fn from_sql(bytes: <B as Backend>::RawValue<'_>) -> deserialize::Result<Self> {
            let ptr = <*const [u8] as FromSql<_, _>>::from_sql(bytes)?;
            uuid::Uuid::from_slice(unsafe { &*ptr })
                .map(Uuid)
                .map_err(|e| e.into())
        }
    }

    impl<B: Backend> ToSql<Binary, B> for Uuid
    where
        [u8]: ToSql<Binary, B>,
    {
        fn to_sql<'b>(&'b self, out: &mut Output<'b, '_, B>) -> serialize::Result {
            ToSql::<Binary, B>::to_sql(self.0.as_bytes(), out)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use diesel::prelude::*;
    use dotenvy::dotenv;
    use std::env;

    fn establish_connection() -> SqliteConnection {
        dotenv().ok();

        let database_url = env::var("DATABASE_URL").expect("DATABASE_URL must be set");
        SqliteConnection::establish(&database_url)
            .unwrap_or_else(|_| panic!("Error connecting to {}", database_url))
    }

    #[test]
    fn test_query() {
        use crate::schema::measurements::dsl::*;

        dotenv().ok();

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
            .find(my_uuid::Uuid::from(
                uuid::Uuid::parse_str("c746b21f-2839-4160-a8eb-70b2c9a7b23c").unwrap(),
            ))
            .select(Measurement::as_select())
            .load(connection)
            .expect("Error loading measurements");

        println!("{:?}", results);
    }
}
