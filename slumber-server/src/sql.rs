extern crate postgres;
extern crate uuid;

use self::postgres::{Connection, TlsMode};
use self::uuid::Uuid;

#[derive(RustcDecodable,RustcEncodable)]
pub struct Values {
    pub vbatt: f64,
    pub hr: i32,
    pub temp: i32,
    pub accel: i32,
}

#[derive(RustcDecodable,RustcEncodable)]
pub struct Data {
    pub uuid: Uuid,
    pub timestamp: i32,
    pub values: Values,
}

pub fn connect() -> Connection {
    Connection::connect("postgres://db:db@localhost/db", TlsMode::None).unwrap()
}

pub fn create_table() {
    let conn = connect();
    conn.execute("CREATE TABLE test2 (
                  uuid UUID,
                  tsmp INT,
                  vbatt DOUBLE PRECISION,
                  hr INT,
                  temp INT,
                  accel INT
                  )",&[]).unwrap();
}

pub fn insert(data: &Data) {
    let conn = connect();
    conn.execute("INSERT INTO test2 VALUES ($1, $2, $3, $4, $5, $6)",
                  &[&data.uuid, &data.timestamp, &data.values.vbatt, 
                  &data.values.hr, &data.values.temp, &data.values.accel
                  ]).unwrap();
}


pub fn select(uuid: &Uuid) -> Vec<Data> {
    let conn = connect();
    let mut list: Vec<Data> = Vec::new();
    for row in &conn.query("SELECT * FROM test2 WHERE uuid = $1", &[uuid]).unwrap() {
        let val = Values {
            vbatt: row.get(2),
            hr: row.get(3),
            temp: row.get(4),
            accel: row.get(5),
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

pub fn select_between(uuid: &Uuid, start: &i32, end: &i32) -> Vec<Data> {
    let conn = connect();
    let mut return_data: Vec<Data> = Vec::new();
    for row in &conn.query("SELECT * FROM test2 
                        WHERE uuid = $1
                        AND tsmp BETWEEN $2 AND $3",
                        &[uuid,start,end]).unwrap() {
        let val = Values {
            vbatt: row.get(2),
            hr: row.get(3),
            temp: row.get(4),
            accel: row.get(5),
        };
        let data = Data {
            uuid: row.get(0),
            timestamp: row.get(1),
            values: val,
        };
        return_data.push(data);
    }
    return_data
}

pub fn conn() {
    let conn = Connection::connect("postgres://pc:r@localhost/pc",
                                   TlsMode::None).unwrap();
    for row in &conn.query("SELECT * FROM test", &[]).unwrap() {
        let val = Values {
            vbatt: row.get(2),
            hr: row.get(3),
            temp: row.get(4),
            accel: row.get(5),
        };
        let data = Data {
            uuid: row.get(0),
            timestamp: row.get(1),
            values: val,
        };
        println!("Found {} at {}", data.uuid, data.timestamp);
    };
}
