#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <noncopyable.hpp>
#include <worker.hpp>
#include <atomic>
#include <stdexcept>
#include <memory>

/**
 * @brief The thread_pool_t class implements thread pool pattern.
 * It is highly scalable and fast.
 * It is header only.
 * It implements both work-stealing and work-distribution balancing startegies.
 */
class thread_pool_t : noncopyable_t {
public:
    enum {AUTODETECT = 0};

    /**
     * @brief thread_pool_t Construct and start new thread pool.
     * @param threads_count Number of threads to create.
     */
    explicit thread_pool_t(size_t threads_count = AUTODETECT);

    /**
     * @brief post Post piece of job to thread pool.
     * @param handler Handler to be called from thread pool worker. It has to be callable as 'handler()'.
     * @throws std::overflow_error if worker's queue is full.
     */
    template <typename Handler>
    void post(Handler &&handler);

private:
    /**
     * @brief get_worker Helper function to select next executing worker.
     * @return Reference to worker.
     */
    worker_t & get_worker();

    std::unique_ptr<worker_t[]> m_workers;
    size_t m_workers_count;
    std::atomic<size_t> m_next_worker;
};


/// Implementation

inline thread_pool_t::thread_pool_t(size_t threads_count)
    : m_next_worker(0)
{
    if (AUTODETECT == threads_count) {
        threads_count = std::thread::hardware_concurrency();
    }

    if (0 == threads_count) {
        threads_count = 1;
    }

    m_workers_count = threads_count;

    m_workers.reset(new worker_t[m_workers_count]);

    for (size_t i = 0; i < m_workers_count; ++i) {
        worker_t *steal_donor = &m_workers[(i + 1) % threads_count];
        m_workers[i].start(i, steal_donor);
    }
}

template <typename Handler>
inline void thread_pool_t::post(Handler &&handler)
{
    if (!get_worker().post(std::forward<Handler>(handler)))
    {
        throw std::overflow_error("worker queue is full");
    }
}

inline worker_t & thread_pool_t::get_worker()
{
    size_t id = worker_t::get_worker_id_for_this_thread();

    if (id > m_workers_count) {
        id = m_next_worker.fetch_add(1, std::memory_order_relaxed) % m_workers_count;
    }

    return m_workers[id];
}

#endif

