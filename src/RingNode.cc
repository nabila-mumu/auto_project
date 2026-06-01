//-------------------- previous without failure and recovery ----------------
// #include "RingNode.h"
// #include <fstream>
// #include <climits>
// #include <queue>

// using namespace omnetpp;

// Define_Module(RingNode);

// /* ---------- STATIC MEMBERS ---------- */
// int RingNode::globalElectionId = 0;
// int RingNode::totalMessages = 0;
// int RingNode::currentLeaderId = -1;

// /* ---------- INITIALIZATION ---------- */
// void RingNode::initialize()
// {
//     nodeId = par("nodeId");
//     successor = -1;

//     electionInProgress = false;
//     isLeader = false;
//     electionId = -1;

//     messagesSent = 0;
//     messagesReceived = 0;

//     msgSizeBytes = 10000; // ✅ ADD

//     int n = gateSize("outGate");
//     sendQueues.resize(n);
//     sendEvents.resize(n);

//     for (int i = 0; i < n; i++) {
//         sendEvents[i] = new cMessage(("sendEvent-" + std::to_string(i)).c_str());
//         sendEvents[i]->addPar("gateIndex") = i;
//     }

//     if (nodeId == 0) {
//         totalMessages = 0;
//         currentLeaderId = -1;
//     }

//     coordinatorReceived = false;

//     buildGateMap();

//     scheduleAt(simTime() + uniform(0.05, 0.15), new cMessage("START_ELECTION"));
// }

// /* ---------- BUILD SUCCESSOR MAP ---------- */
// void RingNode::buildGateMap()
// {
//     neighborToGateIndex.clear();
//     neighbors.clear();

//     for (int i = 0; i < gateSize("outGate"); i++) {
//         cGate *g = gate("outGate", i);
//         if (g->isConnected()) {
//             int destId = g->getPathEndGate()->getOwnerModule()->par("nodeId");
//             neighborToGateIndex[destId] = i;
//             neighbors.insert(destId);
//         }
//     }

//     int minGreater = INT_MAX;
//     int minOverall = INT_MAX;

//     for (int n : neighbors) {
//         if (n > nodeId && n < minGreater)
//             minGreater = n;
//         if (n < minOverall)
//             minOverall = n;
//     }

//     successor = (minGreater != INT_MAX) ? minGreater : minOverall;
// }

// /* ---------- QUEUE SYSTEM ---------- */
// void RingNode::enqueuePacket(cPacket *pkt, int gateIndex)
// {
//     pkt->setByteLength(msgSizeBytes);
//     sendQueues[gateIndex].push(pkt);

//     if (!sendEvents[gateIndex]->isScheduled()) {
//         processQueue(gateIndex);
//     }
// }

// void RingNode::processQueue(int gateIndex)
// {
//     if (sendQueues[gateIndex].empty()) return;

//     cGate *g = gate("outGate", gateIndex);
//     cChannel *ch = g->getTransmissionChannel();

//     simtime_t txFinish = ch->getTransmissionFinishTime();

//     if (txFinish <= simTime()) {
//         cPacket *pkt = sendQueues[gateIndex].front();
//         sendQueues[gateIndex].pop();

//         send(pkt, "outGate", gateIndex);

//         messagesSent++;
//         totalMessages++;

//         if (!sendQueues[gateIndex].empty()) {
//             simtime_t nextTime = g->getTransmissionChannel()->getTransmissionFinishTime();
//             scheduleAt(nextTime, sendEvents[gateIndex]);
//         }
//     } else {
//         scheduleAt(txFinish, sendEvents[gateIndex]);
//     }
// }

// /* ---------- SEND TO SUCCESSOR ---------- */
// void RingNode::sendToSuccessor(cPacket *pkt)
// {
//     auto it = neighborToGateIndex.find(successor);
//     if (it == neighborToGateIndex.end()) {
//         delete pkt;
//         return;
//     }

//     enqueuePacket(pkt, it->second);
// }

// /* ---------- START ELECTION ---------- */
// void RingNode::startElection()
// {
//     electionInProgress = true;
//     electionId = ++globalElectionId;
//     electionStartTime = simTime();

//     cPacket *pkt = new cPacket("ELECTION");
//     pkt->addPar("initiatorId") = nodeId;
//     pkt->addPar("electionId") = electionId;
//     pkt->addPar("maxId") = nodeId;
//     pkt->addPar("startTime").setDoubleValue(simTime().dbl());

//     sendToSuccessor(pkt);
// }

// /* ---------- HANDLE MESSAGE ---------- */
// void RingNode::handleMessage(cMessage *msg)
// {
//     // queue events
//     if (msg->isSelfMessage() && strstr(msg->getName(), "sendEvent")) {
//         int gateIndex = msg->par("gateIndex");
//         processQueue(gateIndex);
//         return;
//     }

//     // stop simulation
//     if (msg->isSelfMessage() && strcmp(msg->getName(), "STOP_SIM") == 0) {
//         endSimulation();
//         delete msg;
//         return;
//     }

//     if (msg->isSelfMessage()) {
//         if (strcmp(msg->getName(), "START_ELECTION") == 0) {
//             startElection();
//         }
//         delete msg;
//         return;
//     }

//     cPacket *pkt = check_and_cast<cPacket *>(msg);

//     messagesReceived++;
//     totalMessages++;

//     /* ---------- ELECTION ---------- */
//     if (strcmp(pkt->getName(), "ELECTION") == 0) {
//         int msgElectionId = pkt->par("electionId");
//         int initiatorId = pkt->par("initiatorId");
//         int maxId = pkt->par("maxId");

//         if (msgElectionId < electionId) {
//             delete pkt;
//             return;
//         }

//         if (msgElectionId > electionId) {
//             electionId = msgElectionId;
//             electionInProgress = true;
//         }

//         if (nodeId > maxId) {
//             pkt->par("maxId") = nodeId;
//         }

//         if (nodeId == initiatorId) {
//             int leader = pkt->par("maxId");

//             electionEndTime = simTime();
//             currentLeaderId = leader;
//             isLeader = (leader == nodeId);
//             electionInProgress = false;

//             simtime_t start = SimTime(pkt->par("startTime").doubleValue());
//             electionTime = simTime() - start;

//             cPacket *coord = new cPacket("COORDINATOR");
//             coord->addPar("leaderId") = leader;
//             coord->addPar("initiatorId") = nodeId;
//             coord->addPar("startTime") = pkt->par("startTime");

//             delete pkt;
//             sendToSuccessor(coord);
//             return;
//         }

//         sendToSuccessor(pkt);
//         return;
//     }

