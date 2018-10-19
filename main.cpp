// A fast hash map, courtesy of https://github.com/skarupke
#include "flat_hash_map/bytell_hash_map.hpp"

// Other files from this project

// stl
#include <string>
#include <vector>
#include <thread>
#include <stdexcept>
#include <thread>
#include <algorithm>
#include <iterator>
#include <memory>
#include <array>

// boost
#include <boost/asio.hpp>

//Debug purposes, remove for final compilation
#include <iostream>

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)


template<class KeyT, class ValT> class Cache {
private:
  ska::bytell_hash_map<KeyT, ValT> cache{};
  std::vector<std::pair<KeyT, ValT>> send_buffer;

public:
  auto run() {
    using boost::asio::ip::tcp;

    // Any program that uses asio need to have at least one io_service object
    boost::asio::io_service io_service;

    // acceptor object needs to be created to listen for new connections
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 5282));

    while(true) {
      tcp::socket socket(io_service);
      acceptor.accept(socket);

      std::array<char, 256> buf{};
      boost::system::error_code error;

      size_t len = socket.read_some(boost::asio::buffer(buf), error);
      std::cout << "!!" << std::endl;

      boost::system::error_code ignored_error;
      boost::asio::write(socket, boost::asio::buffer("message"), ignored_error);
    }

    /*
    const auto possible_val = cache.find('key');
    if(likely(possible_val != cache.end())) {

    } else {

    }
    cache.erase(possible_val);
    */
  }
};


int main() {
  Cache<std::string, std::string> cache_test{};
  cache_test.run();
}
