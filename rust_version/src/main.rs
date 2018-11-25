extern crate tokio;
extern crate tokio_current_thread;
#[macro_use]
extern crate futures;
extern crate bytes;

use tokio::io;
use tokio::net::{TcpListener, TcpStream};
use tokio::prelude::*;
use futures::sync::mpsc;
use futures::future::{self, Either};
use bytes::{BytesMut, Bytes, BufMut};


use std::collections::HashMap;
use std::str;
use std::vec::Vec;
use std::string::String;
use std::io::BufReader;
use std::cell::UnsafeCell;
use std::rc::Rc;


pub struct State {
    sv_cache: UnsafeCell<HashMap<BytesMut, BytesMut>>,
    peers: UnsafeCell<Vec<String>>,
}

impl State {
    fn get_peers(&self) -> &mut Vec<String> {
        unsafe { return &mut *self.peers.get(); }
    }
    fn get_bytes_cache(&self) -> &mut HashMap<BytesMut, BytesMut> {
        unsafe { return &mut *self.sv_cache.get(); }
    }
}

struct Messages {
    socket: TcpStream,
    state: u8,

    operation: BytesMut,

    key_len: usize,
    val_len: usize,

    rd: BytesMut,
    wr: BytesMut,
}

impl Stream for Messages {
    type Item = (BytesMut, BytesMut, BytesMut);
    type Error = io::Error;

    fn poll(&mut self) -> Result<Async<Option<Self::Item>>, Self::Error> {
        let sock_closed = self.fill_read_buf()?.is_ready();

        if self.state == 0 && self.rd.len() > 8 {
            self.operation = self.rd.split_to(1);
            if self.operation == "a" || self.operation == "r" {
                self.key_len = str::from_utf8(&self.rd.split_to(8)).unwrap().parse::<usize>().unwrap();
                self.val_len = str::from_utf8(&self.rd.split_to(8)).unwrap().parse::<usize>().unwrap();
            } else {
                self.key_len = str::from_utf8(&self.rd.split_to(8)).unwrap().parse::<usize>().unwrap();
                self.val_len = 0;
            }
            self.state = 1;
        }

        if self.state == 1 && self.rd.len() >= self.key_len + self.val_len {
            let k = self.rd.split_to(self.key_len);
            let v =  self.rd.split_to(self.val_len);
            self.state = 0;
            return Ok(Async::Ready(Some((self.operation.clone(),v,k))));
        }

        if sock_closed {
            Ok(Async::Ready(None))
        } else {
            Ok(Async::NotReady)
        }
    }
}

impl Messages {
    fn fill_read_buf(&mut self) -> Result<Async<()>, io::Error> {
        loop {
            self.rd.reserve(1024);
            let n = try_ready!(self.socket.read_buf(&mut self.rd));
            if n == 0 {
                return Ok(Async::Ready(()));
            }
        }
    }

    fn buffer(&mut self, msg: &[u8]) {
        self.wr.put(msg);
    }

    fn poll_flush(&mut self) -> Poll<String, io::Error> {
        println!("Flushing write buffer !");
        // As long as there is buffered data to write, try to write it.
        while !self.wr.is_empty() {
            // Try to write some bytes to the socket
            let n = try_ready!(self.socket.poll_write(&self.wr));

            // As long as the wr is not empty, a successful write should
            // never write 0 bytes.
            assert!(n > 0);

            // This discards the first `n` bytes of the buffer.
            let _ = self.wr.split_to(n);
        }

        Ok(Async::Ready(("sfsdgdsgdsgds".to_string())))
    }
}

impl Messages {
    fn new(socket: TcpStream) -> Self {
        Messages {
            socket,
            state: 0,
            operation: BytesMut::new(),
            key_len: 0,
            val_len: 0,
            rd: BytesMut::new(),
            wr: BytesMut::new(),
        }
    }
}

fn process_add(key: BytesMut, val: BytesMut, cache: &mut HashMap<BytesMut, BytesMut>) {
    cache.insert(key, val);
}

fn process_get(key: BytesMut, cache: &mut HashMap<BytesMut, BytesMut>) -> String {
    match cache.get(&key) {
        // @TODO using format here is just for debugging purposes
        Some(val) => return format!("o{:08}{:?}", val.len(), val),
        None => return String::from("e")
    }
}

fn process(socket: TcpStream, state: Rc<State>) {
    let messages = Messages::new(socket);
    let connection = messages.into_future()
        .map_err(|(e, _)| e)
        .map(move |(msg, mut messages)| {
            let write_back = match msg {
                Some(msg) => {
                    if msg.0 == "a" {
                        process_add(msg.1, msg.2, state.get_bytes_cache());
                        // @TODO: Implement replication on other instances
                    } else if msg.0 == "r" {
                        process_add(msg.1, msg.2, state.get_bytes_cache());
                    } else if msg.0 == "g" {

                    } else if msg.0 == "d" {

                    }
                    "o"
                },
                None => {
                    // @TODO handle disconect
                    "e"
                }
            };
            messages.buffer(write_back.as_bytes());
            messages.poll_flush()
    }).then(move |_| Ok( () ));
    tokio_current_thread::spawn(connection)
}

fn main() {
    let state_global = Rc::new( State{sv_cache: UnsafeCell::new(HashMap::new()),
        peers: UnsafeCell::new(Vec::new())} );

    let addr = "127.0.0.1:5282".parse().unwrap();
    let listener = TcpListener::bind(&addr).expect("unable to bind TCP listener");

    let server = listener.incoming()
    .map_err(|e| eprintln!("Accept connection error: {:?}", e))
    .for_each(move |sock| {
        process(sock, state_global.clone());
        Ok(())
    });

    tokio_current_thread::block_on_all(server);


    /*
        let state = state_global.clone();

        let (reader, writer) = sock.split();

        // let metadata: BytesMut = Vec::new();
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
        .fold(writer, |_writer, _response| {
            println!("executing wrtie all");
            write_all(_writer, _response.into_bytes()).map(|(w, _)| w)
        })
        .map_err(|e| eprintln!("Processing error: {:?}", e))
        .then(move |_| Ok( () ));

        tokio_current_thread::spawn(processing);
        Ok(())
*/
        //.map_err(|e| eprintln!("Processing error: {:?}", e)).then(move |_| Ok(()));


        /*
        let lines = lines(BufReader::new(reader));

        let responses = lines.map(move |line: String| {
            let action: String = line.chars().skip(0).take(1).collect();
            let mut message = String::from("Error");
            // Add a key - value pair
            if (action == "a") {
                process_add(&line, state.get_sv_cache());
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

                message = process_get(&line, state.get_sv_cache());
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
}
