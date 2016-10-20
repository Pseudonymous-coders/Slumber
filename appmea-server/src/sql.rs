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

pub fn insert(data: &Data) {
    let conn = Connection::connect("postgres://pc:r@localhost/pc", 
                                   TlsMode::None).unwrap();
    conn.execute("INSERT INTO test VALUES ($1, $2, $3, $4, $5, $6, $7, $8)",
                  &[&data.uuid, &data.timestamp, &data.values.vbatt, 
                  &data.values.hr, &data.values.temp, &data.values.x, 
                  &data.values.y, &data.values.z]).unwrap();
}


pub fn select(uuid: &Uuid) -> Vec<Data> {
    let conn = Connection::connect("postgres://pc:r@localhost/pc",
                                   TlsMode::None).unwrap(); 
    let mut list: Vec<Data> = Vec::new();
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
        list.push(data);
    }
    list
}

pub fn conn() {
    let conn = Connection::connect("postgres://pc:r@localhost/pc",
                                   TlsMode::None).unwrap();
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
