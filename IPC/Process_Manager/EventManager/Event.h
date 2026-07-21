#ifndef _EVENT_H
#define _EVENT_H

class Event 
{
public:
  typedef enum 
  {
    en_STS_SUCCESS = 0,
    en_STS_TIMEOUT = -2,
    en_STS_ERR_OTHER = -3,
  } EN_STS;

  static constexpr int C_TIME_INFINITE = -1;

  Event();

  ~Event();

  bool SetEvent(void);

  bool ResetEvent(void);

  int WaitEvent(int timeout);

  static int WaitMultipleEvent(unsigned char count, Event *events[], int timeout, bool all = false);

  Event(const Event &rhs) = delete;
  Event &operator=(const Event &rhs) = delete;

private:

  static int timedMultiWait(unsigned char count, Event *handles[], bool all, int milliseconds);

  bool isInitialized(void);

  int handle_[2];
};

#endif
