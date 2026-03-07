#include "RingNode.h"
#include <fstream>
#include <set>
#include <map>
#include <sstream>
#include <climits>

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
    // currentLeaderId = -1;
    electionId = -1;

    messagesSent = 0;
    messagesReceived = 0;

    buildGateMap();

    EV << "Node " << nodeId << " initialized with successor " << successor << "\n";

    /* Randomized startup election */
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

            EV << "Node " << nodeId << " gate[" << i << "] -> Node " << destId << "\n";
        }
    }

    /* Choose successor: smallest ID greater than me, else smallest overall */
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

/* ---------- SEND TO SUCCESSOR ---------- */
void RingNode::sendToSuccessor(cMessage *msg)
{
    auto it = neighborToGateIndex.find(successor);
    if (it == neighborToGateIndex.end()) {
        EV << "ERROR: No gate to successor\n";
        delete msg;
        return;
    }

    send(msg, "outGate", it->second);
    messagesSent++;
    totalMessages++;
}

/* ---------- START ELECTION ---------- */
void RingNode::startElection()
{
    electionInProgress = true;
    electionId = ++globalElectionId;
    electionStartTime = simTime();

    EV << "\n*** Node " << nodeId
       << " starts ELECTION (ID " << electionId << ") ***\n";

    cMessage *msg = new cMessage("ELECTION");
    msg->addPar("initiatorId") = nodeId;
    msg->addPar("electionId") = electionId;
    msg->addPar("maxId") = nodeId;
    msg->addPar("startTime") = simTime().dbl();


    sendToSuccessor(msg);
}

/* ---------- HANDLE MESSAGE ---------- */
void RingNode::handleMessage(cMessage *msg)
{
    /* ---------- SELF MESSAGES ---------- */
    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "START_ELECTION") == 0) {
            startElection();
        }
        delete msg;
        return;
    }

    messagesReceived++;
    totalMessages++;

    /* ---------- ELECTION ---------- */
    if (strcmp(msg->getName(), "ELECTION") == 0) {
        int msgElectionId = msg->par("electionId");
        int initiatorId = msg->par("initiatorId");
        int maxId = msg->par("maxId");

        /* Drop older elections */
        if (msgElectionId < electionId) {
            delete msg;
            return;
        }

        /* Adopt newer election */
        if (msgElectionId > electionId) {
            electionId = msgElectionId;
            electionInProgress = true;
        }

        /* Update max ID */
        if (nodeId > maxId) {
            msg->par("maxId") = nodeId;
        }

        /* Election completed one full ring */
        if (nodeId == initiatorId) {
            int leader = msg->par("maxId");

            electionEndTime = simTime();
            currentLeaderId = leader;
            isLeader = (leader == nodeId);
            electionInProgress = false;

            //simtime_t start = msg->par("startTime");
            simtime_t start = SimTime(msg->par("startTime").doubleValue());
            simtime_t end = simTime();
            electionTime = end - start;

            EV << "\n*** Node " << nodeId << " completed election ***\n";
            EV << "Leader elected: Node " << leader << "\n";
            EV << "Election time: "
            //    << (electionEndTime - electionStartTime) << " s\n\n";
                << electionTime << " s\n\n";

            /* Send coordinator */
            cMessage *coord = new cMessage("COORDINATOR");
            coord->addPar("leaderId") = leader;
            coord->addPar("initiatorId") = nodeId;
            coord->addPar("startTime") = msg->par("startTime");


            delete msg;
            sendToSuccessor(coord);
            return;
        }

        sendToSuccessor(msg);
        return;
    }

    /* ---------- COORDINATOR ---------- */
    if (strcmp(msg->getName(), "COORDINATOR") == 0) {
        int leaderId = msg->par("leaderId");
        int initiatorId = msg->par("initiatorId");

        currentLeaderId = leaderId;
        electionInProgress = false;
        isLeader = (leaderId == nodeId);

        simtime_t start = SimTime(msg->par("startTime").doubleValue());
        //simtime_t start = msg->par("startTime");
        simtime_t end = simTime();
        electionTime = end - start;

        EV << "Node " << nodeId
           << " acknowledges leader " << leaderId << "\n";

        if (nodeId == initiatorId) {
            delete msg;
            return;
        }

        sendToSuccessor(msg);
        return;
    }
}

/* ---------- FINISH ---------- */
void RingNode::finish()
{
    EV << "\n=== Node " << nodeId << " Finish ===\n";
    EV << "Leader: " << (isLeader ? "YES" : "NO") << "\n";
    EV << "Messages Sent: " << messagesSent << "\n";
    EV << "Messages Received: " << messagesReceived << "\n";

    /* ---------- CSV OUTPUT (LEADER ONLY) ---------- */
    if (isLeader) {
        std::ofstream csvFile("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv", std::ios::app);
        if (csvFile.is_open()) {
            csvFile << electionTime << "," << totalMessages;
            csvFile.close();
        } else {
            EV << "Error: Unable to open topology_analysis.csv for writing." << endl;
        }
        EV << "CSV file written by leader node\n";
    }
}
