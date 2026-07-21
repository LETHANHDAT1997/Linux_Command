#ifndef EV_HANDLER_H
#define EV_HANDLER_H

#include <set>

enum class EvType 
{
  eEvIn,
  eEvOut,
  eEvPri,
  eEvErr,
  eEvUnknow,
};


class EvHandlerBase 
{
public:

  EvHandlerBase() : m_fd(-1), m_evs() {};
  virtual ~EvHandlerBase() {};
  int GetFd(void) { return m_fd; }

  bool GetEvs(std::set<EvType> *events) 
  {
    bool ret = false;
    if (nullptr != events) 
    {
      *events = m_evs;
      ret = true;
    }
    return ret;
  }

  bool SetEvInfo(int fd, const std::set<EvType> &evtypes) 
  {
    if (0 > fd) 
    {
      return false;
    }

    m_fd = fd;
    m_evs = evtypes;
    return true;
  };

  virtual void EvHandler(const std::set<EvType> &events) = 0;

private:
  int m_fd;               
  std::set<EvType> m_evs; 
};


template <typename T> class EvHandler : public EvHandlerBase 
{
public:
  using CALLBACK_FUNC_t = void (T::*)(const std::set<EvType> &); 
  EvHandler(T *obj, CALLBACK_FUNC_t callback): EvHandlerBase(), m_obj(obj), m_callback(callback) {};
  virtual ~EvHandler() 
  {
    m_callback = nullptr;
    m_obj = nullptr;
  };
  void EvHandler(const std::set<EvType> &events) override 
  {
    (m_obj->*m_callback)(events);
  };

private:
  T *m_obj;                   
  CALLBACK_FUNC_t m_callback; 
};

#endif
