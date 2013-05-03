/**
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */
#ifndef SCHEDULER_HPP_BhjgkfG8
#define SCHEDULER_HPP_BhjgkfG8

#include "threads.hpp"

struct BlockInfo
{
    size_t encoded_size;
    size_t decoded_size;
    bool successfully_encoded;
};

// manages work assignments
class Scheduler
{
public:
    struct WorkItem
    {
        char* in;
        char* out;
        size_t isize;
        BlockInfo* metadata;
    };
    /**
     * 
     * @param wi
     * @return 0 on success, something else on failure
     */
    int getChunk(WorkItem& wi);
    Scheduler(char* in,
              char* out,
              BlockInfo* metadata,
              size_t isize,
              size_t block_size,
              size_t iters,
              size_t min_work_size);
    ~Scheduler();

private:
    MUTEX lock;

    char* in;
    char* current_in;
    char* out;
    char* current_out;
    BlockInfo* metadata;
    BlockInfo* current_metadata;
    size_t size;
    size_t size_left;
    size_t block_size;
    size_t iters_left;

    size_t work_size;

    /**
     * @brief Locks a given mutex when created and unlocks when destroyed.
     */
    class Lock
    {
    public:
        Lock(MUTEX& mutex);
        ~Lock();
    private:
        MUTEX& mutex;
    };
};

#endif // SCHEDULER_HPP_BhjgkfG8
