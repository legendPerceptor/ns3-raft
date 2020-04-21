//
// Created by Hython on 2020/3/16.
//

#ifndef NS3_RAFT_NODE_H
#define NS3_RAFT_NODE_H
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/socket.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "ptr.hxx"
#include "resp_msg.hxx"
#include "req_msg.hxx"
#include "basic_types.hxx"
#include "cornerstone.hxx"
#include <vector>
#include <map>
#include <string>
#include "ns3_service.h"
using namespace ns3;

namespace cornerstone {

    class RaftNode : public Application {
    public:
        static TypeId GetTypeId();

        RaftNode();

        virtual ~RaftNode();

        void SetCluster(std::vector<ptr<srv_config>> cluster);

        void SetPeersAddresses(Ipv4Address ipv4Address);

        void SetIpv4Address(Ipv4Address ipv4Address);

        void SetCaAddress(Ipv4Address ipv4Address);

        void SetNumber(int total);

    protected:
        virtual void DoDispose(void);

        virtual void StartApplication();

        virtual void StopApplication();

        friend raft_server;
        ptr<raft_server> server;
        ptr<cornerstone::ns3impls::ns3_service> ns3Service;
        Ptr<Socket> m_socket;
        Ipv4Address m_ipv4Address;
        std::vector<Ipv4Address> m_peersAddresses; // The ipv4 addresses of peers
        std::map<Ipv4Address, Ptr<Socket>> m_peersSockets; // The sockets of peers
        std::map<std::string, Ptr<Socket>> m_clientsSockets; // The sockets of clients: ipv4 => socket
        std::map<Address, std::string> m_bufferedData;
        std::vector<ptr<srv_config>> cluster_;
        Ipv4Address m_caAddress;
        Ptr<Socket> m_caSocket;

        uint16_t m_raftPort;
        uint32_t m_mtuSize;
        Address m_localAddress;

        int replica_id_;
        int total_nodes;
    };

}
#endif //NS3_RAFT_NODE_H
