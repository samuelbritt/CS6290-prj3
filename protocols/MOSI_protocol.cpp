#include "MOSI_protocol.h"
#include "error_handler.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

#include <stdio.h>

extern Simulator *Sim;
static const char *MOSI_block_states[] = {"X","I","S","O","M", "IM", "IS"};

/*************************
 * Constructor/Destructor.
 *************************/
MOSI_protocol::MOSI_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
	this->state = MOSI_CACHE_I;
	this->name = "MOSI_protocol";
	this->block_states = MOSI_block_states;
}

MOSI_protocol::~MOSI_protocol ()
{
}

const char *MOSI_protocol::get_state_str()
{
	return this->block_states[this->state];
}

void MOSI_protocol::dump (void)
{
    fprintf (stderr, "%s - state: %s\n", this->name, this->get_state_str());
}

void MOSI_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
		case MOSI_CACHE_I: do_cache_I (request); break;
		case MOSI_CACHE_S: do_cache_S (request); break;
		case MOSI_CACHE_O: do_cache_O (request); break;
		case MOSI_CACHE_M: do_cache_M (request); break;
		case MOSI_CACHE_IM: do_cache_IM (request); break;
		case MOSI_CACHE_IS: do_cache_IS (request); break;
		default: error_handler->invalid_state_error();
	}
}

void MOSI_protocol::process_snoop_request (Mreq *request)
{
	switch (state) {
		case MOSI_CACHE_I: do_snoop_I (request); break;
		case MOSI_CACHE_S: do_snoop_S (request); break;
		case MOSI_CACHE_O: do_snoop_O (request); break;
		case MOSI_CACHE_M: do_snoop_M (request); break;
		case MOSI_CACHE_IM: do_snoop_IM (request); break;
		case MOSI_CACHE_IS: do_snoop_IS (request); break;
		default: error_handler->invalid_state_error();
	}
}

inline void MOSI_protocol::do_cache_I (Mreq *request)
{
	switch (request->msg) {
		case LOAD:
			send_GETS(request->addr);
			state = MOSI_CACHE_IS;
			Sim->cache_misses++;
			break;
		case STORE:
			send_GETM(request->addr);
			state = MOSI_CACHE_IM;
			Sim->cache_misses++;
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MOSI_protocol::do_cache_S (Mreq *request)
{
	switch (request->msg) {
		case LOAD:
			send_DATA_to_proc(request->addr);
			break;
		case STORE:
			send_GETM(request->addr);
			state = MOSI_CACHE_IM;
			Sim->cache_misses++;
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MOSI_protocol::do_cache_O (Mreq *request)
{
	do_cache_S(request);
}

inline void MOSI_protocol::do_cache_M (Mreq *request)
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

inline void MOSI_protocol::do_cache_IM (Mreq *request)
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

inline void MOSI_protocol::do_cache_IS (Mreq *request)
{
	do_cache_IM(request);
}

inline void MOSI_protocol::do_snoop_I (Mreq *request)
{
	switch (request->msg) {
		case GETS:
		case GETM:
		case DATA: break;
		default: error_handler->invalid_request_error(request);
	}
}

inline void MOSI_protocol::do_snoop_S (Mreq *request)
{
	switch (request->msg) {
		case GETS:
			break;
		case GETM:
			state = MOSI_CACHE_I;
			break;
		case DATA:
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MOSI_protocol::do_snoop_O (Mreq *request)
{
	switch (request->msg) {
		case GETS:
			set_shared_line();
			send_DATA_on_bus(request->addr, request->src_mid);
			break;
		case GETM:
			set_shared_line();
			send_DATA_on_bus(request->addr, request->src_mid);
			state = MOSI_CACHE_I;
			break;
		case DATA:
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MOSI_protocol::do_snoop_M (Mreq *request)
{
	switch (request->msg) {
		case GETS:
			set_shared_line();
			send_DATA_on_bus(request->addr, request->src_mid);
			state = MOSI_CACHE_O;
			break;
		case GETM:
			set_shared_line();
			send_DATA_on_bus(request->addr, request->src_mid);
			state = MOSI_CACHE_I;
			break;
		case DATA:
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MOSI_protocol::do_snoop_IM (Mreq *request)
{
	switch (request->msg) {
		case GETS:
		case GETM:
			break;
		case DATA:
			send_DATA_to_proc(request->addr);
			state = MOSI_CACHE_M;
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MOSI_protocol::do_snoop_IS (Mreq *request)
{
	switch (request->msg) {
		case GETS:
		case GETM:
			break;
		case DATA:
			send_DATA_to_proc(request->addr);
			state = MOSI_CACHE_S;
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}
