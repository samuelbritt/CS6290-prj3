/*
 * repeater.h
 *
 *  Created on: Jul 27, 2010
 *      Author: jessebeu
 *
 *      Callback logic courtesy of http://www.codeguru.com/cpp/cpp/cpp_mfc/callbacks/article.php/c4129
 *
 */

#ifndef REPEATER_H_
#define REPEATER_H_

#include "types.h"

#define DEFAULT_RATE (1 << 10)

class Repeater_event
{
public:
    timestamp_t execute_time;
    bool is_periodic;
    timestamp_t period;

    virtual void execute() =0;
};

template <class cInstance>
class TRepeater_event : Repeater_event
{
public:
    typedef void (cInstance::*tFunction) ();

    cInstance  *cInst;
    tFunction  pFunction;
    
    TRepeater_event(cInstance  *cInstancePointer,
                    tFunction   pFunctionPointer,
                    bool is_periodic = true,
                    timestamp_t period = DEFAULT_RATE)
    {
        this->cInst = cInstancePointer;
        this->pFunction = pFunctionPointer;
        
        this->is_periodic = is_periodic;
        
        if (is_periodic)
        {
            this->execute_time = period;
            this->period = period;
        }
        else
        {
            fatal_error("Need to add code for random repeater events!");
        }
    }
    
    virtual void execute()
    {
        if (pFunction)
            (cInst->*pFunction)();
        else
            fatal_error("ERROR : the callback function has not been defined !!!!");
        
        if (is_periodic)
            execute_time = execute_time + period;
        else
            fatal_error("Need to add code for random repeater events!");
    }
    
    void SetCallback (cInstance  *cInstancePointer,
                      tFunction   pFunctionPointer)
    {
        cInst     = cInstancePointer;
        pFunction = pFunctionPointer;
    }
};



#endif /* REPEATER_H_ */
