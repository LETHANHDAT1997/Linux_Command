
#include "Event.h"

#include <cerrno>
#include <csignal>
#include <cstring>

#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

// #include "Debug.h"

static constexpr int READ_FD = 0;
static constexpr int WRITE_FD = 1;
static constexpr int WE_PARAM = -1;
static constexpr int WE_TIMEDOUT = -2;
static constexpr int WE_OTHER = -3;
static constexpr unsigned int WAIT_EV_MAX = 10;
static bool isSigSet_ = false;

namespace {

int EpollSetup(int *events, unsigned char cnt) {
  int fd = -1;
  struct epoll_event event;

  if ((fd = epoll_create1(0)) < 0) {
    //_STD_ERR(errno);
    return -1;
  }

  for (unsigned char i = 0; i < cnt; ++i) {
    if (-1 == events[i]) {
      //_LOGE("handle error\n");
      close(fd);
      return -1;
    }

    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN;
    event.data.fd = events[i];
    if (epoll_ctl(fd, EPOLL_CTL_ADD, events[i], &event) != 0) {
      //_STD_ERR(errno);
      close(fd);
      return -1;
    }
  }

  return fd;
}

bool EpollReceivedEvDelete(int epoll_fd, struct epoll_event *rcv_events,
                           int rcv_ev_cnt) {
  for (int i = 0; i < rcv_ev_cnt; ++i) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, rcv_events[i].data.fd, 0) != 0) {
      return false;
    }
  }

  return true;
}

int GetInEventIdx(int fd, int *event_array, unsigned char count) {
  int ret = WE_OTHER;

  for (unsigned char j = 0; j < count; ++j) {
    if (fd == event_array[j]) {
      ret = j;
      break;
    }
  }

  return ret;
}

}; // namespace

Event::Event() {
  this->handle_[READ_FD] = -1;
  this->handle_[WRITE_FD] = -1;

  if (0 == pipe(this->handle_)) {
    int ret = fcntl(this->handle_[WRITE_FD], F_SETFL, O_NONBLOCK);
    if (0 > ret) {
      //_STD_ERR(errno);
    }
  } else {
    //_STD_ERR(errno);
  }
}

Event::~Event() {
  int result = 0;

  if (this->isInitialized()) {
    result = close(this->handle_[READ_FD]);
    result += close(this->handle_[WRITE_FD]);

    if (result) {
      //_LOGE("close failed.\n");
    }
  }
}

bool Event::SetEvent(void) 
{
  bool result = false;
  ssize_t ret = -1;
  unsigned char dummy = 1;

  if (sizeof(dummy) != (ret = write(this->handle_[WRITE_FD], &(dummy), sizeof(dummy)))) 
  {
    if ((ret == -1) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) 
    {
      //_LOGW("event write retry.\n");
      this->ResetEvent();
      ret = write(this->handle_[WRITE_FD], &(dummy), sizeof(dummy));
    } 
    else if ((ret == -1) && (errno == EPIPE)) 
    {
      //_STD_ERR(errno);
    }
    result = false;
  } 
  else 
  {
    result = true;
  }

  return result;
}

bool Event::ResetEvent(void) {
  int result = 0;
  Event *events[] = {this};

  while (1) {
    result = Event::timedMultiWait(1, events, true, 0);
    if (0 > result) {
      break;
    }
  }
  return true;
}

int Event::WaitEvent(int timeout) {
  int wevent_ret = WE_OTHER;
  int ret = en_STS_ERR_OTHER;
  Event *events[] = {this};

  wevent_ret = Event::timedMultiWait(1, events, true, timeout);

  switch (wevent_ret) {
  case 0:
    ret = en_STS_SUCCESS;
    break;
  case WE_TIMEDOUT:
    ret = en_STS_TIMEOUT;
    break;
  default:
    ret = en_STS_ERR_OTHER;
    break;
  }
  return ret;
}

int Event::WaitMultipleEvent(unsigned char count, Event *events[], int timeout,
                             bool all) {
  int wevent_ret = WE_OTHER;
  int ret = en_STS_ERR_OTHER;

  if (0 == events) {
    return en_STS_ERR_OTHER;
  }

  wevent_ret = Event::timedMultiWait(count, events, all, timeout);

  if (all == true) {
    switch (wevent_ret) {
    case 0:
      ret = en_STS_SUCCESS;
      break;
    case WE_TIMEDOUT:
      ret = en_STS_TIMEOUT;
      break;
    default:
      ret = en_STS_ERR_OTHER;
      break;
    }
  } else {
    switch (wevent_ret) {
    case WE_PARAM:
      ret = en_STS_ERR_OTHER;
      break;
    case WE_OTHER:
      ret = en_STS_ERR_OTHER;
      break;
    case WE_TIMEDOUT:
      ret = en_STS_TIMEOUT;
      break;
    default:
      ret = wevent_ret;
      break;
    }
  }

  return ret;
}

int Event::timedMultiWait(unsigned char count, Event *handles[], bool all,
                          int milliseconds) {
  if (false == isSigSet_) {
    signal(SIGPIPE, SIG_IGN);
    isSigSet_ = true;
  }

  int event_array[64];
  int epoll_fd = -1;
  int nfds = -1;
  struct epoll_event rcv_events[WAIT_EV_MAX];
  unsigned char dummy_read = 0;

  if ((0 == count) || (WAIT_EV_MAX < count) || (NULL == handles)) {
    return WE_PARAM;
  }

  for (unsigned char i = 0; i < count; ++i) {
    event_array[i] = handles[i]->handle_[READ_FD];
  }

  if (0 > (epoll_fd = EpollSetup(event_array, count))) {
    return WE_OTHER;
  }

  int result = WE_OTHER;
  bool continue_flag = true;

  while (continue_flag) {
    nfds =
        epoll_wait(epoll_fd, rcv_events, static_cast<int>(count), milliseconds);
    if (nfds > 0) {
      for (int i = 0; i < nfds; ++i) {
        if (sizeof(dummy_read) !=
            read(rcv_events[i].data.fd, &(dummy_read), sizeof(dummy_read))) {
          //_LOGE("read error.\n");
          result = WE_OTHER;
          continue_flag = false;
          close(epoll_fd);
          return result;
        }

        if (false == all) {
          result = GetInEventIdx(rcv_events[i].data.fd, event_array, count);
          break;
        }
      }

      if (false == all) {
        continue_flag = false;
        break;
      }

      int tmp_count = static_cast<int>(count);
      tmp_count -= nfds;
      count = static_cast<unsigned char>(tmp_count);
      if (count == 0) {
        result = 0;
        continue_flag = false;
        break;
      }

      if (false == EpollReceivedEvDelete(epoll_fd, rcv_events, nfds)) {
        result = WE_TIMEDOUT;
        continue_flag = false;
        break;
      }
    } else if (nfds == 0) {
      result = WE_TIMEDOUT;
      continue_flag = false;
    } else {
      if (EINTR == errno) {
        // continue
      } else {
        //_STD_ERR(errno);
        result = WE_OTHER;
        continue_flag = false;
        close(epoll_fd);
        return result;
      }
    }
  }

  close(epoll_fd);

  return result;
}

bool Event::isInitialized(void) {
  bool ret = false;

  if ((-1 != this->handle_[READ_FD]) && (-1 != this->handle_[WRITE_FD])) {
    ret = true;
  }

  return ret;
}
