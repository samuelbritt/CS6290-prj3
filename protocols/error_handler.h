class Protocol;
class Mreq;

class ErrorHandler
{
public:
	ErrorHandler (Protocol *p);
	virtual ~ErrorHandler ();

	void invalid_state_error();
	void invalid_request_error(Mreq *request);
	void multiple_requests_error(Mreq *request);
	void exclusivity_violation_error();

private:
	Protocol *protocol;
};
