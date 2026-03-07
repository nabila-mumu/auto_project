package src;

simple BullyNode
{
    parameters:
        int nodeId;
        double crashTime @unit(s) = default(-1s);
        double electionTimeout @unit(s) = default(0.5s);
        double stopDelay @unit(s) = default(1s);
        @display("i=block/routing");
    gates:
        input inGate[];
        output outGate[];
}

channel LossyChannel extends ned.DatarateChannel
{
    parameters:
        double lossRate = default(0.2);   // 20% message loss
        delay = uniform(5ms, 20ms);
        datarate = 1Mbps;
}

network BullyNetwork
{
    parameters:
        int numNodes = default(10);
    submodules:
        node[numNodes]: BullyNode {
            parameters:
                nodeId = index;
        }

    connections allowunconnected:

    // Original ring
    node[0].outGate++ --> LossyChannel --> node[1].inGate++;
    node[1].outGate++ --> LossyChannel --> node[0].inGate++;

    node[1].outGate++ --> LossyChannel --> node[2].inGate++;
    node[2].outGate++ --> LossyChannel --> node[1].inGate++;

    node[2].outGate++ --> LossyChannel --> node[3].inGate++;
    node[3].outGate++ --> LossyChannel --> node[2].inGate++;

    node[3].outGate++ --> LossyChannel --> node[4].inGate++;
    node[4].outGate++ --> LossyChannel --> node[3].inGate++;

    node[4].outGate++ --> LossyChannel --> node[5].inGate++;
    node[5].outGate++ --> LossyChannel --> node[4].inGate++;

    node[5].outGate++ --> LossyChannel --> node[0].inGate++;
    node[0].outGate++ --> LossyChannel --> node[5].inGate++;

}
