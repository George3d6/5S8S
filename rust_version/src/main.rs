extern crate tokio;

use tokio::prelude::*;
use tokio::io::{lines, write_all};
use tokio::net::TcpListener;

use std::collections::HashMap;
use std::string::String;
use std::io::BufReader;
use std::cell::UnsafeCell;
use std::sync::Arc;

pub struct Cache {
    cache: UnsafeCell<HashMap<String, String>>,
}

impl Cache {
    fn get_self(&self) -> &mut HashMap<String, String> {
        unsafe { return &mut *self.cache.get();}
    }
}

unsafe impl Sync for Cache {}

fn process_add(line: String, cache: &mut HashMap<String, String>) {
    let key_length_str: String = line.chars().skip(1).take(5).collect();
    let key_length = key_length_str.parse::<usize>().unwrap();
    let key: String = line.chars().skip(6).take(key_length).collect();
    let val: String = line.chars().skip(6 + key_length).take(line.len()).collect();
    cache.insert(key, val);
    //println!("{:?}", cache);
}

fn main() {
    let cache_global = Arc::new(  Cache{cache: UnsafeCell::new(HashMap::new())}  );

    let addr = "127.0.0.1:5282".parse().unwrap();
    let listener = TcpListener::bind(&addr).expect("unable to bind TCP listener");

    let server = listener.incoming()
    .map_err(|e| eprintln!("accept failed = {:?}", e))
    .for_each(move |sock| {
        let cache = cache_global.clone();
        let (reader, writer) = sock.split();

        let lines = lines(BufReader::new(reader));

        let responses = lines.map(move |line: String| {
            let action: String = line.chars().skip(0).take(1).collect();
            if (action == "a") {
                process_add(line, cache.get_self())
            }

        });

        let writes = responses.fold(writer, |_writer, _response| {
            write_all(_writer, String::from("You wrote a line").into_bytes()).map(|(w, _)| w)
        });

        let msg = writes.then(move |_| Ok(()));
        tokio::spawn(msg)
    });

    tokio::run(server);
}