//     /* ---------- COORDINATOR ---------- */
//     if (strcmp(pkt->getName(), "COORDINATOR") == 0) {
//         int leaderId = pkt->par("leaderId");
//         int initiatorId = pkt->par("initiatorId");

//         coordinatorReceived = true;

//         currentLeaderId = leaderId;
//         electionInProgress = false;
//         isLeader = (leaderId == nodeId);

//         simtime_t start = SimTime(pkt->par("startTime").doubleValue());
//         electionTime = simTime() - start;

//         // ✅ ONLY STOP AFTER FULL CYCLE
//         if (nodeId == initiatorId) {
//             delete pkt;
//             scheduleAt(simTime() + 0.01, new cMessage("STOP_SIM"));
//             return;
//         }

//         sendToSuccessor(pkt);
//         return;
//     }
// }

// /* ---------- FINISH ---------- */
// void RingNode::finish()
// {
//     if (nodeId != 0) return;

//     std::ofstream csvFile("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);

//     if (!csvFile.is_open()) return;

//     if (currentLeaderId != -1) {
//         csvFile << electionTime << "," << totalMessages << ",";
//     } else {
//         csvFile << "," << ",";
//     }

//     csvFile.close();
// }


//------------------- raw average msg_count --------------------
// #include "RingNode.h"
// #include <fstream>
// #include <climits>
// #include <cstring>

// Define_Module(RingNode);

// std::vector<bool> RingNode::aliveNodes;

// int RingNode::currentLeaderId = -1;
// int RingNode::globalElectionId = 0;
// bool RingNode::globalElectionInProgress = false;

// long RingNode::totalMessages = 0;
// long RingNode::electionStartMessageCount = 0;

// double RingNode::electionStartTime = 0.0;
// double RingNode::totalElectionTime = 0.0;

// long RingNode::totalElectionMessages = 0;
// int RingNode::completedElectionCount = 0;

// RingNode::~RingNode()
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

//     if (crashEvent) cancelAndDelete(crashEvent);
//     if (recoveryEvent) cancelAndDelete(recoveryEvent);
//     if (detectLeaderDownEvent) cancelAndDelete(detectLeaderDownEvent);
//     if (timeoutEvent) cancelAndDelete(timeoutEvent);
// }

// void RingNode::initialize()
// {
//     nodeId = par("nodeId");
//     numNodes = getParentModule()->par("numNodes");
//     bidirectional = par("bidirectional").boolValue();

//     if ((int)aliveNodes.size() != numNodes) {
//         aliveNodes.assign(numNodes, true);

//         currentLeaderId = numNodes - 1;
//         globalElectionId = 0;
//         globalElectionInProgress = false;

//         totalMessages = 0;
//         electionStartMessageCount = 0;

//         electionStartTime = 0.0;
//         totalElectionTime = 0.0;

//         totalElectionMessages = 0;
//         completedElectionCount = 0;
//     }

//     alive = true;
//     aliveNodes[nodeId] = true;

//     isLeader = (nodeId == currentLeaderId);
//     electionInProgress = false;
//     electionId = -1;

//     buildGateMap();

//     int n = gateSize("outGate");
//     sendQueues.resize(n);
//     sendEvents.resize(n, nullptr);

//     for (int i = 0; i < n; i++) {
//         sendEvents[i] = new cMessage(("sendEvent-" + std::to_string(i)).c_str());
//         sendEvents[i]->setKind(i);
//     }

//     crashEvent = new cMessage("CRASH");
//     recoveryEvent = new cMessage("RECOVERY");
//     detectLeaderDownEvent = new cMessage("DETECT_LEADER_DOWN");
//     timeoutEvent = new cMessage("ELECTION_TIMEOUT");

//     scheduleNextCrash();
// }

// void RingNode::buildGateMap()
// {
//     neighborToGateIndex.clear();
//     neighbors.clear();

//     for (int i = 0; i < gateSize("outGate"); i++) {
//         cGate *g = gate("outGate", i);

//         if (g->isConnected()) {
//             int destId = g->getPathEndGate()->getOwnerModule()->par("nodeId");
//             neighborToGateIndex[destId] = i;
//             neighbors.insert(destId);
//         }
//     }
// }

// void RingNode::handleMessage(cMessage *msg)
// {
//     if (msg->isSelfMessage()) {
//         if (strncmp(msg->getName(), "sendEvent", 9) == 0) {
//             processQueue(msg->getKind());
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

//         if (msg == detectLeaderDownEvent) {
//             if (alive) {
//                 startElection();
//             }
//             return;
//         }

//         if (msg == timeoutEvent) {
//             if (alive && electionInProgress && globalElectionInProgress) {
//                 int leader = findHighestAliveNode();
//                 if (leader >= 0) {
//                     completeElection(leader);
//                 }
//             }
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

//     if (type == "ELECTION") {
//         int msgElectionId = pkt->par("electionId");
//         int initiatorId = pkt->par("initiatorId");
//         int direction = pkt->par("direction");
//         int maxId = pkt->par("maxId");

//         if (msgElectionId < globalElectionId) {
//             delete pkt;
//             return;
//         }

//         electionId = msgElectionId;
//         electionInProgress = true;

//         if (nodeId > maxId) {
//             pkt->par("maxId") = nodeId;
//         }

//         if (nodeId == initiatorId) {
//             int leader = pkt->par("maxId");
//             delete pkt;

//             if (globalElectionInProgress) {
//                 completeElection(leader);
//             }

//             return;
//         }

//         sendToNextAlive(pkt, direction);
//         return;
//     }

//     if (type == "COORDINATOR") {
//         int leaderId = pkt->par("leaderId");
//         int initiatorId = pkt->par("initiatorId");
//         int direction = pkt->par("direction");

//         currentLeaderId = leaderId;
//         isLeader = (nodeId == leaderId);
//         electionInProgress = false;

//         if (timeoutEvent->isScheduled()) {
//             cancelEvent(timeoutEvent);
//         }

//         if (nodeId == initiatorId) {
//             delete pkt;
//             return;
//         }

//         sendToNextAlive(pkt, direction);
//         return;
//     }

//     delete pkt;
// }

// void RingNode::scheduleNextCrash()
// {
//     if (!alive) {
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

// void RingNode::crashNode()
// {
//     if (!alive) {
//         return;
//     }

//     bool leaderFailed = (nodeId == currentLeaderId);

//     EV << "Ring node " << nodeId << " crashed at " << simTime();

//     if (leaderFailed) {
//         EV << " leader failed";
//     }

//     EV << "\n";

