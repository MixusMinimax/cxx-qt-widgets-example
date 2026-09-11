// @generated automatically by Diesel CLI.

diesel::table! {
    measurements (id) {
        id -> Binary,
        systolic -> Double,
        diastolic -> Double,
        map -> Double,
        pulse -> Double,
        timestamp -> Double,
    }
}
