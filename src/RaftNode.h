// ------------------- previous version without failure and recovery ------------------
// #ifndef __RAFTNODE_H__
// #define __RAFTNODE_H__

// #include <omnetpp.h>
// #include <set>
// #include <tuple>
// #include <string>
// #include <fstream>

// using namespace omnetpp;

// class RaftNode : public cSimpleModule
// {
//   private:
//     // ================= BASIC =================
//     int nodeId;
//     int numNodes;
//     int msgSizeBytes; // ✅ ADD (10KB)

//     std::string state;   // FOLLOWER / CANDIDATE / LEADER

//     int currentTerm = 0;
//     int votedFor = -1;

//     // ================= ELECTION =================
//     cMessage *electionTimeout = nullptr;
//     cMessage *heartbeatTimer = nullptr;

//     int voteCount = 0;
//     std::set<int> votesReceived;

//     double electionStartTime = 0;

//     // ================= FLOOD CONTROL =================
//     std::set<std::tuple<std::string, int, int>> seenFloods;
//     // (messageType, term, originatorId)

//     // ================= GLOBAL METRICS =================
//     static int globalMessageCount;
//     static bool leaderElected;

//   protected:
//     virtual void initialize() override;
//     virtual void handleMessage(cMessage *msg) override;
//     virtual void finish() override;

//     // ================= CORE FUNCTIONS =================
//     void startElection();
//     void becomeLeader();
//     void becomeFollower(int term);

//     // ================= COMMUNICATION =================
//     // void floodMessage(cMessage *msg, int exceptGate);
//     // void sendToNode(cMessage *msg, int targetId);

//     void floodMessage(cPacket *pkt, int exceptGate);
//     void sendToNode(cPacket *pkt, int targetId);
  

//     // ================= TIME =================
//     void resetElectionTimeout();
// };

// #endif


//----------------------- failure time affected (raw average msg_count) -------------------
// #ifndef __RAFTNODE_H__
// #define __RAFTNODE_H__

// #include <omnetpp.h>
// #include <set>
// #include <queue>
// #include <vector>
// #include <string>
// #include <fstream>

// using namespace omnetpp;

// class RaftNode : public cSimpleModule
// {
//   private:
//     enum NodeState { FOLLOWER, CANDIDATE, LEADER };

//     int nodeId;
//     int numNodes;
//     int msgSizeBytes = 10000;

//     bool alive = true;
//     NodeState state = FOLLOWER;

//     int currentTerm = 0;
//     int votedFor = -1;

//     int voteCount = 0;
//     std::set<int> votesReceived;

//     cMessage *electionTimeoutEvent = nullptr;
//     cMessage *heartbeatEvent = nullptr;
//     cMessage *crashEvent = nullptr;
//     cMessage *recoveryEvent = nullptr;

//     std::vector<std::queue<cPacket *>> sendQueues;
//     std::vector<cMessage *> sendEvents;

//     static std::vector<bool> aliveNodes;

//     static int currentLeaderId;
//     static bool globalElectionInProgress;

//     static double electionStartTime;
//     static double totalElectionTime;

//     static long currentElectionMessages;
//     static long totalElectionMessages;
//     static int completedElectionCount;

//   protected:
//     virtual void initialize() override;
//     virtual void handleMessage(cMessage *msg) override;
//     virtual void finish() override;

//   public:
//     virtual ~RaftNode();

//   private:
//     void scheduleLeaderCrash();
//     void crashNode();
//     void recoverNode();

//     void resetElectionTimeout();
//     void cancelElectionTimeout();

//     void startElection();
//     void becomeFollower(int term);
//     void becomeLeader();

//     void sendRequestVote();
//     void sendVote(int targetId, int term, bool granted);
//     void sendHeartbeat();

//     void handleRequestVote(cPacket *pkt);
//     void handleVote(cPacket *pkt);
//     void handleHeartbeat(cPacket *pkt);

//     void sendToNode(cPacket *pkt, int targetId, bool electionMessage);
//     void broadcastToPeers(cPacket *pkt, bool electionMessage);