//     alive = false;
//     aliveNodes[nodeId] = false;
//     isLeader = false;
//     electionInProgress = false;

//     if (timeoutEvent->isScheduled()) {
//         cancelEvent(timeoutEvent);
//     }

//     if (detectLeaderDownEvent->isScheduled()) {
//         cancelEvent(detectLeaderDownEvent);
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
//         scheduleElectionAfterLeaderCrash();
//     }
// }

// void RingNode::recoverNode()
// {
//     if (alive) {
//         return;
//     }

//     alive = true;
//     aliveNodes[nodeId] = true;

//     isLeader = (nodeId == currentLeaderId);
//     electionInProgress = false;
//     electionId = globalElectionId;

//     EV << "Ring node " << nodeId << " recovered at " << simTime()
//        << " and learned current leader = " << currentLeaderId << "\n";

//     scheduleNextCrash();
// }

// void RingNode::scheduleElectionAfterLeaderCrash()
// {
//     int starter = findLowestAliveNode();

//     if (starter < 0) {
//         currentLeaderId = -1;
//         globalElectionInProgress = false;
//         return;
//     }

//     globalElectionId++;
//     globalElectionInProgress = true;

//     electionStartTime = simTime().dbl();
//     electionStartMessageCount = totalMessages;

//     cModule *starterModule = getParentModule()->getSubmodule("node", starter);
//     RingNode *starterNode = check_and_cast<RingNode *>(starterModule);

//     simtime_t stopDelay = starterNode->par("stopDelay");

//     if (!starterNode->detectLeaderDownEvent->isScheduled()) {
//         starterNode->scheduleAt(simTime() + stopDelay, starterNode->detectLeaderDownEvent);
//     }
// }

// void RingNode::startElection()
// {
//     if (!alive || electionInProgress) {
//         return;
//     }

//     if (!globalElectionInProgress) {
//         globalElectionId++;
//         globalElectionInProgress = true;

//         electionStartTime = simTime().dbl();
//         electionStartMessageCount = totalMessages;
//     }

//     electionId = globalElectionId;
//     electionInProgress = true;

//     sendElectionToken(+1);

//     if (bidirectional) {
//         sendElectionToken(-1);
//     }

//     if (!timeoutEvent->isScheduled()) {
//         simtime_t electionTimeout = par("electionTimeout");
//         scheduleAt(simTime() + electionTimeout, timeoutEvent);
//     }
// }

// void RingNode::sendElectionToken(int direction)
// {
//     cPacket *pkt = new cPacket("ELECTION");
//     pkt->addPar("initiatorId") = nodeId;
//     pkt->addPar("electionId") = globalElectionId;
//     pkt->addPar("maxId") = nodeId;
//     pkt->addPar("direction") = direction;

//     sendToNextAlive(pkt, direction);
// }

// void RingNode::completeElection(int leaderId)
// {
//     if (!globalElectionInProgress) {
//         return;
//     }

//     currentLeaderId = leaderId;
//     isLeader = (nodeId == leaderId);
//     electionInProgress = false;

//     if (timeoutEvent->isScheduled()) {
//         cancelEvent(timeoutEvent);
//     }

//     double thisElectionTime = simTime().dbl() - electionStartTime;
//     long thisElectionMessages = totalMessages - electionStartMessageCount;

//     totalElectionTime += thisElectionTime;
//     totalElectionMessages += thisElectionMessages;
//     completedElectionCount++;

//     EV << "Ring election completed. New leader = " << leaderId
//        << ", election time = " << thisElectionTime
//        << ", messages = " << thisElectionMessages << "\n";

//     globalElectionInProgress = false;

//     sendCoordinatorToken(+1, leaderId);

//     if (bidirectional) {
//         sendCoordinatorToken(-1, leaderId);
//     }
// }

// void RingNode::sendCoordinatorToken(int direction, int leaderId)
// {
//     cPacket *coord = new cPacket("COORDINATOR");
//     coord->addPar("leaderId") = leaderId;
//     coord->addPar("initiatorId") = nodeId;
//     coord->addPar("direction") = direction;

//     sendToNextAlive(coord, direction);
// }

// void RingNode::sendToNextAlive(cPacket *pkt, int direction)
// {
//     if (!alive) {
//         delete pkt;
//         return;
//     }

//     int nextNode = getNextAliveNode(direction);

//     if (nextNode < 0) {
//         delete pkt;
//         return;
//     }

//     auto it = neighborToGateIndex.find(nextNode);

//     if (it == neighborToGateIndex.end()) {
//         EV << "No gate from node " << nodeId << " to node " << nextNode << "\n";
//         delete pkt;
//         return;
//     }

//     enqueuePacket(pkt, it->second);
// }

// int RingNode::getNextAliveNode(int direction) const
// {
//     if (numNodes <= 1) {
//         return -1;
//     }

//     for (int step = 1; step < numNodes; step++) {
//         int candidate;

//         if (direction > 0) {
//             candidate = (nodeId + step) % numNodes;
//         } else {
//             candidate = (nodeId - step + numNodes) % numNodes;
//         }

//         if (candidate != nodeId && isNodeAlive(candidate)) {
//             return candidate;
//         }
//     }

//     return -1;
// }

// void RingNode::enqueuePacket(cPacket *pkt, int gateIndex)
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

//     totalMessages++;

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

// void RingNode::processQueue(int gateIndex)
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

// int RingNode::findLowestAliveNode() const
// {
//     for (int i = 0; i < numNodes; i++) {
//         if (isNodeAlive(i)) {
//             return i;
//         }
//     }

//     return -1;
// }

// int RingNode::findHighestAliveNode() const
// {
//     for (int i = numNodes - 1; i >= 0; i--) {
//         if (isNodeAlive(i)) {
//             return i;
//         }
//     }

//     return -1;
// }

// bool RingNode::isNodeAlive(int id) const
// {
//     return id >= 0 && id < (int)aliveNodes.size() && aliveNodes[id];
// }

// void RingNode::finish()
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

//     csvFile << avgElectionTime << "," << avgMessageCount << "\n";
//     csvFile.close();

//     EV << "Ring completed elections = " << completedElectionCount << "\n";
//     EV << "Ring avg election time = " << avgElectionTime << "\n";
//     EV << "Ring avg message count = " << avgMessageCount << "\n";
// }

//------------------- normalised average for msg_count ----------------------------

// #include "RingNode.h"
// #include <fstream>
// #include <cstring>
// #include <algorithm>
// #include <cmath>

// Define_Module(RingNode);

// std::vector<bool> RingNode::aliveNodes;

