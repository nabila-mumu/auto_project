package src;

channel LossyChannel extends ned.DatarateChannel
{
    parameters:
        double lossRate = default(0.2);
        delay = uniform(5ms, 20ms);
        datarate = 1Mbps;
}