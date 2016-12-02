extern crate postgres;
extern crate uuid;
extern crate std;

use self::postgres::{Connection, TlsMode};
use self::postgres::rows::Row;
use self::uuid::Uuid;
use self::std::collections::HashMap;

#[derive(RustcDecodable,RustcEncodable)]
pub struct Values {
    pub vbatt: f64,
    pub temp: i32,
    pub accel: i32,
}

#[derive(RustcDecodable,RustcEncodable)]
pub struct Data {
    pub uuid: Uuid,
    pub timestamp: i32,
    pub values: Values,
}

#[derive(RustcDecodable,RustcEncodable)]
pub struct User {
    uuid:       Uuid,
    first_name: String,
    last_name:  String,
}

#[derive(RustcDecodable,RustcEncodable)]
pub struct UserLogin {
    pub uuid:     Uuid,
    pub password: String,
}

pub fn connect() -> Connection {
    Connection::connect("postgres://db:db@localhost/db", TlsMode::None).unwrap()
}

pub fn create_table() {
    let conn = connect();
    conn.execute("CREATE TABLE test3 (
                  uuid UUID,
                  tsmp INT,
                  vbatt DOUBLE PRECISION,
                  temp INT,
                  accel INT
                  )",&[]).unwrap();
}

pub fn insert(data: &Data) {
    let conn = connect();
    conn.execute("INSERT INTO test3 VALUES ($1, $2, $3, $4, $5)",
                  &[&data.uuid, &data.timestamp, &data.values.vbatt, 
                    &data.values.temp, &data.values.accel
                  ]).unwrap();
}



fn data_from_row(row: Row) -> Data {
    let val = Values {
        vbatt: row.get(2),
        temp: row.get(3),
        accel: row.get(4),
    };
    let data = Data {
        uuid: row.get(0),
        timestamp: row.get(1),
        values: val,
    };
    data
}

pub fn update(uuid: &Uuid, tsmp: &i32, data: &HashMap<String,String>) -> bool {
    let conn = connect();
    let mut ret = false;

    ret = match &conn.query("SELECT 1 FROM test3 WHERE uuid = $1 AND tsmp = $2;",
                       &[uuid, tsmp]).unwrap().len() {
        &1usize => { &conn.execute("INSERT INTO test3 (uuid, tsmp)
                                    VALUES ($1, $2)", &[uuid, tsmp]).unwrap();
                     true  },
        &0usize => { false },
        _       => { false },
    };/*
            &conn.execute("INSERT INTO test3 (uuid, tsmp)
                           VALUES ($1, $2)", &[uuid, tsmp]).unwrap();
        */           
    
    let valid_keys = ["uuid",   "tsmp", "accelx",   "accely", 
                      "accelz", "temp", "humidity", "vbatt"];
    for &key in valid_keys.iter() {

        println!("hit!");
        match data.get(key) {
            Some(value) => {&conn.execute("UPDATE test3 SET $1 = $2 
                                           WHERE uuid = $3 AND tsmp = $4;",
                                           &[&key, &value, uuid, tsmp]);
                            println!("{}, {}", key, value);},
            None => {},
        }
    }
    ret = true;
    return ret;
}

pub fn select(uuid: &Uuid) -> Vec<Data> {
    let conn = connect();
    let mut list: Vec<Data> = Vec::new();
    for row in &conn.query("SELECT * FROM test3 WHERE uuid = $1", &[uuid]).unwrap() {
        let data = data_from_row(row);
        list.push(data);
    }
    list
}

pub fn select_between(uuid: &Uuid, start: &i32, end: &i32) -> Vec<Data> {
    let conn = connect();
    let mut return_data: Vec<Data> = Vec::new();
    for row in &conn.query("SELECT * FROM test3 
                        WHERE uuid = $1
                        AND tsmp BETWEEN $2 AND $3",
                        &[uuid,start,end]).unwrap() {
        let data = data_from_row(row);
        return_data.push(data);
    }
    return_data
}

pub fn select_all_uuid() -> Vec<String> {
    let conn = connect();
    let mut return_data: Vec<String> = Vec::new();
    for row in &conn.query("SELECT DISTINCT uuid FROM test3", &[]).unwrap() {
        let id: Uuid = row.get(0);
        return_data.push(id.to_string());
    }
    return_data
}

pub fn select_user_login(uuid: Uuid) -> String {
    let conn = connect();
    let mut return_data: String = String::new();
    let string:String =  conn.query("SELECT pswd FROM test_pass
                            WHERE uuid = $1",
                            &[&uuid]).unwrap().get(0).get(0);
    string
}

pub fn delete_between(uuid: &Uuid, start: &i32, end: &i32) -> u64 {
    let conn = connect();
    let mut return_data: Vec<Data> = Vec::new();
    let result: u64 = conn.execute("DELETE FROM test3    
                                     WHERE uuid = $1
                                     AND tsmp BETWEEN $2 AND $3",
                                     &[uuid,start,end]).unwrap();
    return result;
}
pub fn insert_register_user(uuid: Uuid, pass:String) {
    let conn = connect();
    let mut return_bool:bool = false;
    conn.execute("INSERT INTO test_pass VALUES ($1, $2)",
                 &[&uuid, &pass]).unwrap();
}
