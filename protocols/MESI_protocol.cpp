#include "MESI_protocol.h"
#include "error_handler.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

#include <stdio.h>

extern Simulator *Sim;
static const char *MESI_block_states[] = {"X","I","S","E","M", "IM", "IS"};


/*************************
 * Constructor/Destructor.
 *************************/
MESI_protocol::MESI_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
	this->state = MESI_CACHE_I;
	this->name = "MESI_protocol";
	this->block_states = MESI_block_states;
}

MESI_protocol::~MESI_protocol ()
{
}

const char *MESI_protocol::get_state_str()
{
	return this->block_states[this->state];
}

void MESI_protocol::dump (void)
{
    fprintf (stderr, "%s - state: %s\n", this->name, this->get_state_str());
}

void MESI_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
		case MESI_CACHE_I: do_cache_I (request); break;
		case MESI_CACHE_S: do_cache_S (request); break;
		case MESI_CACHE_E: do_cache_E (request); break;
		case MESI_CACHE_M: do_cache_M (request); break;
		case MESI_CACHE_IM: do_cache_IM (request); break;
		case MESI_CACHE_IS: do_cache_IS (request); break;
		default: error_handler->invalid_state_error();
	}
}

void MESI_protocol::process_snoop_request (Mreq *request)
{
	switch (state) {
		case MESI_CACHE_I: do_snoop_I (request); break;
		case MESI_CACHE_S: do_snoop_S (request); break;
		case MESI_CACHE_E: do_snoop_E (request); break;
		case MESI_CACHE_M: do_snoop_M (request); break;
		case MESI_CACHE_IM: do_snoop_IM (request); break;
		case MESI_CACHE_IS: do_snoop_IS (request); break;
		default: error_handler->invalid_state_error();
	}
}

inline void MESI_protocol::do_cache_I (Mreq *request)
{
	switch (request->msg) {
		case LOAD:
			state = MESI_CACHE_IS;
			Sim->cache_misses++;
			send_GETS(request->addr);
			break;
		case STORE:
			state = MESI_CACHE_IM;
			Sim->cache_misses++;
			send_GETM(request->addr);
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MESI_protocol::do_cache_S (Mreq *request)
{
	switch (request->msg) {
		case LOAD:
			checkpoint(request, "Sending from $ to proc");
			send_DATA_to_proc(request->addr);
			break;
		case STORE:
			state = MESI_CACHE_IM;
			Sim->cache_misses++;
			send_GETM(request->addr);
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MESI_protocol::do_cache_E (Mreq *request)
{
	switch (request->msg) {
		case LOAD:
			checkpoint(request, "Sending from $ to proc (exclusive)");
			send_DATA_to_proc(request->addr);
			break;
		case STORE:
			checkpoint(request, "Sending from $ to proc (silent upgrade)");
			state = MESI_CACHE_M;
			Sim->silent_upgrades++;
			send_DATA_to_proc(request->addr);
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MESI_protocol::do_cache_M (Mreq *request)
{
	switch (request->msg) {
		case LOAD:
		case STORE:
			checkpoint(request, "Sending from $ to proc (mod)");
			send_DATA_to_proc(request->addr);
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MESI_protocol::do_cache_IM (Mreq *request)
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

inline void MESI_protocol::do_cache_IS (Mreq *request)
{
	do_cache_IM(request);
}

inline void MESI_protocol::do_snoop_I (Mreq *request)
{
	switch (request->msg) {
		case GETS:
		case GETM:
		case DATA: break;
		default: error_handler->invalid_request_error(request);
	}
}

inline void MESI_protocol::do_snoop_S (Mreq *request)
{
	switch (request->msg) {
		case GETS:
			set_shared_line();
			break;
		case GETM:
			checkpoint(request, "Downgrade to S->I");
			state = MESI_CACHE_I;
			break;
		case DATA:
			set_shared_line();
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MESI_protocol::do_snoop_E (Mreq *request)
{
	switch (request->msg) {
		case GETS:
			checkpoint(request, "Sending E->S");
			state = MESI_CACHE_S;
			set_shared_line();
			send_DATA_on_bus(request->addr, request->src_mid);
			break;
		case GETM:
			checkpoint(request, "Sending E->I");
			state = MESI_CACHE_I;
			set_shared_line();
			send_DATA_on_bus(request->addr, request->src_mid);
			break;
		case DATA:
			error_handler->invalid_request_error(request);
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MESI_protocol::do_snoop_M (Mreq *request)
{
	switch (request->msg) {
		case GETS:
			checkpoint(request, "Sending M->S");
			state = MESI_CACHE_S;
			set_shared_line();
			send_DATA_on_bus(request->addr, request->src_mid);
			break;
		case GETM:
			checkpoint(request, "Sending M->I");
			state = MESI_CACHE_I;
			set_shared_line();
			send_DATA_on_bus(request->addr, request->src_mid);
			break;
		case DATA:
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MESI_protocol::do_snoop_IM (Mreq *request)
{
	switch (request->msg) {
		case GETS:
		case GETM:
			break;
		case DATA:
			checkpoint(request, "Getting Mod access");
			state = MESI_CACHE_M;
			send_DATA_to_proc(request->addr);
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}

inline void MESI_protocol::do_snoop_IS (Mreq *request)
{
	const MESI_cache_state_t states[] = {MESI_CACHE_E, MESI_CACHE_S};
	const char *msgs[] = {
		"Getting Exclusive access",
		"Getting Shared access"
	};
	int shared;
	const char *msg;

	switch (request->msg) {
		case GETS:
		case GETM:
			break;
		case DATA:
			shared = get_shared_line();
			state = states[shared];
			msg = msgs[shared];
			checkpoint(request, msg);
			send_DATA_to_proc(request->addr);
			break;
		default:
			error_handler->invalid_request_error(request);
	}
}
