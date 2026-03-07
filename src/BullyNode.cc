/* Real Bully algorithm rules:
When node detects leader failure --> Sends ELECTION to all nodes with higher ID
If receives OK from higher node --> Stops election and waits
If no OK before timeout --> Declares itself leader
Leader sends COORDINATOR to all
*/
#include <omnetpp.h>
#include <set>
#include <fstream>

using namespace omnetpp;

class BullyNode : public cSimpleModule
{
  private:
    int nodeId;
    int numNodes;

    int leaderId = -1;
    bool electionInProgress = false;
    bool isAlive = true;
    simtime_t electionStartTime; 

    // self messages
    cMessage *startElectionMsg = nullptr;
    cMessage *electionTimeoutMsg = nullptr;
    cMessage *crashMsg = nullptr;
    cMessage *stopMsg = nullptr;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual ~BullyNode();

    void startElection();
    void declareLeader();
    void broadcastCoordinator();
    void crashNode();
};

static long totalMessagesSent = 0;
static long totalMessagesReceived = 0;

Define_Module(BullyNode);

/*-------------------------------------------*/

BullyNode::~BullyNode()
{
    cancelAndDelete(startElectionMsg);
    cancelAndDelete(electionTimeoutMsg);
    cancelAndDelete(crashMsg);
    cancelAndDelete(stopMsg);
}

/*-------------------------------------------*/

void BullyNode::initialize()
{
    nodeId = par("nodeId");
    numNodes = getParentModule()->par("numNodes");

    // Optional crash time parameter
    simtime_t crashTime = par("crashTime");

    if (crashTime >= 0)
    {
        crashMsg = new cMessage("CRASH");
        scheduleAt(crashTime, crashMsg);
    }

    // Only node 0 initiates election at start
    if (nodeId == 0)
    {
        startElectionMsg = new cMessage("START");
        scheduleAt(simTime() + 0.1, startElectionMsg);
    }
}

/*-------------------------------------------*/

void BullyNode::startElection()
{
    if (!isAlive) return;

    if (electionInProgress)
        return;  // already running election

    electionStartTime = simTime();

    EV << "Node " << nodeId << " starts election\n";

    electionInProgress = true;

    for (int i = 0; i < gateSize("outGate"); i++)
    {
        if (gate("outGate", i)->isConnected())
        {
            cMessage *msg = new cMessage("ELECTION");
            msg->addPar("senderId") = nodeId;
            send(msg, "outGate", i);
            totalMessagesSent++;
        }
    }

    if (!electionTimeoutMsg)
        electionTimeoutMsg = new cMessage("ELECTION_TIMEOUT");

    // 🔥 critical fix
    if (electionTimeoutMsg->isScheduled())
        cancelEvent(electionTimeoutMsg);

    scheduleAt(simTime() + par("electionTimeout"), electionTimeoutMsg);
}

/*-------------------------------------------*/

void BullyNode::declareLeader()
{
    leaderId = nodeId;
    electionInProgress = false;

    simtime_t electionEndTime = simTime();
    simtime_t totalElectionTime = electionEndTime - electionStartTime;

    long totalMessages = totalMessagesSent + totalMessagesReceived;

    EV << "\n===== LEADER ELECTED: Node "
       << nodeId << " =====\n\n";

    EV << "Election Time: " << totalElectionTime << "\n";
    EV << "Total Messages: " << totalMessages << "\n\n";

    // ✅ WRITE TO CSV
    std::ofstream file(
        "/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv",
        std::ios::app
    );

    if (file.is_open())
    {
        file << nodeId << ","
             << totalElectionTime.dbl() << ","
             << totalMessages << "\n";

        file.close();
    }
    else
    {
        EV << "ERROR: Could not open analysis.csv\n";
    }

    broadcastCoordinator();  

    // 🔥 schedule simulation stop after delay
    // if (!stopMsg)
    //     stopMsg = new cMessage("STOP_SIM");

    // scheduleAt(simTime() + par("stopDelay"), stopMsg);
    
    endSimulation();
}

/*-------------------------------------------*/

void BullyNode::broadcastCoordinator()
{
    for (int i = 0; i < gateSize("outGate"); i++)
    {
        if (gate("outGate", i)->isConnected())
        {
            cMessage *msg = new cMessage("COORDINATOR");
            msg->addPar("leaderId") = nodeId;
            send(msg, "outGate", i);
            totalMessagesSent++;
        }
    }
}

/*-------------------------------------------*/

void BullyNode::crashNode()
{
    EV << "Node " << nodeId << " crashed!\n";
    isAlive = false;
}

/*-------------------------------------------*/

void BullyNode::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (msg == startElectionMsg)
        {
            startElection();
        }
        else if (msg == electionTimeoutMsg)
        {
            if (electionInProgress)
                declareLeader();
        }
        else if (msg == crashMsg)
        {
            crashNode();
        }
        // 🔥 critical fix
        else if (msg == stopMsg)
        {
            EV << "Stopping simulation after coordinator propagation\n";
            endSimulation();
            delete msg;
            return;
        }
        return;
    }

    if (!isAlive)
    {
        delete msg;
        return;
    }

    totalMessagesReceived++;
    std::string type = msg->getName();

    if (type == "ELECTION")
    {
        int senderId = msg->par("senderId");

        if (nodeId > senderId)
        {
            cMessage *ok = new cMessage("OK");
            // send(ok, msg->getArrivalGate()->getIndex());
            int gateIndex = msg->getArrivalGate()->getIndex();
            send(ok, "outGate", gateIndex);
            totalMessagesSent++;

            startElection();
        }
    }
    else if (type == "OK")
    {
        electionInProgress = false;
    }
    else if (type == "COORDINATOR")
    {
        leaderId = msg->par("leaderId");
        electionInProgress = false;

        // int newLeader = msg->par("leaderId");

        // if (leaderId != newLeader)
        // {
        //     leaderId = newLeader;
        //     electionInProgress = false;

        //     broadcastCoordinator();  // propagate further
        // }

        EV << "Node " << nodeId
           << " recognizes leader "
           << leaderId << "\n";
    }

    delete msg;
}

/*-------------------------------------------*/

void BullyNode::finish()
{
    EV << "Node " << nodeId
       << " final leader: " << leaderId << "\n";
}
