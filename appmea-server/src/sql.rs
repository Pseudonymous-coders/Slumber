extern crate postgres;
extern crate uuid;

use self::postgres::{Connection, TlsMode};
use self::uuid::Uuid;

#[derive(RustcDecodable,RustcEncodable)]
pub struct Values {
    pub vbatt: f64,
    pub hr: i32,
    pub temp: i32,
    pub x: i32,
    pub y: i32,
    pub z: i32,
}

#[derive(RustcDecodable,RustcEncodable)]
pub struct Data {
    pub uuid: Uuid,
    pub timestamp: i32,
    pub values: Values,
}

pub fn select(uuid: &Uuid) {
    let conn = Connection::connect("postgres://pc:r@localhost/pc", TlsMode::None).unwrap(); 
    for row in &conn.query("SELECT * FROM test WHERE id = $1", &[uuid]).unwrap() {
        let val = Values {
            vbatt: row.get(2),
            hr: row.get(3),
            temp: row.get(4),
            x: row.get(5),
            y: row.get(6),
            z: row.get(7),
        };
        let data = Data {
            uuid: row.get(0),
            timestamp: row.get(1),
            values: val,
        };
    }
}

pub fn conn() {
    let conn = Connection::connect("postgres://pc:r@localhost/pc", TlsMode::None).unwrap();
    for row in &conn.query("SELECT * FROM test", &[]).unwrap() {
        let val = Values {
            vbatt: row.get(2),
            hr: row.get(3),
            temp: row.get(4),
            x: row.get(5),
            y: row.get(6),
            z: row.get(7),
        };
        let data = Data {
            uuid: row.get(0),
            timestamp: row.get(1),
            values: val,
        };
        println!("Found {} at {}", data.uuid, data.timestamp);
    };
}
