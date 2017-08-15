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

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <numpy/arrayobject.h>

#define D 500
#define OFFSET 300
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

//My own customized Application
class MyApp : public Application
{
  public:
    MyApp();
    virtual ~MyApp();

    /**
   * Register this type.
   * \return The TypeId.
   */
    static TypeId GetTypeId(void);
    void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void ScheduleTx(void);
    void SendPacket(void);

    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize;
    uint32_t m_nPackets;
    DataRate m_dataRate;
    EventId m_sendEvent;
    bool m_running;
    uint32_t m_packetsSent;
};

MyApp::MyApp()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_nPackets(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0)
{
}

MyApp::~MyApp()
{
    m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId(void)
{
    static TypeId tid = TypeId("MyApp")
                            .SetParent<Application>()
                            .AddConstructor<MyApp>();
    return tid;
}

void MyApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
}

void MyApp::StartApplication(void)
{
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    SendPacket();
}

void MyApp::StopApplication(void)
{
    m_running = false;

    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }
}

void MyApp::SendPacket(void)
{
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);


    if (++m_packetsSent < m_nPackets)
    {
        ScheduleTx();
    } else if (m_packetsSent = m_nPackets)
    {
        cout << Simulator::Now().GetSeconds() << "s" << endl;
        m_packetsSent++;
    }
}

void MyApp::ScheduleTx(void)
{
    if (m_running)
    {
        Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
        m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
    } else {
        cout << "Wrong" << endl;
    }
}

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

    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(ueLteDevs.Get(0)->GetNode()->GetApplication(0));

   //cout << "Received bytes: "
   //      << sink1->GetTotalRx() << endl;

    //cout << "IMSI: " << UEs[0].imsi << " SINR: " << UEs[0].sinr << " RSRP: " << UEs[0].rsrp << endl;

    if (counter < 5000)
    {
        //cout << counter << endl;
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

static void CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
    //cout << Simulator::Now().GetSeconds() << "s " << oldCwnd << "  "
    //     << newCwnd << endl;
}

static void RxDrop(Ptr<const Packet> p)
{
    cout << "RxDrop at " << Simulator::Now().GetSeconds() << endl;
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

    //load and check module
    PyObject *pName = PyUnicode_FromString("controller");
    pModule = PyImport_Import(pName);

    if (!pModule)
    {
        printf("Cant open python file!\n");
        exit(-2);
    }

    pFunc = PyObject_GetAttrString(pModule, "myplot");

    //required for the C++_API
    import_array();

    Py_DECREF(pName);
    cout << "loaded" << endl;
}

int main(int argc, char *argv[])
{
    //Initialize python
    pythonInit(argv[0]);

    double simTime = 200.0;
    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    //CtrlErrorModel and DataErrorModel caused the randomness
    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue(false));

    //Data Error Model is most related to the interference
    Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue(false));

    //Radio link
    Config::SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue(ns3::LteEnbRrc::RLC_AM_ALWAYS));
    Config::SetDefault("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue(true));

    //use PDCCH
    Config::SetDefault("ns3::LteHelper::UsePdschForCqiGeneration", BooleanValue(false));

    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(30.0));
    //Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(10.0));

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    //this is set by default
    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisSpectrumPropagationLossModel"));
    lteHelper->SetHandoverAlgorithmType("ns3::NoOpHandoverAlgorithm");

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

    //The data rate and delay is here
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

    ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, 0.0, 0.0));

    /***Next Part***/
    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    //Error Model
    //Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel>(
    //    "ErrorRate", DoubleValue(0.00001));
    //internetDevices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    

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
    lteHelper->AddX2Interface(enbNodes);

    //install a downlink app, uplink is not considered here
    uint16_t dlPort = 1234;
    uint16_t ulPort = 2000;

    for (uint32_t u = 0; u < ueNodes.GetN(); u++)
    {

        PacketSinkHelper dlPacketSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        ApplicationContainer serverApps = dlPacketSinkHelper.Install(ueNodes.Get(u));
        serverApps.Start(Seconds(1.0));

        Ptr<Socket> mySocket = Socket::CreateSocket(remoteHost, TcpSocketFactory::GetTypeId());
        mySocket->TraceConnectWithoutContext("CongestionWindow",
                                             MakeCallback(&CwndChange));

        Ptr<MyApp> clientApp = CreateObject<MyApp>();
        Address sinkAddress(InetSocketAddress(ueIpIface.GetAddress(u), dlPort));
        clientApp->Setup(mySocket, sinkAddress, 1040, 1000, DataRate("10Mbps"));
        remoteHost->AddApplication(clientApp);
        clientApp->SetStartTime(Seconds(1.0));
        clientApp->SetStopTime(Seconds(simTime));

        lteHelper->ActivateDedicatedEpsBearer(ueLteDevs, EpsBearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT), EpcTft::Default());
    }

   // internetDevices.Get(0)->TraceConnectWithoutContext("PhyRxDrop", MakeCallback (&RxDrop));


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

    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(ueNodes.Get(0)->GetApplication(0));

    std::cout << "Received bytes: "
              << sink1->GetTotalRx() << std::endl;

    Simulator::Destroy();
    Py_DECREF(pFunc);

    return 0;
}
