package src;

simple RingNode
{
    parameters:
        int nodeId;
        double crashTime @unit(s) = default(exponential(1.25s));
        double recoveryTime @unit(s) = default(exponential(0.25s));
        double stopDelay @unit(s) = default(1s);
        double electionTimeout @unit(s) = default(2s);
        bool bidirectional = default(true);
        @display("i=block/routing");
    gates:
        input inGate[];
        input directIn;
        output outGate[];
}

network RingNetwork
{
    parameters:
        int numNodes = default(4);

    submodules:
        node[numNodes]: RingNode {
            parameters:
                nodeId = index;
        }

    connections allowunconnected:

    node[1].outGate++ --> LossyChannel --> node[2].inGate++;
    node[2].outGate++ --> LossyChannel --> node[1].inGate++;
    node[0].outGate++ --> LossyChannel --> node[2].inGate++;
    node[2].outGate++ --> LossyChannel --> node[0].inGate++;
    node[0].outGate++ --> LossyChannel --> node[3].inGate++;
    node[3].outGate++ --> LossyChannel --> node[0].inGate++;
}
