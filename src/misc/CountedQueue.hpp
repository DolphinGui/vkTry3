#pragma once

#include "blockingconcurrentqueue.h"
#include <atomic>

template<typename T, typename Traits>
// counts the number of threads blocked by this. doesn't implement consumer
// tokens yet.
class CountedQueue
{
  using _type = moodycamel::BlockingConcurrentQueue<T, Traits>;
  _type internal;
  std::atomic<uint>
    count; // should probably be replaced with std::latch or barrier for c++20
public:
  inline operator _type() const { return internal; }
  // not sure if this actually works. cpu optimization might screw
  // with this, testing necessary
  template<typename U>
  inline void wait_dequeue(U& item) noexcept
  {
    count++;
    internal.wait_dequeue(item);
    count--;
  }
  template<typename U>
  inline size_t wait_dequeue_bulk(U& item) noexcept
  {
    count++;
    auto result = internal.wait_dequeue(item);
    count--;
    return result;
  }
  template<typename U, typename Rep, typename Period>
  inline bool wait_dequeue_timed(
    U& item,
    std::chrono::duration<Rep, Period> const& timeout) noexcept
  {
    count++;
    auto result = internal.wait_dequeue_timed(item, timeout);
    count--;
    return result;
  }
  template<typename It, typename Rep, typename Period>
  inline size_t wait_dequeue_bulk_timed(
    It itemFirst,
    size_t max,
    std::chrono::duration<Rep, Period> const& timeout) noexcept
  {
    count++;
    auto result = internal.wait_dequeue_bulk_timed(itemFirst, max, timeout);
    count--;
    return result;
  }
  inline uint waiting() const noexcept { return count; }
};
