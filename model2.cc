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
    double rsrp;

    void init(uint32_t imsi, uint32_t cellid, uint32_t nodeid)
    {
        this->imsi = imsi;
        this->cellid = cellid;
        this->nodeid = nodeid;
        this->sinr = 0;
        this->rsrp = 0;
    }
};

UserEquipment UEs[4];

uint16_t getNodeNumber(string context)
{
    uint32_t pos = context.find('/', 10);
    return atoi(context.substr(10, pos - 10).c_str());
}

//Customized controller
void handler2(NetDeviceContainer enbLteDevs, NetDeviceContainer ueLteDevs)
{
    Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy();
    double currTxPower = enbPhy->GetTxPower();

    //cout << std::setprecision(8) << Simulator::Now().GetSeconds() << "s" \
         << "Tx Power: " << currTxPower << endl;
    enbPhy->SetTxPower(30);
}
void handler(NetDeviceContainer enbLteDevs, NetDeviceContainer ueLteDevs)
{
    //Ptr<MobilityModel> mobility = ueLteDevs.Get(1)->GetNode()->GetObject<MobilityModel>();
    //Vector3D pos = mobility->GetPosition();

    //from 0 to 3
    Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy();
    double currTxPower = enbPhy->GetTxPower();

    //cout << std::setprecision(8) << Simulator::Now().GetSeconds() << "s" \
         << "Tx Power: " << currTxPower << endl;

    cout << enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy()->GetTxPower() << "db" << endl;
    //enbPhy->SetTxPower(20.0);

    //get the applications
    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(ueLteDevs.Get(0)->GetNode()->GetApplication(0));
    Ptr<PacketSink> sink2 = DynamicCast<PacketSink>(ueLteDevs.Get(1)->GetNode()->GetApplication(0));
    Ptr<PacketSink> sink3 = DynamicCast<PacketSink>(ueLteDevs.Get(2)->GetNode()->GetApplication(0));
    Ptr<PacketSink> sink4 = DynamicCast<PacketSink>(ueLteDevs.Get(3)->GetNode()->GetApplication(0));

    cout << "Received bytes: "
         << sink1->GetTotalRx() << " "
         << sink2->GetTotalRx() << " "
         << sink3->GetTotalRx() << " "
         << sink4->GetTotalRx() << " " << endl;
    /**/
    //report the SINR of IMSI 1

    cout << "IMSI: " << UEs[0].imsi << " SINR: " << UEs[0].sinr << " RSRP: " << UEs[0].rsrp << endl;
    cout << "IMSI: " << UEs[1].imsi << " SINR: " << UEs[1].sinr << " RSRP: " << UEs[1].rsrp << endl;
    cout << "IMSI: " << UEs[2].imsi << " SINR: " << UEs[2].sinr << " RSRP: " << UEs[2].rsrp << endl;
    cout << "IMSI: " << UEs[3].imsi << " SINR: " << UEs[3].sinr << " RSRP: " << UEs[3].rsrp << endl;

    /* */

    //schedule next control
    Simulator::Schedule(Seconds(10.0), &handler, enbLteDevs, ueLteDevs);
    cout << endl;
}

//Trace Sink
static void ReportCurrentCellRsrpSinr(string context, uint16_t cellId, uint16_t rnti, double rsrp, double sinr)
{
    int id = getNodeNumber(context) - 6;

    //record data
    UEs[id].sinr = sinr;
    UEs[id].rsrp = rsrp;
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

    //CtrlErrorModel and DataErrorModel caused the randomness
    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue(false));

    //Data Error Model is most related to the interference
    Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue(false));
    
    
    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(false));
    //Config::SetDefault("ns3::LteHelper::UsePdschForCqiGeneration", BooleanValue(true));

    //Config::SetDefault("ns3::LteAmc::AmcModel", EnumValue(LteAmc::PiroEW2010));
    //Config::SetDefault("ns3::LteAmc::Ber", DoubleValue(0.00005));
    //GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    Config::SetDefault("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue(false));

    //set TX Power of the enbs an the users
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(20.0));
    Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(10.0));

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    //lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
    lteHelper->SetHandoverAlgorithmType("ns3::NoOpHandoverAlgorithm");

    // NOTE: the PropagationLoss trace source of the SpectrumChannel
    // works only for single-frequency path loss model.
    // e.g., it will work with the following models:
    // ns3::FriisPropagationLossModel,
    // ns3::TwoRayGroundPropagationLossModel,
    // ns3::LogDistancePropagationLossModel,
    // ns3::ThreeLogDistancePropagationLossModel,
    // ns3::NakagamiPropagationLossModel
    // ns3::BuildingsPropagationLossModel
    // etc.
    // but it WON'T work if you ONLY use SpectrumPropagationLossModels such as:
    // ns3::FriisSpectrumPropagationLossModel
    // ns3::ConstantSpectrumPropagationLossModel
    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::LogDistancePropagationLossModel")); //Trace Fading ?

    //lteHelper->SetAttribute("FadingModel", StringValue("ns3::TraceFadingLossModel"));
    //lteHelper->SetFadingModelAttribute("TraceFilename", StringValue(fadingTrace));

    cout << lteHelper->GetFfrAlgorithmType () << endl;


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
    uePositions->Add(Vector(D - 100, D - 100, 10));
    uePositions->Add(Vector(D + 100, D - 100, 10));
    uePositions->Add(Vector(D - 100, D + 100, 10));
    uePositions->Add(Vector(D + 100, D + 100, 10));
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
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        ++ulPort;
        ApplicationContainer clientApps;
        ApplicationContainer serverApps;

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

        serverApps.Start(Seconds(0.01));
        clientApps.Start(Seconds(0.01));
    }

    lteHelper->AddX2Interface(enbNodes);

    lteHelper->EnableMacTraces();
    lteHelper->EnableRlcTraces();

    //Custom Trace Sinks for measuring Handover
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteUePhy/ReportCurrentCellRsrpSinr",
                    MakeCallback(&ReportCurrentCellRsrpSinr));

    //Simulator schedules
    Simulator::Schedule(Seconds(1.0), &handler, enbLteDevs, ueLteDevs);
    Simulator::Schedule(Seconds(5.0), &handler2, enbLteDevs, ueLteDevs);

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
