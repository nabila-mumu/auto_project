#ifndef __RAFTNODE_H_
#define __RAFTNODE_H_

#include <omnetpp.h>
#include <set>
#include <tuple>

using namespace omnetpp;

class RaftNode : public cSimpleModule
{
  private:
    int nodeId;
    int numNodes;

    int currentTerm = 0;
    int votedFor = -1;
    std::string state = "FOLLOWER";

    int voteCount = 0;
    std::set<int> votesReceived;   // ✅ track unique votes

    cMessage *electionTimeout = nullptr;
    cMessage *heartbeatTimer  = nullptr;

    // Dedup ONLY for flooded messages
    std::set<std::tuple<std::string,int,int>> seenFloods;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void startElection();
    void becomeLeader();
    void becomeFollower(int term);

    void floodMessage(cMessage *msg, int exceptGate);
    void sendToNode(cMessage *msg, int targetId);

    void resetElectionTimeout();
};

#endif

// #ifndef RAFTNODE_H
// #define RAFTNODE_H

// #include <omnetpp.h>
// #include <string>
// #include <set>
// #include <tuple>

// using namespace omnetpp;

// class RaftNode : public cSimpleModule
// {
// private:
//     // --- Node identity ---
//     int nodeId;
//     int numNodes;

//     // --- Persistent Raft state ---
//     int  currentTerm = 0;
//     int  votedFor    = -1;
//     std::string state = "FOLLOWER";

//     // --- Volatile candidate state ---
//     int voteCount = 0;

//     // --- Self-messages ---
//     cMessage *electionTimeout = nullptr;
//     cMessage *heartbeatTimer  = nullptr;

//     // --- Flood deduplication ---
//     // Key: (msgType, term, originatorId, seqNo)
//     // seqNo lets the same originator send fresh floods in later terms
//     std::set<std::tuple<std::string,int,int>> seenFloods;

//     // --- Helpers ---
//     void startElection();
//     void becomeLeader();
//     void becomeFollower(int term);

//     void resetElectionTimeout();

//     // Sends msg out on every gate EXCEPT the gate it arrived on.
//     // gateIndex == -1 means "came from self, send on all gates".
//     void floodMessage(cMessage *msg, int exceptGate);

//     // Sends msg directly on the gate that leads toward targetId.
//     // In a fully-connected topology every node is a direct neighbour,
//     // so we find the gate whose far-end module index == targetId.
//     // Falls back to flood if no direct gate found.
//     void sendToNode(cMessage *msg, int targetId);

// protected:
//     virtual void initialize() override;
//     virtual void handleMessage(cMessage *msg) override;
//     virtual void finish() override;
// };

// #endif // RAFTNODE_H