// int RingNode::currentLeaderId = -1;
// int RingNode::globalElectionId = 0;
// bool RingNode::globalElectionInProgress = false;

// int RingNode::electionInitiatorId = -1;
// int RingNode::electionMaxId = -1;
// int RingNode::electionReturnCount = 0;
// int RingNode::expectedElectionReturnCount = 1;

// int RingNode::coordinatorReturnCount = 0;
// int RingNode::expectedCoordinatorReturnCount = 1;

// long RingNode::totalMessages = 0;
// long RingNode::electionStartMessageCount = 0;

// double RingNode::electionStartTime = 0.0;
// double RingNode::totalElectionTime = 0.0;

// long RingNode::totalElectionMessages = 0;
// int RingNode::completedElectionCount = 0;

// RingNode::~RingNode()
// {
//     if (crashEvent) cancelAndDelete(crashEvent);
//     if (recoveryEvent) cancelAndDelete(recoveryEvent);
//     if (detectLeaderDownEvent) cancelAndDelete(detectLeaderDownEvent);
// }

// void RingNode::initialize()
// {
//     nodeId = par("nodeId");
//     numNodes = getParentModule()->par("numNodes");
//     bidirectional = par("bidirectional").boolValue();

//     if ((int)aliveNodes.size() != numNodes) {
//         aliveNodes.assign(numNodes, true);

//         currentLeaderId = numNodes - 1;
//         globalElectionId = 0;
//         globalElectionInProgress = false;

//         electionInitiatorId = -1;
//         electionMaxId = -1;
//         electionReturnCount = 0;
//         expectedElectionReturnCount = 1;

//         coordinatorReturnCount = 0;
//         expectedCoordinatorReturnCount = 1;

//         totalMessages = 0;
//         electionStartMessageCount = 0;

//         electionStartTime = 0.0;
//         totalElectionTime = 0.0;

//         totalElectionMessages = 0;
//         completedElectionCount = 0;
//     }

//     alive = true;
//     aliveNodes[nodeId] = true;

//     isLeader = (nodeId == currentLeaderId);
//     electionInProgress = false;

//     buildGateMap();

//     crashEvent = new cMessage("CRASH");
//     recoveryEvent = new cMessage("RECOVERY");
//     detectLeaderDownEvent = new cMessage("DETECT_LEADER_DOWN");

//     scheduleNextCrash();
// }

// void RingNode::buildGateMap()
// {
//     neighborToGateIndex.clear();

//     for (int i = 0; i < gateSize("outGate"); i++) {
//         cGate *g = gate("outGate", i);

//         if (g->isConnected()) {
//             int destId = g->getPathEndGate()->getOwnerModule()->par("nodeId");
//             neighborToGateIndex[destId] = i;
//         }
//     }
// }

// void RingNode::handleMessage(cMessage *msg)
// {
//     if (msg->isSelfMessage()) {
//         if (msg == crashEvent) {
//             crashNode();
//             return;
//         }

//         if (msg == recoveryEvent) {
//             recoverNode();
//             return;
//         }

//         if (msg == detectLeaderDownEvent) {
//             if (alive) {
//                 startElection();
//             }
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

//     if (type == "ELECTION") {
//         handleElectionToken(pkt);
//         return;
//     }

//     if (type == "COORDINATOR") {
//         handleCoordinatorToken(pkt);
//         return;
//     }

//     delete pkt;
// }

// void RingNode::scheduleNextCrash()
// {
//     if (!alive || crashEvent->isScheduled()) {
//         return;
//     }

//     double crashTime = par("crashTime").doubleValue();

//     if (crashTime >= 0) {
//         scheduleAt(simTime() + crashTime, crashEvent);
//     }
// }

// void RingNode::crashNode()
// {
//     if (!alive) {
//         return;
//     }

//     bool leaderFailed = (nodeId == currentLeaderId);

//     EV << "Ring node " << nodeId << " crashed at " << simTime();

//     if (leaderFailed) {
//         EV << " leader failed";
//     }

//     EV << "\n";

//     alive = false;
//     aliveNodes[nodeId] = false;
//     isLeader = false;
//     electionInProgress = false;

//     if (detectLeaderDownEvent->isScheduled()) {
//         cancelEvent(detectLeaderDownEvent);
//     }

//     double recoveryTime = par("recoveryTime").doubleValue();

//     if (recoveryTime >= 0 && !recoveryEvent->isScheduled()) {
//         scheduleAt(simTime() + recoveryTime, recoveryEvent);
//     }

//     if (leaderFailed) {
//         scheduleElectionAfterLeaderCrash();
//     }
// }

// void RingNode::recoverNode()
// {
//     if (alive) {
//         return;
//     }

//     alive = true;
//     aliveNodes[nodeId] = true;

//     isLeader = (nodeId == currentLeaderId);
//     electionInProgress = false;

//     EV << "Ring node " << nodeId << " recovered at " << simTime()
//        << " and learned current leader = " << currentLeaderId << "\n";

//     scheduleNextCrash();
// }

// void RingNode::scheduleElectionAfterLeaderCrash()
// {
//     int starter = findLowestAliveNode();

//     if (starter < 0) {
//         currentLeaderId = -1;
//         globalElectionInProgress = false;
//         return;
//     }

//     globalElectionId++;
//     globalElectionInProgress = true;

//     electionInitiatorId = starter;
//     electionMaxId = starter;

//     electionReturnCount = 0;
//     expectedElectionReturnCount = bidirectional ? 2 : 1;

//     coordinatorReturnCount = 0;
//     expectedCoordinatorReturnCount = bidirectional ? 2 : 1;

//     electionStartTime = simTime().dbl();
//     electionStartMessageCount = totalMessages;

//     cModule *starterModule = getParentModule()->getSubmodule("node", starter);
//     RingNode *starterNode = check_and_cast<RingNode *>(starterModule);

//     simtime_t stopDelay = starterNode->par("stopDelay");

//     if (!starterNode->detectLeaderDownEvent->isScheduled()) {
//         starterNode->scheduleAt(simTime() + stopDelay, starterNode->detectLeaderDownEvent);
//     }
// }

// void RingNode::startElection()
// {
//     if (!alive || !globalElectionInProgress) {
//         return;
//     }

//     electionInProgress = true;
//     electionMaxId = nodeId;

//     EV << "Ring election started by node " << nodeId
//        << " at " << simTime()
//        << ", electionId = " << globalElectionId << "\n";

//     sendElectionToken(+1);

//     if (bidirectional) {
//         sendElectionToken(-1);
//     }
// }

