extern crate tokio;
extern crate tokio_current_thread;

use tokio::prelude::*;
use tokio::io::{write_all, read_until, read_exact};
use tokio::net::{TcpListener, TcpStream};

use std::collections::HashMap;
use std::str;
use std::vec::Vec;
use std::string::String;
use std::io::BufReader;
use std::cell::UnsafeCell;
use std::rc::Rc;

pub struct State {
    string_cache: UnsafeCell<HashMap<Vec<u8>, Vec<u8>>>,
    peers: UnsafeCell<Vec<String>>,
}

impl State {
    fn get_peers(&self) -> &mut Vec<String> {
        unsafe { return &mut *self.peers.get();}
    }
    fn get_string_cache(&self) -> &mut HashMap<Vec<u8>, Vec<u8>> {
        unsafe { return &mut *self.string_cache.get();}
    }
}


fn process_add(line: &String, cache: &mut HashMap<String, String>) {
    let key_length_str: String = line.chars().skip(1).take(5).collect();
    let key_length = key_length_str.parse::<usize>().unwrap();
    let key: String = line.chars().skip(6).take(key_length).collect();
    let val: String = line.chars().skip(6 + key_length).take(line.len()).collect();
    cache.insert(key, val);
}

fn process_get(line: &String, cache: &mut HashMap<String, String>) -> String {
    let key: String = line.chars().skip(1).take(line.len()).collect();
    match cache.get(&key) {
        Some(val) => return format!("s{}", val),
        None => return String::from("f")
    }
}

fn main() {
    let state_global = Rc::new( State{string_cache: UnsafeCell::new(HashMap::new()),
        peers: UnsafeCell::new(Vec::new())} );

    let addr = "127.0.0.1:5282".parse().unwrap();
    let listener = TcpListener::bind(&addr).expect("unable to bind TCP listener");

    let server = listener.incoming()
    .map_err(|e| eprintln!("Accept connection error: {:?}", e))
    .for_each(move |sock| {
        let state = state_global.clone();

        let (reader, writer) = sock.split();

        // let metadata: Vec<u8> = Vec::new();
        let metadata_future = read_until(BufReader::new(reader), '#' as u8, Vec::new());
        let processing = metadata_future.map(move |(_reader, metadata)| {
            let action = metadata[0];
            let key_len = str::from_utf8(&metadata[1..5]).unwrap().parse::<usize>().unwrap();
            let mut val_len = 0;

            // Replicated Add (from another 5S8S in order to replicate)
            if action == 'a' as u8 || action == 'r' as u8 {
                val_len = str::from_utf8(&metadata[5..13]).unwrap().parse::<usize>().unwrap();
            }

            (_reader, action, key_len, val_len)
        })
        .map_err(|e| eprintln!("Processing error: {:?}", e))
        .map(move |(_reader, action, key_len, val_len)| {
            let buf = vec![0; key_len + val_len];
            read_exact(_reader, buf)
        })
        .map_err(|e| eprintln!("Processing error: {:?}", e))
        .map(|body| {
            println!("{:?}", body);
            return String::from("Success")
        })
        .map_err(|e| eprintln!("Processing error: {:?}", e))
        .map(|_response| {
            println!("executing wrtie all");
            write_all(writer, _response.into_bytes()).map(|(w, _)| w)
        })
        .map_err(|e| eprintln!("Processing error: {:?}", e))
        .then(move |_| Ok(()));

        tokio_current_thread::spawn(processing);
        Ok(())

        //.map_err(|e| eprintln!("Processing error: {:?}", e)).then(move |_| Ok(()));


        /*
        let lines = lines(BufReader::new(reader));

        let responses = lines.map(move |line: String| {
            let action: String = line.chars().skip(0).take(1).collect();
            let mut message = String::from("Error");
            // Add a key - value pair
            if (action == "a") {
                process_add(&line, state.get_string_cache());
                for addr_str in state.get_peers() {
                    let addr = addr_str.parse().unwrap();
                    let linez = line.clone();
                    let write = TcpStream::connect(&addr).map(move |stream| {
                        write_all(stream, linez)
                    });
                    let msg = write.then(|_| {
                      Ok(())
                    });
                    tokio_current_thread::spawn(msg);
                }
                message = String::from("Added");
            }
            // Replicated Add (from another 5S8S in order to replicate)
            if (action == "r") {

            }
            // Get a specific value for a key
            if (action == "g") {

                message = process_get(&line, state.get_string_cache());
            }
            message
        });

        let writes = responses.fold(writer, |_writer, _response| {
            write_all(_writer, _response.into_bytes()).map(|(w, _)| w)
        });

        let msg = writes.then(move |_| Ok(()));
        tokio_current_thread::spawn(msg);
        Ok(())
    */
    });

    tokio_current_thread::block_on_all(server);
}
