#include "memory_client.h"
#include "mreq.h"
#include "sim.h"
#include "hash_table.h"

#include "sharers.h"

extern Simulator Sim;

/*************************
 * Constructor/Destructor.
 *************************/
Memory_client_protocol::Memory_client_protocol (Hash_table *my_table, Hash_entry *my_entry, int tier)
    : Client_protocol (my_table, my_entry, tier)
{
    this->state = MEM_I;
}

Memory_client_protocol::~Memory_client_protocol ()
{    
}

/** Debug dump.  */
void Memory_client_protocol::dump (void)
{
    const char *block_states[11] = {"X","I","E","M","IE","IM","MI", "MEM_CONVERT"};

    assert (this->state < 11);

    fprintf (stderr, "memory_protocol - state: %s \n", block_states[state]);
}

/*************************************
 * Check the state of the cache block.
 *************************************/
bool Memory_client_protocol::invalid_p ()
{
    return (state == MEM_I);
}

bool Memory_client_protocol::transient_p ()
{
    return (!(state == MEM_I ||
              state == MEM_E ||
              state == MEM_M));
}

/******************
 * Process Message.
 ******************/

/** Returns true if message needs no additional processing.  */
inline bool Memory_client_protocol::preprocess_request (Mreq *request)
{
    if (DIR_HISTORY)
        my_entry->add_to_entry_history (request);

    //TODO:  Need to add preprocess to convert other requests? PUTE/M to OLD?
    if (state == MEM_I && request->msg == C_GETSM)
    {
        assert(!my_entry->directory_pro->sharers->is_sharer (request->src_mid.nodeID) &&
               "How'd we get this if still on sharers list?  Didn't get a previous downgrade??");
        /** Failed the client permissions test, therefore this is from an old sharer that is now invalid.  */
        request->msg = C_GETIM;
    }

    if (TICK_TOCK_DEBUG || ADDR_DEBUG)
    {
        request->print_msg (my_table->moduleID, "  Enter --- ");
        my_entry->dump();
    }
    /** Additional processing necessary.  */
    return false;
}

/** Returns true if message needs no additional processing.  */
inline bool Memory_client_protocol::postprocess_request (Mreq *request)
{
    if (TICK_TOCK_DEBUG || ADDR_DEBUG)
    {
        request->print_msg (my_table->moduleID, "  Exit --- ");
        my_entry->dump();
    }

    /** Additional processing necessary.  */
    return false;
}

void Memory_client_protocol::process_request (Mreq *request)
{
    if (preprocess_request (request))
        return;

	switch (state) {
    case MEM_I:  do_I (request); break; 
    case MEM_E:  do_E (request); break;
    case MEM_M:  do_M (request); break;
    case MEM_IE: do_IE (request); break;
    case MEM_IM: do_IM (request); break;
    case MEM_MI: do_MI (request); break;
    default:
        fatal_error ("Memory_protocol - state not valid?\n");
    }

    if (postprocess_request (request))
        return;
}

void Memory_client_protocol::process_replacement (void)
{
    Mreq *r = new Mreq (H_REPLACEMENT, my_entry->tag);

    process_request (r);

    delete r;
}

bool Memory_client_protocol::have_permissions_p (Mreq *request)
{
    if (transient_p () || state == MEM_I)
        return false;

    /** State must be M or E.  */
    switch (request->msg) {
    case C_GETIS:
        return true;

    case C_GETSM:
    case C_GETOM:
        assert(state == MEM_E || state == MEM_M);
    case C_GETIM:
        /** If in E the client must process to transition to M.  */
        if (state == MEM_M)
            return true;
        else
            return false;

    case C_PUTE:
    case C_PUTM:
        return true;

    default:
        return false;
    }
}

bool Memory_client_protocol::have_data_p (Mreq *request)
{
    return have_permissions_p (request);
}

message_t Memory_client_protocol::get_data (void)
{
    message_t data;

    data = MREQ_INVALID;
    switch (state)
    {
    case MEM_I: 
        fatal_error ("Memory_protocol - get_data called in I state!\n");
        break;

    case MEM_E:
        data = C_DATA_E;
        break;

    case MEM_M:
        data = C_DATA_M;
        break;

    default:
        fatal_error ("Memory_protocol - get_data called in unknown state!\n");
    }

    return data;
}

void Memory_client_protocol::dirty_from_lower_tier() {
    assert (state == MEM_E || MEM_M);
    transition_to_M ();
}

