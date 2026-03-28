#include <omnetpp.h>
#include <fstream>
#include <set>
#include <sstream>

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
    int leaderProbeFailures = 0;

    static int totalSentMessages;
    static int totalReceivedMessages;

    int sentMessages = 0;
    int receivedMessages = 0;
    simtime_t electionStartTime;
    simtime_t electionEndTime;

    std::set<int> answeredElections;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void startElection();
    void declareLeader();
    void probeLeader();
    void broadcastCoordinator();

    // ✅ SAFE SEND: only through connected outGate[]
    void sendToAllConnected(cMessage *msg);
};

Define_Module(BullyNode);

/* -------------------------------------------------- */
int BullyNode::totalSentMessages = 0;
int BullyNode::totalReceivedMessages = 0;

void BullyNode::sendToAllConnected(cMessage *msg)
{
    for (int i = 0; i < gateSize("outGate"); i++) {
        if (gate("outGate", i)->isConnected()) {
            send(msg->dup(), "outGate", i);
            sentMessages++;
            totalSentMessages++; 
        }
    }
    delete msg;
}

/* -------------------------------------------------- */

void BullyNode::initialize()
{
    nodeId = par("nodeId");
    numNodes = getParentModule()->par("numNodes");

    currentLeaderId = numNodes - 1;

    scheduleAt(simTime() + uniform(0.1, 0.2), new cMessage("PROBE_LEADER"));
}

/* -------------------------------------------------- */

void BullyNode::probeLeader()
{
    if (currentLeaderId == nodeId) {
        leaderProbeFailures = 0;
        scheduleAt(simTime() + 0.5, new cMessage("PROBE_LEADER"));
        return;
    }

    cMessage *probe = new cMessage("HEARTBEAT");
    probe->addPar("nodeId") = nodeId;

    sendToAllConnected(probe);

    scheduleAt(simTime() + 0.3, new cMessage("PROBE_TIMEOUT"));
}

/* -------------------------------------------------- */

void BullyNode::startElection()
{
    if (electionInProgress) return;

    electionInProgress = true;
    electionStartTime = simTime();
    answeredElections.clear();
    okResponsesReceived = 0;

    EV << "\n*** Node " << nodeId << " starts ELECTION ***\n";

    cMessage *msg = new cMessage("ELECTION");
    msg->addPar("senderId") = nodeId;

    sendToAllConnected(msg);

    scheduleAt(simTime() + 0.3, new cMessage("ELECTION_TIMEOUT"));
}

/* -------------------------------------------------- */

