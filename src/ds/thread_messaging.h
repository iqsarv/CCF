// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once

//#define USE_MPSCQ

#include "ds/logger.h"
#ifdef USE_MPSCQ
#  include "ds/mpscq.h"
#endif

#include <atomic>
#include <cstddef>
#include <map>
#include <thread>

extern std::map<std::thread::id, uint16_t> thread_ids;

namespace enclave
{
  const uint64_t magic_const = 0xba5eball;
  struct alignas(8) ThreadMsg
  {
    void (*cb)(std::unique_ptr<ThreadMsg>);
    std::atomic<ThreadMsg*> next = nullptr;
    uint64_t magic = magic_const;

    ThreadMsg(void (*_cb)(std::unique_ptr<ThreadMsg>)) : cb(_cb) {}

    virtual ~ThreadMsg()
    {
      assert(magic == magic_const);
    }
  };

  template <typename Payload>
  struct alignas(8) Tmsg : public ThreadMsg
  {
    Payload data;

    Tmsg(void (*_cb)(std::unique_ptr<Tmsg<Payload>>)) :
      ThreadMsg(reinterpret_cast<void (*)(std::unique_ptr<ThreadMsg>)>(_cb))

    {}

    virtual ~Tmsg() = default;
  };

  static void init_cb(std::unique_ptr<ThreadMsg> m)
  {
    LOG_INFO_FMT("Init was called");
  }

  class Task
  {
#ifdef USE_MPSCQ
    queue::MPSCQ<ThreadMsg> queue;
#else
    std::atomic<ThreadMsg*> item_head = nullptr;
    ThreadMsg* local_msg = nullptr;
#endif

  public:
    Task()
    {
#ifdef USE_MPSCQ
      auto msg = new ThreadMsg;
      msg->cb = &init_cb;
      queue.init(msg);
#endif
    }

    bool run_next_task()
    {
#ifdef USE_MPSCQ
      if (queue.is_empty())
      {
        return false;
      }

      ThreadMsg* current;
      bool result;
      std::tie(current, result) = queue.dequeue();

      if (result)
      {
        current->cb(std::unique_ptr<ThreadMsg>(current));
      }
#else
      if (local_msg == nullptr && item_head != nullptr)
      {
        local_msg = item_head.exchange(nullptr);
        reverse_local_messages();
      }

      if (local_msg == nullptr)
      {
        return false;
      }

      ThreadMsg* current = local_msg;
      local_msg = local_msg->next;

      current->cb(std::unique_ptr<ThreadMsg>(current));
#endif
      return true;
    }

    void add_task(ThreadMsg* item)
    {
#ifdef USE_MPSCQ
      queue.enqueue(item, item);
#else
      ThreadMsg* tmp_head;
      do
      {
        tmp_head = item_head.load();
        item->next = tmp_head;
      } while (!item_head.compare_exchange_strong(tmp_head, item));
#endif
    }

  private:
#ifndef USE_MPSCQ
    void reverse_local_messages()
    {
      if (local_msg == NULL)
        return;

      ThreadMsg *prev = NULL, *current = NULL, *next = NULL;
      current = local_msg;
      while (current != NULL)
      {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
      }
      // now let the head point at the last node (prev)
      local_msg = prev;
    }
#endif
  };

  class ThreadMessaging
  {
    std::atomic<bool> finished;
    std::vector<Task> tasks;

  public:
    static ThreadMessaging thread_messaging;
    static std::atomic<uint16_t> thread_count;
    static const uint16_t main_thread = 0;

    static const uint16_t max_num_threads = 64;

  public:
    ThreadMessaging(uint16_t num_threads = max_num_threads) :
      finished(false),
      tasks(num_threads)
    {}

    void set_finished(bool v = true)
    {
      finished.store(v);
    }

    void run()
    {
      Task& task = tasks[thread_ids[std::this_thread::get_id()]];

      while (!is_finished())
      {
        task.run_next_task();
      }
    }

    bool run_one(uint16_t tid)
    {
      Task& task = tasks[tid];

      return task.run_next_task();
    }

    template <typename Payload>
    void add_task(uint16_t tid, std::unique_ptr<Tmsg<Payload>> msg)
    {
      Task& task = tasks[tid];

      task.add_task(reinterpret_cast<ThreadMsg*>(msg.release()));
    }

    template <typename RetType, typename InputType>
    static std::unique_ptr<Tmsg<RetType>> ConvertMessage(
      std::unique_ptr<Tmsg<InputType>> msg,
      void (*cb)(std::unique_ptr<Tmsg<RetType>>))
    {
      auto ret = std::unique_ptr<enclave::Tmsg<RetType>>(
        (enclave::Tmsg<RetType>*)msg.release());
      new (ret.get()) enclave::Tmsg<RetType>(cb);
      return ret;
    }

  private:
    bool is_finished()
    {
      return finished.load();
    }
  };
};