// void RingNode::sendElectionToken(int direction)
// {
//     cPacket *pkt = new cPacket("ELECTION");

//     pkt->setByteLength(msgSizeBytes);
//     pkt->addPar("initiatorId") = electionInitiatorId;
//     pkt->addPar("electionId") = globalElectionId;
//     pkt->addPar("maxId") = nodeId;
//     pkt->addPar("direction") = direction;

//     sendToNextAliveNode(pkt, direction);
// }

// void RingNode::handleElectionToken(cPacket *pkt)
// {
//     int msgElectionId = pkt->par("electionId");
//     int initiatorId = pkt->par("initiatorId");
//     int direction = pkt->par("direction");
//     int maxId = pkt->par("maxId");

//     if (msgElectionId < globalElectionId) {
//         delete pkt;
//         return;
//     }

//     if (msgElectionId > globalElectionId) {
//         globalElectionId = msgElectionId;
//     }

//     electionInProgress = true;

//     if (nodeId > maxId) {
//         pkt->par("maxId") = nodeId;
//         maxId = nodeId;
//     }

//     if (nodeId == initiatorId) {
//         electionMaxId = std::max(electionMaxId, maxId);
//         electionReturnCount++;

//         delete pkt;

//         if (electionReturnCount >= expectedElectionReturnCount) {
//             completeElectionDecision();
//         }

//         return;
//     }

//     sendToNextAliveNode(pkt, direction);
// }

// void RingNode::completeElectionDecision()
// {
//     if (!alive || !globalElectionInProgress || nodeId != electionInitiatorId) {
//         return;
//     }

//     int leaderId = electionMaxId;

//     if (!isNodeAlive(leaderId)) {
//         leaderId = findHighestAliveNode();
//     }

//     if (leaderId < 0) {
//         globalElectionInProgress = false;
//         return;
//     }

//     currentLeaderId = leaderId;
//     isLeader = (nodeId == leaderId);
//     electionInProgress = false;

//     EV << "Ring election decision made. New leader = " << leaderId
//        << " at " << simTime() << "\n";

//     sendCoordinatorToken(+1, leaderId);

//     if (bidirectional) {
//         sendCoordinatorToken(-1, leaderId);
//     }
// }

// void RingNode::sendCoordinatorToken(int direction, int leaderId)
// {
//     cPacket *pkt = new cPacket("COORDINATOR");

//     pkt->setByteLength(msgSizeBytes);
//     pkt->addPar("initiatorId") = electionInitiatorId;
//     pkt->addPar("electionId") = globalElectionId;
//     pkt->addPar("leaderId") = leaderId;
//     pkt->addPar("direction") = direction;

//     sendToNextAliveNode(pkt, direction);
// }

// void RingNode::handleCoordinatorToken(cPacket *pkt)
// {
//     int msgElectionId = pkt->par("electionId");
//     int initiatorId = pkt->par("initiatorId");
//     int leaderId = pkt->par("leaderId");
//     int direction = pkt->par("direction");

//     if (msgElectionId < globalElectionId) {
//         delete pkt;
//         return;
//     }

//     currentLeaderId = leaderId;
//     isLeader = (nodeId == leaderId);
//     electionInProgress = false;

//     if (nodeId == initiatorId) {
//         coordinatorReturnCount++;

//         delete pkt;

//         if (coordinatorReturnCount >= expectedCoordinatorReturnCount) {
//             finishElectionMetrics();
//         }

//         return;
//     }

//     sendToNextAliveNode(pkt, direction);
// }

// void RingNode::finishElectionMetrics()
// {
//     if (!globalElectionInProgress) {
//         return;
//     }

//     double thisElectionTime = simTime().dbl() - electionStartTime;
//     long thisElectionMessages = totalMessages - electionStartMessageCount;

//     totalElectionTime += thisElectionTime;
//     totalElectionMessages += thisElectionMessages;
//     completedElectionCount++;

//     EV << "Ring election completed. New leader = " << currentLeaderId
//        << ", election time = " << thisElectionTime
//        << ", messages = " << thisElectionMessages << "\n";

//     globalElectionInProgress = false;
// }

// void RingNode::sendToNextAliveNode(cPacket *pkt, int direction)
// {
//     if (!alive) {
//         delete pkt;
//         return;
//     }

//     int nextAlive = getNextAliveNode(direction);

//     if (nextAlive < 0) {
//         delete pkt;
//         return;
//     }

//     int hops = getPhysicalHopCount(nodeId, nextAlive, direction);
//     simtime_t delay = getLogicalPathDelay(nodeId, nextAlive, direction);
//     double lossRate = getPathLossRate(nodeId, nextAlive, direction);

//     int attempts = 1;

//     while (uniform(0, 1) < lossRate && attempts < 50) {
//         attempts++;
//         delay += par("electionTimeout");
//     }

//     totalMessages += hops * attempts;

//     cModule *targetModule = getParentModule()->getSubmodule("node", nextAlive);
//     RingNode *targetNode = check_and_cast<RingNode *>(targetModule);

//     sendDirect(pkt, delay, SIMTIME_ZERO, targetNode, "directIn");
// }

// int RingNode::getNextAliveNode(int direction) const
// {
//     for (int step = 1; step < numNodes; step++) {
//         int candidate;

//         if (direction > 0) {
//             candidate = (nodeId + step) % numNodes;
//         } else {
//             candidate = (nodeId - step + numNodes) % numNodes;
//         }

//         if (candidate != nodeId && isNodeAlive(candidate)) {
//             return candidate;
//         }
//     }

//     return -1;
// }

// int RingNode::getPhysicalHopCount(int from, int to, int direction) const
// {
//     int hops = 0;
//     int cur = from;

//     while (cur != to && hops <= numNodes) {
//         cur = physicalNeighborOf(cur, direction);
//         hops++;
//     }

//     return hops;
// }

// simtime_t RingNode::getLogicalPathDelay(int from, int to, int direction)
// {
//     double totalDelay = 0.0;
//     int cur = from;
//     int hops = 0;

//     while (cur != to && hops <= numNodes) {
//         int next = physicalNeighborOf(cur, direction);

//         cModule *curModule = getParentModule()->getSubmodule("node", cur);
//         RingNode *curNode = check_and_cast<RingNode *>(curModule);

//         auto it = curNode->neighborToGateIndex.find(next);

//         if (it != curNode->neighborToGateIndex.end()) {
//             cGate *g = curNode->gate("outGate", it->second);
//             cChannel *ch = g->getTransmissionChannel();

//             if (ch) {
//                 if (ch->hasPar("delay")) {
//                     totalDelay += ch->par("delay").doubleValue();
//                 }