void BullyNode::declareLeader()
{
    electionEndTime = simTime();
    electionInProgress = false;
    isLeader = true;
    currentLeaderId = nodeId;
    leaderProbeFailures = 0;

    EV << "\n*** Node " << nodeId << " is LEADER ***\n";
    EV << "Election Time: " << (electionEndTime - electionStartTime) << " seconds\n";

    std::ofstream out("bully_statistics.txt");
    if (out.is_open()) {
        out << "Leader: " << nodeId << "\n";
        out << "Election Time: " << (electionEndTime - electionStartTime) << "\n";
        out << "Messages Sent: " << sentMessages << "\n";
        out.close();
    }

    // Append results to the CSV file
    std::ofstream csvFile("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);
    if (csvFile.is_open()) {
        csvFile << (electionEndTime - electionStartTime) << "," << totalSentMessages << ",";
        csvFile.close();
    } else {
        EV << "Error: Unable to open topology_analysis.csv for writing." << endl;
    }

    broadcastCoordinator();

    // Stop the simulation after the leader is elected
    EV << "Simulation is stopping as the leader has been elected." << endl;
    endSimulation();
}

/* -------------------------------------------------- */

void BullyNode::broadcastCoordinator()
{
    cMessage *msg = new cMessage("COORDINATOR");
    msg->addPar("leaderId") = nodeId;

    sendToAllConnected(msg);
}

/* -------------------------------------------------- */

void BullyNode::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        std::string name = msg->getName();

        if (name == "PROBE_LEADER") {
            probeLeader();
        }
        else if (name == "PROBE_TIMEOUT") {
            leaderProbeFailures++;
            if (leaderProbeFailures >= 3) {
                leaderProbeFailures = 0;
                startElection();
            } else {
                scheduleAt(simTime() + 0.5, new cMessage("PROBE_LEADER"));
            }
        }
        else if (name == "ELECTION_TIMEOUT") {
            if (electionInProgress) {
                declareLeader();
            }
        }

        delete msg;
        return;
    }

    receivedMessages++;
    totalReceivedMessages++;
    std::string type = msg->getName();

    if (type == "ELECTION") {
        int senderId = msg->par("senderId");

        if (nodeId > senderId &&
            answeredElections.find(senderId) == answeredElections.end()) {

            cMessage *ok = new cMessage("OK");
            ok->addPar("responderId") = nodeId;
            sendToAllConnected(ok);

            answeredElections.insert(senderId);

            if (!electionInProgress)
                startElection();
        }
        delete msg;
    }
    else if (type == "OK") {
        if (electionInProgress) {
            electionInProgress = false;
        }
        delete msg;
    }
    else if (type == "COORDINATOR") {
        currentLeaderId = msg->par("leaderId");
        isLeader = (currentLeaderId == nodeId);
        electionInProgress = false;
        leaderProbeFailures = 0;

        scheduleAt(simTime() + 0.5, new cMessage("PROBE_LEADER"));
        delete msg;
    }
    else if (type == "HEARTBEAT") {
        delete msg;
    }
}

/* -------------------------------------------------- */

void BullyNode::finish()
{
    EV << "\n=== Node " << nodeId << " Summary ===\n";
    EV << "Leader: " << (isLeader ? "YES" : "NO") << "\n";
    EV << "Sent: " << sentMessages << "\n";
    EV << "Received: " << receivedMessages << "\n";
}

// #include <omnetpp.h>
// #include <set>
// #include <fstream>

// using namespace omnetpp;

// class BullyNode : public cSimpleModule
// {
//   private:
//     int nodeId;
//     int numNodes;

//     int leaderId = -1;
//     bool electionInProgress = false;
//     bool isAlive = true;

//     simtime_t electionStartTime;

//     // self messages
//     cMessage *startElectionMsg = nullptr;
//     cMessage *electionTimeoutMsg = nullptr;
//     cMessage *crashMsg = nullptr;

//     // global stats
//     static long totalMessagesSent;
//     static long totalMessagesReceived;
//     static int nodesConfirmedLeader;

//   protected:
//     virtual void initialize() override;
//     virtual void handleMessage(cMessage *msg) override;
//     virtual void finish() override;
//     virtual ~BullyNode();

//     void startElection();
//     void declareLeader();
//     void broadcastCoordinator();
//     void crashNode();

//     void sendToAll(cMessage *msg);
// };

// Define_Module(BullyNode);

// /* -------- STATIC VARIABLES -------- */
// long BullyNode::totalMessagesSent = 0;
// long BullyNode::totalMessagesReceived = 0;
// int BullyNode::nodesConfirmedLeader = 0;

// /* -------- DESTRUCTOR -------- */
// BullyNode::~BullyNode()
// {
//     cancelAndDelete(startElectionMsg);
//     cancelAndDelete(electionTimeoutMsg);
//     cancelAndDelete(crashMsg);
// }

// /* -------- SAFE BROADCAST -------- */
// void BullyNode::sendToAll(cMessage *msg)
// {
//     for (int i = 0; i < gateSize("outGate"); i++)
//     {
//         if (gate("outGate", i)->isConnected())
//         {
//             send(msg->dup(), "outGate", i);
//             totalMessagesSent++;
//         }
//     }
//     delete msg;
// }

