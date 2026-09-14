use diesel::deserialize::FromSql;
use diesel::{
    AsExpression, FromSqlRow,
    backend::Backend,
    deserialize,
    serialize::{self, Output, ToSql},
    sql_types::Binary,
};
use std::fmt;
use uuid::Bytes;

#[derive(Clone, Copy, Hash, PartialEq, Eq, Debug, Default, FromSqlRow, AsExpression)]
#[diesel(sql_type = Binary)]
pub struct Uuid(pub uuid::Uuid);

impl Uuid {
    #[cfg(feature = "v4")]
    #[inline]
    pub fn new_v4() -> Self {
        Self(uuid::Uuid::new_v4())
    }

    #[inline]
    pub fn parse_str(s: &str) -> Result<Self, uuid::Error> {
        Ok(Self(uuid::Uuid::parse_str(s)?))
    }

    #[inline]
    pub const fn into_bytes(self) -> Bytes {
        self.0.into_bytes()
    }

    #[inline]
    pub const fn from_bytes(bytes: Bytes) -> Self {
        Uuid(uuid::Uuid::from_bytes(bytes))
    }
}

impl fmt::Display for Uuid {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Display::fmt(&self.0, f)
    }
}

impl fmt::LowerHex for Uuid {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for b in self.0.as_bytes() {
            write!(f, "{:02x}", b)?;
        }
        Ok(())
    }
}

impl fmt::UpperHex for Uuid {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for b in self.0.as_bytes() {
            write!(f, "{:02X}", b)?;
        }
        Ok(())
    }
}

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

#[cfg(test)]
#[cfg(feature = "v4")]
mod tests {
    use super::*;

    #[test]
    fn print_id_blob() {
        let id = Uuid::new_v4();
        println!("{:X}", id);
    }
}
