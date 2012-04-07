#ifndef _MEMORY_CLIENT_H
#define _MEMORY_CLIENT_H

#include "types.h"
#include "enums.h"
#include "module.h"
#include "mreq.h"
#include "protocol.h"

/** Cache states.  */
/** Note: MEM_EI State not needed since we silent evict.  */

typedef enum {
    MEM_I = 1,
    MEM_E,
    MEM_M,

    MEM_IE,
    MEM_IM,
    MEM_MI,
    MEM_CONVERT
} memory_client_state_t;

class Memory_client_protocol : public Client_protocol {
public:
    Memory_client_protocol (Hash_table *my_table, Hash_entry *my_entry, int tier);
    ~Memory_client_protocol ();

    memory_client_state_t state;
    
    void process_request (Mreq *request);
    void process_replacement (void);
    bool have_permissions_p (Mreq *request);
    bool have_data_p (Mreq *request);
    message_t get_data (void);
    void dirty_from_lower_tier();
    bool is_M_or_E();

    bool invalid_p ();
    bool transient_p ();
    void null_entry_handler (Mreq *request);

    void dump (void);

private:
    inline bool preprocess_request (Mreq *request);
    inline bool postprocess_request (Mreq *request);

    inline void do_I (Mreq *request);
    inline void do_E (Mreq *request);
    inline void do_M (Mreq *request);

    inline void do_IE (Mreq *request);
    inline void do_IM (Mreq *request);
    inline void do_MI (Mreq *request);

public:
    inline void transition_to_I (void);
    inline void transition_to_E (void);
    inline void transition_to_M (void);

private:
    inline void transition_to_IE (void);
    inline void transition_to_IM (void);
    inline void transition_to_MI (void);


    inline void issue_GETS (Mreq *request, message_t msg);
    inline void issue_GETM (Mreq *request, message_t msg);
    inline void issue_PUT (Mreq *request, message_t msg);
};

#endif // _MEMORY_CLIENT_H
