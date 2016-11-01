#[macro_use] extern crate nickel;
extern crate rustc_serialize;
extern crate uuid;

mod sql;

use std::collections::BTreeMap;
use nickel::{Nickel, JsonBody, HttpRouter, MediaType, FormBody, QueryString};
use nickel::status::StatusCode;
use rustc_serialize::json::{Json, ToJson};
use rustc_serialize::json;
use uuid::Uuid;
use sql::{Values,Data};

impl ToJson for Values {
    fn to_json(&self) -> Json {
        let mut map = BTreeMap::new();
        map.insert("vbatt".to_string(), self.vbatt.to_json());
        map.insert("hr".to_string(), self.hr.to_json());
        map.insert("temp".to_string(), self.temp.to_json());
        map.insert("accel".to_string(), self.accel.to_json());
        return Json::Object(map);
    }
}

impl ToJson for Data {
    fn to_json(&self) -> Json {
        let mut map = BTreeMap::new();
        // Future Oliver, I hope you can figure out how to get to_Json to work
        // for Uuid to work. Cause I can't.
        //
        // <3
        map.insert("uuid".to_string(), self.uuid.to_string().to_json());
        map.insert("timestamp".to_string(), self.timestamp.to_json());
        map.insert("values".to_string(), self.values.to_json());
        return Json::Object(map);
    }
}

fn main2() {
    let uuid = "43a59d21-6bb5-4fe4-bdb1-81963d7a24a8";
    let real_uuid = Uuid::parse_str(uuid).unwrap();
    println!("{}",real_uuid.to_string());
    sql::select(&real_uuid);
    println!("{}",32.to_json());

}

fn main() {
    let mut server = Nickel::new();
    server.post("/", middleware! { |request, response|
            let data = try_with!(response, {
                request.json_as::<Data>().map_err(|e| (StatusCode::BadRequest, e))
            });
            sql::insert(&data);
            format!("Hello {} at {}", data.uuid, data.timestamp)
    });

    server.get("/", middleware! { |req|
        666.to_json()
    });
    
// This works!
    server.get("/:uuid/", middleware! { |req|

        let uuid = req.param("uuid").unwrap();
        let real_uuid = Uuid::parse_str(uuid).unwrap();
        let data: Vec<Data> = sql::select(&real_uuid);

        data.to_json()
    });      

    server.get("/:uuid/:timestamp", middleware! { |req|

        let uuid = req.param("uuid").unwrap();
        let timestamp = req.param("timestamp").unwrap();
       // "000000000000000000000000000000000000"
        let real_uuid = Uuid::parse_str(uuid).unwrap_or(Uuid::nil());
        let real_tsmp = timestamp.parse::<i32>().unwrap_or(0);

        let data: Vec<Data> = sql::select(&real_uuid);
        let mut return_data: Vec<Data> = Vec::new();

        for item in data {
            if item.timestamp == real_tsmp {
                return_data.push(item);
            }
        }
        return_data.to_json()
    });
    
    server.get("/:uuid", middleware! { |request| 
            format!("No foo vals")
    });

    println!("uhhhh????");
    server.listen("0.0.0.0:6767").unwrap();
}
/*
{
    uuid: uuid,
    timestamp: timestamp,
    values: {
        vbatt: float
        hr: int,
        temp: int,
        x: int,
        y: int,
        z: int
    }
}
*/
