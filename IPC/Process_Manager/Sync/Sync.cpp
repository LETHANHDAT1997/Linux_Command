#include "Sync.h"

#include <cerrno>
#include <ctime>
#include <string>

#include <fcntl.h>
#include <pthread.h>

// #include "Debug.h"

Sync::Sync()
    : pLock_(0), type_(en_TYPE_INTER_THREAD), firstThread_(0), cnt_(0) {
  int ret = -1;

  this->name_[0] = 0;
  this->pLock_ = static_cast<sem_t *>(::malloc(sizeof(sem_t)));
  if (0 != this->pLock_) {
    ret = sem_init(this->pLock_, 0, 1);
    if (0 != ret) {
      //_STD_ERR(errno);
    }
  } else {
    //_LOGE("malloc failed.");
  }
}

Sync::Sync(const char *name, EN_TYPE type)
    : pLock_(0), type_(type), firstThread_(0), cnt_(0) {
  if (0 == name) {
    //_LOGE("invalid arg : name is null\n");
    return;
  }

  int ret = -1;

  (void)strncpy(this->name_, name, C_MAX_NAME_LEN);

  if (en_TYPE_INTER_THREAD == type) {
    this->pLock_ = static_cast<sem_t *>(::malloc(sizeof(sem_t)));
    if (0 != this->pLock_) {
      ret = sem_init(this->pLock_, 0, 1);
      if (0 != ret) {
        //_STD_ERR(errno);
        ::free(this->pLock_);
        this->pLock_ = 0;
      }
    } else {
      //_LOGE("malloc failed.");
    }
  } else if (en_TYPE_INTER_PROCESS == type) {
    std::string tmp = "/";

    tmp += name;
    this->pLock_ = sem_open(tmp.c_str(), O_CREAT, 0777, 1);
    if (SEM_FAILED == this->pLock_) {
      //_STD_ERR(errno);
      this->pLock_ = 0;
    }
  } else {
    //_LOGE("invalid arg\n");
  }
}

Sync::~Sync() {
  if (0 != this->pLock_) {
    if (en_TYPE_INTER_THREAD == this->type_) {
      sem_destroy(this->pLock_);
      ::free(this->pLock_);
      this->pLock_ = 0;
    } else if (en_TYPE_INTER_PROCESS == this->type_) {
      sem_close(this->pLock_);
      this->pLock_ = 0;
    }
  }
}

Sync::EN_STS Sync::Lock(int timeout) {
  if (0 == this->pLock_) {
    //_LOGE("handle is not created\n");
    return en_STS_ERR_OTHER;
  }

  int semRet = -1;
  EN_STS ret = en_STS_ERR_OTHER;
  pthread_t curThread = pthread_self();

  if (curThread == this->firstThread_) {
    this->cnt_++;
    ret = en_STS_SUCCESS;
  } else {
    if (C_TIME_INFINITE == timeout) {

      while ((semRet = sem_wait(this->pLock_)) == -1 && errno == EINTR) {
        continue;
      }
      if (-1 == semRet) {
        //_STD_ERR(errno);
      } else {
        this->firstThread_ = curThread;
        ret = en_STS_SUCCESS;
      }
    } else {
      struct timespec ts = {0, 0};
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec += timeout / 1000;
      ts.tv_nsec += ((timeout % 1000) * 1000000);

      ts.tv_sec += ts.tv_nsec / 1000000000;
      ts.tv_nsec = ts.tv_nsec % 1000000000;

      while ((semRet = sem_timedwait(this->pLock_, &ts)) == -1 &&
             errno == EINTR) {
        continue;
      }
      if (-1 == semRet) {
        if (ETIMEDOUT == errno) {
          ret = en_STS_TIMEOUT;
        } else {
          //_STD_ERR(errno);
        }
      } else {
        this->firstThread_ = curThread;
        ret = en_STS_SUCCESS;
      }
    }
  }

  return ret;
}

void Sync::Unlock(void) {
  if (0 != this->pLock_) {
    if (0 == this->cnt_) {
      this->firstThread_ = 0;
      sem_post(this->pLock_);
    } else {
      this->cnt_--;
    }
  } else {
    //_LOGE("handle is not created\n");
  }

  return;
}
