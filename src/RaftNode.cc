#include "RaftNode.h"

Define_Module(RaftNode);

// ================= INIT =================
void RaftNode::initialize()
{
    nodeId   = getIndex();
    numNodes = getParentModule()->par("numNodes");

    electionTimeout = new cMessage("electionTimeout");
    heartbeatTimer  = new cMessage("heartbeatTimer");

    resetElectionTimeout();
}

// ================= HANDLE =================
void RaftNode::handleMessage(cMessage *msg)
{
    // ---- Timers ----
    if (msg == electionTimeout)
    {
        if (state != "LEADER")
            startElection();
        return;
    }

    if (msg == heartbeatTimer)
    {
        if (state == "LEADER")
        {
            cMessage *hb = new cMessage("Heartbeat");
            hb->addPar("term") = currentTerm;
            hb->addPar("originatorId") = nodeId;

            seenFloods.insert({"Heartbeat", currentTerm, nodeId});
            floodMessage(hb, -1);

            scheduleAt(simTime() + 0.3, heartbeatTimer); // ✅ faster heartbeat
        }
        return;
    }

    int arrivalGate = msg->getArrivalGate() ? msg->getArrivalGate()->getIndex() : -1;

    std::string type = msg->getName();

    if (!msg->hasPar("term") || !msg->hasPar("originatorId"))
    {
        delete msg;
        return;
    }

    int term = msg->par("term");
    int sender = msg->par("originatorId");

    // Step down if higher term
    if (term > currentTerm)
        becomeFollower(term);

    // ================= VOTE (NO DEDUP!) =================
    if (type == "Vote")
    {
        int target = msg->par("targetId");

        if (target == nodeId && state == "CANDIDATE" && term == currentTerm)
        {
            // ✅ prevent duplicate vote counting
            if (votesReceived.count(sender) == 0)
            {
                votesReceived.insert(sender);
                voteCount++;

                EV << "[VOTE] Node " << nodeId
                   << " received vote from " << sender
                   << " (total=" << voteCount << ")\n";

                resetElectionTimeout();   // ✅ prevent interruption

                if (voteCount > numNodes / 2)
                    becomeLeader();
            }
            delete msg;
        }
        else
        {
            sendToNode(msg, target);
        }
        return;
    }

    // ================= FLOOD DEDUP =================
    auto key = std::make_tuple(type, term, sender);

    if (seenFloods.count(key))
    {
        delete msg;
        return;
    }
    seenFloods.insert(key);

    // ================= REQUEST VOTE =================
    if (type == "RequestVote")
    {
        if (term == currentTerm &&
            (votedFor == -1 || votedFor == sender))
        {
            votedFor = sender;

            EV << "[VOTE_GRANT] Node " << nodeId
               << " votes for " << sender
               << " term " << currentTerm << "\n";

            resetElectionTimeout();  // ✅ critical fix

            cMessage *vote = new cMessage("Vote");
            vote->addPar("term") = currentTerm;
            vote->addPar("originatorId") = nodeId;
            vote->addPar("targetId") = sender;

            sendToNode(vote, sender);
        }

        floodMessage(msg, arrivalGate);
        return;
    }

    // ================= HEARTBEAT =================
    if (type == "Heartbeat")
    {
        if (term >= currentTerm)
        {
            becomeFollower(term);
            resetElectionTimeout();
        }

        floodMessage(msg, arrivalGate);
        return;
    }

    delete msg;
}

// ================= ELECTION =================
void RaftNode::startElection()
{
    state = "CANDIDATE";
    currentTerm++;
    votedFor = nodeId;

    voteCount = 1;
    votesReceived.clear();
    votesReceived.insert(nodeId);

    EV << "[ELECTION] Node " << nodeId
       << " starts election for term " << currentTerm << "\n";

    cMessage *rv = new cMessage("RequestVote");
    rv->addPar("term") = currentTerm;
    rv->addPar("originatorId") = nodeId;

    seenFloods.insert({"RequestVote", currentTerm, nodeId});
    floodMessage(rv, -1);

    resetElectionTimeout();
}

// ================= LEADER =================
void RaftNode::becomeLeader()
{
    if (state == "LEADER") return;

    state = "LEADER";

    EV << "[LEADER] Node " << nodeId
       << " became LEADER term " << currentTerm << "\n";

    cancelEvent(electionTimeout);

    cMessage *hb = new cMessage("Heartbeat");
    hb->addPar("term") = currentTerm;
    hb->addPar("originatorId") = nodeId;

    seenFloods.insert({"Heartbeat", currentTerm, nodeId});
    floodMessage(hb, -1);

    scheduleAt(simTime() + 0.3, heartbeatTimer);
}

// ================= FOLLOWER =================
void RaftNode::becomeFollower(int term)
{
    EV << "[FOLLOWER] Node " << nodeId
       << " becomes follower, term " << term << "\n";

    if (term > currentTerm)
    {
        currentTerm = term;
        votedFor = -1;   // ✅ reset ONLY on higher term
    }

    state = "FOLLOWER";

    if (heartbeatTimer->isScheduled())
        cancelEvent(heartbeatTimer);
}