//     void enqueuePacket(cPacket *pkt, int gateIndex, bool electionMessage);
//     void processQueue(int gateIndex);

//     int aliveNodeCount() const;
//     int majorityNeeded() const;
//     bool isNodeAlive(int id) const;
// };

// #endif

///////////////////////// claude //////////////////////////
#ifndef __RAFTNODE_H__
#define __RAFTNODE_H__

#include <omnetpp.h>
#include <set>
#include <map>
#include <queue>
#include <vector>
#include <string>
#include <fstream>

using namespace omnetpp;

class RaftNode : public cSimpleModule
{
  private:
    enum NodeState { FOLLOWER, CANDIDATE, LEADER };

    // ── per-node identity ─────────────────────────────────────────────
    int nodeId;
    int numNodes;
    static constexpr int MSG_SIZE_BYTES = 10000;

    // ── liveness ──────────────────────────────────────────────────────
    bool alive = true;
    static std::vector<bool> aliveNodes;

    // ── Raft persistent state ─────────────────────────────────────────
    NodeState state      = FOLLOWER;
    int  currentTerm     = 0;
    int  votedFor        = -1;

    // ── Raft volatile state (candidate only) ──────────────────────────
    int  voteCount       = 0;
    std::set<int> votesReceived;

    // ── flood deduplication: (msgType, term, originatorId) ───────────
    // We keep the set bounded: only entries for currentTerm are kept.
    std::set<std::tuple<int,int,int>> seenFloods; // (term, originatorId, msgKind)
    // msgKind: 0=RequestVote, 1=Heartbeat

    // ── timers ────────────────────────────────────────────────────────
    cMessage *electionTimeoutEvent = nullptr;
    cMessage *heartbeatEvent       = nullptr;
    cMessage *crashEvent           = nullptr;
    cMessage *recoveryEvent        = nullptr;

    // ── per-gate serialised send queues ──────────────────────────────
    std::vector<std::queue<cPacket *>> sendQueues;
    std::vector<cMessage *>            sendEvents;

    // ── cluster-wide shared state (static = shared across all instances) ──
    static int    currentLeaderId;

    // Election epoch tracking.
    // An epoch covers the period from leader-crash to new-leader-elected.
    // Only one epoch is active at a time; zombie elections don't open a new one.
    static bool   epochInProgress;
    static double epochStartTime;
    static long   epochMessageCount;   // RequestVote + Vote only, not heartbeats

    // Aggregated results
    static double totalElectionTime;
    static long   totalElectionMessages;
    static int    completedElectionCount;

    // One-shot init guard, reset in finish() so run N+1 gets a clean slate
    static bool   globalInitDone;

    static void resetGlobalState(int n);

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

  public:
    virtual ~RaftNode();

  private:
    // crash / recovery
    void scheduleLeaderCrash();
    void crashNode();
    void recoverNode();

    // election timeout management
    void resetElectionTimeout();
    void cancelElectionTimeout();

    // Raft state transitions
    void startElection();
    void becomeFollower(int term);
    void becomeLeader();

    // message builders & senders
    void broadcastRequestVote();
    void sendVote(int candidateId, int term, bool granted);
    void broadcastHeartbeat();

    // message handlers
    void handleRequestVote(cPacket *pkt, int arrivalGate);
    void handleVote(cPacket *pkt);
    void handleHeartbeat(cPacket *pkt, int arrivalGate);

    // transport layer
    // Flood a packet to all gates except exceptGate (-1 = flood all)
    void floodToNeighbors(cPacket *pkt, int exceptGate, bool countable);
    // Unicast: find direct gate to targetId, or drop if no direct link
    void sendToNode(cPacket *pkt, int targetId, bool countable);

    void enqueuePacket(cPacket *pkt, int gateIndex, bool countable);
    void processQueue(int gateIndex);

    // helpers
    int  aliveNodeCount() const;
    int  majorityNeeded()  const;
    bool isNodeAlive(int id) const;
    void clearFloodCache();
};

#endif // __RAFTNODE_H__