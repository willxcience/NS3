/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

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
//#include "ns3/gtk-config-store.h"

using namespace ns3;
using namespace std;

#define numberOfEnbNodes 19
#define numberOfUeNodes 100
#define D 500


//controller
void handler(NetDeviceContainer enbLteDevs, NetDeviceContainer ueLteDevs)
{
    cout << Simulator::Now().As(Time::S) << setprecision(4) << endl;

    Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(i)->GetObject<LteEnbNetDevice>()->GetPhy();
    double currTxPower = enbPhy->GetTxPower();


    NS_LOG_FUNCTION_NOARGS ();
    Config::Connect ("/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/LteUePhy/ReportCurrentCellRsrpSinr", \
    MakeBoundCallback (&PhyStatsCalculator::ReportCurrentCellRsrpSinrCallback, m_phyStats));



    //control
    enbPhy->SetTxPower(100.0);
    //for user device
    //Ptr<LteUePhy> enbPhy = enbLteDevs.Get(i)->GetObject<LteUeNetDevice>()->GetPhy();
}

int main(int argc, char *argv[])
{

    double simTime = 1.1;
    double distance = 500;
    double interPacketInterval = 100;

    // Command line arguments
    CommandLine cmd;
    cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
    cmd.AddValue("distance", "Distance between eNBs [m]", distance);
    cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
    cmd.Parse(argc, argv);

    //set the maxium number of users
    Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(40));

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

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

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numberOfEnbNodes);
    ueNodes.Create(numberOfUeNodes);

    /***********Customized Part***********/

    // Install Mobility Model
    Ptr<ListPositionAllocator> enbPositions = CreateObject<ListPositionAllocator>();
    //Quad 1
    enbPositions->Add(Vector(0, 0, 10));
    enbPositions->Add(Vector(0, 1.414 * D, 10));
    enbPositions->Add(Vector(0, 2.828 * D, 10));

    enbPositions->Add(Vector(1.5 * D, 0.707 * D, 10));
    enbPositions->Add(Vector(1.5 * D, 2.12 * D, 10));

    enbPositions->Add(Vector(3 * D, 0, 10));
    enbPositions->Add(Vector(3 * D, 1.414 * D, 10));

    //Quad 2
    enbPositions->Add(Vector(-1.5 * D, 0.707 * D, 10));
    enbPositions->Add(Vector(-1.5 * D, 2.12 * D, 10));

    enbPositions->Add(Vector(-3 * D, 0, 10));
    enbPositions->Add(Vector(-3 * D, 1.414 * D, 10));

    //Quad 3
    enbPositions->Add(Vector(0, -1.414 * D, 10));
    enbPositions->Add(Vector(0, -2.828 * D, 10));

    enbPositions->Add(Vector(-1.5 * D, -0.707 * D, 10));
    enbPositions->Add(Vector(-1.5 * D, -2.12 * D, 10));

    enbPositions->Add(Vector(-3 * D, -1.414 * D, 10));

    //Quad 4
    enbPositions->Add(Vector(1.5 * D, -0.707 * D, 10));
    enbPositions->Add(Vector(1.5 * D, -2.12 * D, 10));
    enbPositions->Add(Vector(3 * D, -1.414 * D, 10));

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(enbPositions);
    mobility.Install(enbNodes);

    //Set the user equipments positions
    Ptr<ListPositionAllocator> uePositions = CreateObject<ListPositionAllocator>();
    MobilityHelper ueMobility;
    ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    /* Uniform Random distribute user equipments in */
    for (short i = 0; i < ueNodes.GetN(); i++)
    {
        double x = CreateObject<UniformRandomVariable>()->GetValue(-4 * D, 4 * D);
        double y = CreateObject<UniformRandomVariable>()->GetValue(-2.828 * D, 2.828 * D);

        //add the user position Vector(x,y,z)
        //user height is 10
        uePositions->Add(Vector(x, y, 10));
    }

    ueMobility.SetPositionAllocator(uePositions);
    ueMobility.Install(ueNodes);

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
    uint16_t otherPort = 3000;

    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        ++ulPort;
        //++otherPort;

        PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
        //PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort));
        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));
        serverApps.Add(ulPacketSinkHelper.Install(remoteHost));
        //serverApps.Add(packetSinkHelper.Install(ueNodes.Get(u)));

        UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
        dlClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        dlClient.SetAttribute("MaxPackets", UintegerValue(10));

        UdpClientHelper ulClient(remoteHostAddr, ulPort);
        ulClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        ulClient.SetAttribute("MaxPackets", UintegerValue(10));

        clientApps.Add(dlClient.Install(remoteHost));
        clientApps.Add(ulClient.Install(ueNodes.Get(u)));

        /* This part is not enabled
    UdpClientHelper client(ueIpIface.GetAddress(u), otherPort);
    client.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
    client.SetAttribute("MaxPackets", UintegerValue(10));
    if (u + 1 < ueNodes.GetN())
    {
      clientApps.Add(client.Install(ueNodes.Get(u + 1)));
    }
    else
    {
      clientApps.Add(client.Install(ueNodes.Get(0)));
    }
    */
    }

    serverApps.Start(Seconds(0.01));
    clientApps.Start(Seconds(0.01));
    //lteHelper->EnableTraces();

    //schedule an event every 0.1 as a controller
    Simulator::Schedule(Seconds(0.1), &handler, enbLteDevs, ueLteDevs);

    Simulator::Stop(Seconds(simTime));

    clock_t start = clock();
    Simulator::Run();

    /*GtkConfigStore config;
  config.ConfigureAttributes();*/
    clock_t end = clock();
    Simulator::Destroy();

    double time = (double)(end - start) / CLOCKS_PER_SEC;
    cout << "time: " << time << "s" << endl;
    return 0;
}
