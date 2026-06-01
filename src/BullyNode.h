//-------------------- previous without failure and recovery ----------------
// #ifndef BULLYNODE_H
// #define BULLYNODE_H

// #include <omnetpp.h>
// using namespace omnetpp;

// class BullyNode : public cSimpleModule
// {
//   private:
//     int nodeId;
//     bool isLeader = false;
//     bool electionInProgress = false;
//     int okReplies = 0;

//   protected:
//     virtual void initialize() override;
//     virtual void handleMessage(cMessage *msg) override;

//     void startElection();
// };

// #endif

//-------------------- raw average msg_count ----------------
#ifndef BULLYNODE_H
#define BULLYNODE_H

#include <omnetpp.h>
#include <queue>
#include <vector>

using namespace omnetpp;

class BullyNode : public cSimpleModule
{
  private:
    int nodeId;
    int numNodes;

    bool alive = true;
    bool isLeader = false;
    bool electionInProgress = false;

    int okResponsesReceived = 0;
    int msgSizeBytes = 10000;

    std::vector<std::queue<cPacket *>> sendQueues;
    std::vector<cMessage *> sendEvents;

    cMessage *crashEvent = nullptr;
    cMessage *recoveryEvent = nullptr;
    cMessage *detectLeaderDownEvent = nullptr;
    cMessage *timeoutEvent = nullptr;

    static std::vector<bool> aliveNodes;

    static int currentLeaderId;
    static int activeElectionId;
    static bool globalElectionInProgress;

    static long totalSentMessages;
    static long electionStartMessageCount;

    static double electionStartTime;
    static double totalElectionTime;

    static long totalElectionMessages;
    static int completedElectionCount;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

  public:
    virtual ~BullyNode();

  private:
    void scheduleNextCrash();
    void crashNode();
    void recoverNode();

    void scheduleElectionAfterLeaderCrash();

    void startElection();
    void declareLeader();

    void sendToHigherNodes(cPacket *pkt);
    void sendToNode(int destId, cPacket *pkt);
    void broadcastCoordinator();

    void enqueuePacket(cPacket *pkt, int gateIndex);
    void processQueue(int gateIndex);

    int findLowestAliveNode() const;
    int findHighestAliveNode() const;
    bool isNodeAlive(int id) const;
};

#endif