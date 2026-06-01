//-------------------- previous without failure and recovery ----------------
// #ifndef __RINGNODE_H_
// #define __RINGNODE_H_

// #include <omnetpp.h>
// #include <set>
// #include <map>
// #include <queue>

// using namespace omnetpp;

// class RingNode : public cSimpleModule
// {
//   private:
//     int nodeId;
//     int successor;

//     bool electionInProgress;
//     bool isLeader;
//     int electionId;

//     int messagesSent;
//     int messagesReceived;

//     static int globalElectionId;
//     static int totalMessages;
//     static int currentLeaderId;

//     simtime_t electionStartTime;
//     simtime_t electionEndTime;
//     simtime_t electionTime;

//     bool coordinatorReceived;

//     std::map<int,int> neighborToGateIndex;
//     std::set<int> neighbors;

//     // bandwidth-related
//     int msgSizeBytes;
//     std::vector<std::queue<cPacket*>> sendQueues;
//     std::vector<cMessage*> sendEvents;

//   protected:
//     virtual void initialize() override;
//     virtual void handleMessage(cMessage *msg) override;
//     virtual void finish() override;

//     void buildGateMap();
//     void sendToSuccessor(cPacket *pkt);
//     void startElection();

//     void enqueuePacket(cPacket *pkt, int gateIndex);
//     void processQueue(int gateIndex);
// };

// #endif

//------------------- raw average msg_count --------------------
// #ifndef __RINGNODE_H_
// #define __RINGNODE_H_

// #include <omnetpp.h>
// #include <set>
// #include <map>
// #include <queue>
// #include <vector>

// using namespace omnetpp;

// class RingNode : public cSimpleModule
// {
//   private:
//     int nodeId;
//     int numNodes;

//     bool alive = true;
//     bool isLeader = false;
//     bool electionInProgress = false;
//     bool bidirectional = false;

//     int electionId = -1;
//     int msgSizeBytes = 10000;

//     std::map<int, int> neighborToGateIndex;
//     std::set<int> neighbors;

//     std::vector<std::queue<cPacket *>> sendQueues;
//     std::vector<cMessage *> sendEvents;

//     cMessage *crashEvent = nullptr;
//     cMessage *recoveryEvent = nullptr;
//     cMessage *detectLeaderDownEvent = nullptr;
//     cMessage *timeoutEvent = nullptr;

//     static std::vector<bool> aliveNodes;

//     static int currentLeaderId;
//     static int globalElectionId;
//     static bool globalElectionInProgress;

//     static long totalMessages;
//     static long electionStartMessageCount;

//     static double electionStartTime;
//     static double totalElectionTime;

//     static long totalElectionMessages;
//     static int completedElectionCount;

//   protected:
//     virtual void initialize() override;
//     virtual void handleMessage(cMessage *msg) override;
//     virtual void finish() override;

//   public:
//     virtual ~RingNode();

//   private:
//     void buildGateMap();

//     void scheduleNextCrash();
//     void crashNode();
//     void recoverNode();

//     void scheduleElectionAfterLeaderCrash();
//     void startElection();
//     void completeElection(int leaderId);

//     void sendElectionToken(int direction);
//     void sendCoordinatorToken(int direction, int leaderId);

//     void sendToNextAlive(cPacket *pkt, int direction);
//     int getNextAliveNode(int direction) const;

//     void enqueuePacket(cPacket *pkt, int gateIndex);
//     void processQueue(int gateIndex);

//     int findLowestAliveNode() const;
//     int findHighestAliveNode() const;
//     bool isNodeAlive(int id) const;
// };

// #endif

//-------------------- normalized average msg_count ----------------

// #ifndef __RINGNODE_H_
// #define __RINGNODE_H_

// #include <omnetpp.h>
// #include <map>
// #include <queue>
// #include <vector>

// using namespace omnetpp;

// class RingNode : public cSimpleModule
// {
//   private:
//     int nodeId;
//     int numNodes;

//     bool alive = true;
//     bool isLeader = false;
//     bool electionInProgress = false;
//     bool bidirectional = false;

//     int msgSizeBytes = 10000;

