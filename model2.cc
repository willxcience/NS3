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
#define numberOfUeNodes 4
#define D 500

class UserEquipment
{
  public:
    uint32_t imsi;   //like an index
    uint32_t cellid; //which cellid
    uint32_t nodeid;
    double sinr; //SINR averaged among RBs

    void init(uint32_t imsi, uint32_t cellid, uint32_t nodeid)
    {
        this->imsi = imsi;
        this->cellid = cellid;
        this->nodeid = nodeid;
        this->sinr = 0;
    }
};

UserEquipment UEs[4];

uint16_t getNodeNumber(string context)
{
    uint32_t pos = context.find('/', 10);
    return atoi(context.substr(10, pos - 10).c_str());
}

//Customized controller
void handler(NetDeviceContainer enbLteDevs, NetDeviceContainer ueLteDevs)
{
    //Ptr<MobilityModel> mobility = ueLteDevs.Get(1)->GetNode()->GetObject<MobilityModel>();
    //Vector3D pos = mobility->GetPosition();

    Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(3)->GetObject<LteEnbNetDevice>()->GetPhy();
    double currTxPower = enbPhy->GetTxPower();

    //cout << std::setprecision(8) << Simulator::Now().GetSeconds() << "s" \
         << "Tx Power: " << currTxPower << endl;

    enbPhy->SetTxPower(90.0);

    //get the applications
    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(ueLteDevs.Get(0)->GetNode()->GetApplication(0));
    Ptr<PacketSink> sink2 = DynamicCast<PacketSink>(ueLteDevs.Get(1)->GetNode()->GetApplication(0));
    Ptr<PacketSink> sink3 = DynamicCast<PacketSink>(ueLteDevs.Get(2)->GetNode()->GetApplication(0));
    Ptr<PacketSink> sink4 = DynamicCast<PacketSink>(ueLteDevs.Get(3)->GetNode()->GetApplication(0));
    cout << sink1->GetTotalRx() << " "
         << sink2->GetTotalRx() << " "
         << sink3->GetTotalRx() << " "
         << sink4->GetTotalRx() << " " << endl;
    //report the SINR of IMSI 1

    /*
    cout << "IMSI: " << UEs[0].imsi << " SINR: " << UEs[0].sinr << endl;
    cout << "IMSI: " << UEs[1].imsi << " SINR: " << UEs[1].sinr << endl;
    cout << "IMSI: " << UEs[2].imsi << " SINR: " << UEs[2].sinr << endl;
    cout << "IMSI: " << UEs[3].imsi << " SINR: " << UEs[3].sinr << endl;
*/
    //schedule next control
    Simulator::Schedule(Seconds(10.0), &handler, enbLteDevs, ueLteDevs);
}

//Trace Sink
static void ReportCurrentCellRsrpSinr(string context, uint16_t cellId, uint16_t rnti, double rsrp, double sinr)
{
    int id = getNodeNumber(context) - 6;

    //record data
    UEs[id].sinr = sinr;
}

static void NotifyConnectionEstablishedUe(string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    cout << std::setprecision(8) << "Time " << Simulator::Now().GetSeconds()
         << " UE IMSI " << imsi
         << ": connected to CellId " << cellid
         << " with Node ID " << getNodeNumber(context)
         << endl;

    UEs[imsi - 1].init(imsi, cellid, getNodeNumber(context));
    cout << "UE:" << imsi << " initialized!" << endl;
}

//Main
int main(int argc, char *argv[])
{

    double simTime = 500;
    double distance = 500;
    double interPacketInterval = 10;

    //set TX Power of the enbs an the users
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(30.0));
    Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(10.0));
    
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisSpectrumPropagationLossModel"));
    lteHelper->SetHandoverAlgorithmType("ns3::NoOpHandoverAlgorithm");

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
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Mb/s")));
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
    ueNodes.Create(4); //4 user equipments

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
    uePositions->Add(Vector(D * 1.5, D / 2, 10));
    uePositions->Add(Vector(D / 2, D * 1.5, 10));
    uePositions->Add(Vector(1.5 * D, D * 1.5, 10));
    ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    ueMobility.SetPositionAllocator(uePositions);
    ueMobility.Install(ueNodes);

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

    // Install and start applications on UEs and remote host
    uint16_t dlPort = 1234;
    uint16_t ulPort = 2000;

    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {

        ++ulPort;

        PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));

        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));
        serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

        UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
        dlClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        dlClient.SetAttribute("MaxPackets", UintegerValue(1000000));

        UdpClientHelper ulClient(remoteHostAddr, ulPort);
        ulClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        ulClient.SetAttribute("MaxPackets", UintegerValue(1000000));

        clientApps.Add(dlClient.Install(remoteHost));
        clientApps.Add(ulClient.Install(ueNodes.Get(u)));
    }

    serverApps.Start(Seconds(0.01));
    clientApps.Start(Seconds(0.01));

    lteHelper->AddX2Interface(enbNodes);

      lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();

    //Custom Trace Sinks for measuring Handover
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteUePhy/ReportCurrentCellRsrpSinr",
                    MakeCallback(&ReportCurrentCellRsrpSinr));

    //Simulator schedules
    Simulator::Schedule(Seconds(1.0), &handler, enbLteDevs, ueLteDevs);

    Simulator::Stop(Seconds(100));

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
