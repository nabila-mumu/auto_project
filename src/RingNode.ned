package src;

simple RingNode
{
    parameters:
        int nodeId;
        @display("i=block/routing");
    gates:
        input inGate[];
        output outGate[];
}

network RingNetwork
{
    parameters:
        int numNodes = default(5);

    submodules:
        node[numNodes]: RingNode {
            parameters:
                nodeId = index;
        }

    connections allowunconnected:
        node[0].outGate++ --> LossyChannel --> node[1].inGate++;
        node[1].outGate++ --> LossyChannel --> node[2].inGate++;
        node[2].outGate++ --> LossyChannel --> node[3].inGate++;
        node[3].outGate++ --> LossyChannel --> node[4].inGate++;
        node[4].outGate++ --> LossyChannel --> node[0].inGate++;
}