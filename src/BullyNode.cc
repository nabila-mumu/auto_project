//----------------------- previous without failure and recovery -----------------------

// //for checking difference
// #include <omnetpp.h>
// #include <fstream>
// #include <queue>

// using namespace omnetpp;

// class BullyNode : public cSimpleModule
// {
//   private:
//     int nodeId;
//     int numNodes;

//     int currentLeaderId = -1;
//     bool isLeader = false;
//     bool electionInProgress = false;

//     int okResponsesReceived = 0;

//     static int totalSentMessages;
//     static int totalReceivedMessages;

//     int sentMessages = 0;
//     int receivedMessages = 0;

//     simtime_t electionStartTime;
//     simtime_t electionEndTime;
//     simtime_t electionTime;

//     bool coordinatorReceived = false;

//     int msgSizeBytes;

//     // ✅ NEW: per-gate queues
//     std::vector<std::queue<cPacket*>> sendQueues;
//     std::vector<cMessage*> sendEvents;

//   protected:
//     virtual void initialize() override;
//     virtual void handleMessage(cMessage *msg) override;
//     virtual void finish() override;

//     void startElection();
//     void declareLeader();
//     void sendToHigherNodes(cPacket *pkt);
//     void sendToNode(int destId, cPacket *pkt);
//     void broadcastCoordinator();

//     void enqueuePacket(cPacket *pkt, int gateIndex);
//     void processQueue(int gateIndex);
// };

// Define_Module(BullyNode);

// /* ---------- STATIC ---------- */
// int BullyNode::totalSentMessages = 0;
// int BullyNode::totalReceivedMessages = 0;

// /* ---------- INIT ---------- */
// void BullyNode::initialize()
// {
//     nodeId = par("nodeId");
//     numNodes = getParentModule()->par("numNodes");

//     msgSizeBytes = 10000; // 10KB

//     int n = gateSize("outGate");
//     sendQueues.resize(n);
//     sendEvents.resize(n);

//     for (int i = 0; i < n; i++) {
//         sendEvents[i] = new cMessage(("sendEvent-" + std::to_string(i)).c_str());
//         sendEvents[i]->addPar("gateIndex") = i;
//     }

//     if (nodeId == 0) {
//         totalSentMessages = 0;
//         totalReceivedMessages = 0;
//     }

//     currentLeaderId = numNodes - 1;

//     scheduleAt(simTime() + uniform(0.1, 0.2), new cMessage("START"));
// }

// /* ---------- QUEUE SYSTEM ---------- */
// void BullyNode::enqueuePacket(cPacket *pkt, int gateIndex)
// {
//     cGate *g = gate("outGate", gateIndex);
//     cChannel *ch = g->getTransmissionChannel();

//     double lossRate = ch->par("lossRate").doubleValue();

//     // ✅ Apply packet loss
//     if (uniform(0,1) < lossRate) {
//         delete pkt;   // drop packet
//         return;
//     }

//     pkt->setByteLength(msgSizeBytes);
//     sendQueues[gateIndex].push(pkt);

//     if (!sendEvents[gateIndex]->isScheduled()) {
//         processQueue(gateIndex);
//     }
// }

// void BullyNode::processQueue(int gateIndex)
// {
//     if (sendQueues[gateIndex].empty()) return;

//     cGate *g = gate("outGate", gateIndex);
//     cChannel *ch = g->getTransmissionChannel();

//     simtime_t txFinish = ch->getTransmissionFinishTime();

//     if (txFinish <= simTime()) {
//         cPacket *pkt = sendQueues[gateIndex].front();
//         sendQueues[gateIndex].pop();

//         send(pkt, "outGate", gateIndex);

//         sentMessages++;
//         totalSentMessages++;

//         // schedule next if queue not empty
//         if (!sendQueues[gateIndex].empty()) {
//             simtime_t nextTime = g->getTransmissionChannel()->getTransmissionFinishTime();
//             scheduleAt(nextTime, sendEvents[gateIndex]);
//         }
//     } else {
//         scheduleAt(txFinish, sendEvents[gateIndex]);
//     }
// }

// /* ---------- SEND TO SPECIFIC NODE ---------- */
// void BullyNode::sendToNode(int destId, cPacket *pkt)
// {
//     for (int i = 0; i < gateSize("outGate"); i++) {
//         if (gate("outGate", i)->isConnected()) {

//             int id = gate("outGate", i)
//                 ->getPathEndGate()
//                 ->getOwnerModule()
//                 ->par("nodeId");

