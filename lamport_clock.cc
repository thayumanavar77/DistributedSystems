/* TBD write copyright notice */

#include <iostream>
#include <algorithm>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <map>
#include <memory>
#include <functional>
#include <cstdio>
#include <cstdarg>

// some routines to aid in debugging.
#ifdef DEBUG

#define DPRN(...) debug_prn(__FILE__, __LINE__, __func__, __VA_ARGS__)
void
debug_prn(const char *file, int line, const char *function,
          const char *msg, ...)
{
  va_list args;

  fprintf(stdout, "%s:%d  %s: ", file, line, function);
  va_start(args, msg);
  vprintf(msg, args);
  fprintf(stdout, "\n");
  va_end(args);
}
#else 
#define DPRN(...) 
#endif


struct Lamport_clock {
  int m_pid;
  int m_lamport_timestamp;
};

template<typename T>
class Blocking_queue
{
private:
  std::queue<T> m_queue;
  std::mutex m_mutex;
  std::condition_variable m_cv;
public:
  T pop()
  {
    DPRN("Pop entered in queue %p", this); 
    std::unique_lock<std::mutex> lock(m_mutex);
    while(m_queue.empty())
      m_cv.wait(lock);

    auto qelement= m_queue.front();
    m_queue.pop();
    DPRN("Popped element from queue %p", this);
    return qelement;
  }

  void push(const T& qelement)
  {
    DPRN("Pushed element in queue %p", this);
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.push(qelement);
    lock.unlock();
    m_cv.notify_one();
  } 
};




struct Channels{
  std::shared_ptr< Blocking_queue<Lamport_clock> > m_tx_channel;
  std::shared_ptr< Blocking_queue<Lamport_clock> > m_rx_channel;

  Channels()
  {}
  Channels(std::shared_ptr< Blocking_queue<Lamport_clock> > tx_channel,
           std::shared_ptr< Blocking_queue<Lamport_clock> > rx_channel)
          : m_tx_channel(tx_channel),
            m_rx_channel(rx_channel)
  {}
  

};

int global_event_counter;
class Event;

struct Pair_comparator {
  bool operator()(const std::pair<int,int>& p1, const std::pair<int,int>& p2) const
  {
    return p1.first < p2.first;
  }
};

class Process {
private:
  Lamport_clock m_lc;
  std::map< std::pair<int,int>, Channels> m_comm_channel_map;
#if 0
       std::function<bool(std::pair<int,int>&, std::pair<int,int>&)> > m_comm_channel_map(
              [] (const std::pair<int, int>& p1,
                  const std::pair<int, int>& p2)
                { return p1.first < p2.first });
#endif

  std::queue< std::shared_ptr<Event> > m_execute_events;
public:
  Process(int pid)
  {
    m_lc.m_pid= pid;
    m_lc.m_lamport_timestamp= 0; 
  }

  int get_pid()
  {
    return m_lc.m_pid;
  }

  void set_comm_channel(int c, Channels ch)
  {
    m_comm_channel_map.insert(std::make_pair(std::pair<int,int>(m_lc.m_pid,c),
                                             ch));
    DPRN("Set receive channel %p and send channel %p for process pair (%d,%d)", 
            ch.m_tx_channel.get(), ch.m_rx_channel.get(), m_lc.m_pid, c);
  }

  void push_execute_event(std::shared_ptr<Event> e)
  {
    m_execute_events.push(e);
  }
  
  bool has_events()
  {
    return !m_execute_events.empty();
  }

  std::shared_ptr<Event> get_event()
  {
    std::shared_ptr<Event> e= m_execute_events.front();
    m_execute_events.pop();

    return e;
  }

  void send_event(int j)
  {
    trigger_event();
    DPRN("TS comm channel (%d,%d) (%p, %p)", m_lc.m_pid,j,
         m_comm_channel_map[std::pair<int,int>(m_lc.m_pid,j)].m_tx_channel.get(),
         m_comm_channel_map[std::pair<int,int>(m_lc.m_pid,j)].m_rx_channel.get());

    m_comm_channel_map[std::pair<int,int>(m_lc.m_pid,j)].m_tx_channel->push(m_lc);
  }


