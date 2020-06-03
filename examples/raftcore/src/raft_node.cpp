//
// Created by Hython on 2020/3/16.
//

#include "raft_node.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "example_common.h"
#include <sstream>

NS_LOG_COMPONENT_DEFINE("Raft_Server_Node");

namespace cornerstone {

    NS_OBJECT_ENSURE_REGISTERED (RaftNode);


    void ns3_event_listener::on_event(raft_event event) {
        switch (event)
        {
            case raft_event::become_follower:
                std::cout << Simulator::Now()<<"; " <<srv_id_ << " becomes a follower" << std::endl;
                break;
            case raft_event::become_leader:
                std::cout << Simulator::Now()<<"; " << srv_id_ << " becomes a leader" << std::endl;
                break;
            case raft_event::logs_catch_up:
                std::cout << Simulator::Now()<<"; " << srv_id_ << " catch up all logs" << std::endl;
                break;
        }
    }

    class ns3_logger: public logger {
    public:
        ns3_logger() = default;
        virtual void debug(const std::string& log_line){
            NS_LOG_DEBUG(log_line);
        }
        virtual void info(const std::string& log_line){
            NS_LOG_INFO(log_line);
        }
        virtual void warn(const std::string& log_line){
            NS_LOG_WARN(log_line);
        }
        virtual void err(const std::string& log_line){
            NS_LOG_ERROR(log_line);
        }
    };


    TypeId RaftNode::GetTypeId() {
        static TypeId tid = TypeId("cornerstone::RaftNode")
                .SetParent<Application>()
                .AddConstructor<RaftNode>()
                .AddAttribute("LocalAddress",
                        "The local address of the server; the address to bind the Rx socket",
                        AddressValue(),
                        MakeAddressAccessor(&RaftNode::m_localAddress),
                        MakeAddressChecker())
                .AddAttribute("LocalPort",
                        "Port on which the application listen for incoming packets.",
                        UintegerValue(8444),
                        MakeUintegerAccessor(&RaftNode::m_raftPort),
                        MakeUintegerChecker<uint16_t>())
                .AddAttribute ("Mtu",
                               "Maximum transmission unit (in bytes) of the TCP sockets "
                               "used in this application, excluding the compulsory 40 "
                               "bytes TCP header. Typical values are 1460 and 536 bytes. "
                               "The attribute is read-only because the value is randomly "
                               "determined.",
                               TypeId::ATTR_GET,
                               UintegerValue (536),
                               MakeUintegerAccessor (&RaftNode::m_mtuSize),
                               MakeUintegerChecker<uint32_t> ())
                        ;
        return tid;
    }

    RaftNode::RaftNode(){

    }

    RaftNode::~RaftNode() {

    }

    void RaftNode::SetPeersAddresses(Ipv4Address ipv4Address) {
        if (!std::count(m_peersAddresses.begin(),m_peersAddresses.end(),ipv4Address))
        {
            m_peersAddresses.push_back(ipv4Address);
        }
    }

    void RaftNode::SetIpv4Address(Ipv4Address ipv4Address) {
        m_ipv4Address = ipv4Address;
    }

    void RaftNode::SetCaAddress(Ipv4Address ipv4Address) {
        m_caAddress = ipv4Address;
    }

    void RaftNode::SetNumber(int total) {
        total_nodes = total;
    }

    void RaftNode::DoDispose(void) {
        NS_LOG_FUNCTION (this);

        if (!Simulator::IsFinished ())
        {
            StopApplication ();
        }

        Application::DoDispose (); // Chain up.
    }

