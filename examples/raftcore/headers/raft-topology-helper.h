//
// Created by Hython on 2020/3/16.
//

#ifndef NS3_RAFT_TOPOLOGY_HELPER_H
#define NS3_RAFT_TOPOLOGY_HELPER_H
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "cornerstone.hxx"

namespace ns3{
    class RaftTopologyHelper {
    public:
        RaftTopologyHelper(uint32_t nServer, uint32_t nClient, std::string dataRate, std::string delay);

        ~RaftTopologyHelper();

        NodeContainer GetNodes();

        NetDeviceContainer GetDevices();

        Ipv4InterfaceContainer GetInterfaces();

        // ApplicationContainer Install (NodeContainer c);
        ApplicationContainer InstallServer(NodeContainer c);

        Ptr <Application> InstallPrivServer(Ptr <Node> node);

        ApplicationContainer InstallClient(NodeContainer c);

        Ptr <Application> InstallPrivClient(Ptr <Node> node);

        ApplicationContainer InstallCa(NodeContainer c);

        Ptr <Application> InstallPrivCa(Ptr <Node> node);
        // ApplicationContainer GetApplications();

    private:
        NodeContainer m_nodes;
        NodeContainer m_servers;
        NodeContainer m_clients;
        NodeContainer m_ca;
        NetDeviceContainer m_devices;
        Ipv4InterfaceContainer m_interfaces;
        // ApplicationContainer m_apps;
        ApplicationContainer server_apps;
        ApplicationContainer client_apps;
        ApplicationContainer ca_app;
        std::vector<cornerstone::ptr<cornerstone::srv_config>> cluster;

    };
}
#endif //NS3_RAFT_TOPOLOGY_HELPER_H
