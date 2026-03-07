package src;

network BullyNetworkComplete
{
    parameters:
        int numNodes = default(10);

    submodules:
        node[numNodes]: BullyNode {
            parameters:
                nodeId = index;
        }

    connections allowunconnected:
        for i=0..numNodes-1, for j=0..numNodes-1, if i!=j {
            node[i].outGate++ --> LossyChannel --> node[j].inGate++;
        }
}