bool Memory_client_protocol::is_M_or_E()
{
    return (state == MEM_M || state == MEM_E);
}

void Memory_client_protocol::null_entry_handler (Mreq *request) 
{
    fatal_error ("A memory_client should never need to use this");
    do_I (request);
    if (state != MEM_I)
        fatal_error ("MEM Null Entry not in I state!\n");

}

/*************************
 * State Transition Logic
 *************************/

void inline Memory_client_protocol::transition_to_I (void) { state = MEM_I; }
void inline Memory_client_protocol::transition_to_E (void) { state = MEM_E; }
void inline Memory_client_protocol::transition_to_M (void) { state = MEM_M; }
void inline Memory_client_protocol::transition_to_IE (void) { state = MEM_IE; }
void inline Memory_client_protocol::transition_to_IM (void) { state = MEM_IM; }
void inline Memory_client_protocol::transition_to_MI (void) { state = MEM_MI; }

inline void Memory_client_protocol::do_I (Mreq *request)
{
    switch (request->msg) {
    case C_GETIS:
        /** Will get upgraded on the dir side of the protocol.  */
        issue_GETS (request, C_GETIS);
        transition_to_IE ();
        break;
        
    case C_GETIM:
        issue_GETM (request, C_GETIM);
        transition_to_IM ();
        break;
        
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Cache: I state shouldn't see this message\n");
    }
}

inline void Memory_client_protocol::do_E (Mreq *request)
{
    switch (request->msg) {
    case C_GETIS:
        assert(0 && "this valid?  permissions should have checked out...");
        break;
        
    case C_GETIM:
    case C_GETOM:
    case C_GETSM:
        transition_to_M ();
        break;
        
    case H_REPLACEMENT:
        //Silent eviction is allowed since dir-side of protocol (MC) doesn't track anything
        transition_to_I ();
        break;
        
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Cache: E state shouldn't see this message\n");
    }
}

inline void Memory_client_protocol::do_M (Mreq *request)
{
    switch (request->msg) {
    case C_GETIS:
        assert(0 && "this valid?  permissions should have checked out...");
        break;
        
    case C_GETIM:
    case C_GETOM:
    case C_GETSM:
        assert(0 && "this valid?  permissions should have checked out...");
        break;
        
    case H_REPLACEMENT:
        issue_PUT (request, C_PUTM);
        transition_to_MI ();
        my_table->stats->writebacks++;
        break;
        
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Cache: M state shouldn't see this message\n");
    }
}

inline void Memory_client_protocol::do_IE (Mreq *request)
{
    switch (request->msg) {
    case C_DATA_E:
        transition_to_E ();
        break;

    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Cache: IE state shouldn't see this message\n");
    }
}

inline void Memory_client_protocol::do_IM (Mreq *request)
{
    switch (request->msg) {
    case C_DATA_M:
        transition_to_M ();
        break;

    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Cache: IM state shouldn't see this message\n");
    }
}

/** Replacement issued while in M state.  Now in MI waiting for WB_ACK.  */
inline void Memory_client_protocol::do_MI (Mreq *request)
{
    switch (request->msg) {
    case D_WB_ACK:
        transition_to_I ();
        break;

    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Cache: IS state shouldn't see this message\n");
    }
}

//TODO - refactor all of this into protocol.cpp as helpers instead of the massive per protocol replication!

/** Send GETS to home node directory.  */ 
void inline Memory_client_protocol::issue_GETS (Mreq *request, message_t msg)
{
    Mreq *gets; 

    gets = new Mreq (msg, request->addr, request->preq, my_table->moduleID, my_dirID);
    my_table->write_output_port (gets, Global_Clock + my_table->lookup_time);
}

/** Send GETM to home node directory.  */
void inline Memory_client_protocol::issue_GETM (Mreq *request, message_t msg)
{
    assert (state != MEM_M && "Shouldn't issue a GETM to the network if state == M");

    /** Send GETM to home node directory.  */ 
    Mreq *getm;

    getm = new Mreq (msg, request->addr, request->preq, my_table->moduleID, my_dirID);
    my_table->write_output_port (getm, Global_Clock + my_table->lookup_time);
}

/** Send PUTM to home node directory.  */
void inline Memory_client_protocol::issue_PUT (Mreq *request, message_t msg)
{
    Mreq *putx;
    assert (msg == C_PUTM || msg == C_PUTE);

    putx = new Mreq (msg, request->addr, request->preq, my_table->moduleID, my_dirID);
    my_table->write_output_port (putx, Global_Clock + my_table->lookup_time);
}