//                 if (ch->hasPar("datarate")) {
//                     double datarate = ch->par("datarate").doubleValue();

//                     if (datarate > 0) {
//                         totalDelay += ((double)msgSizeBytes * 8.0) / datarate;
//                     }
//                 }
//             }
//         }

//         cur = next;
//         hops++;
//     }

//     return totalDelay;
// }

// double RingNode::getPathLossRate(int from, int to, int direction)
// {
//     double survivalProbability = 1.0;

//     int cur = from;
//     int hops = 0;

//     while (cur != to && hops <= numNodes) {
//         int next = physicalNeighborOf(cur, direction);

//         cModule *curModule = getParentModule()->getSubmodule("node", cur);
//         RingNode *curNode = check_and_cast<RingNode *>(curModule);

//         auto it = curNode->neighborToGateIndex.find(next);

//         if (it != curNode->neighborToGateIndex.end()) {
//             cGate *g = curNode->gate("outGate", it->second);
//             cChannel *ch = g->getTransmissionChannel();

//             if (ch && ch->hasPar("lossRate")) {
//                 double lossRate = ch->par("lossRate").doubleValue();

//                 if (lossRate < 0) lossRate = 0;
//                 if (lossRate > 1) lossRate = 1;

//                 survivalProbability *= (1.0 - lossRate);
//             }
//         }

//         cur = next;
//         hops++;
//     }

//     return 1.0 - survivalProbability;
// }

// int RingNode::physicalNeighborOf(int id, int direction) const
// {
//     if (direction > 0) {
//         return (id + 1) % numNodes;
//     }

//     return (id - 1 + numNodes) % numNodes;
// }

// int RingNode::findLowestAliveNode() const
// {
//     for (int i = 0; i < numNodes; i++) {
//         if (isNodeAlive(i)) {
//             return i;
//         }
//     }

//     return -1;
// }

// int RingNode::findHighestAliveNode() const
// {
//     for (int i = numNodes - 1; i >= 0; i--) {
//         if (isNodeAlive(i)) {
//             return i;
//         }
//     }

//     return -1;
// }

// bool RingNode::isNodeAlive(int id) const
// {
//     return id >= 0 && id < (int)aliveNodes.size() && aliveNodes[id];
// }

// void RingNode::finish()
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

//     csvFile << avgElectionTime << "," << avgMessageCount << ",";
//     csvFile.close();

//     EV << "Ring completed elections = " << completedElectionCount << "\n";
//     EV << "Ring avg election time = " << avgElectionTime << "\n";
//     EV << "Ring avg message count = " << avgMessageCount << "\n";
// }

#include "RingNode.h"

Define_Module(RingNode);

std::vector<bool> RingNode::aliveNodes;

int RingNode::currentLeaderId = -1;
int RingNode::globalElectionId = 0;
bool RingNode::globalElectionInProgress = false;

int RingNode::electionInitiatorId = -1;
int RingNode::electionMaxId = -1;
int RingNode::electionReturnCount = 0;
int RingNode::expectedElectionReturnCount = 1;

int RingNode::coordinatorReturnCount = 0;
int RingNode::expectedCoordinatorReturnCount = 1;

long RingNode::totalMessages = 0;
long RingNode::electionStartMessageCount = 0;

double RingNode::electionStartTime = 0.0;
double RingNode::totalElectionTime = 0.0;

long RingNode::totalElectionMessages = 0;
int RingNode::completedElectionCount = 0;

RingNode::~RingNode()
{
    if (crashEvent) cancelAndDelete(crashEvent);
    if (recoveryEvent) cancelAndDelete(recoveryEvent);
}

void RingNode::initialize()
{
    nodeId = par("nodeId");
    numNodes = getParentModule()->par("numNodes");
    bidirectional = par("bidirectional").boolValue();

    if ((int)aliveNodes.size() != numNodes) {
        aliveNodes.assign(numNodes, true);

        currentLeaderId = numNodes - 1;
        globalElectionId = 0;
        globalElectionInProgress = false;

        electionInitiatorId = -1;
        electionMaxId = -1;
        electionReturnCount = 0;
        expectedElectionReturnCount = 1;

        coordinatorReturnCount = 0;
        expectedCoordinatorReturnCount = 1;

        totalMessages = 0;
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

    buildGateMap();

    crashEvent = new cMessage("CRASH");
    recoveryEvent = new cMessage("RECOVERY");

    if (isLeader) {
        scheduleLeaderCrash();
    }
}

void RingNode::buildGateMap()
{
    neighborToGateIndex.clear();

    for (int i = 0; i < gateSize("outGate"); i++) {
        cGate *g = gate("outGate", i);

        if (g->isConnected()) {
            int destId = g->getPathEndGate()->getOwnerModule()->par("nodeId");
            neighborToGateIndex[destId] = i;
        }
    }
}

void RingNode::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == crashEvent) {
            crashNode();
            return;
        }

        if (msg == recoveryEvent) {
            recoverNode();
            return;
        }

        delete msg;
        return;
    }

    if (!alive) {
        delete msg;
        return;
    }

    if (strcmp(msg->getName(), "START_ELECTION") == 0) {
        delete msg;
        startElection();
        return;
    }

    cPacket *pkt = check_and_cast<cPacket *>(msg);
    std::string type = pkt->getName();

    if (type == "ELECTION") {
        handleElectionToken(pkt);
        return;
    }

    if (type == "COORDINATOR") {
        handleCoordinatorToken(pkt);
        return;
    }

    delete pkt;
}

void RingNode::scheduleLeaderCrash()
{
    if (!alive || !isLeader || crashEvent->isScheduled()) {
        return;
    }

    double crashTime = par("crashTime").doubleValue();

    if (crashTime >= 0) {
        scheduleAt(simTime() + crashTime, crashEvent);
    }
}

void RingNode::crashNode()
{
    if (!alive) return;

    bool leaderFailed = (nodeId == currentLeaderId);

    EV << "Ring node " << nodeId << " crashed at " << simTime();

    if (leaderFailed) EV << " leader failed";

    EV << "\n";

    alive = false;
    aliveNodes[nodeId] = false;
    isLeader = false;
    electionInProgress = false;

    double recoveryTime = par("recoveryTime").doubleValue();

    if (recoveryTime >= 0 && !recoveryEvent->isScheduled()) {
        scheduleAt(simTime() + recoveryTime, recoveryEvent);
    }

    if (leaderFailed) {
        currentLeaderId = -1;
        scheduleElectionAfterLeaderCrash();
    }
}