//             if (id == destId) {
//                 enqueuePacket(pkt, i);
//                 return;
//             }
//         }
//     }
//     delete pkt;
// }

// /* ---------- SEND TO HIGHER NODES ---------- */
// void BullyNode::sendToHigherNodes(cPacket *pkt)
// {
//     bool sent = false;

//     for (int i = 0; i < gateSize("outGate"); i++) {
//         if (gate("outGate", i)->isConnected()) {

//             int id = gate("outGate", i)
//                 ->getPathEndGate()
//                 ->getOwnerModule()
//                 ->par("nodeId");

//             if (id > nodeId) {
//                 enqueuePacket(pkt->dup(), i);
//                 sent = true;
//             }
//         }
//     }

//     delete pkt;

//     if (!sent) {
//         declareLeader();
//     }
// }

// /* ---------- START ELECTION ---------- */
// void BullyNode::startElection()
// {
//     if (electionInProgress) return;

//     electionInProgress = true;
//     electionStartTime = simTime();
//     okResponsesReceived = 0;

//     cPacket *pkt = new cPacket("ELECTION");
//     pkt->addPar("senderId") = nodeId;

//     sendToHigherNodes(pkt);

//     scheduleAt(simTime() + 0.5, new cMessage("TIMEOUT"));
// }

// /* ---------- DECLARE LEADER ---------- */
// void BullyNode::declareLeader()
// {
//     isLeader = true;
//     currentLeaderId = nodeId;
//     electionInProgress = false;

//     broadcastCoordinator();
// }

// /* ---------- BROADCAST ---------- */
// void BullyNode::broadcastCoordinator()
// {
//     for (int i = 0; i < gateSize("outGate"); i++) {
//         if (gate("outGate", i)->isConnected()) {

//             cPacket *pkt = new cPacket("COORD");
//             pkt->addPar("leaderId") = nodeId;

//             enqueuePacket(pkt, i);
//         }
//     }
// }

// /* ---------- HANDLE MESSAGE ---------- */
// void BullyNode::handleMessage(cMessage *msg)
// {
//     // ✅ queue send events
//     if (msg->isSelfMessage() && strstr(msg->getName(), "sendEvent")) {
//         int gateIndex = msg->par("gateIndex");
//         processQueue(gateIndex);
//         return;
//     }

//     if (msg->isSelfMessage()) {

//         if (strcmp(msg->getName(), "START") == 0) {
//             startElection();
//         }
//         else if (strcmp(msg->getName(), "TIMEOUT") == 0) {
//             if (electionInProgress && okResponsesReceived == 0) {
//                 declareLeader();
//             }
//         }
//         else if (strcmp(msg->getName(), "STOP_SIM") == 0) {
//             endSimulation();
//         }

//         delete msg;
//         return;
//     }

//     cPacket *pkt = check_and_cast<cPacket *>(msg);

//     receivedMessages++;
//     totalReceivedMessages++;

//     std::string type = pkt->getName();

//     /* ---------- ELECTION ---------- */
//     if (type == "ELECTION") {

//         int senderId = pkt->par("senderId");

//         if (nodeId > senderId) {

//             cPacket *ok = new cPacket("OK");
//             sendToNode(senderId, ok);

//             if (!electionInProgress)
//                 startElection();
//         }

//         delete pkt;
//     }

//     /* ---------- OK ---------- */
//     else if (type == "OK") {

//         okResponsesReceived++;
//         electionInProgress = false;

//         delete pkt;
//     }

//     /* ---------- COORD ---------- */
//     else if (type == "COORD") {

//         currentLeaderId = pkt->par("leaderId");
//         isLeader = (currentLeaderId == nodeId);
//         electionInProgress = false;

//         coordinatorReceived = true;

//         electionEndTime = simTime();
//         electionTime = electionEndTime - electionStartTime;

//         scheduleAt(simTime() + 0.5, new cMessage("STOP_SIM"));

//         delete pkt;
//     }
// }

// /* ---------- FINISH ---------- */
// void BullyNode::finish()
// {
//     if (nodeId != 0) return;

//     std::ofstream file("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);

//     if (!file.is_open()) return;

//     if (coordinatorReceived)
//         file << electionTime << "," << totalSentMessages << ",";

//     file.close();
// }

//----------------------- raw average msg_count -----------------------

#include "BullyNode.h"
#include <fstream>
#include <cstring>

Define_Module(BullyNode);

std::vector<bool> BullyNode::aliveNodes;

