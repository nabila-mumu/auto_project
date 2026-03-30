package src;

simple RaftNode
{
    parameters:
        int nodeId;
        @display("i=block/routing");
    gates:
        input inGate[];
        output outGate[];
}

network RaftNetwork
{
    parameters:
        int numNodes = default(5);

    submodules:
        node[numNodes]: RaftNode {
            parameters:
                nodeId = index;
        }

    connections allowunconnected:

    // for i=0..numNodes-1, for j=0..numNodes-1, if i!=j {
    //     node[i].outGate++ --> LossyChannel --> node[j].inGate++;
    //}

    // Example: ring (same style as your previous)
    node[0].outGate++ --> LossyChannel --> node[1].inGate++;
    node[1].outGate++ --> LossyChannel --> node[2].inGate++;
    node[2].outGate++ --> LossyChannel --> node[3].inGate++;
    node[3].outGate++ --> LossyChannel --> node[4].inGate++;
    node[4].outGate++ --> LossyChannel --> node[0].inGate++;
}