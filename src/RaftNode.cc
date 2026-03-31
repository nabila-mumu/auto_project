#include "RaftNode.h"

Define_Module(RaftNode);

int RaftNode::globalMessageCount = 0;
bool RaftNode::leaderElected = false;

// ================= INIT =================
void RaftNode::initialize()
{
    nodeId = getIndex();
    numNodes = getParentModule()->par("numNodes");

    state = "FOLLOWER";
    currentTerm = 0;
    votedFor = -1;

    msgSizeBytes = 10000; // ✅ ADD (10KB)

    electionTimeout = new cMessage("electionTimeout");
    heartbeatTimer = new cMessage("heartbeatTimer");

    resetElectionTimeout();
}

// ================= HANDLE =================
void RaftNode::handleMessage(cMessage *msg)
{
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
            cPacket *hb = new cPacket("Heartbeat");
            hb->setByteLength(msgSizeBytes);
            hb->addPar("term") = currentTerm;
            hb->addPar("originatorId") = nodeId;

            seenFloods.insert({"Heartbeat", currentTerm, nodeId});
            floodMessage(hb, -1);

            scheduleAt(simTime() + 0.5, heartbeatTimer);
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

    if (term > currentTerm)
        becomeFollower(term);

    cPacket *pkt = check_and_cast<cPacket *>(msg);

    // ================= VOTE =================
    if (type == "Vote")
    {
        int target = pkt->par("targetId");

        if (target == nodeId && state == "CANDIDATE" && term == currentTerm)
        {
            if (votesReceived.count(sender) == 0)
            {
                votesReceived.insert(sender);
                voteCount++;

                if (voteCount > numNodes / 2)
                    becomeLeader();
            }
            delete pkt;
        }
        else
        {
            sendToNode(pkt, target);
        }
        return;
    }

    // ================= FLOOD DEDUP =================
    auto key = std::make_tuple(type, term, sender);
    if (seenFloods.count(key))
    {
        delete pkt;
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

            resetElectionTimeout();

            cPacket *vote = new cPacket("Vote");
            vote->setByteLength(msgSizeBytes);
            vote->addPar("term") = currentTerm;
            vote->addPar("originatorId") = nodeId;
            vote->addPar("targetId") = sender;

            sendToNode(vote, sender);
        }

        floodMessage(pkt, arrivalGate);
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

        floodMessage(pkt, arrivalGate);
        return;
    }

    delete pkt;
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

    electionStartTime = simTime().dbl();

    cPacket *rv = new cPacket("RequestVote");
    rv->setByteLength(msgSizeBytes);
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

    double electionEndTime = simTime().dbl();

    if (!leaderElected)
    {
        leaderElected = true;

        std::ofstream csvFile;
        csvFile.open("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);

        csvFile << (electionEndTime - electionStartTime) << ","
                << globalMessageCount;

        csvFile.close();

        endSimulation();
    }

    scheduleAt(simTime() + 0.5, heartbeatTimer);
}

// ================= FOLLOWER =================
void RaftNode::becomeFollower(int term)
{
    if (term > currentTerm)
    {
        currentTerm = term;
        votedFor = -1;
    }

    state = "FOLLOWER";

    if (heartbeatTimer->isScheduled())
        cancelEvent(heartbeatTimer);
}

// ================= FLOOD =================
void RaftNode::floodMessage(cPacket *pkt, int exceptGate)
{
    int n = gateSize("outGate");

    for (int i = 0; i < n; i++)
    {
        if (i == exceptGate) continue;

        send(pkt->dup(), "outGate", i);  // ✅ FIXED

        globalMessageCount++;
    }

    delete pkt;
}

// ================= UNICAST =================
void RaftNode::sendToNode(cPacket *pkt, int targetId)
{
    int n = gateSize("outGate");

    for (int i = 0; i < n; i++)
    {
        cGate *g = gate("outGate", i);
        if (!g || !g->isConnected()) continue;

        cModule *peer = g->getNextGate()->getOwnerModule();
        if (peer && peer->getIndex() == targetId)
        {
            send(pkt, "outGate", i);  // ✅ FIXED
            globalMessageCount++;
            return;
        }
    }

    floodMessage(pkt, -1);
}

// ================= TIMEOUT =================
void RaftNode::resetElectionTimeout()
{
    cancelEvent(electionTimeout);

    scheduleAt(simTime() + uniform(0.15, 0.5), electionTimeout);
}

// ================= CLEANUP =================
void RaftNode::finish()
{
    cancelAndDelete(electionTimeout);
    cancelAndDelete(heartbeatTimer);
}