int BullyNode::currentLeaderId = -1;
int BullyNode::activeElectionId = 0;
bool BullyNode::globalElectionInProgress = false;

long BullyNode::totalSentMessages = 0;
long BullyNode::electionStartMessageCount = 0;

double BullyNode::electionStartTime = 0.0;
double BullyNode::totalElectionTime = 0.0;

long BullyNode::totalElectionMessages = 0;
int BullyNode::completedElectionCount = 0;

BullyNode::~BullyNode()
{
    for (int i = 0; i < (int)sendQueues.size(); i++) {
        while (!sendQueues[i].empty()) {
            delete sendQueues[i].front();
            sendQueues[i].pop();
        }
    }

    for (auto event : sendEvents) {
        if (event) {
            cancelAndDelete(event);
        }
    }

    if (crashEvent) cancelAndDelete(crashEvent);
    if (recoveryEvent) cancelAndDelete(recoveryEvent);
    if (detectLeaderDownEvent) cancelAndDelete(detectLeaderDownEvent);
    if (timeoutEvent) cancelAndDelete(timeoutEvent);
}

void BullyNode::initialize()
{
    nodeId = par("nodeId");
    numNodes = getParentModule()->par("numNodes");

    if ((int)aliveNodes.size() != numNodes) {
        aliveNodes.assign(numNodes, true);

        currentLeaderId = numNodes - 1;
        activeElectionId = 0;
        globalElectionInProgress = false;

        totalSentMessages = 0;
        electionStartMessageCount = 0;

        electionStartTime = 0.0;
        totalElectionTime = 0.0;

        totalElectionMessages = 0;
        completedElectionCount = 0;
    }

    alive = true;
    aliveNodes[nodeId] = true;

    isLeader = (nodeId == currentLeaderId);
    electionInProgress = false;
    okResponsesReceived = 0;

    int n = gateSize("outGate");
    sendQueues.resize(n);
    sendEvents.resize(n, nullptr);

    for (int i = 0; i < n; i++) {
        sendEvents[i] = new cMessage(("sendEvent-" + std::to_string(i)).c_str());
        sendEvents[i]->setKind(i);
    }

    crashEvent = new cMessage("CRASH");
    recoveryEvent = new cMessage("RECOVERY");
    detectLeaderDownEvent = new cMessage("DETECT_LEADER_DOWN");
    timeoutEvent = new cMessage("TIMEOUT");

    scheduleNextCrash();
}

void BullyNode::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (strncmp(msg->getName(), "sendEvent", 9) == 0) {
            processQueue(msg->getKind());
            return;
        }

        if (msg == crashEvent) {
            crashNode();
            return;
        }

        if (msg == recoveryEvent) {
            recoverNode();
            return;
        }

        if (msg == detectLeaderDownEvent) {
            if (alive) {
                startElection();
            }
            return;
        }

        if (msg == timeoutEvent) {
            if (alive && electionInProgress && okResponsesReceived == 0) {
                declareLeader();
            }
            return;
        }

        delete msg;
        return;
    }

    if (!alive) {
        delete msg;
        return;
    }

    cPacket *pkt = check_and_cast<cPacket *>(msg);
    std::string type = pkt->getName();

    if (type == "ELECTION") {
        int senderId = pkt->par("senderId");
        int electionId = pkt->par("electionId");

        if (electionId == activeElectionId && nodeId > senderId) {
            cPacket *ok = new cPacket("OK");
            ok->addPar("senderId") = nodeId;
            ok->addPar("electionId") = electionId;
            sendToNode(senderId, ok);

            if (!electionInProgress) {
                startElection();
            }
        }

        delete pkt;
        return;
    }

    if (type == "OK") {
        int electionId = pkt->par("electionId");

        if (electionId == activeElectionId && electionInProgress) {
            okResponsesReceived++;

            if (timeoutEvent->isScheduled()) {
                cancelEvent(timeoutEvent);
            }

            electionInProgress = false;
        }

        delete pkt;
        return;
    }

    if (type == "COORD") {
        int leaderId = pkt->par("leaderId");

        if (isNodeAlive(leaderId)) {
            currentLeaderId = leaderId;
            isLeader = (nodeId == leaderId);
            electionInProgress = false;

            if (timeoutEvent->isScheduled()) {
                cancelEvent(timeoutEvent);
            }
        }

        delete pkt;
        return;
    }

    delete pkt;
}

