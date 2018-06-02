// A fast hash map, courtesy of https://github.com/skarupke
#include "flat_hash_map/bytell_hash_map.hpp"

// Other files from this project
#include "networking.hpp"

// stl
#include <string>
#include <vector>
#include <thread>
#include <stdexcept>
#include <thread>
#include <algorithm>
#include <iterator>

//Debug purposes, remove for final compilation
#include <iostream>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
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
      event.events = EPOLLOUT | EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;

      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1)
      if (epoll_fd == -1)  {
          throw std::runtime_error("epoll_ctl failed for a mystery reason, enjoy debugging this one !");
      }

      /* MAKE SERVER */


      /* MAKE CLIENT */
      /*
      auto conn_result = networking::connect("127.0.0.1", 5282);
      if(conn_result.first != networking::Error::None) {
        throw std::runtime_error("can't connect !!");
      }
      const auto client_socket = conn_result.second;

      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1)
      if (epoll_fd == -1)  {
          throw std::runtime_error("epoll_ctl failed for a mystery reason, enjoy debugging this one !");
      }
      */
      /* MAKE CLIENT */

      constexpr int max_conn = 4096;
      std::array<struct epoll_event, max_conn> events;


      constexpr size_t buf_len = 68512;
      std::array<char,buf_len> arr_buff;

      // Start the "server" loop
      while (true) {
        auto n = epoll_wait(epoll_fd, events.data(), max_conn, -1);
        for (int i = 0; i < n; i++) {
          if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
                std::cerr << "Warning: epoll event error, closing connection !" << std::endl;
                close(events[i].data.fd);
          }

          if (server_socket == events[i].data.fd) {
            std::cout << "Accepting connections !" << std::endl;
            // Block until connected... maybe bad ?
            //while(networking::Error::None == networking::accept_conn(server_socket, event, epoll_fd)) {}
            networking::accept_conn(server_socket, event, epoll_fd);
          } else {
            networking::read_data(events[i].data.fd, arr_buff.data(), buf_len);

            /* Deserialization */
            auto mode = arr_buff[0];

            // Insert this, it comes from another 5S8S instance
            if(mode == 'i' || mode == 's') {
              // Get the key
              auto sep_one_pos = std::find(std::begin(arr_buff), std::begin(arr_buff) + 130, '|');
              auto sep_second_pos = std::find(sep_one_pos + 1, sep_one_pos + 130, '|');

              size_t key_size = std::stoi( std::string(std::begin(arr_buff) + 1, sep_one_pos) );
              size_t val_size  = std::stoi( std::string(sep_one_pos + 1, sep_second_pos) );

              std::string key = std::string(sep_second_pos + 1, sep_one_pos + key_size);
              std::string val = std::string(sep_second_pos + key_size, sep_second_pos + key_size + val_size);

              cache[key] = val;
              // Insert this, it comes from a client
              if(mode == 's') {
                send_buffer.push_back({key, val});
                // Message the other instances with the results !
              }
            }

            // Get me the value for this key
            if(mode == 'g') {
              // Get the key
              auto sep_one_pos = std::find(std::begin(arr_buff), std::begin(arr_buff) + 130, '|');
              size_t key_size = std::stoi( std::string(std::begin(arr_buff) + 1, sep_one_pos) );
              std::string key = std::string(sep_one_pos + 1, sep_one_pos + key_size);

              const auto possible_val = cache.find(key);
              //event.events = event.events | EPOLLOUT; // Is this neede d!?
              if(likely(possible_val == cache.end())) {
                auto err = write(events[i].data.fd, "n", 1);
              } else {
                auto err = write(events[i].data.fd, possible_val->second.c_str(), possible_val->second.size());
              }
              //event.events = event.events | EPOLLIN;// Is this neede d!?
            }
            //write(client_socket, "ABA", 3);
          }
        }
      }
      close(server_socket);
    }
};


int main() {
  Cache<std::string, std::string> cache_test{};
  cache_test.run();
}
