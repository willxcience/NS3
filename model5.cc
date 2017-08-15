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

#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>

#ifdef LINUX
#include <Python.h>
#else
#include <Python/Python.h>
#endif

#define D 5000
#define OFFSET 200
#define numUes 4

using namespace ns3;
using namespace std;

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

//Global Variables
PyObject *pModule, *pFunc;
UserEquipment UEs[1];

uint16_t getNodeNumber(string context)
{
    uint32_t pos = context.find('/', 10);
    return atoi(context.substr(10, pos - 10).c_str());
}

int counter = 0;
double data[5000] = {0};

void handler(NetDeviceContainer enbLteDevs, NetDeviceContainer ueLteDevs)
{
    Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy();
    double currTxPower = enbPhy->GetTxPower();

    PyObject *pArgs, *pValue;
    //pArgs = PyTuple_New(1);
    //PyTuple_SetItem(pArgs, 0, PyInt_FromLong(currTxPower));

    /* call */
    //pValue = PyObject_CallObject(pFunc, pArgs);

    //int res = PyInt_AsLong(pValue);

    //if (currTxPower != res
    //enbPhy->SetTxPower(40);

     cout << "IMSI: " << UEs[0].imsi << " SINR: " << UEs[0].sinr << " RSRP: " << UEs[0].rsrp << endl;

    if (counter < 5000)
    {
        cout << counter << endl;
        data[counter++] = UEs[0].sinr;
    }

    //schedule next control
    Simulator::Schedule(Seconds(0.1), &handler, enbLteDevs, ueLteDevs);
    //cout << endl;
}

//Trace Sink
static void ReportCurrentCellRsrpSinr(string context, uint16_t cellId, uint16_t rnti, double rsrp, double sinr)
{
    int id = getNodeNumber(context) - 3;

    //cout << "CellId " << cellId << " SINR: " << id << endl;
    //record data

    //cout << id << endl;
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

void pythonInit(char *argv)
{
    Py_SetProgramName(argv); /* optional but recommended */
    Py_Initialize();

    if (!Py_IsInitialized())
    {
        printf("init error\n");
        exit(-1);
    }

    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");
    /* import */
    pModule = PyImport_ImportModule("controller");

    if (!pModule)
    {
        printf("Cant open python file!\n");
        exit(-2);
    }

    pFunc = PyObject_GetAttrString(pModule, "control");
    cout << "loaded" << endl;
}

int main(int argc, char *argv[])
{
    //Initialize python
    pythonInit(argv[0]);

    double simTime = 500.0;
    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    //CtrlErrorModel and DataErrorModel caused the randomness
    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue(false));

    //Data Error Model is most related to the interference
    Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue(true));

    //Radio link
    Config::SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue(ns3::LteEnbRrc::RLC_UM_ALWAYS));
    Config::SetDefault("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue(false));

    //use PDCCH
    Config::SetDefault("ns3::LteHelper::UsePdschForCqiGeneration", BooleanValue(false));

    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(30.0));
    Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(10.0));

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    //this is set by default
    //lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisSpectrumPropagationLossModel"));
    lteHelper->SetHandoverAlgorithmType("ns3::NoOpHandoverAlgorithm");

    lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
    lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue ("fading_trace_ETU_3kmph.fad"));

    //lteHelper->SetFadingModelAttribute("TraceFilename", StringValue("fading_trace_EPA_3kmph.fad"));
    lteHelper->SetFadingModelAttribute("TraceLength", TimeValue(Seconds(10.0)));
    lteHelper->SetFadingModelAttribute("SamplesNum", UintegerValue(10000));
    lteHelper->SetFadingModelAttribute("WindowSize", TimeValue(Seconds(0.5)));
    lteHelper->SetFadingModelAttribute("RbNum", UintegerValue(100));
    

    /***********Internet***********/

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the internet
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

    /***********Mobility***********/
    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(1);
    ueNodes.Create(1); //4 user equipments

    // Install Mobility Model in eNB
    Ptr<ListPositionAllocator> enbPositions = CreateObject<ListPositionAllocator>();
    //Quad 1
    enbPositions->Add(Vector(0, 0, 10));
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(enbPositions);
    enbMobility.Install(enbNodes);

    //Set the user equipments positions
    MobilityHelper ueMobility;
    Ptr<ListPositionAllocator> uePositions = CreateObject<ListPositionAllocator>();
    uePositions->Add(Vector(100, 0, 10));
    ueMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");

    ueMobility.SetPositionAllocator(uePositions);
    ueMobility.Install(ueNodes);

    ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(1.0, 0.0, 0.0));

    /***Next Part***/
    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // we install the IP stack on the UEs
    internet.Install(ueNodes);

    Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    // assign IP address to UEs
    for (uint32_t u = 0; u < ueNodes.GetN(); u++)
    {
        Ptr<Node> ue = ueNodes.Get(u);

        // set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    lteHelper->Attach(ueLteDevs);

    Ptr<EpcTft> tft = Create<EpcTft>();
    //types of epsbearer
    lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(0), EpsBearer(EpsBearer::GBR_CONV_VOICE), tft);

    //install a downlink app, uplink is not considered here
    uint16_t dlPort = 1234;
    uint16_t ulPort = 2000;
    for (uint32_t u = 0; u < ueNodes.GetN(); u++)
    {
        ulPort++;
        Ptr<Node> ue = ueNodes.Get(u);
        PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
                                          InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        ApplicationContainer serverApps = packetSinkHelper.Install(ue);
        serverApps.Start(Seconds(1.0));
        UdpClientHelper client(ueIpIface.GetAddress(u), dlPort);
        client.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
        client.SetAttribute("MaxPackets", UintegerValue(10000));
        ApplicationContainer clientApps = client.Install(remoteHost);
        clientApps.Start(Seconds(1.0));
    }
    //Custom Trace Sinks for measuring Handover
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteUePhy/ReportCurrentCellRsrpSinr",
                    MakeCallback(&ReportCurrentCellRsrpSinr));

    Simulator::Schedule(Seconds(1.0), &handler, enbLteDevs, ueLteDevs);

    Simulator::Stop(Seconds(simTime));

    cout << "Starting Simulation" << endl;
    //time the simulation
    clock_t start = clock();
    Simulator::Run();
    clock_t end = clock();

    double time = (double)(end - start) / CLOCKS_PER_SEC;
    cout << simTime << "s took: " << time << "s to finish." << endl;
    Simulator::Destroy();

    ofstream myfile("data.txt", ios::trunc);

    for (int i = 0; i < 5000; i++)
    {
        myfile << data[i] << " ";
    }

    myfile.close();

    Py_Finalize();
    return 0;
}
