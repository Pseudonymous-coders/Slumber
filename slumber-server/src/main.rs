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
    let x = sql::select_between(&real_uuid,&3,&9);
    for i in x {
        println!("{}",i.timestamp);
    };
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
/*
    server.get("/:uuid", middleware! { |req|


        let uuid = req.param("uuid").unwrap();
        let real_uuid = Uuid::parse_str(uuid).unwrap_or(Uuid::nil());
        let data: Vec<Data> = sql::select(&real_uuid);

        data.to_json()
    });      */

    server.get("/user_data", middleware! { |req, res|
        
        let foo  = &req.query();

        let uuid:Uuid = match foo.get("uuid") {
            Some(s) => match Uuid::parse_str(s) {
                Ok(u)  => u,
                Err(e) => Uuid::nil(),
            },
            None => Uuid::nil()
        };

        let start:i32 = match foo.get("start") {
            Some(s) => match s.parse::<i32>() {
                Ok(n)  => n,
                Err(e) => i32::min_value(),
            },
            None => i32::min_value()
        };

        let end:i32 = match foo.get("end")  {
            Some(s) => match s.parse::<i32>() {
                Ok(n)  => n,
                Err(e) => i32::max_value(),
            },
            None => i32::max_value()
        };

        if uuid.is_nil() {
            (StatusCode::BadRequest, "Uuid paramter was not valid".to_string())
        } else { 
            let data: Vec<Data> = sql::select_between(&uuid, &start, &end);
            (StatusCode::Ok, data.to_json().to_string())
        }
    });
    
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
