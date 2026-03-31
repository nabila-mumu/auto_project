// -----------------------31.03.26 -----------------------

//for checking difference
#include <omnetpp.h>
#include <fstream>
#include <queue>

using namespace omnetpp;

class BullyNode : public cSimpleModule
{
  private:
    int nodeId;
    int numNodes;

    int currentLeaderId = -1;
    bool isLeader = false;
    bool electionInProgress = false;

    int okResponsesReceived = 0;

    static int totalSentMessages;
    static int totalReceivedMessages;

    int sentMessages = 0;
    int receivedMessages = 0;

    simtime_t electionStartTime;
    simtime_t electionEndTime;
    simtime_t electionTime;

    bool coordinatorReceived = false;

    int msgSizeBytes;

    // ✅ NEW: per-gate queues
    std::vector<std::queue<cPacket*>> sendQueues;
    std::vector<cMessage*> sendEvents;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void startElection();
    void declareLeader();
    void sendToHigherNodes(cPacket *pkt);
    void sendToNode(int destId, cPacket *pkt);
    void broadcastCoordinator();

    void enqueuePacket(cPacket *pkt, int gateIndex);
    void processQueue(int gateIndex);
};

Define_Module(BullyNode);

/* ---------- STATIC ---------- */
int BullyNode::totalSentMessages = 0;
int BullyNode::totalReceivedMessages = 0;

/* ---------- INIT ---------- */
void BullyNode::initialize()
{
    nodeId = par("nodeId");
    numNodes = getParentModule()->par("numNodes");

    msgSizeBytes = 10000; // 10KB

    int n = gateSize("outGate");
    sendQueues.resize(n);
    sendEvents.resize(n);

    for (int i = 0; i < n; i++) {
        sendEvents[i] = new cMessage(("sendEvent-" + std::to_string(i)).c_str());
        sendEvents[i]->addPar("gateIndex") = i;
    }

    if (nodeId == 0) {
        totalSentMessages = 0;
        totalReceivedMessages = 0;
    }

    currentLeaderId = numNodes - 1;

    scheduleAt(simTime() + uniform(0.1, 0.2), new cMessage("START"));
}

