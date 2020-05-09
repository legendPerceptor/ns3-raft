//
// Created by Hython on 2020/3/16.
//

#include "raft-topology-helper.h"
#include <iostream>
#include <string>

using namespace ns3;

int main(int argc, char**argv) {
    CommandLine cmd;
    cmd.Parse(argc, argv);

    uint32_t nServer = 7;
    uint32_t nClient = 1;

    std::string dataRate= "100Mbps";
    std::string delay = "2ms";

    RaftTopologyHelper raftTopologyHelper(nServer, nClient, dataRate, delay);

    Simulator::Stop(Seconds(3));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}