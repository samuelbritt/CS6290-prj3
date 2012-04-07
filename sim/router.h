#ifndef __ROUTER_H
#define __ROUTER_H

#include "types.h"
#include "settings.h"
#include "stat_engine.h"

#define FLIT_DEBUG 0

//#define EXPRESS_CHANNEL_LEN 4
//#define EXPRESS_CHANNEL_ACTIVE true

class Torus;

typedef enum {
    ROUTER_LOCAL = 0,
    ROUTER_N,
    ROUTER_E,
    ROUTER_S,
    ROUTER_W,
    ROUTER_NX,  //express links
    ROUTER_EX,
    ROUTER_SX,
    ROUTER_WX,
    ROUTER_CROSSBAR_SIZE
} router_direction_t;
 
typedef enum {
    PIPELINE_IB = 0,
    PIPELINE_RC,
    PIPELINE_VCA,
    PIPELINE_SA,
    PIPELINE_ST,
    PIPELINE_LT,
    PIPELINE_LOCAL,
    PIPELINE_SIZE
} pipeline_stage_t;

class Network_interface;
class Network_request;
class Router;
class Virtual_channel;
class Input_buffer;

class Flit {
public:
    Flit ();
    ~Flit ();

    int id;

    int dest_id;
    bool is_valid;
    bool is_head;
    bool is_tail;
    
    /** added to the head flit to distinguish if it has traversed the torus
        backedge already. */
    bool crossed_torus_backedge;

    int express_link_delay;

    Virtual_channel *my_curr_virt_channel;
    Virtual_channel *my_next_virt_channel;
    pipeline_stage_t my_stage;
    Network_request *my_request;
    
    void print_me();

#if FLIT_DEBUG
    timestamp enqued_on_IB;     
#endif
    
};

class Virtual_channel {
public:
    Virtual_channel ();
    Virtual_channel (Router *r, Input_buffer *ib, router_direction_t dir, int num_buff_per_vc);
    ~Virtual_channel ();
    
    //private:
    Router *my_router;
    Input_buffer *my_input_buffer;

    Flit *head;
    
    //logically don't need to actually maintain a buffer
    //just a count of how many are in use, actual entries already in flit_list
    
    //status fields
    router_direction_t from;
    router_direction_t to;
    bool to_is_express;

    Virtual_channel *next_virt_channel_in_route;
    bool in_use;
    int free_buffer_entries;
    int num_of_buffer_entries;
    
    void consume_buffer_entry ();
    void add_flit_to_buffer (Flit *f);
    void remove_flit_from_buffer (Flit *f);
};

class Input_buffer {
public:
    Input_buffer ();
    Input_buffer (Router *r, router_direction_t dir, int num_vc, int num_buff_per_vc);
    ~Input_buffer ();
    Virtual_channel *assign_vc (bool is_torus_flit);

    bool is_torus_backedge;
    
private:
    int num_of_vcs;
    Virtual_channel **vc;

    //replication of vcs for torus deadlock avoidance
    int num_of_vcs_torus;
    Virtual_channel **vc_torus;

    router_direction_t my_direction;
    Router *my_router;
};

class Router {
public:
    Router (int id, int x_coor, int y_coor, int link_width, int vc_per_input, int buff_per_vc);
    ~Router ();
    
    void from_xbar_to_next_node ();
    void tick ();
    void tock ();
    
    //private: 
    int my_id;
    bool have_express_N, have_express_E, have_express_S, have_express_W;
    int express_N_len, express_S_len, express_E_len, express_W_len;
    int x_coor, y_coor;
    Network_interface *my_network_interface;
    
    int link_wires;
    Input_buffer **my_ports;
    Input_buffer *my_neighbor_ports[ROUTER_CROSSBAR_SIZE];
    
    LIST<Flit*> flit_list;
    LIST<Flit*> flit_leaving_list;
    LIST<Flit*> flit_leaving_SA_list;
    
    Router_stat_engine * stats;

    //Pipeline stages:
    // Input Buffering -> Route Computation -> VC Alloc -> Switch Alloc -> Switch Traversal
    void do_pipeline_NEW (Flit* f);
    void do_pipeline_IB (Flit* f);
    void do_pipeline_RC (Flit* f);
    void do_pipeline_VCA (Flit* f);
    void do_pipeline_SA (Flit* f);
    void do_pipeline_ST (Flit* f);
    void do_pipeline_LT (Flit* f);
    void do_pipeline_LOCAL (Flit* f);
    
    void calc_next_in_route_for (Virtual_channel *this_vc);
    void allocate_next_virtual_channel_for (Virtual_channel *this_vc);
    
    bool xbar_switch_in[ROUTER_CROSSBAR_SIZE];
    bool xbar_switch_out[ROUTER_CROSSBAR_SIZE];

    void dump_flits();
};

class Router_stat_engine : public Stat_engine
{
public:
	Persistent_histogram_stat<int> router_stall_hist;

	Router * my_router;

	Router_stat_engine ();
	Router_stat_engine (Module * m);
	Router_stat_engine (Router * router);
    ~Router_stat_engine ();

    void global_stat_merge (Stat_engine *);
    void print_stats (ostream & out);
    void clear_stats();

    void start_warmup ();
    void end_warmup ();
    void save_samples ();
};

#endif
