#include "../test.h"

TEST(hardware_concurrency) {
    print("\n\t\tHardware concurrency: {}.\n", os_get_hardware_concurrency());
    For(range(45)) print(" ");
}

file_scope void thread_ids(void *) { print("\t\tMy thread id is {}.\n", Context.ThreadID); }

TEST(ids) {
    print("\n\t\tMain thread's id is {}.\n", Context.ThreadID);

    thread::thread t1;
    t1.init_and_launch(thread_ids);
    t1.wait();

    thread::thread t2;
    t2.init_and_launch(thread_ids);
    t2.wait();

    thread::thread t3;
    t3.init_and_launch(thread_ids);
    t3.wait();

    For(range(45)) print(" ");
}

thread_local file_scope s32 TLSVar;
file_scope void thread_tls(void *) { TLSVar = 2; }

TEST(thread_local_storage) {
    TLSVar = 1;

    thread::thread t1;
    t1.init_and_launch(thread_tls);
    t1.wait();

    assert_eq(TLSVar, 1);
}

file_scope thread::mutex Mutex;
file_scope s32 Count = 0;

file_scope void thread_lock_free(void *) {
    For(range(10000)) {
        atomic_inc(&Count);
    }
}

TEST(lock_free) {
    Count = 0;

    array<thread::thread> threads;
    defer(free(threads));

    For(range(100)) {
        array_append(threads)->init_and_launch(thread_lock_free);
    }

    For(threads) {
        it.wait();
    }

    assert_eq(Count, 100 * 10000);
}

file_scope void thread_lock(void *) {
    For(range(10000)) {
        Mutex.lock();
        ++Count;
        Mutex.unlock();
    }
}

TEST(mutex_lock) {
    Count = 0;

    Mutex.init();

    array<thread::thread> threads;
    defer(free(threads));

    For(range(100)) {
        array_append(threads)->init_and_launch(thread_lock);
    }

    For(threads) {
        it.wait();
    }

    assert_eq(Count, 100 * 10000);
}

file_scope thread::fast_mutex FastMutex;

file_scope void thread_lock2(void *) {
    For(range(10000)) {
        FastMutex.lock();
        ++Count;
        FastMutex.unlock();
    }
}

TEST(fast_mutex_lock) {
    Count = 0;

    array<thread::thread> threads;
    defer(free(threads));
    For(range(100)) {
        array_append(threads)->init_and_launch(thread_lock2);
    }

    For(threads) {
        it.wait();
    }

    assert_eq(Count, 100 * 10000);
}

file_scope thread::condition_variable Cond;

file_scope void thread_condition_notifier(void *) {
    Mutex.lock();
    --Count;
    Cond.notify_all();
    Mutex.unlock();
}

file_scope void thread_condition_waiter(void *) {
    Mutex.lock();
    while (Count > 0) {
        Cond.wait(&Mutex);
    }
    assert_eq(Count, 0);
    Mutex.unlock();
}

TEST(condition_variable) {
    Count = 40;

    Cond.init();
    defer(Cond.release());

    thread::thread t1;
    t1.init_and_launch(thread_condition_waiter);

    // These will decrease Count by 1 when they finish)
    array<thread::thread> threads;
    free(threads);
    For(range(Count)) {
        array_append(threads)->init_and_launch(thread_condition_notifier);
    }

    t1.wait();

    For(threads) {
        it.wait();
    }

    Mutex.release();
}

TEST(context) {
    auto *old = Context.Alloc.Function;

    allocator differentAlloc = {};
    PUSH_ALLOC(differentAlloc) {
        auto threadFunction = [&](void *) {
            assert_eq((void *) Context.Alloc.Function, (void *) differentAlloc.Function);
            []() {
                PUSH_ALLOC(Context.TempAlloc) {
                    assert_eq((void *) Context.Alloc.Function, (void *) Context.TempAlloc.Function);
                    return;
                }
            }();
            assert_eq((void *) Context.Alloc.Function, (void *) differentAlloc.Function);
        };

        thread::thread t1;
        t1.init_and_launch(&threadFunction);
        t1.wait();
    }
    assert_eq((void *) Context.Alloc.Function, (void *) old);
}