//     std::map<int, int> neighborToGateIndex;

//     cMessage *crashEvent = nullptr;
//     cMessage *recoveryEvent = nullptr;
//     cMessage *detectLeaderDownEvent = nullptr;

//     static std::vector<bool> aliveNodes;

//     static int currentLeaderId;
//     static int globalElectionId;
//     static bool globalElectionInProgress;

//     static int electionInitiatorId;
//     static int electionMaxId;
//     static int electionReturnCount;
//     static int expectedElectionReturnCount;

//     static int coordinatorReturnCount;
//     static int expectedCoordinatorReturnCount;

//     static long totalMessages;
//     static long electionStartMessageCount;

//     static double electionStartTime;
//     static double totalElectionTime;

//     static long totalElectionMessages;
//     static int completedElectionCount;

//   protected:
//     virtual void initialize() override;
//     virtual void handleMessage(cMessage *msg) override;
//     virtual void finish() override;

//   public:
//     virtual ~RingNode();

//   private:
//     void buildGateMap();

//     void scheduleNextCrash();
//     void crashNode();
//     void recoverNode();

//     void scheduleElectionAfterLeaderCrash();
//     void startElection();

//     void sendElectionToken(int direction);
//     void handleElectionToken(cPacket *pkt);

//     void completeElectionDecision();

//     void sendCoordinatorToken(int direction, int leaderId);
//     void handleCoordinatorToken(cPacket *pkt);

//     void finishElectionMetrics();

//     void sendToNextAliveNode(cPacket *pkt, int direction);

//     int getNextAliveNode(int direction) const;
//     int getPhysicalHopCount(int from, int to, int direction) const;
//     simtime_t getLogicalPathDelay(int from, int to, int direction);
//     double getPathLossRate(int from, int to, int direction);

//     int physicalNeighborOf(int id, int direction) const;

//     int findLowestAliveNode() const;
//     int findHighestAliveNode() const;
//     bool isNodeAlive(int id) const;
// };

// #endif

#ifndef __RINGNODE_H_
#define __RINGNODE_H_

#include <omnetpp.h>
#include <map>
#include <vector>
#include <queue>
#include <algorithm>
#include <fstream>

using namespace omnetpp;

class RingNode : public cSimpleModule
{
  private:
    int nodeId;
    int numNodes;

    bool alive = true;
    bool isLeader = false;
    bool electionInProgress = false;
    bool bidirectional = false;

    int msgSizeBytes = 10000;

    std::map<int, int> neighborToGateIndex;

    cMessage *crashEvent = nullptr;
    cMessage *recoveryEvent = nullptr;

    static std::vector<bool> aliveNodes;

    static int currentLeaderId;
    static int globalElectionId;
    static bool globalElectionInProgress;

    static int electionInitiatorId;
    static int electionMaxId;
    static int electionReturnCount;
    static int expectedElectionReturnCount;

    static int coordinatorReturnCount;
    static int expectedCoordinatorReturnCount;

    static long totalMessages;
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
    virtual ~RingNode();

  private:
    void buildGateMap();

    void scheduleLeaderCrash();
    void crashNode();
    void recoverNode();

    void scheduleElectionAfterLeaderCrash();
    void startElection();

    void sendElectionToken(int direction, int remaining);
    void handleElectionToken(cPacket *pkt);
    void completeElectionSegment(int segmentMaxId);
    void completeElectionDecision();

    void sendCoordinatorToken(int direction, int leaderId, int remaining);
    void handleCoordinatorToken(cPacket *pkt);
    void completeCoordinatorSegment();
    void finishElectionMetrics();

    void sendToNextAliveNode(cPacket *pkt, int direction);

    int getNextAliveNode(int direction) const;
    int countAliveNodes() const;

    std::vector<int> shortestPath(int src, int dest) const;
    int getPathHopCount(const std::vector<int>& path) const;
    simtime_t getPathDelay(const std::vector<int>& path);
    double getPathLossRate(const std::vector<int>& path);

    int findLowestAliveNode() const;
    int findHighestAliveNode() const;
    bool isNodeAlive(int id) const;
};

#endif
