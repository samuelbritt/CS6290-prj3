#include <assert.h>
#include <iostream>

#include "network.h"
#include "router.h"
#include "sim.h"

using namespace std;

#define EXPRESS_LINK_DELAY 1    //how many extra cycles (i.e. 1 means 2 cycles to traverse)

extern Simulator Sim;

/********
 * Flits
 ********/
Flit::Flit ()
{
    crossed_torus_backedge = false;
}

Flit::~Flit ()
{
}

/*******************
 * Virtual Channels
 *******************/
Virtual_channel::Virtual_channel (Router *r, Input_buffer *ib, router_direction_t dir,  int num_buff_per_vc)
{
    from = dir;
    my_router = r;
    my_input_buffer = ib;
    num_of_buffer_entries = num_buff_per_vc;
    free_buffer_entries = num_buff_per_vc;
    in_use = false;
    to_is_express = false;
}

Virtual_channel::~Virtual_channel ()
{
}

void Virtual_channel::consume_buffer_entry ()
{
    assert (free_buffer_entries != 0 && in_use && "make sure we're allowed to consume a buffer");
    free_buffer_entries--;
}

void Virtual_channel::add_flit_to_buffer (Flit *f)
{
    if (f->is_head)
    {
        //we're the first flit for this packet, so set things up
        head = f;
    }
    //does nothing since flits aren't really stored in the vc
    //and the buffer entry was consumed earlier by consume_buffer_entry
}

void Virtual_channel::remove_flit_from_buffer (Flit *f) {
    free_buffer_entries++;
    if (f->is_tail)
    {
        in_use = false;
        assert (free_buffer_entries == num_of_buffer_entries && "wrong buffer count?");
    }
}

/******************
 * Network buffers
 ******************/

Input_buffer::Input_buffer(Router *r, router_direction_t dir, int num_vc, int num_buff_per_vc)
{
    my_router = r;
    num_of_vcs = num_vc;
    my_direction = dir;
    vc = new Virtual_channel*[num_vc];
    for(int i = 0; i < num_vc; i++)
        vc[i] = new Virtual_channel(r, this, dir, num_buff_per_vc);

    if (Sim.settings.network_topology == TORUS)
    {
        is_torus_backedge = false;
        num_of_vcs_torus = num_vc;
        vc_torus = new Virtual_channel*[num_vc];
        for(int i = 0; i < num_vc; i++)
            vc_torus[i] = new Virtual_channel(r, this, dir, num_buff_per_vc);
    }
}

Input_buffer::~Input_buffer()
{
    for(int i = 0; i < num_of_vcs; i++)
        delete vc[i];

    delete [] vc;
}

Virtual_channel* Input_buffer::assign_vc(bool is_torus_flit)
{
    if (is_torus_flit)
    {
        for (int i = 0; i < num_of_vcs_torus; i++)
        {
            if (!vc_torus[i]->in_use)
            {
                vc_torus[i]->in_use = true;
                return vc_torus[i];
            }
        }
    }
    else
    {
        for (int i = 0; i < num_of_vcs; i++)
        {
            if (!vc[i]->in_use)
            {
                vc[i]->in_use = true;
                return vc[i];
            }
        }
    }
    return NULL;
}

/*************************************************
 * Router constructor, destructor, and functions.
 *************************************************/
Router::Router(int id, int x_coor, int y_coor, int link_width, int vc_per_input, int buff_per_vc)
{
    this->my_id = id;
    this->x_coor = x_coor;
    this->y_coor = y_coor;

    this->have_express_N = false;
    this->have_express_S = false;
    this->have_express_E = false;
    this->have_express_W = false;

    this->express_N_len = 0;
    this->express_S_len = 0;
    this->express_E_len = 0;
    this->express_W_len = 0;

    this->link_wires = link_width;
    this->my_ports = new Input_buffer*[ROUTER_CROSSBAR_SIZE];

    this->stats = new Router_stat_engine(this);

    for(int i = 0; i < ROUTER_CROSSBAR_SIZE; i++)
        my_ports[i] = new Input_buffer (this, (router_direction_t)i, vc_per_input, buff_per_vc);

    flit_list.clear();
    flit_leaving_list.clear();
}