    void RaftNode::StartApplication() {
        NS_LOG_FUNCTION(this);
        NS_LOG_INFO("Node " << GetNode()->GetId() << " starting...");
        replica_id_ = GetNode()->GetId();
        int ret;
        if(!m_socket) {
            m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
            if (Ipv4Address::IsMatchingType (m_localAddress)) {
                const Ipv4Address ipv4 = Ipv4Address::ConvertFrom (m_localAddress);
                const InetSocketAddress inetSocket = InetSocketAddress (ipv4, m_raftPort);
                NS_LOG_INFO (this << " Binding on " << ipv4
                                  << " port " << m_raftPort
                                  << " / " << inetSocket << ".");
                ret = m_socket->Bind (inetSocket);
                NS_LOG_DEBUG (this << " Bind() return value= " << ret
                                   << " GetErrNo= "
                                   << m_socket->GetErrno () << ".");
            }else if (Ipv6Address::IsMatchingType(m_localAddress)) {
                const Ipv6Address ipv6 = Ipv6Address::ConvertFrom (m_localAddress);
                const Inet6SocketAddress inet6Socket = Inet6SocketAddress (ipv6,
                                                                           m_raftPort);
                NS_LOG_INFO (this << " Binding on " << ipv6
                                  << " port " << m_raftPort
                                  << " / " << inet6Socket << ".");
                ret = m_socket->Bind (inet6Socket);
                NS_LOG_DEBUG (this << " Bind() return value= " << ret
                                   << " GetErrNo= "
                                   << m_socket->GetErrno () << ".");
            }

            ret = m_socket->Listen();
            NS_LOG_DEBUG (this << " Listen () return value= " << ret
                               << " GetErrNo= " << m_socket->GetErrno ()
                               << ".");
            NS_UNUSED (ret);
        }

        NS_ASSERT_MSG (m_socket != 0, "Failed creating socket.");
        for(uint32_t i = 0; i < m_peersAddresses.size(); i++)
        {

            Ipv4Address ipv4address = m_peersAddresses[i];
            NS_LOG_DEBUG("peer["<<i<<"] ip:"<<ipv4address);
            std::map<Ipv4Address, Ptr<Socket>>::iterator it = m_peersSockets.find(ipv4address);
            if (it == m_peersSockets.end()) //Create the socket if it doesn't exist
            {
                m_peersSockets[ipv4address] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
                //Should I connect it here?
                m_peersSockets[ipv4address]->Connect (InetSocketAddress (ipv4address, m_raftPort));
            }
        }
//        m_caSocket = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
//        m_caSocket->Connect (InetSocketAddress (m_caAddress, m_raftPort));
        int lowBound, highBound;
        if(GetNode()->GetId()<3){
            lowBound=4;
            highBound=8;
        }else{
            lowBound = 10;
            highBound =20;
        }

        ns3Service = cs_new<cornerstone::ns3impls::ns3_service>(m_socket, &m_peersSockets);
        ptr<logger> l(cs_new<ns3_logger>());
        ptr<rpc_listener> listener(ns3Service->create_rpc_listener(m_raftPort,l));
        ptr<delayed_task_scheduler> scheduler =ns3Service;
        ptr<rpc_client_factory> rpc_cli_factory = ns3Service;
        raft_params* params(new raft_params());
        (*params).with_election_timeout_lower(lowBound)
                .with_election_timeout_upper(highBound)
                .with_hb_interval(100)
                .with_max_append_size(200)
                .with_rpc_failure_backoff(50)
                .with_prevote_enabled(false);
        ptr<state_machine> smachine(cs_new<echo_state_machine>());
        ptr<state_mgr> smgr(cs_new<simple_state_mgr>(GetNode()->GetId(),cluster_));
        context* ctx(new context(smgr, smachine, listener, l, rpc_cli_factory, scheduler, cs_new<ns3_event_listener>(GetNode()->GetId()), params));
        ptr<raft_server> server(cs_new<raft_server>(ctx));
        listener->listen(server);
        this->server = server;
    }

    void RaftNode::StopApplication() {
        NS_LOG_FUNCTION (this);
        NS_LOG_INFO("Node "<<GetNode()->GetId()<<" stoping...");
        for (std::vector<Ipv4Address>::iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i) //close the outgoing sockets
        {
            m_peersSockets[*i]->Close ();
        }
        if (m_socket)
        {
            m_socket->Close ();
            m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
        }
    }

    void RaftNode::SetCluster(std::vector<ptr<srv_config>> cluster) {
        cluster_ = cluster;
    }
}