// /* -------- INITIALIZE -------- */
// void BullyNode::initialize()
// {
//     nodeId = par("nodeId");
//     numNodes = getParentModule()->par("numNodes");

//     // if (nodeId == 0)
//     // {
//     //     totalMessagesSent = 0;
//     //     totalMessagesReceived = 0;
//     //     nodesConfirmedLeader = 0;
//     // }

//     // optional crash
//     simtime_t crashTime = par("crashTime");
//     if (crashTime >= 0)
//     {
//         crashMsg = new cMessage("CRASH");
//         scheduleAt(crashTime, crashMsg);
//     }

//     // start election from node 0
//     // if (nodeId == 0)
//     if (uniform(0,1) < 0.3)  
//     {
//         totalMessagesSent = 0;
//         totalMessagesReceived = 0;
//         nodesConfirmedLeader = 0;

//         startElectionMsg = new cMessage("START");
//         scheduleAt(simTime() + 0.1, startElectionMsg);
//     }
// }

// /* -------- START ELECTION -------- */
// void BullyNode::startElection()
// {
//     if (!isAlive || electionInProgress)
//         return;

//     EV << "Node " << nodeId << " starts election\n";

//     electionStartTime = simTime();
//     electionInProgress = true;

//     cMessage *msg = new cMessage("ELECTION");
//     msg->addPar("senderId") = nodeId;

//     sendToAll(msg);

//     if (!electionTimeoutMsg)
//         electionTimeoutMsg = new cMessage("ELECTION_TIMEOUT");

//     if (electionTimeoutMsg->isScheduled())
//         cancelEvent(electionTimeoutMsg);

//     scheduleAt(simTime() + par("electionTimeout"), electionTimeoutMsg);
// }

// /* -------- DECLARE LEADER -------- */
// void BullyNode::declareLeader()
// {
//     leaderId = nodeId;
//     electionInProgress = false;

//     simtime_t totalTime = simTime() - electionStartTime;

//     EV << "\n===== LEADER ELECTED: Node "
//        << nodeId << " =====\n";

//     EV << "Election Time: " << totalTime << "\n";

//     // write results
//     std::ofstream file("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);
//     if (file.is_open())
//     {
//         file << nodeId << ","
//              << totalTime.dbl() << ","
//              << (totalMessagesSent + totalMessagesReceived)
//              << "\n";
//         file.close();
//     }

//     broadcastCoordinator();
// }

// /* -------- BROADCAST LEADER -------- */
// void BullyNode::broadcastCoordinator()
// {
//     cMessage *msg = new cMessage("COORDINATOR");
//     msg->addPar("leaderId") = nodeId;

//     sendToAll(msg);
// }

// /* -------- CRASH -------- */
// void BullyNode::crashNode()
// {
//     EV << "Node " << nodeId << " crashed!\n";
//     isAlive = false;
// }

// /* -------- HANDLE MESSAGE -------- */
// void BullyNode::handleMessage(cMessage *msg)
// {
//     if (msg->isSelfMessage())
//     {
//         if (msg == startElectionMsg)
//         {
//             startElection();
//         }
//         else if (msg == electionTimeoutMsg)
//         {
//             if (electionInProgress)
//                 declareLeader();
//         }
//         else if (msg == crashMsg)
//         {
//             crashNode();
//         }

//         return;
//     }

//     if (!isAlive)
//     {
//         delete msg;
//         return;
//     }

//     totalMessagesReceived++;
//     std::string type = msg->getName();

//     if (type == "ELECTION")
//     {
//         int senderId = msg->par("senderId");

//         if (nodeId > senderId)
//         {
//             cMessage *ok = new cMessage("OK");
//             ok->addPar("responderId") = nodeId;

//             sendToAll(ok);

//             startElection();
//         }
//     }
//     else if (type == "OK")
//     {
//         electionInProgress = false;
//     }
//     else if (type == "COORDINATOR")
//     {
//         leaderId = msg->par("leaderId");
//         electionInProgress = false;

