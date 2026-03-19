#include "Packet.h"

namespace qos_sim {

Packet::Packet(int cur_id, int f_id, FlowType f_type, int size, double gen_time)
    : id(cur_id), flow_id(f_id), type(f_type), size_bytes(size),
      generation_time(gen_time), enqueue_time(-1), dequeue_time(-1), 
      arrival_time(-1), is_dropped(false) {
}

} // namespace qos_sim
