//
// Created by Hython on 2020/3/16.
//

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "raft-topology-helper.h"
#include "raft_node.h"
#include "client_node.h"
#include "ptr.hxx"
#include "srv_config.hxx"

using namespace cornerstone;
namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("Raft_Network");
    RaftTopologyHelper::RaftTopologyHelper(uint32_t nServer, uint32_t nClient, std::string dataRate,
                                           std::string delay) {

        NS_LOG_INFO("Topology start");
        m_nodes.Create(nServer + nClient + 1);
        for(uint32_t i=0; i<nServer;i++) {
            m_servers.Add(m_nodes.Get(i));
        }
        for(uint32_t i=nServer; i<nServer+nClient;i++) {
            m_clients.Add(m_nodes.Get(i));
        }
        m_ca.Add(m_nodes.Get(nServer+nClient));
        CsmaHelper csma;
        csma.SetChannelAttribute("DataRate", StringValue(dataRate));
        csma.SetChannelAttribute("Delay", StringValue(delay));
        LogComponentEnableAll (LOG_PREFIX_FUNC);
        LogComponentEnableAll (LOG_PREFIX_TIME);
        LogComponentEnable("RPC_SESSION",LogLevel(LOG_DEBUG|LOG_INFO));
        LogComponentEnable("Raft_Server_Node",LogLevel(LOG_DEBUG|LOG_INFO));
        LogComponentEnable("Raft_Client_Node",LogLevel(LOG_DEBUG|LOG_INFO|LOG_ERROR));
        //LogComponentEnable("Raft_Network",LogLevel(LOG_DEBUG|LOG_INFO));
        m_devices = csma.Install(m_nodes);

        InternetStackHelper stack;
        stack.Install(m_nodes);

        Ipv4AddressHelper address;
        address.SetBase("10.1.1.0", "255.255.255.0");
        m_interfaces = address.Assign(m_devices);


        for(uint32_t i=0;i<nServer;i++) {
            std::ostringstream os;
            os<<"tcp://" << m_interfaces.GetAddress(i) << ":" << 8444;
            NS_LOG_DEBUG("i:"<<i<<", node "<<m_nodes.Get(i)->GetId()<<"-"<<os.str());
            cluster.push_back(cs_new<srv_config>(m_nodes.Get(i)->GetId(),os.str()));
        }

        server_apps = InstallServer(m_servers);
        client_apps = InstallClient(m_clients);

//        ca_app = InstallCa(m_ca);
        server_apps.Start (Seconds (0.5));
        server_apps.Stop (Seconds (3.0));
        client_apps.Start (Seconds (2.3));
        client_apps.Stop (Seconds (2.8));
//        ca_app.Start (Seconds (1.0));
//        ca_app.Stop (Seconds (30.0));
        NS_LOG_INFO("Topology End.");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    }


    RaftTopologyHelper::~RaftTopologyHelper() {

    }

    NodeContainer RaftTopologyHelper::GetNodes() {
        return NodeContainer();
    }

    NetDeviceContainer RaftTopologyHelper::GetDevices() {
        return NetDeviceContainer();
    }

    Ipv4InterfaceContainer RaftTopologyHelper::GetInterfaces() {
        return Ipv4InterfaceContainer();
    }

    ApplicationContainer RaftTopologyHelper::InstallServer(NodeContainer c) {
        ApplicationContainer apps;
        for(auto i = c.Begin(); i!=c.End();++i) {
            apps.Add(InstallPrivServer(*i));
        }
        return apps;
    }

    Ptr<Application> RaftTopologyHelper::InstallPrivServer(Ptr<Node> node) {
        ObjectFactory m_factory;
        //先没有故障
        m_factory.SetTypeId("cornerstone::RaftNode");
        m_factory.Set("LocalAddress",AddressValue(m_interfaces.GetAddress(node->GetId())));
        Ptr<cornerstone::RaftNode> app = m_factory.Create<cornerstone::RaftNode>();
        for(uint32_t i = 0; i < m_servers.GetN(); i++)
        {
            if(i!=node->GetId())
                app->SetPeersAddresses(m_interfaces.GetAddress(i));
            else
                app->SetIpv4Address(m_interfaces.GetAddress(i));
        }
        app->SetCaAddress(m_interfaces.GetAddress(m_nodes.GetN() - 1));
        app->SetCluster(cluster);
        node->AddApplication (app);
        return app;
    }

    ApplicationContainer RaftTopologyHelper::InstallClient(NodeContainer c) {
        ApplicationContainer apps;
        for(auto i=c.Begin(); i!=c.End(); i++) {
            apps.Add(InstallPrivClient(*i));
        }
        return apps;
    }

    Ptr<Application> RaftTopologyHelper::InstallPrivClient(Ptr<Node> node) {
        ObjectFactory m_factory;
        //先没有故障
        m_factory.SetTypeId("cornerstone::ClientNode");
        m_factory.Set("RemoteServerAddress", AddressValue(m_interfaces.GetAddress(0)));
        m_factory.Set("RemoteServerPort",UintegerValue(8444));
        Ptr<cornerstone::ClientNode> app = m_factory.Create<cornerstone::ClientNode>();
        //记录所有Server地址以备故障时切换
        for(uint32_t i = 0; i < m_servers.GetN(); i++)
        {
            if(i!=node->GetId())
                app->SetPeersAddresses(m_interfaces.GetAddress(i));
            else
                app->SetIpv4Address(m_interfaces.GetAddress(i));
        }
        node->AddApplication(app);
        return app;
    }

    ApplicationContainer RaftTopologyHelper::InstallCa(NodeContainer c) {
        return ApplicationContainer();
    }

    Ptr<Application> RaftTopologyHelper::InstallPrivCa(Ptr<Node> node) {
        return Ptr<Application>();
    }
}