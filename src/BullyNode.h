#ifndef BULLYNODE_H
#define BULLYNODE_H

#include <omnetpp.h>
using namespace omnetpp;

class BullyNode : public cSimpleModule
{
  private:
    int nodeId;
    bool isLeader = false;
    bool electionInProgress = false;
    int okReplies = 0;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    void startElection();
};

#endif

