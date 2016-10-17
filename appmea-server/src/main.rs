#[macro_use] extern crate nickel;
extern crate rustc_serialize;

use std::collections::BTreeMap;
use nickel::{Nickel, JsonBody, HttpRouter, MediaType};
use nickel::status::StatusCode;
use rustc_serialize::json::{Json, ToJson};

#[derive(RustcDecodable,RustcEncodable)]
struct Values {
    vbatt: f64,
    hr: usize,
    temp: usize,
    x: usize,
    y: usize,
    z: usize,
}

#[derive(RustcDecodable,RustcEncodable)]
struct Data {
    uuid: usize,
    timestamp: usize,
    values: Values,
}

impl ToJson for Values {
    fn to_json(&self) -> Json {
        let mut map = BTreeMap::new();
        map.insert("vbatt".to_string(), self.vbatt.to_json());
        map.insert("hr".to_string(), self.hr.to_json());
        map.insert("temp".to_string(), self.temp.to_json());
        map.insert("x".to_string(), self.x.to_json());
        map.insert("y".to_string(), self.y.to_json());
        map.insert("z".to_string(), self.z.to_json());
        return Json::Object(map);
    }
}

impl ToJson for Data {
    fn to_json(&self) -> Json {
        let mut map = BTreeMap::new();
        map.insert("uuid".to_string(), self.uuid.to_json());
        map.insert("timestamp".to_string(), self.timestamp.to_json());
        map.insert("values".to_string(), self.values.to_json());
        return Json::Object(map);
    }
}

fn main() {
    let mut server = Nickel::new();
    
    server.post("/", middleware! { |request, response|
            let data = try_with!(response, {
                request.json_as::<Data>().map_err(|e| (StatusCode::BadRequest, e))
            });
            format!("Hello {} at {}", data.uuid, data.timestamp)
    });

    server.get("/:uuid/:timestamp", middleware! { |req|

        let uuid = req.param("uuid").unwrap();
        let timestamp = req.param("timestamp").unwrap();

        let vals = Values {
            vbatt: 0.3f64,
            hr: 1,
            temp: 1,
            x: 1,
            y: 1,
            z: 1,
        };

        let data = Data {
            uuid: uuid.parse::<usize>().unwrap(),
            timestamp: timestamp.parse::<usize>().unwrap(),
            values: vals,
        };
        data.to_json();
    });

    server.listen("127.0.0.1:6767");
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
