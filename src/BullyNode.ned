package src;

simple BullyNode
{
    parameters:
        int nodeId;
        double crashTime @unit(s) = default(1.25s);
        double recoveryTime @unit(s) = default(0.25s);
        double electionTimeout @unit(s) = default(0.5s);
        double stopDelay @unit(s) = default(1s);
        @display("i=block/routing");
    gates:
        input inGate[];
        output outGate[];
}

network BullyNetwork
{
    parameters:
        int numNodes = default(4);
    submodules:
        node[numNodes]: BullyNode {
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
