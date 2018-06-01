#pragma once

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


constexpr int32_t max_events = 4096


enum class NwErr {
  Ok
  GetAddrInfo
}


std::pair<NwErr, int> bind(const std::string & port) {
  struct addrinfo hints;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC; /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* TCP */
  hints.ai_flags = AI_PASSIVE; /* All interfaces */

  struct addrinfo* result;
  int sockt = getaddrinfo(nullptr, port.c_str(), &hints, &result);

  if (sockt != 0) {
      return {NwErr::GetAddrInfo, -1};
  }

  struct addrinfo* rp;
  int socketfd;
}
