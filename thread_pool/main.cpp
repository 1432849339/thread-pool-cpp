#include "thread_pool.hpp"

#include "asio_thread_pool.hpp"

#include "mpsc_bounded_queue.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <vector>

static const int THREADS_COUNT = 2;
static const size_t REPOST_COUNT = 100000;

struct heavy_t
{
    bool verbose;
    std::vector<char> resource;

    heavy_t(bool verbose = false)
        : verbose(verbose)
        , resource(100*1024*1024)
    {
        if (!verbose)
            return;
        std::cout << "heavy default constructor" << std::endl;
    }

    heavy_t(const heavy_t &o)
        : verbose(o.verbose)
        , resource(o.resource)
    {
        if (!verbose)
            return;
        std::cout << "heavy copy constructor" << std::endl;
    }

    heavy_t(heavy_t &&o)
        : verbose(o.verbose)
        , resource(std::move(o.resource))
    {
        if (!verbose)
            return;
        std::cout << "heavy move constructor" << std::endl;
    }

    heavy_t & operator==(const heavy_t &o)
    {
        verbose = o.verbose;
        resource = o.resource;
        if (!verbose)
            return *this;
        std::cout << "heavy copy operator" << std::endl;
        return *this;
    }

    heavy_t & operator==(const heavy_t &&o)
    {
        verbose = o.verbose;
        resource = std::move(o.resource);
        if (!verbose)
            return *this;
        std::cout << "heavy move operator" << std::endl;
        return *this;
    }
};


struct repost_job_t
{
    //heavy_t heavy;

    thread_pool_t *thread_pool;
    asio_thread_pool_t *asio_thread_pool;

    size_t counter;
    long long int begin_count;

    explicit repost_job_t(thread_pool_t *thread_pool)
        : thread_pool(thread_pool)
        , asio_thread_pool(0)
        , counter(0)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    explicit repost_job_t(asio_thread_pool_t *asio_thread_pool)
        : thread_pool(0)
        , asio_thread_pool(asio_thread_pool)
        , counter(0)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    void operator()()
    {
        if (counter++ < REPOST_COUNT)
        {
            if (asio_thread_pool)
            {
                asio_thread_pool->post(*this);
            }
            if (thread_pool)
            {
                thread_pool->post(*this);
            }
        }
        else
        {
            long long int end_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::cout << "reposted " << counter
                      << " in " << (double)(end_count - begin_count)/(double)1000000 << " ms"
                      << std::endl;
        }
    }
};

struct copy_task_t
{
    heavy_t heavy;

    copy_task_t() : heavy(true) {}

    void operator()()
    {
        std::cout << "copy_task_t::operator()()" << std::endl;
    }
};

void test_queue()
{
    mpsc_bounded_queue_t<int, 2> queue;
    int e = 0;
    assert(!queue.move_pop(e));
    assert(queue.move_push(1));
    assert(queue.move_push(2));
    assert(!queue.move_push(3));
    assert(queue.move_pop(e) && e == 1);
    assert(queue.move_pop(e) && e == 2);
    assert(!queue.move_pop(e));
    assert(queue.move_push(3));
    assert(queue.move_pop(e) && e == 3);
}

void test_standalone_func()
{
}

struct test_member_t
{
    int useless(int i, int j, const heavy_t &heavy)
    {
        (void)heavy;
        return i + j;
    }

} test_member;

int main(int, const char *[])
{
    using namespace std::placeholders;

    test_queue();

    std::cout << "*******begin tests*******" << std::endl;
    {
        std::cout << "***thread_pool_t***" << std::endl;

        thread_pool_t thread_pool(THREADS_COUNT);

        thread_pool.post(test_standalone_func);

        std::cout << "Copy test1 [ENTER]" << std::endl;
        thread_pool.post(copy_task_t());
        std::cin.get();

        std::cout << "Copy test2 [ENTER]" << std::endl;
        thread_pool.post(std::bind(&test_member_t::useless, &test_member, 1, 2, heavy_t(true)));
        std::cin.get();

        thread_pool.post(repost_job_t(&thread_pool));
        thread_pool.post(repost_job_t(&thread_pool));
        thread_pool.post(repost_job_t(&thread_pool));
        thread_pool.post(repost_job_t(&thread_pool));

        std::cout << "Repost test [ENTER]" << std::endl;
        std::cin.get();
    }

    {
        std::cout << "***asio_thread_pool_t***" << std::endl;

        asio_thread_pool_t asio_thread_pool(THREADS_COUNT);

        std::cout << "Copy test [ENTER]" << std::endl;
        asio_thread_pool.post(copy_task_t());
        std::cin.get();

        asio_thread_pool.post(repost_job_t(&asio_thread_pool));
        asio_thread_pool.post(repost_job_t(&asio_thread_pool));
        asio_thread_pool.post(repost_job_t(&asio_thread_pool));
        asio_thread_pool.post(repost_job_t(&asio_thread_pool));

        std::cout << "Repost test [ENTER]" << std::endl;
        std::cin.get();
    }

    return 0;
}
