#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"

#include <iostream>
#include <iomanip>
#include <string>
//#include "ns3/gtk-config-store.h"

using namespace ns3;
using namespace std;

#define numberOfEnbNodes 4
#define numberOfUeNodes 1
#define D 500

//Customized controller
void handler(NetDeviceContainer enbLteDevs, NetDeviceContainer ueLteDevs)
{
    Ptr<MobilityModel> mobility = ueLteDevs.Get(0)->GetNode()->GetObject<MobilityModel>();
    Vector3D pos = mobility->GetPosition();

    //cout << std::setprecision(8) << Simulator::Now().GetSeconds() << "s" \
         << " X: " << pos.x << " Y: " << pos.y << endl;

    //schedule next control
    Simulator::Schedule(Seconds(10.0), &handler, enbLteDevs, ueLteDevs);
}

double handoverTrigger = 256;
Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

static void NotifyConnectionEstablishedUe(string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    cout << std::setprecision(8) << "Time " << Simulator::Now().GetSeconds()
         << " UE IMSI " << imsi
         << ": connected to CellId " << cellid
         << " with RNTI " << rnti << "\r\n"
         << endl;
}

static void NotifyHandoverStartUe(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti, uint16_t targetCellId)
{
    cout << std::setprecision(8) << "Time " << Simulator::Now().GetSeconds() << "s "
         << " From " << cellid << " To " << targetCellId << endl;
}

static void
NotifyHandoverEndOkUe(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    cout << "Handover Success!" << endl;

    handoverTrigger = 10;
    lteHelper->SetHandoverAlgorithmAttribute("Hysteresis",
                                             DoubleValue(1.0));
    lteHelper->SetHandoverAlgorithmAttribute("TimeToTrigger",
                        TimeValue(MilliSeconds(handoverTrigger)));

}

int main(int argc, char *argv[])
{

    double simTime = 500;
    double distance = 500;
    double interPacketInterval = 10;

    //set the maxium number of users
    Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(160));

    
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");

    //set up Strongest cell Handover
    //Details: https://www.nsnam.org/docs/models/html/lte-user.html#sec-automatic-handover
    lteHelper->SetHandoverAlgorithmType("ns3::A3RsrpHandoverAlgorithm");
    lteHelper->SetHandoverAlgorithmAttribute("Hysteresis",
                                             DoubleValue(3.0));
    lteHelper->SetHandoverAlgorithmAttribute("TimeToTrigger",
                                             TimeValue(MilliSeconds(handoverTrigger)));

    //Trace Fading ?
    //lteHelper->SetAttribute("FadingModel", StringValue("ns3::TraceFadingLossModel"));
    //lteHelper->SetFadingModelAttribute("TraceFilename", StringValue(fadingTrace));

    /* EPC Settings */
    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);

    // interface 0 is localhost, 1 is the p2p device
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    /* EPC END */

    /***********Mobility***********/
    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(4);
    ueNodes.Create(1);

    // Install Mobility Model in eNB
    Ptr<ListPositionAllocator> enbPositions = CreateObject<ListPositionAllocator>();
    //Quad 1
    enbPositions->Add(Vector(D / 2, D / 2, 10));
    enbPositions->Add(Vector(D * 1.5, D / 2, 10));
    enbPositions->Add(Vector(D / 2, D * 1.5, 10));
    enbPositions->Add(Vector(1.5 * D, D * 1.5, 10));
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(enbPositions);
    enbMobility.Install(enbNodes);

    //Set the user equipments positions
    MobilityHelper ueMobility;
    Ptr<ListPositionAllocator> uePositions = CreateObject<ListPositionAllocator>();
    uePositions->Add(Vector(D / 2, D / 2, 10));
    ueMobility.SetPositionAllocator(uePositions);
    ueMobility.SetMobilityModel("ns3::WaypointMobilityModel");
    ueMobility.Install(ueNodes);

    Ptr<WaypointMobilityModel> waypoints =
        DynamicCast<WaypointMobilityModel>(ueNodes.Get(0)->GetObject<MobilityModel>());

    uint16_t n = 1;
    waypoints->AddWaypoint(Waypoint(Seconds((n++) * 50), Vector(250, 250, 0)));
    waypoints->AddWaypoint(Waypoint(Seconds((n++) * 50), Vector(250, 750, 0)));
    waypoints->AddWaypoint(Waypoint(Seconds((n++) * 50), Vector(750, 750, 0)));
    waypoints->AddWaypoint(Waypoint(Seconds((n++) * 50), Vector(750, 250, 0)));
    waypoints->AddWaypoint(Waypoint(Seconds((n++) * 50), Vector(250, 250, 0)));
    waypoints->AddWaypoint(Waypoint(Seconds((n++) * 50), Vector(250, 750, 0)));
    waypoints->AddWaypoint(Waypoint(Seconds((n++) * 50), Vector(750, 750, 0)));
    waypoints->AddWaypoint(Waypoint(Seconds((n++) * 50), Vector(750, 250, 0)));
    waypoints->AddWaypoint(Waypoint(Seconds((n++) * 50), Vector(250, 250, 0)));

    //Set RandomWalk2dModel
    //ueMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", \
                                "Speed", StringValue("ns3::NormalRandomVariable[Mean=40|Variance=80|Bound=200]"), \
                                "Bounds", StringValue("0|1000|0|1000"));

    /***Next Part***/
    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    /***Automatic attachment***/
    lteHelper->Attach(ueLteDevs);
    
    // randomize a bit start times to avoid simulation artifacts
    // (e.g., buffer overflows due to packet transmissions happening
    // exactly at the same time)
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.010));

    // Install and start applications on UEs and remote host
    uint16_t dlPort = 1234;
    uint16_t ulPort = 2000;

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {

        ApplicationContainer clientApps;
        ApplicationContainer serverApps;

        ++ulPort;

        PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));

        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));
        serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

        UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
        dlClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        dlClient.SetAttribute("MaxPackets", UintegerValue(1000));

        UdpClientHelper ulClient(remoteHostAddr, ulPort);
        ulClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        ulClient.SetAttribute("MaxPackets", UintegerValue(1000));

        clientApps.Add(dlClient.Install(remoteHost));
        clientApps.Add(ulClient.Install(ueNodes.Get(u)));

        Time startTime = Seconds(startTimeSeconds->GetValue());
        serverApps.Start(Seconds(startTime));
        clientApps.Start(Seconds(startTime));
    }

    lteHelper->AddX2Interface(enbNodes);

    //Custom Trace Sinks for measuring Handover
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));

    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart", \
                    MakeCallback(&NotifyHandoverStartUe));

    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                    MakeCallback(&NotifyHandoverEndOkUe));

    //Simulator schedules
    Simulator::Schedule(Seconds(0.0), &handler, enbLteDevs, ueLteDevs);

    Simulator::Stop(Seconds(simTime));

    //set up a timer to measure the performance

    cout << "Starting Simulation" << endl;

    //time the simulation
    clock_t start = clock();
    Simulator::Run();
    clock_t end = clock();
    Simulator::Destroy();

    double time = (double)(end - start) / CLOCKS_PER_SEC;
    cout << simTime << "s took: " << time << "s to finish." << endl;

    return 0;
}