Router::~Router ()
{
    //input buffers
    for (int i = 0; i < ROUTER_CROSSBAR_SIZE; i++)
        delete my_ports[i];

    delete [] my_ports;
    
    //wipe out any flits living in this router

    LIST<Flit *>::iterator it;
    for (it = flit_list.begin () ; it != flit_list.end () ; it++)
    {
        if ((*it)->is_tail)
        {
            if ((*it)->my_request->mreq != NULL)
                delete (*it)->my_request->mreq;

            delete (*it)->my_request;
        }
        delete (*it);        
    }
    flit_list.clear ();
    flit_leaving_list.clear ();
    flit_leaving_SA_list.clear ();

    delete stats;
}

void Router::tick () 
{
    int i;
    LIST<Flit *>::iterator it;
#if (0) && FLIT_DEBUG
    pipeline_stage_t pre_state;
#endif

    //advance all flits in the router
    //
    //traverse flit list and advance through pipeline
    //results in age-priority flit processing
    if (flit_list.size () == 0)
        return;

    //First clear any state left over from last cycle
    for (i = 0; i < ROUTER_CROSSBAR_SIZE; i++)
    {
        xbar_switch_in[i] = false;
        xbar_switch_out[i] = false;
    }

    for (it = flit_list.begin (); it != flit_list.end () ; it++)
    {
#if (0) && FLIT_DEBUG
        pre_state = (*it)->my_stage;
#endif
        switch ((*it)->my_stage) {
            case PIPELINE_IB:
                do_pipeline_IB (*it);
                do_pipeline_RC (*it);    //combined stages for 3 stage pipe
                break;
            case PIPELINE_RC:
                fatal_error ("new 3 stage pipe, should never see a flit in RC\n");
                do_pipeline_RC (*it);
                break;
            case PIPELINE_VCA:
                do_pipeline_VCA (*it);
                break;
            case PIPELINE_SA:
                do_pipeline_SA (*it);
                break;
            case PIPELINE_ST:
                do_pipeline_ST (*it);
                do_pipeline_LT (*it);    //combined stages for 3 stage pipe
                break;
            case PIPELINE_LT:
                //can again due to express links taking multiple cycles
                //assert(0 && "new 3 stage pipe, should never see a flit in LT");
                do_pipeline_LT (*it);
                break;
            default:
                fatal_error ("Invalid router pipeline stage\n");
                break;
        }
#if (0) && FLIT_DEBUG
        if (pre_state != (*it)->my_stage)
            (*it)->print_me ();
#endif
    }
}

void Router::tock()
{
    LIST<Flit *>::iterator it;

    for (it = flit_leaving_SA_list.begin () ; it != flit_leaving_SA_list.end () ; it++)
    {
        (*it)->my_curr_virt_channel->remove_flit_from_buffer((*it));
        (*it)->my_curr_virt_channel = (*it)->my_next_virt_channel;
        (*it)->my_next_virt_channel = NULL;
    }
    flit_leaving_SA_list.clear();

    //remove all flits that have left this router from the list and 
    //add to the next node's list 
    //
    //NOTE: pipeline stage is already set to IB
    for(it = flit_leaving_list.begin() ; it != flit_leaving_list.end() ; it++)
    {
        flit_list.remove(*it);

        //reached destination node, so garbage collect it now
        if ((*it)->dest_id == my_id)
        {
            (*it)->my_curr_virt_channel->remove_flit_from_buffer(*it);
            delete (*it);
        } 
        else
        {
            (*it)->my_curr_virt_channel->my_router->flit_list.push_back(*it);
        }
    }
    flit_leaving_list.clear();
}

  
void Router::do_pipeline_IB(Flit* f) 
{
    //to have gotten here you already know there is definately space
    //so add to the buffer and progress through the pipeline

    f->my_curr_virt_channel->add_flit_to_buffer(f);
    f->my_stage = PIPELINE_RC;
    
    if (f->is_tail)
        f->my_request->router_tail_time = Global_Clock;

#if FLIT_DEBUG
    f->enqued_on_IB = Sim.global_clock;
#endif

}

