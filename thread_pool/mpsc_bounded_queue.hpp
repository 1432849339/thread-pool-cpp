#ifndef MPSC_QUEUE_HPP
#define MPSC_QUEUE_HPP

#include <noncopyable.hpp>

#include <atomic>
#include <type_traits>
#include <cstddef>

/// Inspired by
///

/**
 * @brief The mpmc_bounded_queue_t class implements bounded multi-producers/multi-consumers lock-free queue.
 * Doesn't accept non-movabe types as T.
 * Inspired by Dmitry Vyukov's mpmc queue.
 * http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
 */
template <typename T, unsigned BUFFER_SIZE>
class mpmc_bounded_queue_t : noncopyable_t {
    enum {BUFFER_MASK = BUFFER_SIZE - 1};

    static_assert(std::is_move_constructible<T>::value, "Type have to be move-constructable.");

public:
    /**
     * @brief mpmc_bounded_queue_t Constructor.
     */
    mpmc_bounded_queue_t();

    /**
     * @brief push Push data to queue.
     * @param data Data to be pushed.
     * @return true on success.
     */
    template <typename U>
    bool push(U &&data);

    /**
     * @brief pop Pop data from queue.
     * @param data Place to store popped data.
     * @return true on sucess.
     */
    bool pop(T &data);

private:
    struct cell_t {
        std::atomic<size_t> sequence;
        T data;
    };

    typedef char cacheline_pad_t[64];

    cacheline_pad_t pad0;
    cell_t m_buffer[BUFFER_SIZE];
    cacheline_pad_t pad1;
    std::atomic<size_t> m_enqueue_pos;
    cacheline_pad_t pad2;
    std::atomic<size_t> m_dequeue_pos;
    cacheline_pad_t pad3;
};


/// Implementation

template <typename T, unsigned BUFFER_SIZE>
inline mpmc_bounded_queue_t<T, BUFFER_SIZE>::mpmc_bounded_queue_t()
    : m_enqueue_pos(0)
    , m_dequeue_pos(0)
{
    static_assert(std::is_move_constructible<T>::value, "Should be of movable type");
    static_assert((BUFFER_SIZE >= 2) && ((BUFFER_SIZE & (BUFFER_SIZE - 1)) == 0),
                  "buffer size should be a power of 2");

    for (size_t i = 0; i < BUFFER_SIZE; ++i)
    {
        m_buffer[i].sequence = i;
    }
}

template <typename T, unsigned BUFFER_SIZE>
template <typename U>
inline bool mpmc_bounded_queue_t<T, BUFFER_SIZE>::push(U &&data)
{
    cell_t *cell;
    size_t pos = m_enqueue_pos.load(std::memory_order_relaxed);
    for (;;) {
        cell = &m_buffer[pos & BUFFER_MASK];
        size_t seq = cell->sequence.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t)seq - (intptr_t)pos;
        if (dif == 0) {
            if (m_enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                break;
            }
        } else if (dif < 0) {
            return false;
        } else {
            pos = m_enqueue_pos.load(std::memory_order_relaxed);
        }
    }

    cell->data = std::forward<U>(data);

    cell->sequence.store(pos + 1, std::memory_order_release);

    return true;
}

template <typename T, unsigned BUFFER_SIZE>
inline bool mpmc_bounded_queue_t<T, BUFFER_SIZE>::pop(T &data)
{
    cell_t *cell;
    size_t pos = m_dequeue_pos.load(std::memory_order_relaxed);
    for (;;) {
        cell = &m_buffer[pos & BUFFER_MASK];
        size_t seq = cell->sequence.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
        if (dif == 0) {
            if (m_dequeue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                break;
            }
        } else if (dif < 0) {
            return false;
        } else {
            pos = m_dequeue_pos.load(std::memory_order_relaxed);
        }
    }

    data = std::move(cell->data);

    cell->sequence.store(pos + BUFFER_MASK + 1, std::memory_order_release);

    return true;
}

#endif
