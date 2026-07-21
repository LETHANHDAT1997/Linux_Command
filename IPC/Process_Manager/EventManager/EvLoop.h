
#ifndef EV_LOOP_H
#define EV_LOOP_H

#include <cstdint>
#include <unordered_map>

#include "EvHandler.h"
#include "Sync.h"

class EvLoop 
{
public:
  EvLoop();
  virtual ~EvLoop();
  bool AddEvHandler(EvHandlerBase *ev_obj);
  bool EvLoop(void);
  bool ExitEvLoop(void);
  EvLoop(const EvLoop &rhs) = delete;
  EvLoop &operator=(const EvLoop &rhs) = delete;

private:
  bool CommonAddEv(int fd, uint32_t ev);
  bool m_is_init_fail; 
  int m_epfd;          
  int m_evd;           
  std::unordered_map<int, EvHandlerBase *> m_ev_map; 
  Sync m_sync; 
};

#endif 