  void receive_event(int j)
  {
    DPRN("TR comm channel (%d,%d) (%p, %p)", m_lc.m_pid,j,
         m_comm_channel_map[std::pair<int,int>(m_lc.m_pid,j)].m_tx_channel.get(),
         m_comm_channel_map[std::pair<int,int>(m_lc.m_pid,j)].m_rx_channel.get());

    Lamport_clock sender_clock=  m_comm_channel_map[std::make_pair(m_lc.m_pid,j)].m_rx_channel->pop();
    
    m_lc.m_lamport_timestamp= std::max(sender_clock.m_lamport_timestamp, 
                                       m_lc.m_lamport_timestamp);
    trigger_event();
  }


  void trigger_event()
  {
    global_event_counter++;
    m_lc.m_lamport_timestamp++;
    std::cout<<" Process : " <<m_lc.m_pid<<"   "<<m_lc.m_lamport_timestamp<<std::endl;
  }

};
class Event {
public:
  virtual void execute_event(std::shared_ptr<Process> p) = 0;
  virtual ~Event()
  {}
  
};
class Instr_event : public Event
{
public:
  virtual void execute_event(std::shared_ptr<Process> p)
  {
    DPRN("Instruction Event");
    p->trigger_event();
  }

  ~Instr_event()
  {}
};
class Send_event : public Event
{
private:
  int m_recv_id;
public:
  Send_event(int recv_id) : m_recv_id(recv_id) 
  { }

  virtual void execute_event(std::shared_ptr<Process> p)
  {
    DPRN("Send Event to process %d", m_recv_id);
    p->send_event(m_recv_id);
  }

  ~Send_event()
  {}
};

class Received_event : public Event
{
private:
  int m_sender_id;
public:
  Received_event(int sender_id) : m_sender_id(sender_id)
  { }

  virtual void execute_event(std::shared_ptr<Process> p)
  {
    DPRN("Receive event from process %d", m_sender_id);
    p->receive_event(m_sender_id);
  } 

 ~Received_event()
 {} 
};

void process_event_execute_fn(std::shared_ptr<Process> p)
{
  while(p->has_events())
  {
    std::shared_ptr<Event> e= p->get_event();
    DPRN("Executing event from process: %d", p->get_pid());
    e->execute_event(p);
  };
}

int
main(int argc, char **argv)
{
  int num_process;
  std::cin>>num_process;

  std::thread t[num_process];
  std::shared_ptr<Process> p[num_process];
  for (int i= 0; i < num_process; i++)
  {
    p[i]= std::shared_ptr<Process>(new Process(i));
  }

  for(int i= 0; i < num_process; i++)
  {
    int num_events;
    std::cin>>num_events;
    for(int j= 0; j < num_events; j++)
    {
      char event_type;
      std::cin>>event_type;
      if (event_type == 'I')
        p[i]->push_execute_event(std::shared_ptr<Event>(new Instr_event()));

      else if (event_type == 'S')
      {
        int recv_id;
        std::cin>>recv_id;
        p[i]->push_execute_event(std::shared_ptr<Event>(new Send_event(recv_id)));

      }
      else if (event_type == 'R')
      {
        int sender_id;
        std::cin>>sender_id;
        p[i]->push_execute_event(std::shared_ptr<Event>(new Received_event(sender_id)));
      }
    }
  }

  // Allocate memory for message queue. Each process has a pair of transmit
  // and receive queues to every other process (even including itself). The
  // transmit queue for process i to process j is the receive queue
  for(int i= 0; i < num_process; i++)
  {
    for(int j= i+1; j < num_process; j++)
    {
      std::shared_ptr< Blocking_queue<Lamport_clock> > q1=
          std::shared_ptr< Blocking_queue<Lamport_clock> >(new Blocking_queue<Lamport_clock>);
      std::shared_ptr< Blocking_queue<Lamport_clock> > q2=
          std::shared_ptr< Blocking_queue<Lamport_clock> >(new Blocking_queue<Lamport_clock>);
    
      DPRN("Comm channels for process %d and %d : %p and %p",i,j, q1.get(), q2.get());
      p[i]->set_comm_channel(j,Channels(q1,q2));
      p[j]->set_comm_channel(i,Channels(q2,q1));
    }
  }

  for(int i= 0; i < num_process; i++)
    t[i]= std::thread(&process_event_execute_fn,p[i]);

  for(int i= 0; i < num_process; ++i)
    t[i].join();
  
  return 0;
}  
