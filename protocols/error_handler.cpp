#include "error_handler.h"
#include "protocol.h"
#include "../sim/sim.h"
#include "../sim/mreq.h"

ErrorHandler::ErrorHandler(Protocol *p)
{
	this->protocol = p;
}
ErrorHandler::~ErrorHandler()
{
}

void ErrorHandler::invalid_state_error()
{
        fatal_error ("%s->state not valid?\n", protocol->get_name());
}

void ErrorHandler::invalid_request_error(Mreq *request)
{
        request->print_msg (protocol->get_id(), "ERROR");
        fatal_error ("Client: %s state shouldn't see this message\n",
		     protocol->get_state_str());
}

void ErrorHandler::multiple_requests_error(Mreq *request)
{
	request->print_msg (protocol->get_id(), "ERROR");
	fatal_error("Should only have one outstanding request per processor!\n");
}

void ErrorHandler::exclusivity_violation_error()
{
    	fatal_error ("Should not see data for this line!  I have the line!\n");
}
