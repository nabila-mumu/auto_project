package src;

simple RaftNode
{
    parameters:
        int nodeId;
        double crashTime @unit(s) = default(exponential(1.25s));
        double recoveryTime @unit(s) = default(exponential(0.25s));
        double electionTimeoutMin @unit(s) = default(0.15s);
        double electionTimeoutMax @unit(s) = default(0.5s);
        double heartbeatInterval @unit(s) = default(0.05s);
        @display("i=block/routing");
    gates:
        input inGate[];
        output outGate[];
}

network RaftNetwork
{
    parameters:
        int numNodes = default(4);

    submodules:
        node[numNodes]: RaftNode {
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