void Router::do_pipeline_RC(Flit* f) 
{
    //if head, setup routing info for virt channel
    //otherwise do nothing

    if (f->is_head)
        calc_next_in_route_for(f->my_curr_virt_channel);
    
    if (f->my_curr_virt_channel->to_is_express)
        f->express_link_delay = EXPRESS_LINK_DELAY;
    else 
        f->express_link_delay = 0;
    
    f->my_stage = PIPELINE_VCA;
}

void Router::do_pipeline_VCA(Flit* f) 
{
    //allocate a virtual channel in the next node if head
    //otherwise just assign vc already given to this packet 
    if (f->is_head)
        allocate_next_virtual_channel_for (f->my_curr_virt_channel);

    //if a vc hasn't been assigned yet, can't make progress
    //NOTE: if a vc isn't availible, multiple flits can sit in this stage together
    //      this isn't a problem though cause they will be staggered again in the SA stage

    if (f->my_curr_virt_channel->next_virt_channel_in_route != NULL)
    {
        //now check if a buffer slot is open, if so setup, consume and move to next stage
        if (f->my_curr_virt_channel->next_virt_channel_in_route->free_buffer_entries > 0)
        {
            f->my_next_virt_channel = f->my_curr_virt_channel->next_virt_channel_in_route;
            f->my_next_virt_channel->consume_buffer_entry();
            f->my_stage = PIPELINE_SA;
        }
        else if (Sim.settings.net_infinite_bw)
        {
            //assert(0 && "infinite BW stalled? no buffer?");
        }
    } 
    else if (Sim.settings.net_infinite_bw)
    {
        //assert(0 && "infinite BW stalled? no VC?");
    }
}

void Router::do_pipeline_SA(Flit* f) 
{
    if (Sim.settings.net_infinite_bw) 
    {
            //logically no longer a part of this channel
            //is now a part of the next vc (en route to the buffer)
            flit_leaving_SA_list.push_back (f);
            f->my_stage = PIPELINE_ST;
    }
    else 
    {
        //checks the xbar for the next cycle's transfers 
        //and is when a buffer entry is officially freed   

        //check if xbar input and output are availible next cycle or already reserved
        if (xbar_switch_in[f->my_curr_virt_channel->from] == false &&
            xbar_switch_out[f->my_curr_virt_channel->to] == false)
        {
            //signal that these ports of the xbar are in use next cycle
            xbar_switch_in[f->my_curr_virt_channel->from] = true;
            xbar_switch_out[f->my_curr_virt_channel->to] = true;

            //logically no longer a part of this channel
            //is now a part of the next vc (en route to the buffer)
            flit_leaving_SA_list.push_back (f);
            f->my_stage = PIPELINE_ST;
        }
    }
}

void Router::do_pipeline_ST(Flit* f)
{
    //models the cycle delay to traverse the crossbar
    f->my_stage = PIPELINE_LT;
}

void Router::do_pipeline_LT(Flit* f)
{
    //models the cycle delay to traverse the link
    assert (f);

    if (f->express_link_delay == 0)
    {
        if (f->is_tail)
        {
        	stats->router_stall_hist.collect((int)(Global_Clock - f->my_request->router_tail_time));
        	f->my_request->router_tail_time = Global_Clock;
        }

        //check if this is the destination processor
        if (f->dest_id == my_id)
        {
            //flit is now in the processor, so enqueue it if it's the tail (the last one)
            if (f->is_tail)
            {            
                assert (f->my_request);
                my_network_interface->dequeue_request_queue.push (f->my_request);
            }

            //flit is now in the processing element 
            flit_leaving_list.push_back(f);
        } 
        else
        {
            //flit is now in the next node
            flit_leaving_list.push_back(f);
            f->my_stage = PIPELINE_IB;
        }
    }
    else
    {
        f->express_link_delay--;
    }
}

