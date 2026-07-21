#include "EvLoop.h"
#include <cstdio>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace 
{
constexpr int C_EV_MAX = 10;
}

namespace 
{

bool ConvEvtypeToPublic(uint32_t ev, std::set<EvType> *pub_events) 
{
  if (nullptr == pub_events) 
  {
    return false;
  }

  if (ev & EPOLLIN) 
  {
    pub_events->insert(EvType::eEvIn);
  }

  if (ev & EPOLLOUT) 
  {
    pub_events->insert(EvType::eEvOut);
  }

  if (ev & EPOLLPRI) 
  {
    pub_events->insert(EvType::eEvPri);
  }

  if (ev & EPOLLERR) 
  {
    pub_events->insert(EvType::eEvErr);
  }

  return true;
}

static uint32_t ConvEvtypeToPrivate(const std::set<EvType> &pub_events) 
{
  uint32_t ev = 0;
  for (auto it = pub_events.begin(); it != pub_events.end(); ++it) 
  {
    switch (*it) 
    {
    case EvType::eEvIn:
      ev |= EPOLLIN;
      break;
    case EvType::eEvOut:
      ev |= EPOLLOUT;
      break;
    case EvType::eEvPri:
      ev |= EPOLLPRI;
      break;
    case EvType::eEvErr:
      ev |= EPOLLERR;
      break;
    default:
      break;
    }
  }
  return ev;
}

} // namespace

EvLoop::EvLoop()
: m_is_init_fail(false), m_epfd(-1), m_evd(-1), m_ev_map(), m_sync() 
{
  if (0 > (m_epfd = epoll_create(C_EV_MAX))) 
  {
    m_is_init_fail = true;
    //_STD_ERR(errno);
    return;
  }

  if (0 > (m_evd = eventfd(0, 0))) 
  {
    m_is_init_fail = true;
    //_STD_ERR(errno);
    return;
  }

  if (false == CommonAddEv(m_evd, EPOLLIN)) 
  {
    m_is_init_fail = true;
    //_LOGE("add exit event failed.\n");
  }
}

EvLoop::~EvLoop() 
{
  if (-1 != m_evd) 
  {
    if (0 > close(m_evd)) 
    {
      //_STD_ERR(errno);
    }
  }
  m_evd = -1;

  if (-1 != m_epfd) 
  {
    if (0 > close(m_epfd)) 
    {
      //_STD_ERR(errno);
    }
  }
  m_epfd = -1;
}

bool EvLoop::AddEvHandler(EvHandlerBase *ev_obj) 
{
  if (true == m_is_init_fail) 
  {
    //_LOGE("Init fail occurred.");
    return false;
  }

  if (nullptr == ev_obj) 
  {
    //_LOGE("Event handler object is null.");
    return false;
  }

  int fd = ev_obj->GetFd();
  std::set<EvType> events;
  if (false == ev_obj->GetEvs(&events)) 
  {
    //_LOGE("cann't get events.\n");
    return false;
  }

  SyncAuto syncauto(&m_sync);

  if (m_ev_map.find(fd) != m_ev_map.end()) 
  {
    //_LOGE("already added hdl.\n");
    return false;
  }

  if (false == CommonAddEv(fd, ConvEvtypeToPrivate(events))) 
  {
    //_LOGE("add event failed.\n");
    return false;
  }

  m_ev_map[fd] = ev_obj;

  return true;
}

bool EvLoop::EvLoop(void) 
{
  if (true == m_is_init_fail) 
  {
    //_LOGE("Init fail occurred.");
    return false;
  }

  bool ret = false;
  struct epoll_event rev[C_EV_MAX];
  bool is_loop_run = true;
  while (is_loop_run) 
  {
    int n_ev = epoll_wait(m_epfd, rev, C_EV_MAX, -1);

    if (0 > n_ev) 
    {
      //_STD_ERR(errno);
      is_loop_run = false;
      break;
    }

    for (int n = 0; n_ev > n; n++) 
    {
      m_sync.Lock();
      auto it = m_ev_map.find(rev[n].data.fd);
      if (m_ev_map.end() != it) 
      {
        std::set<EvType> pub_events;
        if (true == ConvEvtypeToPublic(rev[n].events, &pub_events)) 
        {
          if (!pub_events.empty()) 
          {
            it->second->EvHandler(pub_events);
          }
        }
      }
      m_sync.Unlock();

      if ((rev[n].data.fd == m_evd) && (rev[n].events & EPOLLIN)) 
      {
        eventfd_t dummy = 0;
        eventfd_read(m_evd, &dummy);
        is_loop_run = false;
        ret = true;
      }
    }
  }

  //_LOGD("exit event loop.\n");
  return ret;
}

bool EvLoop::ExitEvLoop(void) 
{
  if (true == m_is_init_fail) 
  {
    //_LOGE("Init fail occurred.");
    return false;
  }

  if (0 > m_evd) 
  {
    //_LOGE("Event obj not open.");
    return false;
  }

  if (0 > eventfd_write(m_evd, 1)) 
  {
    //_STD_ERR(errno);
    return false;
  }

  return true;
}

bool EvLoop::CommonAddEv(int fd, uint32_t ev) 
{
  struct epoll_event epev;
  memset(&epev, 0, sizeof(epev));
  epev.events = ev;
  epev.data.fd = fd;
  if (0 > epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &epev)) 
  {
    //_STD_ERR(errno);
    return false;
  }
  return true;
}