/* ---------- QUEUE SYSTEM ---------- */
void BullyNode::enqueuePacket(cPacket *pkt, int gateIndex)
{
    cGate *g = gate("outGate", gateIndex);
    cChannel *ch = g->getTransmissionChannel();

    double lossRate = ch->par("lossRate").doubleValue();

    // ✅ Apply packet loss
    if (uniform(0,1) < lossRate) {
        delete pkt;   // drop packet
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
    if (sendQueues[gateIndex].empty()) return;

    cGate *g = gate("outGate", gateIndex);
    cChannel *ch = g->getTransmissionChannel();

    simtime_t txFinish = ch->getTransmissionFinishTime();

    if (txFinish <= simTime()) {
        cPacket *pkt = sendQueues[gateIndex].front();
        sendQueues[gateIndex].pop();

        send(pkt, "outGate", gateIndex);

        sentMessages++;
        totalSentMessages++;

        // schedule next if queue not empty
        if (!sendQueues[gateIndex].empty()) {
            simtime_t nextTime = g->getTransmissionChannel()->getTransmissionFinishTime();
            scheduleAt(nextTime, sendEvents[gateIndex]);
        }
    } else {
        scheduleAt(txFinish, sendEvents[gateIndex]);
    }
}

/* ---------- SEND TO SPECIFIC NODE ---------- */
void BullyNode::sendToNode(int destId, cPacket *pkt)
{
    for (int i = 0; i < gateSize("outGate"); i++) {
        if (gate("outGate", i)->isConnected()) {

            int id = gate("outGate", i)
                ->getPathEndGate()
                ->getOwnerModule()
                ->par("nodeId");

            if (id == destId) {
                enqueuePacket(pkt, i);
                return;
            }
        }
    }
    delete pkt;
}

/* ---------- SEND TO HIGHER NODES ---------- */
void BullyNode::sendToHigherNodes(cPacket *pkt)
{
    bool sent = false;

    for (int i = 0; i < gateSize("outGate"); i++) {
        if (gate("outGate", i)->isConnected()) {

            int id = gate("outGate", i)
                ->getPathEndGate()
                ->getOwnerModule()
                ->par("nodeId");

            if (id > nodeId) {
                enqueuePacket(pkt->dup(), i);
                sent = true;
            }
        }
    }

    delete pkt;

    if (!sent) {
        declareLeader();
    }
}

/* ---------- START ELECTION ---------- */
void BullyNode::startElection()
{
    if (electionInProgress) return;

    electionInProgress = true;
    electionStartTime = simTime();
    okResponsesReceived = 0;

    cPacket *pkt = new cPacket("ELECTION");
    pkt->addPar("senderId") = nodeId;

    sendToHigherNodes(pkt);

    scheduleAt(simTime() + 0.5, new cMessage("TIMEOUT"));
}

/* ---------- DECLARE LEADER ---------- */
void BullyNode::declareLeader()
{
    isLeader = true;
    currentLeaderId = nodeId;
    electionInProgress = false;

    broadcastCoordinator();
}

/* ---------- BROADCAST ---------- */
void BullyNode::broadcastCoordinator()
{
    for (int i = 0; i < gateSize("outGate"); i++) {
        if (gate("outGate", i)->isConnected()) {

            cPacket *pkt = new cPacket("COORD");
            pkt->addPar("leaderId") = nodeId;

            enqueuePacket(pkt, i);
        }
    }
}

/* ---------- HANDLE MESSAGE ---------- */
void BullyNode::handleMessage(cMessage *msg)
{
    // ✅ queue send events
    if (msg->isSelfMessage() && strstr(msg->getName(), "sendEvent")) {
        int gateIndex = msg->par("gateIndex");
        processQueue(gateIndex);
        return;
    }

    if (msg->isSelfMessage()) {

        if (strcmp(msg->getName(), "START") == 0) {
            startElection();
        }
        else if (strcmp(msg->getName(), "TIMEOUT") == 0) {
            if (electionInProgress && okResponsesReceived == 0) {
                declareLeader();
            }
        }
        else if (strcmp(msg->getName(), "STOP_SIM") == 0) {
            endSimulation();
        }

        delete msg;
        return;
    }

    cPacket *pkt = check_and_cast<cPacket *>(msg);

    receivedMessages++;
    totalReceivedMessages++;

    std::string type = pkt->getName();

    /* ---------- ELECTION ---------- */
    if (type == "ELECTION") {

        int senderId = pkt->par("senderId");

        if (nodeId > senderId) {

            cPacket *ok = new cPacket("OK");
            sendToNode(senderId, ok);

            if (!electionInProgress)
                startElection();
        }

        delete pkt;
    }

    /* ---------- OK ---------- */
    else if (type == "OK") {

        okResponsesReceived++;
        electionInProgress = false;

        delete pkt;
    }

    /* ---------- COORD ---------- */
    else if (type == "COORD") {

        currentLeaderId = pkt->par("leaderId");
        isLeader = (currentLeaderId == nodeId);
        electionInProgress = false;

        coordinatorReceived = true;

        electionEndTime = simTime();
        electionTime = electionEndTime - electionStartTime;

        scheduleAt(simTime() + 0.5, new cMessage("STOP_SIM"));

        delete pkt;
    }
}

/* ---------- FINISH ---------- */
void BullyNode::finish()
{
    if (nodeId != 0) return;

    std::ofstream file("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);

    if (!file.is_open()) return;

    if (coordinatorReceived)
        file << electionTime << "," << totalSentMessages << ",";

    file.close();
}
// ---------------- bandwidth not fixed -----------------------
// #include <omnetpp.h>
// #include <fstream>

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

//   protected:
//     virtual void initialize() override;
//     virtual void handleMessage(cMessage *msg) override;
//     virtual void finish() override;

//     void startElection();
//     void declareLeader();
//     void sendToHigherNodes(cMessage *msg);
//     void sendToNode(int destId, cMessage *msg);
//     void broadcastCoordinator();
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

//     if (nodeId == 0) {
//         totalSentMessages = 0;
//         totalReceivedMessages = 0;
//     }

//     currentLeaderId = numNodes - 1;

//     scheduleAt(simTime() + uniform(0.1, 0.2), new cMessage("START"));
// }

// /* ---------- SEND ---------- */
// void BullyNode::sendToNode(int destId, cMessage *msg)
// {
//     for (int i = 0; i < gateSize("outGate"); i++) {
//         if (gate("outGate", i)->isConnected()) {

//             int id = gate("outGate", i)
//                 ->getPathEndGate()
//                 ->getOwnerModule()
//                 ->par("nodeId");

//             if (id == destId) {
//                 send(msg, "outGate", i);
//                 sentMessages++;
//                 totalSentMessages++;
//                 return;
//             }
//         }
//     }
//     delete msg;
// }

// void BullyNode::sendToHigherNodes(cMessage *msg)
// {
//     bool sent = false;

//     for (int i = 0; i < gateSize("outGate"); i++) {
//         if (gate("outGate", i)->isConnected()) {

//             int id = gate("outGate", i)
//                 ->getPathEndGate()
//                 ->getOwnerModule()
//                 ->par("nodeId");

//             if (id > nodeId) {
//                 send(msg->dup(), "outGate", i);
//                 sentMessages++;
//                 totalSentMessages++;
//                 sent = true;
//             }
//         }
//     }

//     delete msg;

//     if (!sent) {
//         declareLeader();
//     }
// }

// /* ---------- START ---------- */
// void BullyNode::startElection()
// {
//     if (electionInProgress) return;

//     electionInProgress = true;
//     electionStartTime = simTime();
//     okResponsesReceived = 0;

//     cMessage *msg = new cMessage("ELECTION");
//     msg->addPar("senderId") = nodeId;

//     sendToHigherNodes(msg);

//     scheduleAt(simTime() + 0.5, new cMessage("TIMEOUT"));
// }

// /* ---------- DECLARE ---------- */
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
//             cMessage *msg = new cMessage("COORD");
//             msg->addPar("leaderId") = nodeId;

//             send(msg, "outGate", i);
//             sentMessages++;
//             totalSentMessages++;
//         }
//     }
// }

// /* ---------- HANDLE ---------- */
// void BullyNode::handleMessage(cMessage *msg)
// {
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

//     receivedMessages++;
//     totalReceivedMessages++;

//     std::string type = msg->getName();

//     /* ---------- ELECTION ---------- */
//     if (type == "ELECTION") {

//         int senderId = msg->par("senderId");

//         if (nodeId > senderId) {

//             cMessage *ok = new cMessage("OK");
//             sendToNode(senderId, ok);

//             if (!electionInProgress)
//                 startElection();
//         }

//         delete msg;
//     }

//     /* ---------- OK ---------- */
//     else if (type == "OK") {

//         okResponsesReceived++;
//         electionInProgress = false;

//         delete msg;
//     }

//     /* ---------- COORD ---------- */
//     else if (type == "COORD") {

//         currentLeaderId = msg->par("leaderId");
//         isLeader = (currentLeaderId == nodeId);
//         electionInProgress = false;

//         coordinatorReceived = true;

//         electionEndTime = simTime();
//         electionTime = electionEndTime - electionStartTime;

//         scheduleAt(simTime() + 0.5, new cMessage("STOP_SIM"));

//         delete msg;
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