//         EV << "Node " << nodeId
//            << " accepts leader "
//            << leaderId << "\n";

//         nodesConfirmedLeader++;

//         if (nodesConfirmedLeader == numNodes)
//         {
//             EV << "\n===== ALL NODES AGREED =====\n";
//             EV << "Total Messages: "
//                << (totalMessagesSent + totalMessagesReceived)
//                << "\n";

//             // endSimulation();
//         }
//     }

//     delete msg;
// }

// /* -------- FINISH -------- */
// void BullyNode::finish()
// {
//     EV << "Node " << nodeId
//        << " final leader: " << leaderId << "\n";
// }
// -------------------------------------------------------------------------------------------------

// /* Real Bully algorithm rules:
// When node detects leader failure --> Sends ELECTION to all nodes with higher ID
// If receives OK from higher node --> Stops election and waits
// If no OK before timeout --> Declares itself leader
// Leader sends COORDINATOR to all
// */
// #include <omnetpp.h>
// #include <set>
// #include <fstream>

// using namespace omnetpp;

// class BullyNode : public cSimpleModule
// {
//   private:
//     int nodeId;
//     int numNodes;

//     int leaderId = -1;
//     bool electionInProgress = false;
//     bool isAlive = true;
//     simtime_t electionStartTime; 

//     // self messages
//     cMessage *startElectionMsg = nullptr;
//     cMessage *electionTimeoutMsg = nullptr;
//     cMessage *crashMsg = nullptr;
//     cMessage *stopMsg = nullptr;

//   protected:
//     virtual void initialize() override;
//     virtual void handleMessage(cMessage *msg) override;
//     virtual void finish() override;
//     virtual ~BullyNode();

//     void startElection();
//     void declareLeader();
//     void broadcastCoordinator();
//     void crashNode();
// };

// static long totalMessagesSent = 0;
// static long totalMessagesReceived = 0;

// Define_Module(BullyNode);

// /*-------------------------------------------*/

// BullyNode::~BullyNode()
// {
//     cancelAndDelete(startElectionMsg);
//     cancelAndDelete(electionTimeoutMsg);
//     cancelAndDelete(crashMsg);
//     cancelAndDelete(stopMsg);
// }

// /*-------------------------------------------*/

// void BullyNode::initialize()
// {
//     nodeId = par("nodeId");
//     numNodes = getParentModule()->par("numNodes");

//     // Optional crash time parameter
//     simtime_t crashTime = par("crashTime");

//     if (crashTime >= 0)
//     {
//         crashMsg = new cMessage("CRASH");
//         scheduleAt(crashTime, crashMsg);
//     }

//     // Only node 0 initiates election at start
//     if (nodeId == 0)
//     {
//         startElectionMsg = new cMessage("START");
//         scheduleAt(simTime() + 0.1, startElectionMsg);
//     }
// }

// /*-------------------------------------------*/

// void BullyNode::startElection()
// {
//     if (!isAlive) return;

//     if (electionInProgress)
//         return;  // already running election

//     electionStartTime = simTime();

//     EV << "Node " << nodeId << " starts election\n";

//     electionInProgress = true;

//     for (int i = 0; i < gateSize("outGate"); i++)
//     {
//         if (gate("outGate", i)->isConnected())
//         {
//             cMessage *msg = new cMessage("ELECTION");
//             msg->addPar("senderId") = nodeId;
//             send(msg, "outGate", i);
//             totalMessagesSent++;
//         }
//     }

//     if (!electionTimeoutMsg)
//         electionTimeoutMsg = new cMessage("ELECTION_TIMEOUT");

//     // 🔥 critical fix
//     if (electionTimeoutMsg->isScheduled())
//         cancelEvent(electionTimeoutMsg);

//     scheduleAt(simTime() + par("electionTimeout"), electionTimeoutMsg);
// }

