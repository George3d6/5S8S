#pragma once

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <cstring>
#include <netdb.h>
#include <array>



// Based on this example: https://gist.github.com/dtoma/417ce9c8f827c9945a9192900aff805b ... well, kinda was based on it, heavily modified now
namespace networking {
  enum class Error {
    None = 0
    ,Bind = 1
    ,GetAddrInfo = 2
    ,ERR_F_GETFL = 3
    ,ERR_F_SETFL = 4
    ,ERR_EAGAIN = 5 //No data
    ,ERR_EWOULDBLOCK = 6 // Can't do w/o blocking thread
    ,AcceptConn = 7
    ,Epoll_ctl = 8
    ,RemoteClosedConn = 9
    ,Connect = 10
  };


  std::pair<Error, int> bind(const std::string & port) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = AI_PASSIVE; /* All interfaces */

    struct addrinfo* result;
    int sockt = getaddrinfo(nullptr, port.c_str(), &hints, &result);

    if (sockt != 0) {
        return {Error::GetAddrInfo, -1};
    }

    struct addrinfo* rp;
    int socket_fd;
    for(rp = result; rp != nullptr; rp = rp->ai_next) {
      socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_fd == -1) {
            continue;
        }

        sockt = bind(socket_fd, rp->ai_addr, rp->ai_addrlen);
        if (sockt == 0) {
            break;
        }

        close(socket_fd);
    }

    if (rp == nullptr) {
        return {Error::Bind, -1};
    }

    freeaddrinfo(result);

    return {Error::None, socket_fd};;
  };


  // Open the file descriptor in non-blocking mode
  Error make_socket_nonblocking(int socket_fd) {
    // Get the flags set for the socket fd
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        return Error::ERR_F_GETFL;
    }

    flags = flags | O_NONBLOCK; //(I have no fucking idea what this bwor is doing)
    // This command reset every other flags: no append, no async, no direct, no atime, and no nonblocking
    int s = fcntl(socket_fd, F_SETFL, flags);
    if (s == -1) {
        return Error::ERR_F_SETFL;
    }

    return Error::None;
  };


  Error accept_conn(int socket_fd, struct epoll_event& event, int epoll_fd) {
      struct sockaddr in_addr;
      socklen_t in_len = sizeof(in_addr);
      int in_fd = accept(socket_fd, &in_addr, &in_len);

      if (in_fd == -1) {
        if (errno == EAGAIN) {
          return Error::ERR_EAGAIN;
        }
        if (errno == EWOULDBLOCK) {
          return Error::ERR_EWOULDBLOCK;
        }
        return Error::AcceptConn;
      }

      const auto err = make_socket_nonblocking(in_fd);
      if (err != Error::None) {
        return err;
      }

      event.data.fd = in_fd;
      event.events = EPOLLOUT | EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;

      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, in_fd, &event) == -1) {
          return Error::Epoll_ctl;
      }

      return Error::None;
  };


  Error read_data(int fd, char * buf, size_t buf_len) {
    auto count = read(fd, buf, buf_len);

    if (count == -1) {
        if (errno == EAGAIN) {
            return Error::ERR_EAGAIN;
        }
    }

    if (count == 0) {
      close(fd);
      return Error::RemoteClosedConn;
    }

    return Error::None;
  };


  std::pair<Error, int> connect(const char * addr, uint32_t port) {
    struct sockaddr_in srv_addr;

    auto socket_fd = socket(AF_UNSPEC, SOCK_STREAM, 0);

    srv_addr.sin_family = AF_UNSPEC;
    srv_addr.sin_addr.s_addr = inet_addr(addr);
    srv_addr.sin_port = htons(port);

    if (connect(socket_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
      return {Error::Connect, -1};
    }

    make_socket_nonblocking(socket_fd);

    return {Error::None, socket_fd};
  }

  void write_data(int fd, const char * buf, size_t buf_len) {
    auto i = 0;
    while (i < 5) {
      i++;
      auto err = write(fd, buf, buf_len);
      if (err == -1) {
        std::cout << err << " " << errno << std::endl;
      } else {
        break;
      }
    }
    if (i == 5) {
      close(fd);
    }
  }
}
