/**
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */
#include "scheduler.hpp"
#include <algorithm> 

using namespace std;

Scheduler::Scheduler(char* in,
                     char* out,
                     BlockInfo* metadata,
                     size_t isize,
                     size_t block_size,
                     size_t iters,
                     size_t min_work_size) :
        in(in),
        current_in(in),
        out(out),
        current_out(out),
        metadata(metadata),
        current_metadata(metadata),
        size(isize),
        size_left(isize),
        block_size(block_size),
        iters_left(iters)
{
    create_mutex(&lock);
    work_size = (min_work_size / block_size) * block_size;
    if (work_size < min_work_size)
        work_size += block_size;
}
Scheduler::~Scheduler()
{
    destroy_mutex(&lock);
}
int Scheduler::getChunk(Scheduler::WorkItem& wi)
{
    // grab a mutex
    Lock l = Lock(lock); // unlocks automatically

    if (iters_left == 0)
        return -1;

    size_t in_chunk_size = min(work_size, size_left);
    unsigned no_of_blocks = in_chunk_size / block_size;
    if (no_of_blocks * block_size < in_chunk_size)
        ++no_of_blocks;

    wi.isize = in_chunk_size;
    wi.in = current_in;
    wi.out = current_out;
    wi.metadata = current_metadata;

    if (in_chunk_size == size_left) // we grabbed the last chunk of this iteration
    {
        --iters_left;
        current_in = in;
        current_out = out;
        current_metadata = metadata;
        size_left = size;
    }
    else
    {
        current_in += in_chunk_size;
        current_out += no_of_blocks * block_size;
        current_metadata += no_of_blocks;
        size_left -= in_chunk_size;
    }

    return 0;
}
Scheduler::Lock::Lock(MUTEX& mutex) :
        mutex(mutex)
{
    lock_mutex(&mutex);
}
Scheduler::Lock::~Lock()
{
    unlock_mutex(&mutex);
}
