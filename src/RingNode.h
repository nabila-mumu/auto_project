#ifndef RINGNODE_H
#define RINGNODE_H

#include <omnetpp.h>
#include <vector>
#include <map>
#include <set>
using namespace omnetpp;

class RingNode : public cSimpleModule
{
  private:
    int nodeId;
    int successor = -1;
    int electionId = -1;
    bool isLeader = false;
    bool electionInProgress = false;

    
    simtime_t electionStartTime;
    simtime_t electionEndTime;
    simtime_t electionTime;
    
    static int totalMessages;
    static bool topologyLoaded;
    static int numNodes;
    static int currentLeaderId;
    static int globalElectionId;
    static std::vector<std::vector<int>> topology;
    
    int messagesReceived = 0;
    int messagesSent = 0;
    
    std::map<int, int> neighborToGateIndex;
    std::set<int> neighbors;
    std::vector<int> participantIds;
    
    void loadTopology();
    void buildGateMap();
    void sendToSuccessor(cMessage *msg);
    void startElection();

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif
