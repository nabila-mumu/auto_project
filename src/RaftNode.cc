//-------------------- previous version without failure and recovery ------------------
// #include "RaftNode.h"

// Define_Module(RaftNode);

// int RaftNode::globalMessageCount = 0;
// bool RaftNode::leaderElected = false;

// // ================= INIT =================
// void RaftNode::initialize()
// {
//     nodeId = getIndex();
//     numNodes = getParentModule()->par("numNodes");

//     state = "FOLLOWER";
//     currentTerm = 0;
//     votedFor = -1;

//     msgSizeBytes = 10000; // ✅ ADD (10KB)

//     electionTimeout = new cMessage("electionTimeout");
//     heartbeatTimer = new cMessage("heartbeatTimer");

//     resetElectionTimeout();
// }

// // ================= HANDLE =================
// void RaftNode::handleMessage(cMessage *msg)
// {
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
//             cPacket *hb = new cPacket("Heartbeat");
//             hb->setByteLength(msgSizeBytes);
//             hb->addPar("term") = currentTerm;
//             hb->addPar("originatorId") = nodeId;

//             seenFloods.insert({"Heartbeat", currentTerm, nodeId});
//             floodMessage(hb, -1);

//             scheduleAt(simTime() + 0.5, heartbeatTimer);
//         }
//         return;
//     }

//     int arrivalGate = msg->getArrivalGate() ? msg->getArrivalGate()->getIndex() : -1;

//     std::string type = msg->getName();

//     if (!msg->hasPar("term") || !msg->hasPar("originatorId"))
//     {
//         delete msg;
//         return;
//     }

//     int term = msg->par("term");
//     int sender = msg->par("originatorId");

//     if (term > currentTerm)
//         becomeFollower(term);

//     cPacket *pkt = check_and_cast<cPacket *>(msg);

//     // ================= VOTE =================
//     if (type == "Vote")
//     {
//         int target = pkt->par("targetId");

//         if (target == nodeId && state == "CANDIDATE" && term == currentTerm)
//         {
//             if (votesReceived.count(sender) == 0)
//             {
//                 votesReceived.insert(sender);
//                 voteCount++;

//                 if (voteCount > numNodes / 2)
//                     becomeLeader();
//             }
//             delete pkt;
//         }
//         else
//         {
//             sendToNode(pkt, target);
//         }
//         return;
//     }

//     // ================= FLOOD DEDUP =================
//     auto key = std::make_tuple(type, term, sender);
//     if (seenFloods.count(key))
//     {
//         delete pkt;
//         return;
//     }
//     seenFloods.insert(key);

//     // ================= REQUEST VOTE =================
//     if (type == "RequestVote")
//     {
//         if (term == currentTerm &&
//             (votedFor == -1 || votedFor == sender))
//         {
//             votedFor = sender;

//             resetElectionTimeout();

//             cPacket *vote = new cPacket("Vote");
//             vote->setByteLength(msgSizeBytes);
//             vote->addPar("term") = currentTerm;
//             vote->addPar("originatorId") = nodeId;
//             vote->addPar("targetId") = sender;

//             sendToNode(vote, sender);
//         }

//         floodMessage(pkt, arrivalGate);
//         return;
//     }

//     // ================= HEARTBEAT =================
//     if (type == "Heartbeat")
//     {
//         if (term >= currentTerm)
//         {
//             becomeFollower(term);
//             resetElectionTimeout();
//         }

//         floodMessage(pkt, arrivalGate);
//         return;
//     }

//     delete pkt;
// }

// // ================= ELECTION =================
// void RaftNode::startElection()
// {
//     state = "CANDIDATE";
//     currentTerm++;
//     votedFor = nodeId;

//     voteCount = 1;
//     votesReceived.clear();
//     votesReceived.insert(nodeId);

//     electionStartTime = simTime().dbl();

//     cPacket *rv = new cPacket("RequestVote");
//     rv->setByteLength(msgSizeBytes);
//     rv->addPar("term") = currentTerm;
//     rv->addPar("originatorId") = nodeId;

//     seenFloods.insert({"RequestVote", currentTerm, nodeId});
//     floodMessage(rv, -1);

//     resetElectionTimeout();
// }

// // ================= LEADER =================
// void RaftNode::becomeLeader()
// {
//     if (state == "LEADER") return;

//     state = "LEADER";

//     double electionEndTime = simTime().dbl();

//     if (!leaderElected)
//     {
//         leaderElected = true;

//         std::ofstream csvFile;
//         csvFile.open("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);

//         csvFile << (electionEndTime - electionStartTime) << ","
//                 << globalMessageCount;

//         csvFile.close();

//         endSimulation();
//     }

//     scheduleAt(simTime() + 0.5, heartbeatTimer);
// }

// // ================= FOLLOWER =================
// void RaftNode::becomeFollower(int term)
// {
//     if (term > currentTerm)
//     {
//         currentTerm = term;
//         votedFor = -1;
//     }

//     state = "FOLLOWER";

//     if (heartbeatTimer->isScheduled())
//         cancelEvent(heartbeatTimer);
// }

// // ================= FLOOD =================
// void RaftNode::floodMessage(cPacket *pkt, int exceptGate)
// {
//     int n = gateSize("outGate");

//     for (int i = 0; i < n; i++)
//     {
//         if (i == exceptGate) continue;

//         send(pkt->dup(), "outGate", i);  // ✅ FIXED

//         globalMessageCount++;
//     }

//     delete pkt;
// }

// // ================= UNICAST =================
// void RaftNode::sendToNode(cPacket *pkt, int targetId)
// {
//     int n = gateSize("outGate");

//     for (int i = 0; i < n; i++)
//     {
//         cGate *g = gate("outGate", i);
//         if (!g || !g->isConnected()) continue;

//         cModule *peer = g->getNextGate()->getOwnerModule();
//         if (peer && peer->getIndex() == targetId)
//         {
//             send(pkt, "outGate", i);  // ✅ FIXED
//             globalMessageCount++;
//             return;
//         }
//     }

//     floodMessage(pkt, -1);
// }

// // ================= TIMEOUT =================
// void RaftNode::resetElectionTimeout()
// {
//     cancelEvent(electionTimeout);

//     scheduleAt(simTime() + uniform(0.15, 0.5), electionTimeout);
// }

// // ================= CLEANUP =================
// void RaftNode::finish()
// {
//     cancelAndDelete(electionTimeout);
//     cancelAndDelete(heartbeatTimer);
// }


//------------------ failure rate effect (raw average msg_count) ------------------
// #include "RaftNode.h"

// Define_Module(RaftNode);

// std::vector<bool> RaftNode::aliveNodes;

// int RaftNode::currentLeaderId = -1;
// bool RaftNode::globalElectionInProgress = false;

// double RaftNode::electionStartTime = 0.0;
// double RaftNode::totalElectionTime = 0.0;

// long RaftNode::currentElectionMessages = 0;
// long RaftNode::totalElectionMessages = 0;
// int RaftNode::completedElectionCount = 0;

// RaftNode::~RaftNode()
// {
//     for (int i = 0; i < (int)sendQueues.size(); i++) {
//         while (!sendQueues[i].empty()) {
//             delete sendQueues[i].front();
//             sendQueues[i].pop();
//         }
//     }

//     for (auto event : sendEvents) {
//         if (event) {
//             cancelAndDelete(event);
//         }
//     }

//     if (electionTimeoutEvent) cancelAndDelete(electionTimeoutEvent);
//     if (heartbeatEvent) cancelAndDelete(heartbeatEvent);
//     if (crashEvent) cancelAndDelete(crashEvent);
//     if (recoveryEvent) cancelAndDelete(recoveryEvent);
// }

// void RaftNode::initialize()
// {
//     nodeId = par("nodeId");
//     numNodes = getParentModule()->par("numNodes");

//     if ((int)aliveNodes.size() != numNodes) {
//         aliveNodes.assign(numNodes, true);

//         currentLeaderId = numNodes - 1;
//         globalElectionInProgress = false;

//         electionStartTime = 0.0;
//         totalElectionTime = 0.0;

//         currentElectionMessages = 0;
//         totalElectionMessages = 0;
//         completedElectionCount = 0;
//     }

//     alive = true;
//     aliveNodes[nodeId] = true;

//     currentTerm = 0;
//     votedFor = -1;
//     voteCount = 0;
//     votesReceived.clear();

//     state = (nodeId == currentLeaderId) ? LEADER : FOLLOWER;

//     electionTimeoutEvent = new cMessage("ELECTION_TIMEOUT");
//     heartbeatEvent = new cMessage("HEARTBEAT_TIMER");
//     crashEvent = new cMessage("CRASH");
//     recoveryEvent = new cMessage("RECOVERY");

//     int n = gateSize("outGate");
//     sendQueues.resize(n);
//     sendEvents.resize(n, nullptr);

//     for (int i = 0; i < n; i++) {
//         sendEvents[i] = new cMessage(("SEND_EVENT_" + std::to_string(i)).c_str());
//         sendEvents[i]->setKind(i);
//     }

//     if (state == LEADER) {
//         sendHeartbeat();
//         scheduleAt(simTime() + par("heartbeatInterval"), heartbeatEvent);
//         scheduleLeaderCrash();
//     } else {
//         resetElectionTimeout();
//     }
// }

// void RaftNode::handleMessage(cMessage *msg)
// {
//     if (msg->isSelfMessage()) {
//         if (strncmp(msg->getName(), "SEND_EVENT_", 11) == 0) {
//             processQueue(msg->getKind());
//             return;
//         }

//         if (msg == electionTimeoutEvent) {
//             if (alive && state != LEADER) {
//                 startElection();
//             }
//             return;
//         }

//         if (msg == heartbeatEvent) {
//             if (alive && state == LEADER) {
//                 sendHeartbeat();
//                 scheduleAt(simTime() + par("heartbeatInterval"), heartbeatEvent);
//             }
//             return;
//         }

//         if (msg == crashEvent) {
//             crashNode();
//             return;
//         }

//         if (msg == recoveryEvent) {
//             recoverNode();
//             return;
//         }

//         delete msg;
//         return;
//     }

//     if (!alive) {
//         delete msg;
//         return;
//     }

//     cPacket *pkt = check_and_cast<cPacket *>(msg);
//     std::string type = pkt->getName();

//     if (type == "RequestVote") {
//         handleRequestVote(pkt);
//         return;
//     }

//     if (type == "Vote") {
//         handleVote(pkt);
//         return;
//     }

//     if (type == "Heartbeat") {
//         handleHeartbeat(pkt);
//         return;
//     }

//     delete pkt;
// }

// void RaftNode::scheduleLeaderCrash()
// {
//     if (!alive || state != LEADER) {
//         return;
//     }

//     if (crashEvent->isScheduled()) {
//         return;
//     }

//     double crashTime = par("crashTime").doubleValue();

//     if (crashTime >= 0) {
//         scheduleAt(simTime() + crashTime, crashEvent);
//     }
// }

// void RaftNode::crashNode()
// {
//     if (!alive) {
//         return;
//     }

//     bool leaderFailed = (nodeId == currentLeaderId && state == LEADER);

//     EV << "Raft node " << nodeId << " crashed at " << simTime();

//     if (leaderFailed) {
//         EV << " leader failed";
//     }

//     EV << "\n";

//     alive = false;
//     aliveNodes[nodeId] = false;

//     state = FOLLOWER;
//     votedFor = -1;
//     voteCount = 0;
//     votesReceived.clear();

//     cancelElectionTimeout();

//     if (heartbeatEvent->isScheduled()) {
//         cancelEvent(heartbeatEvent);
//     }

//     for (int i = 0; i < gateSize("outGate"); i++) {
//         while (!sendQueues[i].empty()) {
//             delete sendQueues[i].front();
//             sendQueues[i].pop();
//         }

//         if (sendEvents[i]->isScheduled()) {
//             cancelEvent(sendEvents[i]);
//         }
//     }

//     double recoveryTime = par("recoveryTime").doubleValue();

//     if (recoveryTime >= 0 && !recoveryEvent->isScheduled()) {
//         scheduleAt(simTime() + recoveryTime, recoveryEvent);
//     }

//     if (leaderFailed) {
//         currentLeaderId = -1;
//         globalElectionInProgress = true;
//         electionStartTime = simTime().dbl();
//         currentElectionMessages = 0;
//     }
// }

// void RaftNode::recoverNode()
// {
//     if (alive) {
//         return;
//     }

//     alive = true;
//     aliveNodes[nodeId] = true;

//     state = FOLLOWER;
//     votedFor = -1;
//     voteCount = 0;
//     votesReceived.clear();

//     EV << "Raft node " << nodeId << " recovered at " << simTime()
//        << " and learned current leader = " << currentLeaderId << "\n";

//     resetElectionTimeout();
// }

// void RaftNode::resetElectionTimeout()
// {
//     if (!alive || state == LEADER) {
//         return;
//     }

//     if (electionTimeoutEvent->isScheduled()) {
//         cancelEvent(electionTimeoutEvent);
//     }

//     double minTimeout = par("electionTimeoutMin").doubleValue();
//     double maxTimeout = par("electionTimeoutMax").doubleValue();

//     scheduleAt(simTime() + uniform(minTimeout, maxTimeout), electionTimeoutEvent);
// }

// void RaftNode::cancelElectionTimeout()
// {
//     if (electionTimeoutEvent && electionTimeoutEvent->isScheduled()) {
//         cancelEvent(electionTimeoutEvent);
//     }
// }

// void RaftNode::startElection()
// {
//     if (!alive) {
//         return;
//     }

//     state = CANDIDATE;
//     currentTerm++;
//     votedFor = nodeId;

//     voteCount = 1;
//     votesReceived.clear();
//     votesReceived.insert(nodeId);

//     if (!globalElectionInProgress) {
//         globalElectionInProgress = true;
//         electionStartTime = simTime().dbl();
//         currentElectionMessages = 0;
//     }

//     EV << "Raft node " << nodeId << " started election at "
//        << simTime() << ", term = " << currentTerm << "\n";

//     sendRequestVote();

//     if (voteCount >= majorityNeeded()) {
//         becomeLeader();
//         return;
//     }

//     resetElectionTimeout();
// }

// void RaftNode::becomeFollower(int term)
// {
//     if (term > currentTerm) {
//         currentTerm = term;
//         votedFor = -1;
//     }

//     if (state == LEADER && heartbeatEvent->isScheduled()) {
//         cancelEvent(heartbeatEvent);
//     }

//     if (crashEvent->isScheduled()) {
//         cancelEvent(crashEvent);
//     }

//     state = FOLLOWER;
//     voteCount = 0;
//     votesReceived.clear();
// }

// void RaftNode::becomeLeader()
// {
//     if (!alive || state == LEADER) {
//         return;
//     }

//     state = LEADER;
//     currentLeaderId = nodeId;

//     cancelElectionTimeout();

//     double thisElectionTime = simTime().dbl() - electionStartTime;

//     totalElectionTime += thisElectionTime;
//     totalElectionMessages += currentElectionMessages;
//     completedElectionCount++;

//     EV << "Raft election completed. New leader = " << nodeId
//        << ", election time = " << thisElectionTime
//        << ", messages = " << currentElectionMessages << "\n";

//     globalElectionInProgress = false;

//     sendHeartbeat();

//     if (!heartbeatEvent->isScheduled()) {
//         scheduleAt(simTime() + par("heartbeatInterval"), heartbeatEvent);
//     }

//     scheduleLeaderCrash();
// }

// void RaftNode::sendRequestVote()
// {
//     cPacket *rv = new cPacket("RequestVote");

//     rv->setByteLength(msgSizeBytes);
//     rv->addPar("term") = currentTerm;
//     rv->addPar("candidateId") = nodeId;

//     broadcastToPeers(rv, true);
// }

// void RaftNode::sendVote(int targetId, int term, bool granted)
// {
//     cPacket *vote = new cPacket("Vote");

//     vote->setByteLength(msgSizeBytes);
//     vote->addPar("term") = term;
//     vote->addPar("voterId") = nodeId;
//     vote->addPar("targetId") = targetId;
//     vote->addPar("granted") = granted;

//     sendToNode(vote, targetId, true);
// }

// void RaftNode::sendHeartbeat()
// {
//     cPacket *hb = new cPacket("Heartbeat");

//     hb->setByteLength(msgSizeBytes);
//     hb->addPar("term") = currentTerm;
//     hb->addPar("leaderId") = nodeId;

//     broadcastToPeers(hb, false);
// }

// void RaftNode::handleRequestVote(cPacket *pkt)
// {
//     int term = pkt->par("term");
//     int candidateId = pkt->par("candidateId");

//     if (term > currentTerm) {
//         becomeFollower(term);
//     }

//     bool granted = false;

//     if (term == currentTerm &&
//         candidateId != nodeId &&
//         isNodeAlive(candidateId) &&
//         (votedFor == -1 || votedFor == candidateId)) {
//         granted = true;
//         votedFor = candidateId;
//         resetElectionTimeout();
//     }

//     sendVote(candidateId, currentTerm, granted);

//     delete pkt;
// }

// void RaftNode::handleVote(cPacket *pkt)
// {
//     int term = pkt->par("term");
//     int voterId = pkt->par("voterId");
//     int targetId = pkt->par("targetId");
//     bool granted = pkt->par("granted").boolValue();

//     if (term > currentTerm) {
//         becomeFollower(term);
//         resetElectionTimeout();
//         delete pkt;
//         return;
//     }

//     if (state == CANDIDATE &&
//         targetId == nodeId &&
//         term == currentTerm &&
//         granted &&
//         votesReceived.count(voterId) == 0) {
//         votesReceived.insert(voterId);
//         voteCount++;

//         if (voteCount >= majorityNeeded()) {
//             delete pkt;
//             becomeLeader();
//             return;
//         }
//     }

//     delete pkt;
// }

// void RaftNode::handleHeartbeat(cPacket *pkt)
// {
//     int term = pkt->par("term");
//     int leaderId = pkt->par("leaderId");

//     if (term >= currentTerm && isNodeAlive(leaderId)) {
//         currentTerm = term;
//         currentLeaderId = leaderId;

//         if (state != FOLLOWER) {
//             becomeFollower(term);
//         }

//         resetElectionTimeout();
//     }

//     delete pkt;
// }

// void RaftNode::sendToNode(cPacket *pkt, int targetId, bool electionMessage)
// {
//     if (!alive || !isNodeAlive(targetId)) {
//         delete pkt;
//         return;
//     }

//     for (int i = 0; i < gateSize("outGate"); i++) {
//         cGate *g = gate("outGate", i);

//         if (!g || !g->isConnected()) {
//             continue;
//         }

//         cGate *endGate = g->getPathEndGate();

//         if (!endGate) {
//             continue;
//         }

//         cModule *peer = endGate->getOwnerModule();

//         if (peer && peer->par("nodeId").intValue() == targetId) {
//             enqueuePacket(pkt, i, electionMessage);
//             return;
//         }
//     }

//     delete pkt;
// }

// void RaftNode::broadcastToPeers(cPacket *pkt, bool electionMessage)
// {
//     if (!alive) {
//         delete pkt;
//         return;
//     }

//     for (int i = 0; i < gateSize("outGate"); i++) {
//         cGate *g = gate("outGate", i);

//         if (!g || !g->isConnected()) {
//             continue;
//         }

//         cGate *endGate = g->getPathEndGate();

//         if (!endGate) {
//             continue;
//         }

//         int targetId = endGate->getOwnerModule()->par("nodeId");

//         if (!isNodeAlive(targetId)) {
//             continue;
//         }

//         enqueuePacket(pkt->dup(), i, electionMessage);
//     }

//     delete pkt;
// }

// void RaftNode::enqueuePacket(cPacket *pkt, int gateIndex, bool electionMessage)
// {
//     if (!alive) {
//         delete pkt;
//         return;
//     }

//     cGate *g = gate("outGate", gateIndex);
//     cChannel *ch = g->getTransmissionChannel();

//     double lossRate = 0.0;

//     if (ch && ch->hasPar("lossRate")) {
//         lossRate = ch->par("lossRate").doubleValue();
//     }

//     if (electionMessage && globalElectionInProgress) {
//         currentElectionMessages++;
//     }

//     if (uniform(0, 1) < lossRate) {
//         delete pkt;
//         return;
//     }

//     pkt->setByteLength(msgSizeBytes);
//     sendQueues[gateIndex].push(pkt);

//     if (!sendEvents[gateIndex]->isScheduled()) {
//         processQueue(gateIndex);
//     }
// }

// void RaftNode::processQueue(int gateIndex)
// {
//     if (!alive || sendQueues[gateIndex].empty()) {
//         return;
//     }

//     cGate *g = gate("outGate", gateIndex);
//     cChannel *ch = g->getTransmissionChannel();

//     simtime_t txFinish = ch ? ch->getTransmissionFinishTime() : simTime();

//     if (txFinish <= simTime()) {
//         cPacket *pkt = sendQueues[gateIndex].front();
//         sendQueues[gateIndex].pop();

//         send(pkt, "outGate", gateIndex);

//         if (!sendQueues[gateIndex].empty()) {
//             simtime_t nextTime = ch ? ch->getTransmissionFinishTime() : simTime();
//             scheduleAt(nextTime, sendEvents[gateIndex]);
//         }
//     } else {
//         scheduleAt(txFinish, sendEvents[gateIndex]);
//     }
// }

// int RaftNode::aliveNodeCount() const
// {
//     int count = 0;

//     for (bool a : aliveNodes) {
//         if (a) {
//             count++;
//         }
//     }

//     return count;
// }

// int RaftNode::majorityNeeded() const
// {
//     int aliveCount = aliveNodeCount();

//     if (aliveCount <= 0) {
//         return 1;
//     }

//     return (aliveCount / 2) + 1;
// }

// bool RaftNode::isNodeAlive(int id) const
// {
//     return id >= 0 && id < (int)aliveNodes.size() && aliveNodes[id];
// }

// void RaftNode::finish()
// {
//     if (nodeId != 0) {
//         return;
//     }

//     std::ofstream csvFile("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);

//     if (!csvFile.is_open()) {
//         EV << "Could not open analysis.csv\n";
//         return;
//     }

//     double avgElectionTime = 0.0;
//     double avgMessageCount = 0.0;

//     if (completedElectionCount > 0) {
//         avgElectionTime = totalElectionTime / completedElectionCount;
//         avgMessageCount = (double)totalElectionMessages / completedElectionCount;
//     }

//     csvFile << avgElectionTime << "," << avgMessageCount << " ";
//     csvFile.close();

//     EV << "Raft completed elections = " << completedElectionCount << "\n";
//     EV << "Raft avg election time = " << avgElectionTime << "\n";
//     EV << "Raft avg message count = " << avgMessageCount << "\n";
// }

//////////////////////////////// claude /////////////////////////////
#include "RaftNode.h"
#include <fstream>

Define_Module(RaftNode);

// ─────────────────────────────────────────────────────────────────────────────
//  Static member definitions
// ─────────────────────────────────────────────────────────────────────────────
std::vector<bool> RaftNode::aliveNodes;

int    RaftNode::currentLeaderId        = -1;

bool   RaftNode::epochInProgress        = false;
double RaftNode::epochStartTime         = 0.0;
long   RaftNode::epochMessageCount      = 0;

double RaftNode::totalElectionTime      = 0.0;
long   RaftNode::totalElectionMessages  = 0;
int    RaftNode::completedElectionCount = 0;

bool   RaftNode::globalInitDone         = false;

// ─────────────────────────────────────────────────────────────────────────────
//  resetGlobalState  – called exactly once per simulation run (by node 0)
// ─────────────────────────────────────────────────────────────────────────────
/*static*/ void RaftNode::resetGlobalState(int n)
{
    aliveNodes.assign(n, true);

    // Pick node 0 as the bootstrap leader: it has degree 4 and can heartbeat
    // 4 direct neighbours immediately, giving most nodes quick suppression of
    // their election timers.  Node n-1 (degree 1) caused constant timeout storms.
    currentLeaderId        = 0;

    epochInProgress        = false;
    epochStartTime         = 0.0;
    epochMessageCount      = 0;

    totalElectionTime      = 0.0;
    totalElectionMessages  = 0;
    completedElectionCount = 0;

    globalInitDone         = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Destructor
// ─────────────────────────────────────────────────────────────────────────────
RaftNode::~RaftNode()
{
    for (auto &q : sendQueues)
        while (!q.empty()) { delete q.front(); q.pop(); }

    for (auto *ev : sendEvents)
        if (ev) cancelAndDelete(ev);

    cancelAndDelete(electionTimeoutEvent);
    cancelAndDelete(heartbeatEvent);
    cancelAndDelete(crashEvent);
    cancelAndDelete(recoveryEvent);
}

// ─────────────────────────────────────────────────────────────────────────────
//  initialize
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::initialize()
{
    nodeId   = par("nodeId");
    numNodes = getParentModule()->par("numNodes");

    // Node 0 resets global state at the start of every run.
    // Checking nodeId==0 avoids the repeated-reset problem (nodes 1..9 would
    // otherwise each clobber the state after node 0 already set it up).
    if (nodeId == 0) {
        resetGlobalState(numNodes);
    }

    // ── per-node Raft state ────────────────────────────────────────────
    alive       = true;
    // aliveNodes[nodeId] will be set true in resetGlobalState for nodeId==0,
    // and must be set here for all others (they initialize after node 0).
    aliveNodes[nodeId] = true;

    state       = (nodeId == currentLeaderId) ? LEADER : FOLLOWER;
    currentTerm = 0;
    votedFor    = -1;
    voteCount   = 0;
    votesReceived.clear();
    seenFloods.clear();

    // ── timers ────────────────────────────────────────────────────────
    electionTimeoutEvent = new cMessage("ELECTION_TIMEOUT");
    heartbeatEvent       = new cMessage("HEARTBEAT_TIMER");
    crashEvent           = new cMessage("CRASH");
    recoveryEvent        = new cMessage("RECOVERY");

    // ── per-gate send queues ──────────────────────────────────────────
    int n = gateSize("outGate");
    sendQueues.assign(n, std::queue<cPacket *>());
    sendEvents.assign(n, nullptr);
    for (int i = 0; i < n; i++) {
        std::string name = "SEND_" + std::to_string(nodeId) + "_" + std::to_string(i);
        sendEvents[i] = new cMessage(name.c_str());
        sendEvents[i]->setKind(i);
    }

    // ── kick-off ──────────────────────────────────────────────────────
    if (state == LEADER) {
        broadcastHeartbeat();
        scheduleAt(simTime() + par("heartbeatInterval"), heartbeatEvent);
        scheduleLeaderCrash();
    } else {
        resetElectionTimeout();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  handleMessage
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::handleMessage(cMessage *msg)
{
    // ── self messages ──────────────────────────────────────────────────
    if (msg->isSelfMessage()) {

        if (msg->getKind() >= 0 &&
            strncmp(msg->getName(), "SEND_", 5) == 0) {
            processQueue(msg->getKind());
            return;
        }

        if (msg == electionTimeoutEvent) {
            if (alive && state != LEADER)
                startElection();
            return;
        }

        if (msg == heartbeatEvent) {
            if (alive && state == LEADER) {
                broadcastHeartbeat();
                scheduleAt(simTime() + par("heartbeatInterval"), heartbeatEvent);
            }
            return;
        }

        if (msg == crashEvent)    { crashNode();   return; }
        if (msg == recoveryEvent) { recoverNode(); return; }

        delete msg;
        return;
    }

    // ── network messages ───────────────────────────────────────────────
    if (!alive) { delete msg; return; }

    cPacket *pkt      = check_and_cast<cPacket *>(msg);
    std::string type  = pkt->getName();
    int arrivalGate   = pkt->getArrivalGate() ? pkt->getArrivalGate()->getIndex() : -1;

    if      (type == "RequestVote") { handleRequestVote(pkt, arrivalGate); }
    else if (type == "Vote")        { handleVote(pkt);                     }
    else if (type == "Heartbeat")   { handleHeartbeat(pkt, arrivalGate);   }
    else                            { delete pkt;                          }
}

// ─────────────────────────────────────────────────────────────────────────────
//  scheduleLeaderCrash
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::scheduleLeaderCrash()
{
    if (!alive || state != LEADER || crashEvent->isScheduled()) return;

    double ct = par("crashTime").doubleValue();
    if (ct >= 0)
        scheduleAt(simTime() + ct, crashEvent);
}

// ─────────────────────────────────────────────────────────────────────────────
//  crashNode
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::crashNode()
{
    if (!alive) return;

    bool wasLeader = (state == LEADER && nodeId == currentLeaderId);

    EV << "[t=" << simTime() << "] Node " << nodeId << " CRASH"
       << (wasLeader ? " [leader]" : "") << "\n";

    alive              = false;
    aliveNodes[nodeId] = false;

    // Reset Raft state; will be wiped cleanly on recovery too
    state     = FOLLOWER;
    votedFor  = -1;
    voteCount = 0;
    votesReceived.clear();
    seenFloods.clear();

    cancelElectionTimeout();
    if (heartbeatEvent->isScheduled()) cancelEvent(heartbeatEvent);
    if (crashEvent->isScheduled())     cancelEvent(crashEvent);

    // Drain send queues
    for (int i = 0; i < (int)sendQueues.size(); i++) {
        while (!sendQueues[i].empty()) {
            delete sendQueues[i].front();
            sendQueues[i].pop();
        }
        if (sendEvents[i]->isScheduled())
            cancelEvent(sendEvents[i]);
    }

    // Schedule recovery
    double rt = par("recoveryTime").doubleValue();
    if (rt >= 0 && !recoveryEvent->isScheduled())
        scheduleAt(simTime() + rt, recoveryEvent);

    // Open election epoch.
    // KEY FIX: we ALWAYS reset the epoch when the leader crashes, even if
    // epochInProgress was already true from zombie elections.  The real election
    // starts NOW (at leader-crash time), so both the start time and the message
    // counter must be zeroed here.
    if (wasLeader) {
        currentLeaderId   = -1;
        epochInProgress   = true;
        epochStartTime    = simTime().dbl();
        epochMessageCount = 0;   // discard any zombie-election messages counted before
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  recoverNode
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::recoverNode()
{
    if (alive) return;

    alive              = true;
    aliveNodes[nodeId] = true;

    state     = FOLLOWER;
    votedFor  = -1;
    voteCount = 0;
    votesReceived.clear();
    seenFloods.clear();
    // Preserve currentTerm — the node re-joins at whatever term it knew.
    // It will update via the next heartbeat or RequestVote.

    EV << "[t=" << simTime() << "] Node " << nodeId << " RECOVERED"
       << " leader=" << currentLeaderId << "\n";

    resetElectionTimeout();
}

// ─────────────────────────────────────────────────────────────────────────────
//  resetElectionTimeout
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::resetElectionTimeout()
{
    if (!alive || state == LEADER) return;

    if (electionTimeoutEvent->isScheduled())
        cancelEvent(electionTimeoutEvent);

    double lo = par("electionTimeoutMin").doubleValue();
    double hi = par("electionTimeoutMax").doubleValue();
    scheduleAt(simTime() + uniform(lo, hi), electionTimeoutEvent);
}

void RaftNode::cancelElectionTimeout()
{
    if (electionTimeoutEvent && electionTimeoutEvent->isScheduled())
        cancelEvent(electionTimeoutEvent);
}

// ─────────────────────────────────────────────────────────────────────────────
//  startElection
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::startElection()
{
    if (!alive) return;

    state = CANDIDATE;
    currentTerm++;
    votedFor = nodeId;
    voteCount = 1;          // self-vote
    votesReceived.clear();
    votesReceived.insert(nodeId);
    seenFloods.clear();     // fresh flood dedup for new term

    // NOTE: we do NOT open or reset epochInProgress here.
    // The epoch is owned by the leader-crash event.  We just participate.
    // If no epoch is open yet (e.g. very first election before any crash),
    // we open one now so the very first election is also measured.
    if (!epochInProgress) {
        epochInProgress   = true;
        epochStartTime    = simTime().dbl();
        epochMessageCount = 0;
    }

    EV << "[t=" << simTime() << "] Node " << nodeId
       << " starts election term=" << currentTerm
       << " (need " << majorityNeeded() << "/" << aliveNodeCount() << ")\n";

    broadcastRequestVote();

    // Edge case: single node
    if (voteCount >= majorityNeeded()) {
        becomeLeader();
        return;
    }

    resetElectionTimeout();
}

// ─────────────────────────────────────────────────────────────────────────────
//  becomeFollower
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::becomeFollower(int term)
{
    if (term > currentTerm) {
        currentTerm = term;
        votedFor    = -1;
    }

    if (state == LEADER) {
        if (heartbeatEvent->isScheduled()) cancelEvent(heartbeatEvent);
        if (crashEvent->isScheduled())     cancelEvent(crashEvent);
    }

    state     = FOLLOWER;
    voteCount = 0;
    votesReceived.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
//  becomeLeader
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::becomeLeader()
{
    if (!alive || state == LEADER) return;

    state           = LEADER;
    currentLeaderId = nodeId;
    cancelElectionTimeout();

    // Close the epoch
    if (epochInProgress) {
        double elapsed = simTime().dbl() - epochStartTime;
        totalElectionTime     += elapsed;
        totalElectionMessages += epochMessageCount;
        completedElectionCount++;
        epochInProgress = false;

        EV << "[t=" << simTime() << "] NEW LEADER=" << nodeId
           << " term=" << currentTerm
           << " electionTime=" << elapsed
           << " msgs=" << epochMessageCount
           << " (#" << completedElectionCount << ")\n";
    }

    broadcastHeartbeat();
    if (!heartbeatEvent->isScheduled())
        scheduleAt(simTime() + par("heartbeatInterval"), heartbeatEvent);

    scheduleLeaderCrash();
}

// ─────────────────────────────────────────────────────────────────────────────
//  broadcastRequestVote
//
//  Floods a RequestVote to ALL directly connected alive neighbors.
//  The flood is then re-propagated by each receiver (handleRequestVote),
//  so the message reaches the entire cluster despite the sparse topology.
//  Deduplication via seenFloods prevents infinite loops.
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::broadcastRequestVote()
{
    // Mark as seen so we don't re-flood our own RequestVote if it echoes back
    auto key = std::make_tuple(currentTerm, nodeId, 0);
    seenFloods.insert(key);

    cPacket *rv = new cPacket("RequestVote");
    rv->setByteLength(MSG_SIZE_BYTES);
    rv->addPar("term")        = currentTerm;
    rv->addPar("candidateId") = nodeId;

    floodToNeighbors(rv, /*exceptGate=*/-1, /*countable=*/true);
}

// ─────────────────────────────────────────────────────────────────────────────
//  sendVote  – unicast directly back to candidateId
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::sendVote(int candidateId, int term, bool granted)
{
    cPacket *vote = new cPacket("Vote");
    vote->setByteLength(MSG_SIZE_BYTES);
    vote->addPar("term")        = term;
    vote->addPar("voterId")     = nodeId;
    vote->addPar("candidateId") = candidateId;
    vote->addPar("granted")     = granted;

    // Try direct link first; sendToNode drops silently if no direct link.
    // Votes are small and directed; we do NOT flood them to avoid vote storms.
    sendToNode(vote, candidateId, /*countable=*/true);
}

// ─────────────────────────────────────────────────────────────────────────────
//  broadcastHeartbeat  – flood heartbeats so all nodes hear from the leader
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::broadcastHeartbeat()
{
    auto key = std::make_tuple(currentTerm, nodeId, 1);
    seenFloods.insert(key);

    cPacket *hb = new cPacket("Heartbeat");
    hb->setByteLength(MSG_SIZE_BYTES);
    hb->addPar("term")     = currentTerm;
    hb->addPar("leaderId") = nodeId;

    floodToNeighbors(hb, /*exceptGate=*/-1, /*countable=*/false);
}

// ─────────────────────────────────────────────────────────────────────────────
//  handleRequestVote
//
//  Receives a flooded RequestVote.  Grants or denies vote, then re-floods
//  the RequestVote onward to all other neighbors (except the gate it came from)
//  so that the entire cluster hears it.
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::handleRequestVote(cPacket *pkt, int arrivalGate)
{
    int term        = pkt->par("term");
    int candidateId = pkt->par("candidateId");

    // Dedup: if we've already seen this (term, candidateId) RequestVote, just flood onward
    auto key = std::make_tuple(term, candidateId, 0);
    bool alreadySeen = (seenFloods.count(key) > 0);

    if (term > currentTerm)
        becomeFollower(term);

    if (!alreadySeen) {
        seenFloods.insert(key);

        bool granted = false;
        if (term == currentTerm &&
            candidateId != nodeId &&
            (votedFor == -1 || votedFor == candidateId))
        {
            granted  = true;
            votedFor = candidateId;
            resetElectionTimeout();
        }

        sendVote(candidateId, currentTerm, granted);
    }

    // Always re-flood onward (regardless of dup) so all nodes in the
    // cluster receive it — but only actually flood if first time seen.
    // If dup: we already flooded it on first receipt, don't flood again.
    if (!alreadySeen) {
        floodToNeighbors(pkt->dup(), arrivalGate, /*countable=*/false);
        // Note: RequestVote re-floods are NOT counted as election messages.
        // Only the original send from the candidate (in broadcastRequestVote)
        // and the Vote replies are counted.
    }

    delete pkt;
}

// ─────────────────────────────────────────────────────────────────────────────
//  handleVote
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::handleVote(cPacket *pkt)
{
    int  term        = pkt->par("term");
    int  voterId     = pkt->par("voterId");
    int  candidateId = pkt->par("candidateId");
    bool granted     = pkt->par("granted").boolValue();

    if (term > currentTerm) {
        becomeFollower(term);
        resetElectionTimeout();
        delete pkt;
        return;
    }

    if (state      == CANDIDATE &&
        candidateId == nodeId   &&
        term        == currentTerm &&
        granted                 &&
        votesReceived.count(voterId) == 0)
    {
        votesReceived.insert(voterId);
        voteCount++;

        EV << "[t=" << simTime() << "] Node " << nodeId
           << " got vote from " << voterId
           << " (" << voteCount << "/" << majorityNeeded() << ")\n";

        if (voteCount >= majorityNeeded()) {
            delete pkt;
            becomeLeader();
            return;
        }
    }

    delete pkt;
}

// ─────────────────────────────────────────────────────────────────────────────
//  handleHeartbeat
//
//  Accepts heartbeat, resets election timer, re-floods to all other neighbors.
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::handleHeartbeat(cPacket *pkt, int arrivalGate)
{
    int term     = pkt->par("term");
    int leaderId = pkt->par("leaderId");

    auto key = std::make_tuple(term, leaderId, 1);
    bool alreadySeen = (seenFloods.count(key) > 0);

    if (!alreadySeen) {
        seenFloods.insert(key);

        if (term >= currentTerm && isNodeAlive(leaderId)) {
            if (term > currentTerm || state != FOLLOWER)
                becomeFollower(term);
            currentLeaderId = leaderId;
            resetElectionTimeout();
        }

        floodToNeighbors(pkt->dup(), arrivalGate, /*countable=*/false);
    }

    delete pkt;
}

// ─────────────────────────────────────────────────────────────────────────────
//  floodToNeighbors
//
//  Sends a dup of pkt to every connected alive neighbor except exceptGate.
//  Ownership of pkt is consumed (deleted here).
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::floodToNeighbors(cPacket *pkt, int exceptGate, bool countable)
{
    if (!alive) { delete pkt; return; }

    int n = gateSize("outGate");
    for (int i = 0; i < n; i++) {
        if (i == exceptGate) continue;

        cGate *g = gate("outGate", i);
        if (!g || !g->isConnected()) continue;

        cGate *endGate = g->getPathEndGate();
        if (!endGate) continue;

        cModule *peer = endGate->getOwnerModule();
        if (!peer) continue;

        int peerId = peer->par("nodeId").intValue();
        if (!isNodeAlive(peerId)) continue;

        enqueuePacket(pkt->dup(), i, countable);
    }

    delete pkt;
}

// ─────────────────────────────────────────────────────────────────────────────
//  sendToNode  – unicast via direct gate to targetId; drop if no direct link
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::sendToNode(cPacket *pkt, int targetId, bool countable)
{
    if (!alive || !isNodeAlive(targetId)) { delete pkt; return; }

    for (int i = 0; i < gateSize("outGate"); i++) {
        cGate *g = gate("outGate", i);
        if (!g || !g->isConnected()) continue;

        cGate *endGate = g->getPathEndGate();
        if (!endGate) continue;

        cModule *peer = endGate->getOwnerModule();
        if (peer && peer->par("nodeId").intValue() == targetId) {
            enqueuePacket(pkt, i, countable);
            return;
        }
    }

    // No direct link — drop (Vote can't be delivered; candidate will re-election)
    delete pkt;
}

// ─────────────────────────────────────────────────────────────────────────────
//  enqueuePacket
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::enqueuePacket(cPacket *pkt, int gateIndex, bool countable)
{
    if (!alive) { delete pkt; return; }

    cGate    *g  = gate("outGate", gateIndex);
    cChannel *ch = g->getTransmissionChannel();

    // Simulate packet loss
    double lossRate = 0.0;
    if (ch && ch->hasPar("lossRate"))
        lossRate = ch->par("lossRate").doubleValue();

    if (uniform(0, 1) < lossRate) {
        delete pkt;
        return;   // lost — not counted
    }

    // Count only successfully enqueued election messages (RequestVote from
    // candidate + Vote replies).  Re-floods of RequestVote and all heartbeats
    // are countable=false.
    if (countable && epochInProgress)
        epochMessageCount++;

    pkt->setByteLength(MSG_SIZE_BYTES);
    sendQueues[gateIndex].push(pkt);

    if (!sendEvents[gateIndex]->isScheduled())
        processQueue(gateIndex);
}

// ─────────────────────────────────────────────────────────────────────────────
//  processQueue  – drain one gate's queue respecting channel tx finish time
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::processQueue(int gateIndex)
{
    if (!alive || sendQueues[gateIndex].empty()) return;

    cGate    *g  = gate("outGate", gateIndex);
    cChannel *ch = g->getTransmissionChannel();

    simtime_t txFinish = ch ? ch->getTransmissionFinishTime() : simTime();

    if (txFinish <= simTime()) {
        cPacket *pkt = sendQueues[gateIndex].front();
        sendQueues[gateIndex].pop();
        send(pkt, "outGate", gateIndex);

        if (!sendQueues[gateIndex].empty()) {
            simtime_t next = ch ? ch->getTransmissionFinishTime() : simTime();
            scheduleAt(next, sendEvents[gateIndex]);
        }
    } else {
        scheduleAt(txFinish, sendEvents[gateIndex]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  clearFloodCache  – called when term advances to prevent stale dedup entries
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::clearFloodCache()
{
    seenFloods.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────
int RaftNode::aliveNodeCount() const
{
    int c = 0;
    for (bool a : aliveNodes) if (a) c++;
    return c;
}

int RaftNode::majorityNeeded() const
{
    int a = aliveNodeCount();
    return (a <= 0) ? 1 : (a / 2) + 1;
}

bool RaftNode::isNodeAlive(int id) const
{
    return id >= 0 && id < (int)aliveNodes.size() && aliveNodes[id];
}

// ─────────────────────────────────────────────────────────────────────────────
//  finish  – write one CSV row, only from node 0
// ─────────────────────────────────────────────────────────────────────────────
void RaftNode::finish()
{
    if (nodeId != 0) return;

    double avgTime = 0.0;
    double avgMsgs = 0.0;

    if (completedElectionCount > 0) {
        avgTime = totalElectionTime      / completedElectionCount;
        avgMsgs = (double)totalElectionMessages / completedElectionCount;
    }

    EV << "=== Raft run summary ===\n"
       << "  Completed elections : " << completedElectionCount << "\n"
       << "  Avg election time   : " << avgTime << " s\n"
       << "  Avg message count   : " << avgMsgs << "\n";

    std::ofstream csv(
        "/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv",
        std::ios::app);

    if (csv.is_open()) {
        csv << avgTime << "," << avgMsgs << " ";
        csv.close();
    } else {
        EV << "WARNING: could not open analysis.csv\n";
    }

    // Mark global state as stale so the next run's node-0 initialize() resets it
    globalInitDone = false;
}