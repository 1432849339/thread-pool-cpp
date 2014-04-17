thread-pool-cpp11
=================

1. Fast.
2. Scalable.
3. Header-only.
4. Work stealing balancing strategy.

Example run:
Post job to thread pool is up to 10 times faster than for boost::asio based thread pool.

    *******begin tests*******
    ***thread_pool_t***
    Repost test [ENTER]
    reposted 1000001 in 406.25 ms
    reposted 1000001 in 406.25 ms
    reposted 1000001 in 406.25 ms
    reposted 1000001 in 406.25 ms

    ***asio_thread_pool_t***
    Repost test [ENTER]
    reposted 1000001 in 3906.25 ms
    reposted 1000001 in 3921.88 ms
    reposted 1000001 in 3921.88 ms
    reposted 1000001 in 3921.88 ms

See main.cpp for benchmark code.

