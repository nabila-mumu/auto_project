package src;

channel LossyChannel extends ned.DatarateChannel
{
    parameters:
        double lossRate = default(0.2);
        delay = 80ms;
        datarate = 10Mbps;
}
