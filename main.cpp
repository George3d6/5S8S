// A fast hash map, courtesy of https://github.com/skarupke
#include "flat_hash_map/bytell_hash_map.hpp"

// Other files from this project
#include "networking.hpp"

// stl
#include <string>
#include <vector>
#include <thread>
#include <stdexcept>

//boost
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
using boost::asio::ip::tcp;

//Debug purposes, remove for final compilation
#include <iostream>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <cstring>
#include <netdb.h>
#include <array>


#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)


template<class KeyT, class ValT> class Cache {
  private:
    ska::bytell_hash_map<KeyT, ValT> cache{};
    std::vector<std::pair<KeyT, ValT>> send_buffer;

  public:
    inline void add(KeyT && key, ValT && val) {
      cache[key] = val;
      send_buffer.emplace_back({key, val});
    }

    inline auto * get(const KeyT & key) {
      const auto possible_val = cache.find(key);
      if(likely(possible_val != cache.end())) {
        return possible_val;
      }
      return nullptr;
    }

    auto run() {
      /* MAKE SERVER */

      // Create the TCP server socket
      const auto bind_res = networking::bind("5282");
      if(bind_res.first != networking::Error::None) {
        throw std::runtime_error("can't bind to port 5282 !");
      }
      const auto server_socket = bind_res.second;

      // Make the socket non blocking
      const auto err = networking::make_socket_nonblocking(server_socket);
      if (err != networking::Error::None) {
        throw std::runtime_error("Unprintable networking error !");
      }

      if (listen(server_socket, SOMAXCONN) == -1) {
        throw std::runtime_error("Can't listen !");
      }

      int epoll_fd = epoll_create1(0);
      if (epoll_fd == -1) {
        throw std::runtime_error("epoll_create1 failed for a mystery reason, enjoy debugging this one !");
      }

      //Possible code duplication
      struct epoll_event event;
      event.data.fd = server_socket;
      event.events = EPOLLIN | EPOLLET;

      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1)
      if (epoll_fd == -1)  {
          throw std::runtime_error("epoll_ctl failed for a mystery reason, enjoy debugging this one !");
      }

      /* MAKE SERVER */


      /* MAKE CLIENT */

      auto conn_result = networking::connect("127.0.0.1", 5282);
      if(conn_result.first != networking::Error::None) {
        throw std::runtime_error("can't connect !!");
      }
      const auto client_socket = conn_result.second;

      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1)
      if (epoll_fd == -1)  {
          throw std::runtime_error("epoll_ctl failed for a mystery reason, enjoy debugging this one !");
      }

      /* MAKE CLIENT */

      constexpr int max_conn = 4096;
      std::array<struct epoll_event, max_conn> events;


      // Start the "server" loop
      while (true) {
        auto n = epoll_wait(epoll_fd, events.data(), max_conn, -1);
        std::cout << n << std::endl;
        for (int i = 0; i < n; i++) {
          if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN)) {
                std::cerr << "Warning: epoll event error, closing connection !\n";
                close(events[i].data.fd);
          } else if (server_socket == events[i].data.fd) {
            std::cout << "Accepting connections !" << std::endl;
            // Block until connected... maybe bad ?
            //while(networking::Error::None == networking::accept_conn(server_socket, event, epoll_fd)) {}
            networking::accept_conn(server_socket, event, epoll_fd);
          } else {
            std::cout << "Reading data!" << std::endl;
            auto fd = events[i].data.fd;
            constexpr size_t buf_len = 512;
            std::array<char,buf_len> arr_buf;
            networking::read_data(fd, arr_buf.data(), buf_len);
            std::cout << "Got character: " << arr_buf.data() << std::endl;
            write(client_socket, "ABA", 3);
          }
        }
      }
    }

    auto run2() {
      boost::asio::io_service io_service;
      tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 5282));
      tcp::socket socket(io_service);
    }
};


int main() {
  Cache<int64_t, std::string> cache_test{};
  cache_test.run2();
  /*
  ska::bytell_hash_map<int64_t, std::string> cache{};
  cache[12] = std::string{"test"};
  std::cout << "\"" << (*(cache.find(12) ) ).second  << "\"" << std::endl;
  return 0;
  */
}
