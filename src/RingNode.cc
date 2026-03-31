#include "RingNode.h"
#include <fstream>
#include <climits>
#include <queue>

using namespace omnetpp;

Define_Module(RingNode);

/* ---------- STATIC MEMBERS ---------- */
int RingNode::globalElectionId = 0;
int RingNode::totalMessages = 0;
int RingNode::currentLeaderId = -1;

/* ---------- INITIALIZATION ---------- */
void RingNode::initialize()
{
    nodeId = par("nodeId");
    successor = -1;

    electionInProgress = false;
    isLeader = false;
    electionId = -1;

    messagesSent = 0;
    messagesReceived = 0;

    msgSizeBytes = 10000; // ✅ ADD

    int n = gateSize("outGate");
    sendQueues.resize(n);
    sendEvents.resize(n);

    for (int i = 0; i < n; i++) {
        sendEvents[i] = new cMessage(("sendEvent-" + std::to_string(i)).c_str());
        sendEvents[i]->addPar("gateIndex") = i;
    }

    if (nodeId == 0) {
        totalMessages = 0;
        currentLeaderId = -1;
    }

    coordinatorReceived = false;

    buildGateMap();

    scheduleAt(simTime() + uniform(0.05, 0.15), new cMessage("START_ELECTION"));
}

/* ---------- BUILD SUCCESSOR MAP ---------- */
void RingNode::buildGateMap()
{
    neighborToGateIndex.clear();
    neighbors.clear();

    for (int i = 0; i < gateSize("outGate"); i++) {
        cGate *g = gate("outGate", i);
        if (g->isConnected()) {
            int destId = g->getPathEndGate()->getOwnerModule()->par("nodeId");
            neighborToGateIndex[destId] = i;
            neighbors.insert(destId);
        }
    }

    int minGreater = INT_MAX;
    int minOverall = INT_MAX;

    for (int n : neighbors) {
        if (n > nodeId && n < minGreater)
            minGreater = n;
        if (n < minOverall)
            minOverall = n;
    }

    successor = (minGreater != INT_MAX) ? minGreater : minOverall;
}

/* ---------- QUEUE SYSTEM ---------- */
void RingNode::enqueuePacket(cPacket *pkt, int gateIndex)
{
    pkt->setByteLength(msgSizeBytes);
    sendQueues[gateIndex].push(pkt);

    if (!sendEvents[gateIndex]->isScheduled()) {
        processQueue(gateIndex);
    }
}

void RingNode::processQueue(int gateIndex)
{
    if (sendQueues[gateIndex].empty()) return;

    cGate *g = gate("outGate", gateIndex);
    cChannel *ch = g->getTransmissionChannel();

    simtime_t txFinish = ch->getTransmissionFinishTime();

    if (txFinish <= simTime()) {
        cPacket *pkt = sendQueues[gateIndex].front();
        sendQueues[gateIndex].pop();

        send(pkt, "outGate", gateIndex);

        messagesSent++;
        totalMessages++;

        if (!sendQueues[gateIndex].empty()) {
            simtime_t nextTime = g->getTransmissionChannel()->getTransmissionFinishTime();
            scheduleAt(nextTime, sendEvents[gateIndex]);
        }
    } else {
        scheduleAt(txFinish, sendEvents[gateIndex]);
    }
}

/* ---------- SEND TO SUCCESSOR ---------- */
void RingNode::sendToSuccessor(cPacket *pkt)
{
    auto it = neighborToGateIndex.find(successor);
    if (it == neighborToGateIndex.end()) {
        delete pkt;
        return;
    }

    enqueuePacket(pkt, it->second);
}

/* ---------- START ELECTION ---------- */
void RingNode::startElection()
{
    electionInProgress = true;
    electionId = ++globalElectionId;
    electionStartTime = simTime();

    cPacket *pkt = new cPacket("ELECTION");
    pkt->addPar("initiatorId") = nodeId;
    pkt->addPar("electionId") = electionId;
    pkt->addPar("maxId") = nodeId;
    pkt->addPar("startTime").setDoubleValue(simTime().dbl());

    sendToSuccessor(pkt);
}

/* ---------- HANDLE MESSAGE ---------- */
void RingNode::handleMessage(cMessage *msg)
{
    // queue events
    if (msg->isSelfMessage() && strstr(msg->getName(), "sendEvent")) {
        int gateIndex = msg->par("gateIndex");
        processQueue(gateIndex);
        return;
    }

    // stop simulation
    if (msg->isSelfMessage() && strcmp(msg->getName(), "STOP_SIM") == 0) {
        endSimulation();
        delete msg;
        return;
    }

    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "START_ELECTION") == 0) {
            startElection();
        }
        delete msg;
        return;
    }

    cPacket *pkt = check_and_cast<cPacket *>(msg);

    messagesReceived++;
    totalMessages++;

    /* ---------- ELECTION ---------- */
    if (strcmp(pkt->getName(), "ELECTION") == 0) {
        int msgElectionId = pkt->par("electionId");
        int initiatorId = pkt->par("initiatorId");
        int maxId = pkt->par("maxId");

        if (msgElectionId < electionId) {
            delete pkt;
            return;
        }

        if (msgElectionId > electionId) {
            electionId = msgElectionId;
            electionInProgress = true;
        }

        if (nodeId > maxId) {
            pkt->par("maxId") = nodeId;
        }

        if (nodeId == initiatorId) {
            int leader = pkt->par("maxId");

            electionEndTime = simTime();
            currentLeaderId = leader;
            isLeader = (leader == nodeId);
            electionInProgress = false;

            simtime_t start = SimTime(pkt->par("startTime").doubleValue());
            electionTime = simTime() - start;

            cPacket *coord = new cPacket("COORDINATOR");
            coord->addPar("leaderId") = leader;
            coord->addPar("initiatorId") = nodeId;
            coord->addPar("startTime") = pkt->par("startTime");

            delete pkt;
            sendToSuccessor(coord);
            return;
        }

        sendToSuccessor(pkt);
        return;
    }

    /* ---------- COORDINATOR ---------- */
    if (strcmp(pkt->getName(), "COORDINATOR") == 0) {
        int leaderId = pkt->par("leaderId");
        int initiatorId = pkt->par("initiatorId");

        coordinatorReceived = true;

        currentLeaderId = leaderId;
        electionInProgress = false;
        isLeader = (leaderId == nodeId);

        simtime_t start = SimTime(pkt->par("startTime").doubleValue());
        electionTime = simTime() - start;

        // ✅ ONLY STOP AFTER FULL CYCLE
        if (nodeId == initiatorId) {
            delete pkt;
            scheduleAt(simTime() + 0.01, new cMessage("STOP_SIM"));
            return;
        }

        sendToSuccessor(pkt);
        return;
    }
}

/* ---------- FINISH ---------- */
void RingNode::finish()
{
    if (nodeId != 0) return;

    std::ofstream csvFile("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);

    if (!csvFile.is_open()) return;

    if (currentLeaderId != -1) {
        csvFile << electionTime << "," << totalMessages << ",";
    } else {
        csvFile << "," << ",";
    }

    csvFile.close();
}