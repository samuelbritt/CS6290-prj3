#include "MSI_protocol.h"
#include "error_handler.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

#include <stdio.h>

extern Simulator *Sim;
static const char *MSI_block_states[] = {"X", "I", "S", "M", "IM", "IS"};

/*************************
 * Constructor/Destructor.
 *************************/
MSI_protocol::MSI_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
	this->state = MSI_CACHE_I;
	this->name = "MSI_protocol";
	this->block_states = MSI_block_states;
}

MSI_protocol::~MSI_protocol ()
{
}

const char *MSI_protocol::get_state_str()
{
	return this->block_states[this->state];
}

void MSI_protocol::dump (void)
{
    fprintf (stderr, "%s - state: %s\n", this->name, this->get_state_str());
}

void MSI_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
		case MSI_CACHE_I: do_cache_I (request); break;
		case MSI_CACHE_S: do_cache_S (request); break;
		case MSI_CACHE_M: do_cache_M (request); break;
		case MSI_CACHE_IM: do_snoop_IM (request); break;
		case MSI_CACHE_IS: do_snoop_IS (request); break;
		default: error_handler->invalid_state_error();
    }
}

void MSI_protocol::process_snoop_request (Mreq *request)
{
	switch (state) {
		case MSI_CACHE_I: do_snoop_I (request); break;
		case MSI_CACHE_S: do_snoop_S (request); break;
		case MSI_CACHE_M: do_snoop_M (request); break;
		case MSI_CACHE_IM: do_snoop_IM (request); break;
		case MSI_CACHE_IS: do_snoop_IS (request); break;
		default: error_handler->invalid_state_error();
	}
}

inline void MSI_protocol::do_cache_I (Mreq *request)
{
	switch (request->msg) {
		case LOAD:
			send_GETS(request->addr);
			state = MSI_CACHE_IS;
			Sim->cache_misses++;
			break;
		case STORE:
			send_GETM(request->addr);
			state = MSI_CACHE_IM;
			Sim->cache_misses++;
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MSI_protocol::do_cache_S (Mreq *request)
{
	switch (request->msg) {
		case LOAD:
			send_DATA_to_proc(request->addr);
			break;
		case STORE:
			send_GETM(request->addr);
			state = MSI_CACHE_IM;
			Sim->cache_misses++;
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MSI_protocol::do_cache_M (Mreq *request)
{
	switch (request->msg) {
		case LOAD:
		case STORE:
			send_DATA_to_proc(request->addr);
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MSI_protocol::do_cache_IM (Mreq *request)
{
	switch (request->msg) {
		case LOAD:
		case STORE:
			error_handler->multiple_requests_error(request);
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MSI_protocol::do_cache_IS (Mreq *request)
{
	do_cache_IM(request);
}

inline void MSI_protocol::do_snoop_I (Mreq *request)
{
	switch (request->msg) {
		case GETS:
		case GETM:
		case DATA: break;
		default: error_handler->invalid_request_error(request);
	}
}

inline void MSI_protocol::do_snoop_S (Mreq *request)
{
	switch (request->msg) {
		case GETS:
			break;
		case GETM:
			state = MSI_CACHE_I;
			break;
		case DATA:
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MSI_protocol::do_snoop_M (Mreq *request)
{
	switch (request->msg) {
		case GETS:
			set_shared_line();
			send_DATA_on_bus(request->addr, request->src_mid);
			state = MSI_CACHE_S;
			break;
		case GETM:
			set_shared_line();
			send_DATA_on_bus(request->addr, request->src_mid);
			state = MSI_CACHE_I;
			break;
		case DATA:
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MSI_protocol::do_snoop_IM (Mreq *request)
{
	switch (request->msg) {
		case GETS:
		case GETM:
			break;
		case DATA:
			send_DATA_to_proc(request->addr);
			state = MSI_CACHE_M;
			break;
		default:
			error_handler->invalid_request_error(request);
	}

}

inline void MSI_protocol::do_snoop_IS (Mreq *request)
{
	switch (request->msg) {
		case GETS:
		case GETM:
			break;
		case DATA:
			send_DATA_to_proc(request->addr);
			state = MSI_CACHE_S;
			break;
		default:
			error_handler->invalid_request_error(request);
	}

}
