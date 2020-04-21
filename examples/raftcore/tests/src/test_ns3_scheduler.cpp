//
// Created by Hython on 2020/3/13.
//

#include "cornerstone.hxx"
//#include "ns3_service.h"
#include <iostream>

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace cornerstone;
int _timer_fired_counter = 0;

NS_LOG_COMPONENT_DEFINE ("test_ns3_scheduler");

//class ns3_service_test {
//public:
//    void schedule(ptr<delayed_task> &task, int32 milliseconds) {
//        task->reset();
//        ns3::Simulator::Schedule(ns3::MilliSeconds(milliseconds),&delayed_task::execute, task);
//    }
//};



using namespace ns3;


class MyApp : public Application
{
public:

    MyApp ();
    virtual ~MyApp();

    void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, ptr<delayed_task> &t);


private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void ScheduleTx (void);
    void SendPacket (void);

    void Timer_handler_(ptr<delayed_task> &task) {
        task->execute();
    }

    Ptr<Socket>     m_socket;
    Address         m_peer;
    uint32_t        m_packetSize;
    uint32_t        m_nPackets;
    DataRate        m_dataRate;
    EventId         m_sendEvent;
    bool            m_running;
    uint32_t        m_packetsSent;
    ptr<delayed_task> task;
};

MyApp::MyApp ()
        : m_socket (0),
          m_peer (),
          m_packetSize (0),
          m_nPackets (0),
          m_dataRate (0),
          m_sendEvent (),
          m_running (false),
          m_packetsSent (0),
          task(0)
{
}

MyApp::~MyApp()
{
    m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, ptr<delayed_task> &t)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
    task = t;
}

void
MyApp::StartApplication (void)
{
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind ();
    m_socket->Connect (m_peer);
    SendPacket ();
}

void
MyApp::StopApplication (void)
{
    m_running = false;

    if (m_sendEvent.IsRunning ())
    {
        Simulator::Cancel (m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
    Ptr<Packet> packet = Create<Packet> (m_packetSize);
    m_socket->Send (packet);

    if (++m_packetsSent < m_nPackets)
    {
        ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
    if (m_running)
    {
        //Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
        m_sendEvent = Simulator::Schedule (Seconds(1), &MyApp::SendPacket, this);
        Simulator::Schedule(Seconds(1),&MyApp::Timer_handler_, this, task);
    }
}

static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
    NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
}

static void
RxDrop (Ptr<const Packet> p)
{
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}

int
main (int argc, char *argv[])
{
    CommandLine cmd;
    cmd.Parse (argc, argv);

    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install (nodes);

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
    devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    uint16_t sinkPort = 8080;
    Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
    sinkApps.Start (Seconds (0.));
    sinkApps.Stop (Seconds (20.));

    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
    //ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));

    timer_task<void>::executor handler = (std::function<void()>)([]() -> void {
        _timer_fired_counter++;
        std::cout<<"counter: "<<_timer_fired_counter<<std::endl;
    });
    timer_task<void>::executor handler2 = (std::function<void()>)([]() -> void {
        std::cout<<"counter: "<<_timer_fired_counter<<std::endl;
    });
    ptr<delayed_task> task(cs_new<timer_task<void>>(handler));
    ptr<delayed_task> task2(cs_new<timer_task<void>>(handler2));


    Ptr<MyApp> app = CreateObject<MyApp> ();
    //Ptr<MyApp> app(new MyApp(task));
    app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000, DataRate ("1Mbps"), task);
    nodes.Get (0)->AddApplication (app);
    app->SetStartTime (Seconds (1.));
    app->SetStopTime (Seconds (20.));

    devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));

    Simulator::Stop (Seconds (20));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}



//int test_ns3_scheduler() {
//
//    ns3_service_test svc;
//    timer_task<void>::executor handler = (std::function<void()>)([]() -> void {
//        _timer_fired_counter++;
//    });
//    timer_task<void>::executor handler2 = (std::function<void()>)([]() -> void {
//        std::cout<<"counter: "<<_timer_fired_counter<<std::endl;
//    });
//    ptr<delayed_task> task(cs_new<timer_task<void>>(handler));
//    ptr<delayed_task> task2(cs_new<timer_task<void>>(handler2));
//    std::cout << "scheduled to wait for 200ms" << std::endl;
//    svc.schedule(task, 200);
//    svc.schedule(task2, 250);
//
//    std::cout << "scheduled to wait for 300ms" << std::endl;
//    svc.schedule(task, 300);
//    svc.schedule(task2, 350);
//
//
//    std::cout << "scheduled to wait for 500ms" << std::endl;
//    svc.schedule(task, 500);
//    svc.schedule(task2,550);
//
//    //svc.stop();
//
//
//    NS_LOG_INFO ("Run Simulation.");
//    ns3::Simulator::Run ();
//
//
//    ns3::Simulator::Destroy ();
//    NS_LOG_INFO ("Done.");
//
//
//    return 0;
//}
