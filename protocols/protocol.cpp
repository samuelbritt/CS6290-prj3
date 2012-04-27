#include "protocol.h"
#include "error_handler.h"
#include "../sim/sharers.h"
#include "../sim/hash_table.h"
#include "../sim/sim.h"

#include <stdio.h>

extern Simulator * Sim;
static const char *default_block_states[] = {"-"};

Protocol::Protocol (Hash_table *my_table, Hash_entry *my_entry)
{
    this->my_table = my_table;
    this->my_entry = my_entry;
    this->error_handler = new ErrorHandler(this);
    this->block_states = default_block_states;
    this->state = 0;
}

Protocol::~Protocol ()
{
	delete this->error_handler;
}

const char *Protocol::get_name()
{
	return this->name;
}

ModuleID Protocol::get_id()
{
	return this->my_table->moduleID;
}

const char *Protocol::get_state_str()
{
	return default_block_states[0];
}

void Protocol::send_GETM(paddr_t addr)
{
	/* Create a new message to send on the bus */
	Mreq * new_request;
	/* The arguments to Mreq are -- msg, address, src_id (optional), dest_id (optional) */
	new_request = new Mreq(GETM,addr);
	/* This will but the message in the bus' arbitration queue to sent */
	this->my_table->write_to_bus(new_request);
}

void Protocol::send_GETS(paddr_t addr)
{
	/* Create a new message to send on the bus */
	Mreq * new_request;
	/* The arguments to Mreq are -- msg, address, src_id (optional), dest_id (optional) */
	new_request = new Mreq(GETS,addr);
	/* This will but the message in the bus' arbitration queue to sent */
	this->my_table->write_to_bus(new_request);
}

void Protocol::send_DATA_on_bus(paddr_t addr, ModuleID dest)
{
	/* Create a new message to send on the bus */
	Mreq * new_request;
	/* The arguments to Mreq are -- msg, address, src_id (optional), dest_id (optional) */
	// When DATA is sent on the bus it _MUST_ have a destination module
	new_request = new Mreq(DATA, addr, my_table->moduleID, dest);
	/* Debug Message -- DO NOT REMOVE or you won't match the validation runs */
	fprintf(stderr,"**** DATA_SEND Cache: %d -- Clock: %llu\n",my_table->moduleID.nodeID,Global_Clock);
	/* This will but the message in the bus' arbitration queue to sent */
	this->my_table->write_to_bus(new_request);

	Sim->cache_to_cache_transfers++;
}

void Protocol::send_DATA_to_proc(paddr_t addr)
{
	/* Create a new message to send on the bus */
	Mreq * new_request;
	/* The arguments to Mreq are -- msg, address, src_id (optional), dest_id (optional) */
	// When data is sent from a cache to proc, there is no need to set the src and dest
	new_request = new Mreq(DATA,addr);
	/* This writes the message into the processor's input buffer.  The processor
	 * only expects to ever receive DATA messages
	 */
	this->my_table->write_to_proc(new_request);
}

void Protocol::set_shared_line ()
{
	// Set the bus' shared line
	Sim->bus->shared_line = true;
}

bool Protocol::get_shared_line ()
{
	// Find out if the shared line is active
	return Sim->bus->is_shared_active();
}

#ifdef DEBUG
void Protocol::checkpoint(Mreq *request, const char *str)
{
	fprintf(stderr, "%d %p: %s\n", my_table->moduleID.nodeID,
		(void *) request->addr, str);
}
#else
void Protocol::checkpoint(Mreq *request, const char *str)
{
}
#endif
