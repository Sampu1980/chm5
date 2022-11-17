//
//
// Copyright(c) Infinera 2021-2022
//
#ifndef DcoSecProcEventListener_h
#define DcoSecProcEventListener_h

#include <vector>
#include <map>
#include <queue>
#include <google/protobuf/message.h>
#include "SingletonService.h"
#include <InfnDeQueueActor.h>
#include <DcoSecProcEvent.h>

#define NXP_EVENT_QUEUE_SIZE 256
using namespace std;

// ================================================
//@{
//  DcoSecProcEventListener listens for all events from Collector and 
//  updates the registered listeners
//
//  Similar to Observer pattern. DcoSecProcEventListener implements 
//  a hook method to recieve all events from Collector and 
//  updates Listeners. 
//@}
// ================================================

class DcoSecProcEventListener : public InfnDeQueueActor
{

    public:
        // ================================================
        //@{
        // Constructor
        // @param 
        //@}
        // ================================================
        DcoSecProcEventListener(uint32 evtQueueSize);

        // ================================================
        //@{
        // Destructor
        //@}
        // ================================================
        virtual ~DcoSecProcEventListener();

        // ================================================
        //@{
        //   Declare a NULL copy function to prevent people from
        //   copying instances.   
        //@}
        // ================================================
        DcoSecProcEventListener( const DcoSecProcEventListener & rOther );

        // ================================================
        //@{
        //   Declare a NULL assignment function to prevent people from
        //   assigning instances.   
        //@}
        // ================================================
        DcoSecProcEventListener & operator=( const DcoSecProcEventListener & rOther );

        // ================================================
        //@{
        //   Returns true if the DcoSecProcEventListener is running
        //@}
        // ================================================
        bool    IsRunning() const;

        // ================================================
        //@{
        //   Process the event and updates the subscribers
        //@}
        // ================================================
        virtual void ImplementHandleMsg( DcoSecProcEvent *pMessage );


        // ================================================
        //@{
        //   Post IkeEvents to IKE Listener
        //@}
        // ================================================
        ZStatus PostEvent(DcoSecProcEvent *pMessage);

        void Dump( ostringstream & os ) const ;


        //private:
};

typedef SingletonService< DcoSecProcEventListener > DcoSecProcEventListenerService;

#endif /* DcoSecProcEventListener_h */