// ================= FLOOD =================
void RaftNode::floodMessage(cMessage *msg, int exceptGate)
{
    int n = gateSize("outGate");

    for (int i = 0; i < n; i++)
    {
        if (i == exceptGate) continue;
        send(msg->dup(), "outGate", i);
    }

    delete msg;
}

// ================= UNICAST =================
void RaftNode::sendToNode(cMessage *msg, int targetId)
{
    int n = gateSize("outGate");

    for (int i = 0; i < n; i++)
    {
        cGate *g = gate("outGate", i);
        if (!g || !g->isConnected()) continue;

        cModule *peer = g->getNextGate()->getOwnerModule();
        if (peer && peer->getIndex() == targetId)
        {
            send(msg, "outGate", i);
            return;
        }
    }

    // fallback
    floodMessage(msg, -1);
}

// ================= TIMEOUT =================
void RaftNode::resetElectionTimeout()
{
    cancelEvent(electionTimeout);
    scheduleAt(simTime() + uniform(5.0, 10.0), electionTimeout); // ✅ larger range
}

// ================= CLEANUP =================
void RaftNode::finish()
{
    cancelAndDelete(electionTimeout);
    cancelAndDelete(heartbeatTimer);
}


// #include "RaftNode.h"

// Define_Module(RaftNode);

// // ============================================================
// // Initialize
// // ============================================================
// void RaftNode::initialize()
// {
//     nodeId   = getIndex();
//     numNodes = getParentModule()->par("numNodes");

//     electionTimeout = new cMessage("electionTimeout");
//     heartbeatTimer  = new cMessage("heartbeatTimer");

//     // Stagger startup so not every node fires simultaneously
//     resetElectionTimeout();
// }

// // ============================================================
// // Main message handler
// // ============================================================
// void RaftNode::handleMessage(cMessage *msg)
// {
//     // ---- Self-timers ----------------------------------------
//     if (msg == electionTimeout)
//     {
//         if (state != "LEADER")
//             startElection();
//         return;
//     }

//     if (msg == heartbeatTimer)
//     {
//         if (state == "LEADER")
//         {
//             // Build a fresh heartbeat and flood it
//             cMessage *hb = new cMessage("Heartbeat");
//             hb->addPar("term")        = currentTerm;
//             hb->addPar("originatorId") = nodeId;
//             // Mark it as seen so we don't re-flood our own heartbeat
//             seenFloods.insert(std::make_tuple(std::string("Heartbeat"),
//                                               currentTerm, nodeId));
//             floodMessage(hb, -1);
//             scheduleAt(simTime() + 0.3, heartbeatTimer);
//         }
//         return;
//     }

//     // ---- Network messages -----------------------------------
//     // Determine which gate this arrived on so we don't echo it back
//     int arrivalGate = msg->getArrivalGate()
//                         ? msg->getArrivalGate()->getIndex()
//                         : -1;

//     std::string type = msg->getName();

//     // Every network message must carry term + originatorId
//     if (!msg->hasPar("term") || !msg->hasPar("originatorId"))
//     {
//         delete msg;
//         return;
//     }

//     int term        = msg->par("term");
//     int originatorId = msg->par("originatorId");

//     // ========================
//     // VOTE (unicast – no flood dedup needed, just check it's for us)
//     // ========================
//     if (type == "Vote")
//     {
//         // Votes carry targetId; only the intended recipient processes them
//         if (!msg->hasPar("targetId")) { delete msg; return; }
//         int target = msg->par("targetId");

//         if (target == nodeId)
//         {
//             // It reached us – process
//             if (state == "CANDIDATE" && term == currentTerm)
//             {
//                 voteCount++;
//                 EV << "[VOTE] Node " << nodeId
//                    << " received vote from " << originatorId
//                    << " (total=" << voteCount << ")\n";

//                 if (voteCount > numNodes / 2)
//                     becomeLeader();
//             }
//             delete msg;
//         }
//         else
//         {
//             // Not for us – forward toward the target
//             sendToNode(msg, target);   // sendToNode deletes msg
//         }
//         return;
//     }

//     // ========================
//     // FLOOD MESSAGES: RequestVote, Heartbeat
//     // ========================

//     // Step down on higher term (for any flooded message)
//     if (term > currentTerm)
//         becomeFollower(term);

//     // Flood deduplication key: (type, term, originator)
//     auto floodKey = std::make_tuple(type, term, originatorId);

//     if (seenFloods.count(floodKey))
//     {
//         delete msg;
//         return;
//     }
//     seenFloods.insert(floodKey);

//     // ========================
//     // REQUEST VOTE
//     // ========================
//     if (type == "RequestVote")
//     {
//         // Grant vote if we haven't voted this term (or voted for same candidate)
//         if (term == currentTerm &&
//             (votedFor == -1 || votedFor == originatorId))
//         {
//             votedFor = originatorId;