// /*-------------------------------------------*/

// void BullyNode::declareLeader()
// {
//     leaderId = nodeId;
//     electionInProgress = false;

//     simtime_t electionEndTime = simTime();
//     simtime_t totalElectionTime = electionEndTime - electionStartTime;

//     long totalMessages = totalMessagesSent + totalMessagesReceived;

//     EV << "\n===== LEADER ELECTED: Node "
//        << nodeId << " =====\n\n";

//     EV << "Election Time: " << totalElectionTime << "\n";
//     EV << "Total Messages: " << totalMessages << "\n\n";

//     // ✅ WRITE TO CSV
//     std::ofstream file(
//         "/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv",
//         std::ios::app
//     );

//     if (file.is_open())
//     {
//         file << nodeId << ","
//              << totalElectionTime.dbl() << ","
//              << totalMessages << "\n";

//         file.close();
//     }
//     else
//     {
//         EV << "ERROR: Could not open analysis.csv\n";
//     }

//     broadcastCoordinator();  

//     // 🔥 schedule simulation stop after delay
//     // if (!stopMsg)
//     //     stopMsg = new cMessage("STOP_SIM");

//     // scheduleAt(simTime() + par("stopDelay"), stopMsg);
    
//     endSimulation();
// }

// /*-------------------------------------------*/

// void BullyNode::broadcastCoordinator()
// {
//     for (int i = 0; i < gateSize("outGate"); i++)
//     {
//         if (gate("outGate", i)->isConnected())
//         {
//             cMessage *msg = new cMessage("COORDINATOR");
//             msg->addPar("leaderId") = nodeId;
//             send(msg, "outGate", i);
//             totalMessagesSent++;
//         }
//     }
// }

// /*-------------------------------------------*/

// void BullyNode::crashNode()
// {
//     EV << "Node " << nodeId << " crashed!\n";
//     isAlive = false;
// }

// /*-------------------------------------------*/

// void BullyNode::handleMessage(cMessage *msg)
// {
//     if (msg->isSelfMessage())
//     {
//         if (msg == startElectionMsg)
//         {
//             startElection();
//         }
//         else if (msg == electionTimeoutMsg)
//         {
//             if (electionInProgress)
//                 declareLeader();
//         }
//         else if (msg == crashMsg)
//         {
//             crashNode();
//         }
//         // 🔥 critical fix
//         else if (msg == stopMsg)
//         {
//             EV << "Stopping simulation after coordinator propagation\n";
//             endSimulation();
//             delete msg;
//             return;
//         }
//         return;
//     }

//     if (!isAlive)
//     {
//         delete msg;
//         return;
//     }

//     totalMessagesReceived++;
//     std::string type = msg->getName();

//     if (type == "ELECTION")
//     {
//         int senderId = msg->par("senderId");

//         if (nodeId > senderId)
//         {
//             cMessage *ok = new cMessage("OK");
//             // send(ok, msg->getArrivalGate()->getIndex());
//             int gateIndex = msg->getArrivalGate()->getIndex();
//             send(ok, "outGate", gateIndex);
//             totalMessagesSent++;

//             startElection();
//         }
//     }
//     else if (type == "OK")
//     {
//         electionInProgress = false;
//     }
//     else if (type == "COORDINATOR")
//     {
//         leaderId = msg->par("leaderId");
//         electionInProgress = false;

//         // int newLeader = msg->par("leaderId");

//         // if (leaderId != newLeader)
//         // {
//         //     leaderId = newLeader;
//         //     electionInProgress = false;

//         //     broadcastCoordinator();  // propagate further
//         // }

//         EV << "Node " << nodeId
//            << " recognizes leader "
//            << leaderId << "\n";
//     }

//     delete msg;
// }

// /*-------------------------------------------*/

// void BullyNode::finish()
// {
//     EV << "Node " << nodeId
//        << " final leader: " << leaderId << "\n";
// }