void Router::calc_next_in_route_for(Virtual_channel *this_vc)
{
    int target_y_coor, target_x_coor;
    target_x_coor = this_vc->head->dest_id % Sim.NOC->x_dim;
    target_y_coor = this_vc->head->dest_id / Sim.NOC->x_dim;

    assert(target_y_coor != -1);
    assert(target_x_coor != -1);

    this_vc-> to_is_express = false;

    if(Sim.settings.network_topology == EXPRESS_MESH)
    {
         //Express channels with simple routing, travel NS(x), then EW (y)
        if (have_express_S && y_coor + express_S_len <= target_y_coor) {
            this_vc->to = ROUTER_SX;
            this_vc-> to_is_express = true;
        } 
        else if (have_express_N && y_coor - express_N_len >= target_y_coor) {
            this_vc->to = ROUTER_NX;
            this_vc-> to_is_express = true;
        } 
        else if (have_express_E && x_coor + express_E_len <= target_x_coor) {
            this_vc->to = ROUTER_EX;
            this_vc-> to_is_express = true;
        } 
        else if (have_express_W && x_coor - express_W_len >= target_x_coor) {
            this_vc->to = ROUTER_WX;
            this_vc-> to_is_express = true;
        } 
        //unit simple routing, travel NS(x), then EW (y)
        else if (y_coor < target_y_coor) {
            this_vc->to = ROUTER_S;
        } 
        else if (y_coor > target_y_coor) {
            this_vc->to = ROUTER_N;
        } 
        else if (x_coor < target_x_coor) {
            this_vc->to = ROUTER_E;
        } 
        else if (x_coor > target_x_coor) {
            this_vc->to = ROUTER_W;
        } 
        else {
            //we should be at the destination
            assert(this_vc->head->dest_id == my_id);
            this_vc->to = ROUTER_LOCAL;
        }
        return;
    }
    else if (Sim.settings.network_topology == TORUS)
    {
        //simple routing, travel NS(x), then EW (y)
        bool x_invert = (ABS(target_x_coor - x_coor) > (Sim.settings.network_x_dimension >> 1));
        bool y_invert = (ABS(target_y_coor - y_coor) > (Sim.settings.network_y_dimension >> 1));

        if (target_y_coor - y_coor > 0) {
            if(y_invert)
            {
                this_vc->to = ROUTER_N;
                if(y_coor == 0)
                    this_vc->head->crossed_torus_backedge = true;
            }
            else
                this_vc->to = ROUTER_S;
        }
        else if (target_y_coor - y_coor < 0) {
            if(y_invert)
            {
                this_vc->to = ROUTER_S;
                if(y_coor == Sim.NOC->y_dim - 1)
                    this_vc->head->crossed_torus_backedge = true;
            }
            else
                this_vc->to = ROUTER_N;
        }
        else if (target_x_coor - x_coor > 0) {
            if(x_invert)
            {
                this_vc->to = ROUTER_W;
                if(x_coor == 0)
                    this_vc->head->crossed_torus_backedge = true;
            }
            else
                this_vc->to = ROUTER_E;
        }
        else if (target_x_coor - x_coor < 0) {
            if(x_invert)
            {
                this_vc->to = ROUTER_E;
                if(x_coor == Sim.NOC->x_dim - 1)
                    this_vc->head->crossed_torus_backedge = true;
            }
            else
                this_vc->to = ROUTER_W;
        }
        else {
            //we should be at the destination
            assert(this_vc->head->dest_id == my_id);
            this_vc->to = ROUTER_LOCAL;
        }
        return;
    }
    else if (Sim.settings.network_topology == MESH)
    {
        //simple routing, travel NS(x), then EW (y)
        if (y_coor < target_y_coor) {
            this_vc->to = ROUTER_S;
        } 
        else if (y_coor > target_y_coor) {
            this_vc->to = ROUTER_N;
        } 
        else if (x_coor < target_x_coor) {
            this_vc->to = ROUTER_E;
        } 
        else if (x_coor > target_x_coor) {
            this_vc->to = ROUTER_W;
        } 
        else {
            //we should be at the destination
            assert(this_vc->head->dest_id == my_id);
            this_vc->to = ROUTER_LOCAL;
        }
        return;
    }

    fatal_error("No Topology?? Should never get here...");
}