void BullyNode::scheduleNextCrash()
{
    if (!alive) {
        return;
    }

    if (crashEvent->isScheduled()) {
        return;
    }

    double crashTime = par("crashTime").doubleValue();

    if (crashTime >= 0) {
        scheduleAt(simTime() + crashTime, crashEvent);
    }
}

void BullyNode::crashNode()
{
    if (!alive) {
        return;
    }

    bool leaderFailed = (nodeId == currentLeaderId);

    EV << "Node " << nodeId << " crashed at " << simTime();

    if (leaderFailed) {
        EV << " leader failed";
    }

    EV << "\n";

    alive = false;
    aliveNodes[nodeId] = false;
    isLeader = false;
    electionInProgress = false;

    if (timeoutEvent->isScheduled()) {
        cancelEvent(timeoutEvent);
    }

    if (detectLeaderDownEvent->isScheduled()) {
        cancelEvent(detectLeaderDownEvent);
    }

    for (int i = 0; i < gateSize("outGate"); i++) {
        while (!sendQueues[i].empty()) {
            delete sendQueues[i].front();
            sendQueues[i].pop();
        }

        if (sendEvents[i]->isScheduled()) {
            cancelEvent(sendEvents[i]);
        }
    }

    double recoveryTime = par("recoveryTime").doubleValue();

    if (recoveryTime >= 0 && !recoveryEvent->isScheduled()) {
        scheduleAt(simTime() + recoveryTime, recoveryEvent);
    }

    if (leaderFailed) {
        scheduleElectionAfterLeaderCrash();
    }
}

void BullyNode::recoverNode()
{
    if (alive) {
        return;
    }

    alive = true;
    aliveNodes[nodeId] = true;

    isLeader = (nodeId == currentLeaderId);
    electionInProgress = false;
    okResponsesReceived = 0;

    EV << "Node " << nodeId << " recovered at " << simTime()
       << " and learned current leader = " << currentLeaderId << "\n";

    scheduleNextCrash();
}

void BullyNode::scheduleElectionAfterLeaderCrash()
{
    int starter = findLowestAliveNode();

    if (starter < 0) {
        currentLeaderId = -1;
        globalElectionInProgress = false;
        return;
    }

    activeElectionId++;
    globalElectionInProgress = true;

    electionStartTime = simTime().dbl();
    electionStartMessageCount = totalSentMessages;

    cModule *starterModule = getParentModule()->getSubmodule("node", starter);
    BullyNode *starterNode = check_and_cast<BullyNode *>(starterModule);

    simtime_t stopDelay = starterNode->par("stopDelay");

    if (!starterNode->detectLeaderDownEvent->isScheduled()) {
        starterNode->scheduleAt(simTime() + stopDelay, starterNode->detectLeaderDownEvent);
    }
}

void BullyNode::startElection()
{
    if (!alive || electionInProgress) {
        return;
    }

    if (!globalElectionInProgress) {
        activeElectionId++;
        globalElectionInProgress = true;

        electionStartTime = simTime().dbl();
        electionStartMessageCount = totalSentMessages;
    }

    electionInProgress = true;
    okResponsesReceived = 0;

    cPacket *pkt = new cPacket("ELECTION");
    pkt->addPar("senderId") = nodeId;
    pkt->addPar("electionId") = activeElectionId;

    sendToHigherNodes(pkt);

    if (!timeoutEvent->isScheduled()) {
        simtime_t electionTimeout = par("electionTimeout");
        scheduleAt(simTime() + electionTimeout, timeoutEvent);
    }
}

void BullyNode::declareLeader()
{
    if (!alive) {
        return;
    }

    int highestAlive = findHighestAliveNode();

    if (nodeId != highestAlive) {
        electionInProgress = false;
        return;
    }

    currentLeaderId = nodeId;
    isLeader = true;
    electionInProgress = false;

    if (timeoutEvent->isScheduled()) {
        cancelEvent(timeoutEvent);
    }

    broadcastCoordinator();

    if (globalElectionInProgress) {
        double thisElectionTime = simTime().dbl() - electionStartTime;
        long thisElectionMessages = totalSentMessages - electionStartMessageCount;

        totalElectionTime += thisElectionTime;
        totalElectionMessages += thisElectionMessages;
        completedElectionCount++;

        EV << "Election completed. New leader = " << nodeId
           << ", election time = " << thisElectionTime
           << ", messages = " << thisElectionMessages << "\n";

        globalElectionInProgress = false;
    }
}

