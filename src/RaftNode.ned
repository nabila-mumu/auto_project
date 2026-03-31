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
        int numNodes = default(15);

    submodules:
        node[numNodes]: RaftNode {
            parameters:
                nodeId = index;
        }

    connections allowunconnected:

    node[4].outGate++ --> LossyChannel --> node[9].inGate++;
    node[9].outGate++ --> LossyChannel --> node[4].inGate++;

    node[3].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[3].inGate++;

    node[5].outGate++ --> LossyChannel --> node[10].inGate++;
    node[10].outGate++ --> LossyChannel --> node[5].inGate++;

    node[8].outGate++ --> LossyChannel --> node[9].inGate++;
    node[9].outGate++ --> LossyChannel --> node[8].inGate++;

    node[0].outGate++ --> LossyChannel --> node[5].inGate++;
    node[5].outGate++ --> LossyChannel --> node[0].inGate++;

    node[11].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[11].inGate++;

    node[0].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[0].inGate++;

    node[2].outGate++ --> LossyChannel --> node[11].inGate++;
    node[11].outGate++ --> LossyChannel --> node[2].inGate++;

    node[6].outGate++ --> LossyChannel --> node[11].inGate++;
    node[11].outGate++ --> LossyChannel --> node[6].inGate++;

    node[7].outGate++ --> LossyChannel --> node[10].inGate++;
    node[10].outGate++ --> LossyChannel --> node[7].inGate++;

    node[3].outGate++ --> LossyChannel --> node[6].inGate++;
    node[6].outGate++ --> LossyChannel --> node[3].inGate++;

    node[5].outGate++ --> LossyChannel --> node[12].inGate++;
    node[12].outGate++ --> LossyChannel --> node[5].inGate++;

    node[8].outGate++ --> LossyChannel --> node[11].inGate++;
    node[11].outGate++ --> LossyChannel --> node[8].inGate++;

    node[9].outGate++ --> LossyChannel --> node[10].inGate++;
    node[10].outGate++ --> LossyChannel --> node[9].inGate++;

    node[0].outGate++ --> LossyChannel --> node[7].inGate++;
    node[7].outGate++ --> LossyChannel --> node[0].inGate++;

    node[2].outGate++ --> LossyChannel --> node[4].inGate++;
    node[4].outGate++ --> LossyChannel --> node[2].inGate++;

    node[1].outGate++ --> LossyChannel --> node[8].inGate++;
    node[8].outGate++ --> LossyChannel --> node[1].inGate++;

    node[6].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[6].inGate++;

    node[7].outGate++ --> LossyChannel --> node[12].inGate++;
    node[12].outGate++ --> LossyChannel --> node[7].inGate++;

    node[3].outGate++ --> LossyChannel --> node[8].inGate++;
    node[8].outGate++ --> LossyChannel --> node[3].inGate++;

    node[5].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[5].inGate++;

    node[9].outGate++ --> LossyChannel --> node[12].inGate++;
    node[12].outGate++ --> LossyChannel --> node[9].inGate++;

    node[0].outGate++ --> LossyChannel --> node[9].inGate++;
    node[9].outGate++ --> LossyChannel --> node[0].inGate++;

    node[1].outGate++ --> LossyChannel --> node[10].inGate++;
    node[10].outGate++ --> LossyChannel --> node[1].inGate++;

    node[7].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[7].inGate++;

    node[12].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[12].inGate++;

    node[3].outGate++ --> LossyChannel --> node[10].inGate++;
    node[10].outGate++ --> LossyChannel --> node[3].inGate++;

    node[5].outGate++ --> LossyChannel --> node[7].inGate++;
    node[7].outGate++ --> LossyChannel --> node[5].inGate++;

    node[0].outGate++ --> LossyChannel --> node[2].inGate++;
    node[2].outGate++ --> LossyChannel --> node[0].inGate++;

    node[9].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[9].inGate++;

    node[1].outGate++ --> LossyChannel --> node[3].inGate++;
    node[3].outGate++ --> LossyChannel --> node[1].inGate++;

    node[13].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[13].inGate++;

    node[1].outGate++ --> LossyChannel --> node[12].inGate++;
    node[12].outGate++ --> LossyChannel --> node[1].inGate++;

    node[3].outGate++ --> LossyChannel --> node[12].inGate++;
    node[12].outGate++ --> LossyChannel --> node[3].inGate++;

    node[5].outGate++ --> LossyChannel --> node[9].inGate++;
    node[9].outGate++ --> LossyChannel --> node[5].inGate++;

    node[4].outGate++ --> LossyChannel --> node[11].inGate++;
    node[11].outGate++ --> LossyChannel --> node[4].inGate++;

    node[1].outGate++ --> LossyChannel --> node[5].inGate++;
    node[5].outGate++ --> LossyChannel --> node[1].inGate++;

    node[1].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[1].inGate++;

    node[2].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[2].inGate++;

    node[7].outGate++ --> LossyChannel --> node[9].inGate++;
    node[9].outGate++ --> LossyChannel --> node[7].inGate++;

    node[3].outGate++ --> LossyChannel --> node[5].inGate++;
    node[5].outGate++ --> LossyChannel --> node[3].inGate++;

    node[3].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[3].inGate++;

    node[5].outGate++ --> LossyChannel --> node[11].inGate++;
    node[11].outGate++ --> LossyChannel --> node[5].inGate++;

    node[4].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[4].inGate++;

    node[8].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[8].inGate++;

    node[1].outGate++ --> LossyChannel --> node[7].inGate++;
    node[7].outGate++ --> LossyChannel --> node[1].inGate++;

    node[2].outGate++ --> LossyChannel --> node[6].inGate++;
    node[6].outGate++ --> LossyChannel --> node[2].inGate++;

    node[7].outGate++ --> LossyChannel --> node[11].inGate++;
    node[11].outGate++ --> LossyChannel --> node[7].inGate++;

    node[3].outGate++ --> LossyChannel --> node[7].inGate++;
    node[7].outGate++ --> LossyChannel --> node[3].inGate++;

    node[4].outGate++ --> LossyChannel --> node[6].inGate++;
    node[6].outGate++ --> LossyChannel --> node[4].inGate++;

    node[5].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[5].inGate++;

    node[10].outGate++ --> LossyChannel --> node[12].inGate++;
    node[12].outGate++ --> LossyChannel --> node[10].inGate++;

    node[1].outGate++ --> LossyChannel --> node[9].inGate++;
    node[9].outGate++ --> LossyChannel --> node[1].inGate++;

    node[0].outGate++ --> LossyChannel --> node[11].inGate++;
    node[11].outGate++ --> LossyChannel --> node[0].inGate++;

    node[2].outGate++ --> LossyChannel --> node[8].inGate++;
    node[8].outGate++ --> LossyChannel --> node[2].inGate++;

    node[6].outGate++ --> LossyChannel --> node[8].inGate++;
    node[8].outGate++ --> LossyChannel --> node[6].inGate++;

    node[3].outGate++ --> LossyChannel --> node[9].inGate++;
    node[9].outGate++ --> LossyChannel --> node[3].inGate++;

    node[5].outGate++ --> LossyChannel --> node[6].inGate++;
    node[6].outGate++ --> LossyChannel --> node[5].inGate++;

    node[4].outGate++ --> LossyChannel --> node[8].inGate++;
    node[8].outGate++ --> LossyChannel --> node[4].inGate++;

    node[1].outGate++ --> LossyChannel --> node[2].inGate++;
    node[2].outGate++ --> LossyChannel --> node[1].inGate++;

    node[0].outGate++ --> LossyChannel --> node[4].inGate++;
    node[4].outGate++ --> LossyChannel --> node[0].inGate++;

    node[10].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[10].inGate++;

    node[1].outGate++ --> LossyChannel --> node[11].inGate++;
    node[11].outGate++ --> LossyChannel --> node[1].inGate++;

    node[0].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[0].inGate++;

    node[11].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[11].inGate++;

    node[2].outGate++ --> LossyChannel --> node[10].inGate++;
    node[10].outGate++ --> LossyChannel --> node[2].inGate++;

    node[6].outGate++ --> LossyChannel --> node[10].inGate++;
    node[10].outGate++ --> LossyChannel --> node[6].inGate++;

    node[12].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[12].inGate++;

    node[3].outGate++ --> LossyChannel --> node[11].inGate++;
    node[11].outGate++ --> LossyChannel --> node[3].inGate++;

    node[4].outGate++ --> LossyChannel --> node[10].inGate++;
    node[10].outGate++ --> LossyChannel --> node[4].inGate++;

    node[8].outGate++ --> LossyChannel --> node[10].inGate++;
    node[10].outGate++ --> LossyChannel --> node[8].inGate++;

    node[1].outGate++ --> LossyChannel --> node[4].inGate++;
    node[4].outGate++ --> LossyChannel --> node[1].inGate++;

    node[0].outGate++ --> LossyChannel --> node[6].inGate++;
    node[6].outGate++ --> LossyChannel --> node[0].inGate++;

    node[2].outGate++ --> LossyChannel --> node[3].inGate++;
    node[3].outGate++ --> LossyChannel --> node[2].inGate++;

    node[1].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[1].inGate++;

    node[2].outGate++ --> LossyChannel --> node[12].inGate++;
    node[12].outGate++ --> LossyChannel --> node[2].inGate++;

    node[3].outGate++ --> LossyChannel --> node[4].inGate++;
    node[4].outGate++ --> LossyChannel --> node[3].inGate++;

    node[4].outGate++ --> LossyChannel --> node[12].inGate++;
    node[12].outGate++ --> LossyChannel --> node[4].inGate++;

    node[8].outGate++ --> LossyChannel --> node[12].inGate++;
    node[12].outGate++ --> LossyChannel --> node[8].inGate++;

    node[9].outGate++ --> LossyChannel --> node[11].inGate++;
    node[11].outGate++ --> LossyChannel --> node[9].inGate++;

    node[0].outGate++ --> LossyChannel --> node[8].inGate++;
    node[8].outGate++ --> LossyChannel --> node[0].inGate++;

    node[1].outGate++ --> LossyChannel --> node[6].inGate++;
    node[6].outGate++ --> LossyChannel --> node[1].inGate++;

    node[2].outGate++ --> LossyChannel --> node[5].inGate++;
    node[5].outGate++ --> LossyChannel --> node[2].inGate++;

    node[2].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[2].inGate++;

    node[6].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[6].inGate++;

    node[7].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[7].inGate++;

    node[4].outGate++ --> LossyChannel --> node[5].inGate++;
    node[5].outGate++ --> LossyChannel --> node[4].inGate++;

    node[4].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[4].inGate++;

    node[0].outGate++ --> LossyChannel --> node[1].inGate++;
    node[1].outGate++ --> LossyChannel --> node[0].inGate++;

    node[8].outGate++ --> LossyChannel --> node[14].inGate++;
    node[14].outGate++ --> LossyChannel --> node[8].inGate++;

    node[10].outGate++ --> LossyChannel --> node[11].inGate++;
    node[11].outGate++ --> LossyChannel --> node[10].inGate++;

    node[9].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[9].inGate++;

    node[0].outGate++ --> LossyChannel --> node[10].inGate++;
    node[10].outGate++ --> LossyChannel --> node[0].inGate++;

    node[2].outGate++ --> LossyChannel --> node[7].inGate++;
    node[7].outGate++ --> LossyChannel --> node[2].inGate++;

    node[6].outGate++ --> LossyChannel --> node[7].inGate++;
    node[7].outGate++ --> LossyChannel --> node[6].inGate++;

    node[4].outGate++ --> LossyChannel --> node[7].inGate++;
    node[7].outGate++ --> LossyChannel --> node[4].inGate++;

    node[5].outGate++ --> LossyChannel --> node[8].inGate++;
    node[8].outGate++ --> LossyChannel --> node[5].inGate++;

    node[0].outGate++ --> LossyChannel --> node[3].inGate++;
    node[3].outGate++ --> LossyChannel --> node[0].inGate++;

    node[10].outGate++ --> LossyChannel --> node[13].inGate++;
    node[13].outGate++ --> LossyChannel --> node[10].inGate++;

    node[0].outGate++ --> LossyChannel --> node[12].inGate++;
    node[12].outGate++ --> LossyChannel --> node[0].inGate++;

    node[2].outGate++ --> LossyChannel --> node[9].inGate++;
    node[9].outGate++ --> LossyChannel --> node[2].inGate++;

    node[11].outGate++ --> LossyChannel --> node[12].inGate++;
    node[12].outGate++ --> LossyChannel --> node[11].inGate++;

    node[6].outGate++ --> LossyChannel --> node[9].inGate++;
    node[9].outGate++ --> LossyChannel --> node[6].inGate++;

    node[7].outGate++ --> LossyChannel --> node[8].inGate++;
    node[8].outGate++ --> LossyChannel --> node[7].inGate++;

}
