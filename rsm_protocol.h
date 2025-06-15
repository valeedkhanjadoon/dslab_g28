#ifndef rsm_protocol_h
#define rsm_protocol_h

#include "rpc.h"


class rsm_client_protocol {
 public:
  enum xxstatus { OK, ERR, NOTPRIMARY, BUSY};
  typedef int status;
  enum rpc_numbers {
    invoke = 0x9001,
    members,
  };
};


struct viewstamp {
  viewstamp (unsigned int _vid = 0, unsigned int _seqno = 0) {
    vid = _vid;
    seqno = _seqno;
  };
  unsigned int vid;
  unsigned int seqno;
};

class rsm_protocol {
 public:
  enum xxstatus { OK, ERR, BUSY};
  typedef int status;
  enum rpc_numbers {
    invoke = 0x10001,
    transferreq,
    transferdonereq,
    joinreq,
  };

  struct transferres {
    std::string state;
    viewstamp last;
  };
  
  struct joinres {
    std::string log;
  };
};

inline bool operator==(viewstamp a, viewstamp b) {
  return a.vid == b.vid && a.seqno == b.seqno;
}

inline bool operator>(viewstamp a, viewstamp b) {
  return (a.vid > b.vid) || ((a.vid == b.vid) && a.seqno > b.seqno);
}

inline bool operator!=(viewstamp a, viewstamp b) {
  return a.vid != b.vid || a.seqno != b.seqno;
}

inline marshall& operator<<(marshall &m, viewstamp v)
{
  m << v.vid;
  m << v.seqno;
  return m;
}

inline unmarshall& operator>>(unmarshall &u, viewstamp &v) {
  u >> v.vid;
  u >> v.seqno;
  return u;
}

// Add this overload for marshall to handle size_t
inline marshall& operator<<(marshall &m, size_t s) {
  m << static_cast<uint64_t>(s); // Convert size_t to uint64_t for serialization
  return m;
}

inline unmarshall& operator>>(unmarshall &u, size_t &s) {
  uint64_t temp;
  u >> temp; // Deserialize into a temporary uint64_t
  s = static_cast<size_t>(temp); // Convert back to size_t
  return u;
}

inline marshall &
operator<<(marshall &m, rsm_protocol::transferres r)
{
  m << r.state;
  m << r.last;
  return m;
}

inline unmarshall &
operator>>(unmarshall &u, rsm_protocol::transferres &r)
{
  u >> r.state;
  u >> r.last;
  return u;
}

inline marshall &
operator<<(marshall &m, rsm_protocol::joinres r)
{
  m << r.log;
  return m;
}

inline unmarshall &
operator>>(unmarshall &u, rsm_protocol::joinres &r)
{
  u >> r.log;
  return u;
}

class rsm_test_protocol {
 public:
  enum xxstatus { OK, ERR};
  typedef int status;
  enum rpc_numbers {
    net_repair = 0x12001,
    breakpoint = 0x12002,
  };
};

#endif 
