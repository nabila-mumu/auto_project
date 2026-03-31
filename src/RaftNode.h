// ------------------- till now last version ------------------
#ifndef __RAFTNODE_H__
#define __RAFTNODE_H__

#include <omnetpp.h>
#include <set>
#include <tuple>
#include <string>
#include <fstream>

using namespace omnetpp;

class RaftNode : public cSimpleModule
{
  private:
    // ================= BASIC =================
    int nodeId;
    int numNodes;
    int msgSizeBytes; // ✅ ADD (10KB)

    std::string state;   // FOLLOWER / CANDIDATE / LEADER

    int currentTerm = 0;
    int votedFor = -1;

    // ================= ELECTION =================
    cMessage *electionTimeout = nullptr;
    cMessage *heartbeatTimer = nullptr;

    int voteCount = 0;
    std::set<int> votesReceived;

    double electionStartTime = 0;

    // ================= FLOOD CONTROL =================
    std::set<std::tuple<std::string, int, int>> seenFloods;
    // (messageType, term, originatorId)

    // ================= GLOBAL METRICS =================
    static int globalMessageCount;
    static bool leaderElected;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    // ================= CORE FUNCTIONS =================
    void startElection();
    void becomeLeader();
    void becomeFollower(int term);

    // ================= COMMUNICATION =================
    // void floodMessage(cMessage *msg, int exceptGate);
    // void sendToNode(cMessage *msg, int targetId);

    void floodMessage(cPacket *pkt, int exceptGate);
    void sendToNode(cPacket *pkt, int targetId);
  

    // ================= TIME =================
    void resetElectionTimeout();
};

#endif