void BullyNode::sendToHigherNodes(cPacket *pkt)
{
    bool sent = false;

    for (int i = 0; i < gateSize("outGate"); i++) {
        if (!gate("outGate", i)->isConnected()) {
            continue;
        }

        int destId = gate("outGate", i)
                         ->getPathEndGate()
                         ->getOwnerModule()
                         ->par("nodeId");

        if (destId > nodeId && isNodeAlive(destId)) {
            enqueuePacket(pkt->dup(), i);
            sent = true;
        }
    }

    delete pkt;

    if (!sent) {
        declareLeader();
    }
}

void BullyNode::sendToNode(int destId, cPacket *pkt)
{
    if (!isNodeAlive(destId)) {
        delete pkt;
        return;
    }

    for (int i = 0; i < gateSize("outGate"); i++) {
        if (!gate("outGate", i)->isConnected()) {
            continue;
        }

        int id = gate("outGate", i)
                     ->getPathEndGate()
                     ->getOwnerModule()
                     ->par("nodeId");

        if (id == destId) {
            enqueuePacket(pkt, i);
            return;
        }
    }

    delete pkt;
}

void BullyNode::broadcastCoordinator()
{
    for (int i = 0; i < gateSize("outGate"); i++) {
        if (!gate("outGate", i)->isConnected()) {
            continue;
        }

        int destId = gate("outGate", i)
                         ->getPathEndGate()
                         ->getOwnerModule()
                         ->par("nodeId");

        if (!isNodeAlive(destId)) {
            continue;
        }

        cPacket *pkt = new cPacket("COORD");
        pkt->addPar("leaderId") = nodeId;
        pkt->addPar("electionId") = activeElectionId;

        enqueuePacket(pkt, i);
    }
}

void BullyNode::enqueuePacket(cPacket *pkt, int gateIndex)
{
    if (!alive) {
        delete pkt;
        return;
    }

    cGate *g = gate("outGate", gateIndex);
    cChannel *ch = g->getTransmissionChannel();

    double lossRate = 0.0;

    if (ch && ch->hasPar("lossRate")) {
        lossRate = ch->par("lossRate").doubleValue();
    }

    totalSentMessages++;

    if (uniform(0, 1) < lossRate) {
        delete pkt;
        return;
    }

    pkt->setByteLength(msgSizeBytes);
    sendQueues[gateIndex].push(pkt);

    if (!sendEvents[gateIndex]->isScheduled()) {
        processQueue(gateIndex);
    }
}

void BullyNode::processQueue(int gateIndex)
{
    if (!alive || sendQueues[gateIndex].empty()) {
        return;
    }

    cGate *g = gate("outGate", gateIndex);
    cChannel *ch = g->getTransmissionChannel();

    simtime_t txFinish = ch ? ch->getTransmissionFinishTime() : simTime();

    if (txFinish <= simTime()) {
        cPacket *pkt = sendQueues[gateIndex].front();
        sendQueues[gateIndex].pop();

        send(pkt, "outGate", gateIndex);

        if (!sendQueues[gateIndex].empty()) {
            simtime_t nextTime = ch ? ch->getTransmissionFinishTime() : simTime();
            scheduleAt(nextTime, sendEvents[gateIndex]);
        }
    } else {
        scheduleAt(txFinish, sendEvents[gateIndex]);
    }
}

int BullyNode::findLowestAliveNode() const
{
    for (int i = 0; i < numNodes; i++) {
        if (isNodeAlive(i)) {
            return i;
        }
    }

    return -1;
}

int BullyNode::findHighestAliveNode() const
{
    for (int i = numNodes - 1; i >= 0; i--) {
        if (isNodeAlive(i)) {
            return i;
        }
    }

    return -1;
}

bool BullyNode::isNodeAlive(int id) const
{
    return id >= 0 && id < (int)aliveNodes.size() && aliveNodes[id];
}

void BullyNode::finish()
{
    if (nodeId != 0) {
        return;
    }

    std::ofstream file("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);

    if (!file.is_open()) {
        EV << "Could not open analysis.csv\n";
        return;
    }

    double avgElectionTime = 0.0;
    double avgMessageCount = 0.0;

    if (completedElectionCount > 0) {
        avgElectionTime = totalElectionTime / completedElectionCount;
        avgMessageCount = (double)totalElectionMessages / completedElectionCount;
    }

    file << avgElectionTime << "," << avgMessageCount << ",";
    file.close();

    EV << "Bully completed elections = " << completedElectionCount << "\n";
    EV << "Bully avg election time = " << avgElectionTime << "\n";
    EV << "Bully avg message count = " << avgMessageCount << "\n";
}