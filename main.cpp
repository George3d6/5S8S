// A fast hash map, courtesy of https://github.com/skarupke
#include "flat_hash_map/bytell_hash_map.hpp"

// stl
#include <string>
#include <vector>
#include <thread>

// Linux specific stuff
#include <sys/epoll.h>

//Debug purposes, remove for final compilation
#include <iostream>


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
};


int main() {
  Cache<int64_t, std::string> cache_test{};
  //
  ska::bytell_hash_map<int64_t, std::string> cache{};
  cache[12] = std::string{"test"};
  std::cout << "\"" << (*(cache.find(12) ) ).second  << "\"" << std::endl;
  return 0;
}