void Router::allocate_next_virtual_channel_for (Virtual_channel *this_vc)
{
    assert (this_vc);
    assert (this_vc->my_router);
    assert (this_vc->my_router->my_neighbor_ports);
    assert (this_vc->my_router->my_neighbor_ports[this_vc->to]);

    //little confusing, so here goes... the virtual channel passed in is
    //trying to reserve a virtual channel in a neighbor. So it queries the Input
    //port of the neighboring router in the pre-calculated direction (from RC stage)
    //which will either return a pointer to a free virtual channel in that input (and 
    //reserve it) or will return NULL
    this_vc->next_virt_channel_in_route = this_vc->my_router->my_neighbor_ports[this_vc->to]->assign_vc(this_vc->head->crossed_torus_backedge);
}

void Router::dump_flits()
{
    fprintf(stderr, "My_id: %d\n", my_id);
    LIST<Flit *>::iterator it;
    for (it = flit_list.begin(); it != flit_list.end(); it++)
    {
#if FLIT_DEBUG
        fprintf(stderr, "IB time: %llu\t\t", (*it)->enqued_on_IB);
#endif
        if((*it)->is_head)
            fprintf(stderr, "Head\t");
        if((*it)->is_tail)
            fprintf(stderr, "Tail\t");

        fprintf(stderr, "my_request: 0x%lx \tFrom: %d To: %d Stage: %d\n", (unsigned long) (*it)->my_request, (*it)->my_request->src_id, (*it)->my_request->dest_id, (int) (*it)->my_stage);

        (*it)->print_me();
    }
}

void Flit::print_me()
{
    
    fprintf(stderr, "FlitID: %d\tdest_id: %d\tis_head: %d\tis_tail: %d\t", id, dest_id, (int) is_head, (int) is_tail);
    fprintf(stderr, "\taddr: 0x%x\tmy_stage: %d\treq->src: %d\treq->dest: %d\n", (unsigned int) my_request->mreq->addr, (int) my_stage, my_request->src_id, my_request->dest_id);
}



Router_stat_engine::Router_stat_engine () :
	Stat_engine(NULL),
	router_stall_hist("Router Stall Cycle Counts", "", 5, 5, 0),
	my_router(NULL)
{
}

Router_stat_engine::Router_stat_engine(Module * m) :
	Stat_engine(m),
	router_stall_hist("Router Stall Cycle Counts", "", 5, 5, 0),
	my_router(NULL)
{
}

Router_stat_engine::Router_stat_engine(Router * router) :
	Stat_engine(NULL),
	router_stall_hist("SIM Router Stall Cycle Counts", "", 5, 5, 0),
	my_router(router)
{
}

Router_stat_engine::~Router_stat_engine ()
{
}


void Router_stat_engine::global_stat_merge(Stat_engine * e)
{
	Router_stat_engine * global_engine = (Router_stat_engine*)e;
	global_engine->router_stall_hist.merge(&router_stall_hist);
}

void Router_stat_engine::print_stats (ostream & out)
{
	//if (my_module && my_module->name)
    //out << my_module->name << " OUTPUT" << endl;

	router_stall_hist.print(out);

}

void Router_stat_engine::clear_stats()
{
	router_stall_hist.clear();
}

void Router_stat_engine::start_warmup ()
{

}

void Router_stat_engine::end_warmup ()
{

}

void Router_stat_engine::save_samples ()
{

}