void RingNode::recoverNode()
{
    if (alive) return;

    alive = true;
    aliveNodes[nodeId] = true;

    isLeader = (nodeId == currentLeaderId);
    electionInProgress = false;

    EV << "Ring node " << nodeId << " recovered at " << simTime()
       << " and learned current leader = " << currentLeaderId << "\n";

    if (isLeader) {
        scheduleLeaderCrash();
    }
}

void RingNode::scheduleElectionAfterLeaderCrash()
{
    int starter = findLowestAliveNode();

    if (starter < 0) {
        currentLeaderId = -1;
        globalElectionInProgress = false;
        return;
    }

    globalElectionId++;
    globalElectionInProgress = true;

    electionInitiatorId = starter;
    electionMaxId = starter;

    electionReturnCount = 0;
    expectedElectionReturnCount = bidirectional ? 2 : 1;

    coordinatorReturnCount = 0;
    expectedCoordinatorReturnCount = bidirectional ? 2 : 1;

    electionStartTime = simTime().dbl();
    electionStartMessageCount = totalMessages;

    cModule *starterModule = getParentModule()->getSubmodule("node", starter);
    RingNode *starterNode = check_and_cast<RingNode *>(starterModule);

    cMessage *startMsg = new cMessage("START_ELECTION");
    simtime_t stopDelay = starterNode->par("stopDelay");

    sendDirect(startMsg, stopDelay, SIMTIME_ZERO, starterNode, "directIn");
}

void RingNode::startElection()
{
    if (!alive || !globalElectionInProgress) {
        return;
    }

    electionInProgress = true;

    int aliveCount = countAliveNodes();

    if (aliveCount <= 1) {
        completeElectionDecision();
        return;
    }

    EV << "Ring election started by node " << nodeId
       << ", bidirectional = " << bidirectional << "\n";

    if (!bidirectional) {
        expectedElectionReturnCount = 1;
        sendElectionToken(+1, aliveCount);
    } else {
        int clockwiseCount = (aliveCount - 1 + 1) / 2;
        int counterClockwiseCount = (aliveCount - 1) - clockwiseCount;

        expectedElectionReturnCount = 0;

        if (clockwiseCount > 0) {
            expectedElectionReturnCount++;
            sendElectionToken(+1, clockwiseCount);
        }

        if (counterClockwiseCount > 0) {
            expectedElectionReturnCount++;
            sendElectionToken(-1, counterClockwiseCount);
        }

        if (expectedElectionReturnCount == 0) {
            completeElectionDecision();
        }
    }
}

void RingNode::sendElectionToken(int direction, int remaining)
{
    cPacket *pkt = new cPacket("ELECTION");

    pkt->setByteLength(msgSizeBytes);
    pkt->addPar("electionId") = globalElectionId;
    pkt->addPar("maxId") = nodeId;
    pkt->addPar("direction") = direction;
    pkt->addPar("remaining") = remaining;

    sendToNextAliveNode(pkt, direction);
}

void RingNode::handleElectionToken(cPacket *pkt)
{
    int msgElectionId = pkt->par("electionId");
    int direction = pkt->par("direction");
    int maxId = pkt->par("maxId");
    int remaining = pkt->par("remaining");

    if (msgElectionId < globalElectionId) {
        delete pkt;
        return;
    }

    electionInProgress = true;

    if (nodeId > maxId) {
        pkt->par("maxId") = nodeId;
        maxId = nodeId;
    }

    remaining--;
    pkt->par("remaining") = remaining;

    if (remaining <= 0) {
        delete pkt;
        completeElectionSegment(maxId);
        return;
    }

    sendToNextAliveNode(pkt, direction);
}

void RingNode::completeElectionSegment(int segmentMaxId)
{
    if (!globalElectionInProgress) return;

    electionMaxId = std::max(electionMaxId, segmentMaxId);
    electionReturnCount++;

    if (electionReturnCount >= expectedElectionReturnCount) {
        completeElectionDecision();
    }
}

void RingNode::completeElectionDecision()
{
    if (!globalElectionInProgress) return;

    int leaderId = electionMaxId;

    if (!isNodeAlive(leaderId)) {
        leaderId = findHighestAliveNode();
    }

    if (leaderId < 0) {
        globalElectionInProgress = false;
        return;
    }

    currentLeaderId = leaderId;
    isLeader = (nodeId == leaderId);
    electionInProgress = false;

    EV << "Ring election decision made. New leader = " << leaderId << "\n";

    int aliveCount = countAliveNodes();

    if (!bidirectional) {
        expectedCoordinatorReturnCount = 1;
        sendCoordinatorToken(+1, leaderId, aliveCount);
    } else {
        int clockwiseCount = (aliveCount - 1 + 1) / 2;
        int counterClockwiseCount = (aliveCount - 1) - clockwiseCount;

        expectedCoordinatorReturnCount = 0;

        if (clockwiseCount > 0) {
            expectedCoordinatorReturnCount++;
            sendCoordinatorToken(+1, leaderId, clockwiseCount);
        }

        if (counterClockwiseCount > 0) {
            expectedCoordinatorReturnCount++;
            sendCoordinatorToken(-1, leaderId, counterClockwiseCount);
        }

        if (expectedCoordinatorReturnCount == 0) {
            finishElectionMetrics();
        }
    }
}

void RingNode::sendCoordinatorToken(int direction, int leaderId, int remaining)
{
    cPacket *pkt = new cPacket("COORDINATOR");

    pkt->setByteLength(msgSizeBytes);
    pkt->addPar("electionId") = globalElectionId;
    pkt->addPar("leaderId") = leaderId;
    pkt->addPar("direction") = direction;
    pkt->addPar("remaining") = remaining;

    sendToNextAliveNode(pkt, direction);
}

void RingNode::handleCoordinatorToken(cPacket *pkt)
{
    int msgElectionId = pkt->par("electionId");
    int leaderId = pkt->par("leaderId");
    int direction = pkt->par("direction");
    int remaining = pkt->par("remaining");

    if (msgElectionId < globalElectionId) {
        delete pkt;
        return;
    }

    currentLeaderId = leaderId;
    isLeader = (nodeId == leaderId);
    electionInProgress = false;

    if (isLeader) {
        scheduleLeaderCrash();
    }

    remaining--;
    pkt->par("remaining") = remaining;

    if (remaining <= 0) {
        delete pkt;
        completeCoordinatorSegment();
        return;
    }

    sendToNextAliveNode(pkt, direction);
}

