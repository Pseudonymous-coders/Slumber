extern crate uuid;
extern crate bcrypt;

use self::bcrypt::{DEFAULT_COST, hash, verify};
use self::uuid::Uuid;
use sql;

use sql::{User, UserLogin};

pub fn register(uuid: Uuid, pass: String) -> bool {
    let hashed = hash(&pass, DEFAULT_COST).unwrap();
    
    println!("{:?}", hashed);

    sql::insert_register_user(uuid, hashed);
    
    true
}

pub fn login(uuid: Uuid, pass: String) -> bool {
    let db_hashed = sql::select_user_login(uuid); 

    let verified = verify(&pass, &db_hashed).unwrap();
    
    verified
}