//             EV << "[VOTE_GRANT] Node " << nodeId
//                << " votes for " << originatorId
//                << " term " << currentTerm << "\n";

//             // ✅ ADD THIS LINE
//             resetElectionTimeout();

//             cMessage *vote = new cMessage("Vote");
//             vote->addPar("term")        = currentTerm;
//             vote->addPar("originatorId") = nodeId;
//             vote->addPar("targetId")    = originatorId;

//             sendToNode(vote, originatorId);   // unicast, not flood
//         }

//         // Continue the flood so all other nodes also see the RequestVote
//         floodMessage(msg, arrivalGate);
//         return;
//     }

//     // ========================
//     // HEARTBEAT
//     // ========================
//     if (type == "Heartbeat")
//     {
//         if (term >= currentTerm)
//         {
//             becomeFollower(term);
//             resetElectionTimeout();
//         }

//         // Continue the flood
//         floodMessage(msg, arrivalGate);
//         return;
//     }

//     // Unknown message type
//     delete msg;
// }

// // ============================================================
// // Election
// // ============================================================
// void RaftNode::startElection()
// {
//     state        = "CANDIDATE";
//     currentTerm++;
//     votedFor     = nodeId;
//     voteCount    = 1;   // vote for self

//     EV << "[ELECTION] Node " << nodeId
//        << " starts election for term " << currentTerm << "\n";

//     cMessage *rv = new cMessage("RequestVote");
//     rv->addPar("term")        = currentTerm;
//     rv->addPar("originatorId") = nodeId;

//     // Mark as seen before flooding so our own re-arrival is ignored
//     seenFloods.insert(std::make_tuple(std::string("RequestVote"),
//                                       currentTerm, nodeId));
//     floodMessage(rv, -1);

//     resetElectionTimeout();
// }

// void RaftNode::becomeLeader()
// {
//     if (state == "LEADER") return;   // guard against double-promotion
//     state = "LEADER";

//     EV << "[LEADER] Node " << nodeId
//        << " became LEADER for term " << currentTerm << "\n";

//     cancelEvent(electionTimeout);

//     // Send immediate heartbeat then start periodic timer
//     cMessage *hb = new cMessage("Heartbeat");
//     hb->addPar("term")        = currentTerm;
//     hb->addPar("originatorId") = nodeId;
//     seenFloods.insert(std::make_tuple(std::string("Heartbeat"),
//                                       currentTerm, nodeId));
//     floodMessage(hb, -1);

//     if (!heartbeatTimer->isScheduled())
//         scheduleAt(simTime() + 0.3, heartbeatTimer);
// }

// void RaftNode::becomeFollower(int term)
// {
//     EV << "[FOLLOWER] Node " << nodeId
//        << " becomes follower, term " << term << "\n";

//     state       = "FOLLOWER";
//     // currentTerm = term;
//     // votedFor    = -1;
//     if (term > currentTerm) {
//         currentTerm = term;
//         votedFor = -1;   // ✅ reset ONLY on higher term
//     }

//     // Only cancel heartbeat timer if we were the leader
//     if (heartbeatTimer->isScheduled())
//         cancelEvent(heartbeatTimer);
// }

// // ============================================================
// // Flood: send msg on all gates except 'exceptGate'
// // ============================================================
// void RaftNode::floodMessage(cMessage *msg, int exceptGate)
// {
//     int n = gateSize("outGate");
//     bool sent = false;

//     for (int i = 0; i < n; i++)
//     {
//         if (i == exceptGate) continue;
//         send(msg->dup(), "outGate", i);
//         sent = true;
//     }

//     if (!sent)
//     {
//         // No gates to send on (single-node network or all blocked)
//     }
//     delete msg;
// }

// // ============================================================
// // Unicast toward targetId
// // ============================================================
// void RaftNode::sendToNode(cMessage *msg, int targetId)
// {
//     int n = gateSize("outGate");

//     // Try to find a gate whose far-end module index == targetId
//     for (int i = 0; i < n; i++)
//     {
//         cGate *g = gate("outGate", i);
//         if (!g || !g->isConnected()) continue;

//         cModule *peer = g->getNextGate()->getOwnerModule();
//         if (peer && peer->getIndex() == targetId)
//         {
//             send(msg, "outGate", i);
//             return;
//         }
//     }

//     // No direct link to target – fall back to flooding
//     // (works correctly in ring, tree, or sparse topologies)
//     floodMessage(msg, -1);
// }

// // ============================================================
// // Election timeout reset (randomised to avoid split votes)
// // ============================================================
// void RaftNode::resetElectionTimeout()
// {
//     cancelEvent(electionTimeout);
//     // Wide window (3–7 s) minimises split-vote probability
//     double timeout = uniform(3.0, 7.0);
//     scheduleAt(simTime() + timeout, electionTimeout);
// }

// // ============================================================
// // Cleanup
// // ============================================================
// void RaftNode::finish()
// {
//     cancelAndDelete(electionTimeout);
//     cancelAndDelete(heartbeatTimer);
// }