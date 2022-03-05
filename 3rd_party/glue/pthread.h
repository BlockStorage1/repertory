#ifndef REPERTORY_PTHREAD_H
#define REPERTORY_PTHREAD_H
#ifdef _WIN32

#include <mutex>
#include <condition_variable>

#define pthread_mutex_t std::mutex *
#define pthread_cond_t std::condition_variable *

static void pthread_mutex_init(pthread_mutex_t *mtx, void *) { *mtx = new std::mutex(); }

static void pthread_mutex_destroy(pthread_mutex_t *mtx) {
  delete *mtx;
  *mtx = nullptr;
}

static void pthread_mutex_lock(pthread_mutex_t *mtx) { (*mtx)->lock(); }

static void pthread_mutex_unlock(pthread_mutex_t *mtx) { (*mtx)->unlock(); }

static void pthread_cond_init(pthread_cond_t *cond, void *) {
  *cond = new std::condition_variable();
}

static void pthread_cond_destroy(pthread_cond_t *cond) {
  delete *cond;
  *cond = nullptr;
}

static void pthread_cond_signal(pthread_cond_t *cond) { (*cond)->notify_one(); }

static void pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx) {
  std::unique_lock<std::mutex> l(**mtx);
  (*cond)->wait(l);
}
#endif // _WIN32
#endif // REPERTORY_PTHREAD_H
