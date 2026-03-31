#ifndef __RINGNODE_H_
#define __RINGNODE_H_

#include <omnetpp.h>
#include <set>
#include <map>
#include <queue>

using namespace omnetpp;

class RingNode : public cSimpleModule
{
  private:
    int nodeId;
    int successor;

    bool electionInProgress;
    bool isLeader;
    int electionId;

    int messagesSent;
    int messagesReceived;

    static int globalElectionId;
    static int totalMessages;
    static int currentLeaderId;

    simtime_t electionStartTime;
    simtime_t electionEndTime;
    simtime_t electionTime;

    bool coordinatorReceived;

    std::map<int,int> neighborToGateIndex;
    std::set<int> neighbors;

    // bandwidth-related
    int msgSizeBytes;
    std::vector<std::queue<cPacket*>> sendQueues;
    std::vector<cMessage*> sendEvents;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void buildGateMap();
    void sendToSuccessor(cPacket *pkt);
    void startElection();

    void enqueuePacket(cPacket *pkt, int gateIndex);
    void processQueue(int gateIndex);
};

#endif
