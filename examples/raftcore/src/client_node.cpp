//
// Created by Hython on 2020/4/16.
//

#include "client_node.h"
#include "ns3all.h"
#include <ns3/uinteger.h>
#include <ns3/log.h>
#include <ns3/packet.h>
#include <ns3/socket.h>
#include <ns3/tcp-socket-factory.h>
#include <ns3/inet-socket-address.h>
#include <ns3/inet6-socket-address.h>
#include <ns3_service.h>
#include "cornerstone.hxx"
#include <sstream>

NS_LOG_COMPONENT_DEFINE("Raft_Client_Node");

namespace cornerstone {

    NS_OBJECT_ENSURE_REGISTERED (ClientNode);

    void ClientNode::DoDispose() {
        NS_LOG_FUNCTION (this);

        if (!Simulator::IsFinished ())
        {
            StopApplication ();
        }

        Application::DoDispose (); // Chain up.
    }

    void ClientNode::StartApplication() {
        NS_LOG_FUNCTION(this);
        if(m_state == NOT_STARTED) {
            OpenConnection();
        }  else
        {
            NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                             << " for StartApplication().");
        }

    }

    void ClientNode::StopApplication() {
        NS_LOG_FUNCTION(this);
        SwitchToState(STOPPED);
        m_socket->Close ();
        m_socket->SetConnectCallback (MakeNullCallback<void, Ptr<Socket> > (),
                                      MakeNullCallback<void, Ptr<Socket> > ());
        m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

    TypeId ClientNode::GetTypeId() {
        static TypeId tid = TypeId("cornerstone::ClientNode")
                .SetParent<Application> ()
                .AddConstructor<ClientNode> ()
                .AddAttribute ("RemoteServerAddress",
                               "The address of the destination server.",
                               AddressValue (),
                               MakeAddressAccessor (&ClientNode::m_remoteServerAddress),
                               MakeAddressChecker ())
                .AddAttribute ("RemoteServerPort",
                               "The destination port of the outbound packets.",
                               UintegerValue (8444), // the default HTTP port
                               MakeUintegerAccessor (&ClientNode::m_remoteServerPort),
                               MakeUintegerChecker<uint16_t> ());
        return tid;
    }

    void ClientNode::OpenConnection() {
        if(m_state==NOT_STARTED || m_state==READING) {
            m_socket = Socket::CreateSocket (GetNode (),
                                             TcpSocketFactory::GetTypeId ());
            int ret;
            SwitchToState (CONNECTING);
            if (Ipv4Address::IsMatchingType (m_remoteServerAddress))
            {
                ret = m_socket->Bind ();
                NS_LOG_DEBUG (this << " Bind() return value= " << ret
                                   << " GetErrNo= " << m_socket->GetErrno () << ".");

                Ipv4Address ipv4 = Ipv4Address::ConvertFrom (m_remoteServerAddress);
                InetSocketAddress inetSocket = InetSocketAddress (ipv4,
                                                                  m_remoteServerPort);
                NS_LOG_INFO (this << " Connecting to " << ipv4
                                  << " port " << m_remoteServerPort
                                  << " / " << inetSocket << ".");
                ret = m_socket->Connect (inetSocket);
                NS_LOG_DEBUG (this << " Connect() return value= " << ret
                                   << " GetErrNo= " << m_socket->GetErrno () << ".");
            }
            else if (Ipv6Address::IsMatchingType (m_remoteServerAddress))
            {
                ret = m_socket->Bind6 ();
                NS_LOG_DEBUG (this << " Bind6() return value= " << ret
                                   << " GetErrNo= " << m_socket->GetErrno () << ".");

                Ipv6Address ipv6 = Ipv6Address::ConvertFrom (m_remoteServerAddress);
                Inet6SocketAddress inet6Socket = Inet6SocketAddress (ipv6,
                                                                     m_remoteServerPort);
                NS_LOG_INFO (this << " connecting to " << ipv6
                                  << " port " << m_remoteServerPort
                                  << " / " << inet6Socket << ".");
                ret = m_socket->Connect (inet6Socket);
                NS_LOG_DEBUG (this << " Connect() return value= " << ret
                                   << " GetErrNo= " << m_socket->GetErrno () << ".");
            }

            NS_UNUSED (ret); // Mute compiler warning.
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
                    m_peersSockets[ipv4address]->Connect (InetSocketAddress (ipv4address, m_remoteServerPort));
                }
            }

            ns3Service = cs_new<cornerstone::ns3impls::ns3_service>(m_socket, &m_peersSockets);



            m_socket->SetConnectCallback(MakeCallback(&ClientNode::ConnectionSucceededCallback, this),
                    MakeCallback(&ClientNode::ConnectionFailedCallback, this));

        }else {
            NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                             << " for OpenConnection().");
        }
    }

    ClientNode::ClientNode():m_state(NOT_STARTED),m_socket(0) {

    }

    void ClientNode::ConnectionFailedCallback (Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION (this << socket);

        if (m_state == CONNECTING)
        {
        NS_LOG_ERROR ("Client failed to connect"
                              << " to remote address " << m_remoteServerAddress
                              << " port " << m_remoteServerPort << ".");
        }
        else
        {
        NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                         << " for ConnectionFailed().");
        }
    }


    std::string ClientNode::GetStateString(State_t state) {
        switch(state){
            case NOT_STARTED:
                return "NOT_STARTED";
            case READING:
                return "READING";
            case STOPPED:
                return "STOPPED";
            case CONNECTING:
                return "CONNECTING";
            default:
                NS_FATAL_ERROR ("Unknown state");
                return "FATAL_ERROR";
                break;
        }
    }

    void ClientNode::SwitchToState(ClientNode::State_t state) {
        const std::string oldState = GetStateString ();
        const std::string newState = GetStateString (state);
        NS_LOG_FUNCTION (this << oldState << newState);

        m_state = state;
        NS_LOG_INFO (this << " HttpClient " << oldState
                          << " --> " << newState << ".");

    }

    std::string ClientNode::GetStateString() {
        return GetStateString(m_state);
    }

    void ClientNode::ConnectionSucceededCallback(Ptr<Socket> socket) {
        if (m_state == CONNECTING)
        {
            NS_ASSERT_MSG (m_socket == socket, "Invalid socket.");
            //m_connectionEstablishedTrace (this);
            socket->SetRecvCallback (MakeCallback (&ClientNode::ReceivedDataCallback,
                                                   this));
            //NS_ASSERT (m_embeddedObjectsToBeRequested == 0);
            m_eventRequestMainObject = Simulator::ScheduleNow (
                    &ClientNode::RequestMainObject, this);
        }
        else
        {
            NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                             << " for ConnectionSucceeded().");
        }
    }

    void ClientNode::ReceivedDataCallback(Ptr<Socket> socket) {
        HandleRead(socket);
    }
    void ClientNode::HandleRead(ns3::Ptr<ns3::Socket> socket) {
        NS_LOG_INFO(socket);
        Ptr<Packet> packet;
        Address from;

        while(packet = socket->RecvFrom(from)) {
            if(packet->GetSize() == 0){
                break;
            }
            if(InetSocketAddress::IsMatchingType(from)) {
                std::string delimiter = "#";
                std::string parsedPacket;
                // [C++ 11] Use smart pointer to prevent memory leaks
                std::shared_ptr<char> packetInfo(new char[packet->GetSize() + 1],
                                                 [](char *p){delete []p;});
                std::ostringstream totalStream;
                packet->CopyData(reinterpret_cast<uint8_t *>(packetInfo.get()), packet->GetSize());
                packetInfo.get()[packet->GetSize()] = '\0';
                totalStream << m_bufferedData[from] << packetInfo.get();
                std::string totalReceivedData(totalStream.str());
                NS_LOG_INFO( " Total Received Data: "<< totalReceivedData);

                size_t pos = 0;
                while((pos = totalReceivedData.find(delimiter))!=std::string::npos) {
                    parsedPacket = totalReceivedData.substr(0, pos);
                    NS_LOG_INFO("Parsed Packet: " << parsedPacket);

                    rapidjson::Document d;
                    d.Parse(parsedPacket.c_str());

                    if(!d.IsObject()){
                        NS_LOG_WARN("The parsed packet is corrupted");
                        totalReceivedData.erase(0, pos + delimiter.length());
                        continue;
                    }
                    NS_LOG_INFO("The data from the server: "<<parsedPacket);

                    totalReceivedData.erase(0,pos+delimiter.length());
                }
                m_bufferedData[from] = totalReceivedData;

            }
        }
    }


    void ClientNode::RequestMainObject() {
        //问题主要是Leader可能会变，如何通知客户节点
        //现在的策略是在raft_server内核里转发消息给领导者节点
        //这样的问题是如果指定的这一个服务器宕机，那客户与集群的连接就丢失了

            std::ostringstream os;
            Ipv4Address ipv4 = Ipv4Address::ConvertFrom (m_peersAddresses[0]);
            os<<"tcp://"<<ipv4<<":"<<m_remoteServerPort;
            NS_LOG_DEBUG("TCP ADDR)"<<os.str());
            std::string tcp_addr = os.str();
            ptr<rpc_client> client= ns3Service->create_client(os.str());
            ptr<req_msg> msg = cs_new<req_msg>(0, msg_type::client_request, 0, 1, 0, 0, 0);

            std::ostringstream echo_mes;
            echo_mes<<"the world is beautiful with";
            std::string str = echo_mes.str();
            NS_LOG_DEBUG("THE Original BUF SIZE:"<<str.size());
            bufptr buf = buffer::alloc(str.size());
            for(int i=0;i<str.size();i++){
                buf->put((byte)str[i]);
            }
            //buf->put(str);
            buf->pos(0);
            msg->log_entries().push_back(cs_new<log_entry>(0, std::move(buf)));

            // The handler will not be called
            //auto self = this->shared_from_this();
            rpc_handler handler = (rpc_handler)([&client](ptr<resp_msg>& rsp, const ptr<rpc_exception>& err) -> void {
                NS_LOG_DEBUG("The client handler was called");
                if(!rsp->get_accepted()){

                    NS_LOG_ERROR("The client's request was not accepted by the server");

                }else{

                    NS_LOG_INFO("The client's request has been handled successfully");

                }
            });

            clients_rpc_.insert({client,handler});

            client->send(msg, handler);



    }

    void ClientNode::SetPeersAddresses(Ipv4Address ipv4Address) {

            if (!std::count(m_peersAddresses.begin(),m_peersAddresses.end(),ipv4Address))
            {
                m_peersAddresses.push_back(ipv4Address);
            }

    }

    void ClientNode::SetIpv4Address(Ipv4Address ipv4Address) {
        m_ipv4Address = ipv4Address;
    }
}