void RingNode::completeCoordinatorSegment()
{
    if (!globalElectionInProgress) return;

    coordinatorReturnCount++;

    if (coordinatorReturnCount >= expectedCoordinatorReturnCount) {
        finishElectionMetrics();
    }
}

void RingNode::finishElectionMetrics()
{
    if (!globalElectionInProgress) return;

    double thisElectionTime = simTime().dbl() - electionStartTime;
    long thisElectionMessages = totalMessages - electionStartMessageCount;

    totalElectionTime += thisElectionTime;
    totalElectionMessages += thisElectionMessages;
    completedElectionCount++;

    EV << "Ring election completed. New leader = " << currentLeaderId
       << ", election time = " << thisElectionTime
       << ", messages = " << thisElectionMessages << "\n";

    globalElectionInProgress = false;
}

void RingNode::sendToNextAliveNode(cPacket *pkt, int direction)
{
    if (!alive) {
        delete pkt;
        return;
    }

    int nextAlive = getNextAliveNode(direction);

    if (nextAlive < 0) {
        delete pkt;
        return;
    }

    std::vector<int> path = shortestPath(nodeId, nextAlive);

    if (path.empty()) {
        EV << "No path from " << nodeId << " to " << nextAlive << "\n";
        delete pkt;
        return;
    }

    int hops = getPathHopCount(path);
    simtime_t delay = getPathDelay(path);
    double lossRate = getPathLossRate(path);

    int attempts = 1;

    while (uniform(0, 1) < lossRate && attempts < 50) {
        attempts++;
        delay += par("electionTimeout");
    }

    totalMessages += hops * attempts;

    cModule *targetModule = getParentModule()->getSubmodule("node", nextAlive);
    RingNode *targetNode = check_and_cast<RingNode *>(targetModule);

    sendDirect(pkt, delay, SIMTIME_ZERO, targetNode, "directIn");
}

int RingNode::getNextAliveNode(int direction) const
{
    for (int step = 1; step < numNodes; step++) {
        int candidate;

        if (direction > 0) {
            candidate = (nodeId + step) % numNodes;
        } else {
            candidate = (nodeId - step + numNodes) % numNodes;
        }

        if (candidate != nodeId && isNodeAlive(candidate)) {
            return candidate;
        }
    }

    return -1;
}

int RingNode::countAliveNodes() const
{
    int count = 0;

    for (bool x : aliveNodes) {
        if (x) count++;
    }

    return count;
}

std::vector<int> RingNode::shortestPath(int src, int dest) const
{
    std::vector<int> parent(numNodes, -1);
    std::vector<bool> visited(numNodes, false);
    std::queue<int> q;

    visited[src] = true;
    q.push(src);

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        if (u == dest) break;

        cModule *uModule = getParentModule()->getSubmodule("node", u);
        RingNode *uNode = check_and_cast<RingNode *>(uModule);

        for (auto &entry : uNode->neighborToGateIndex) {
            int v = entry.first;

            if (!visited[v]) {
                visited[v] = true;
                parent[v] = u;
                q.push(v);
            }
        }
    }

    if (!visited[dest]) {
        return {};
    }

    std::vector<int> path;
    int cur = dest;

    while (cur != -1) {
        path.push_back(cur);
        cur = parent[cur];
    }

    std::reverse(path.begin(), path.end());
    return path;
}

int RingNode::getPathHopCount(const std::vector<int>& path) const
{
    if (path.size() < 2) return 0;
    return path.size() - 1;
}

simtime_t RingNode::getPathDelay(const std::vector<int>& path)
{
    double totalDelay = 0.0;

    for (int i = 0; i + 1 < (int)path.size(); i++) {
        int u = path[i];
        int v = path[i + 1];

        cModule *uModule = getParentModule()->getSubmodule("node", u);
        RingNode *uNode = check_and_cast<RingNode *>(uModule);

        auto it = uNode->neighborToGateIndex.find(v);

        if (it == uNode->neighborToGateIndex.end()) continue;

        cGate *g = uNode->gate("outGate", it->second);
        cChannel *ch = g->getTransmissionChannel();

        if (ch) {
            if (ch->hasPar("delay")) {
                totalDelay += ch->par("delay").doubleValue();
            }

            if (ch->hasPar("datarate")) {
                double datarate = ch->par("datarate").doubleValue();

                if (datarate > 0) {
                    totalDelay += ((double)msgSizeBytes * 8.0) / datarate;
                }
            }
        }
    }

    return totalDelay;
}

double RingNode::getPathLossRate(const std::vector<int>& path)
{
    double survivalProbability = 1.0;

    for (int i = 0; i + 1 < (int)path.size(); i++) {
        int u = path[i];
        int v = path[i + 1];

        cModule *uModule = getParentModule()->getSubmodule("node", u);
        RingNode *uNode = check_and_cast<RingNode *>(uModule);

        auto it = uNode->neighborToGateIndex.find(v);

        if (it == uNode->neighborToGateIndex.end()) continue;

        cGate *g = uNode->gate("outGate", it->second);
        cChannel *ch = g->getTransmissionChannel();

        if (ch && ch->hasPar("lossRate")) {
            double lossRate = ch->par("lossRate").doubleValue();

            if (lossRate < 0) lossRate = 0;
            if (lossRate > 1) lossRate = 1;

            survivalProbability *= (1.0 - lossRate);
        }
    }

    return 1.0 - survivalProbability;
}

int RingNode::findLowestAliveNode() const
{
    for (int i = 0; i < numNodes; i++) {
        if (isNodeAlive(i)) return i;
    }

    return -1;
}

int RingNode::findHighestAliveNode() const
{
    for (int i = numNodes - 1; i >= 0; i--) {
        if (isNodeAlive(i)) return i;
    }

    return -1;
}

bool RingNode::isNodeAlive(int id) const
{
    return id >= 0 && id < (int)aliveNodes.size() && aliveNodes[id];
}

void RingNode::finish()
{
    if (nodeId != 0) return;

    std::ofstream csvFile("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);

    if (!csvFile.is_open()) {
        EV << "Could not open analysis.csv\n";
        return;
    }

    double avgElectionTime = 0.0;
    double avgMessageCount = 0.0;

    if (completedElectionCount > 0) {
        avgElectionTime = totalElectionTime / completedElectionCount;
        avgMessageCount = (double)totalElectionMessages / completedElectionCount;
    }

    csvFile << avgElectionTime << "," << avgMessageCount << ",";
    csvFile.close();

    EV << "Ring completed elections = " << completedElectionCount << "\n";
    EV << "Ring avg election time = " << avgElectionTime << "\n";
    EV << "Ring avg message count = " << avgMessageCount << "\n";
}