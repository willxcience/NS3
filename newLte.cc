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
    //cout << Simulator::Now().As(Time::S) << setw(4) << endl;

    Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy();
    double currTxPower = enbPhy->GetTxPower();

    //control
    //enbPhy->SetTxPower(100.0);
    //for user device
    //Ptr<LteUePhy> enbPhy = enbLteDevs.Get(i)->GetObject<LteUeNetDevice>()->GetPhy();
}

//Customized Trace Function
static void ReportCurrentCellRsrpSinr(string context, uint16_t cellId, uint16_t rnti, double rsrp, double sinr)
{
    //cout << Simulator::Now().GetSeconds() << " " << cellId << endl;
}

static void NotifyConnectionEstablishedUe(string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << std::setprecision(8) << "Time " << Simulator::Now().GetSeconds()
              << " UE IMSI " << imsi
              << ": connected to CellId " << cellid
              << " with RNTI " << rnti
              << std::endl;
}

int main(int argc, char *argv[])
{

    double simTime = 1000;
    double distance = 500;
    double interPacketInterval = 10;

    //set the maxium number of users
    Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(160));

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");

    //set up Strongest cell Handover
    //Details: https://www.nsnam.org/docs/models/html/lte-user.html#sec-automatic-handover
    lteHelper->SetHandoverAlgorithmType("ns3::A3RsrpHandoverAlgorithm");
    lteHelper->SetHandoverAlgorithmAttribute("Hysteresis",
                                             DoubleValue(3.0));
    lteHelper->SetHandoverAlgorithmAttribute("TimeToTrigger",
                                             TimeValue(MilliSeconds(256)));

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
    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisPropagationLossModel"));

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
    ueNodes.Create(4);

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
      ueMobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                     "X", StringValue ("500.0"),
                                     "Y", StringValue ("500.0"),
                                     "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=200]"));
      
    ueMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                "Bounds", RectangleValue(Rectangle(0, 1000.0, 0, 1000.0)), "Speed", StringValue("20"));

    /*MobilityHelper ueMobility;
    ueMobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                    "X", StringValue ("100.0"),
                                    "Y", StringValue ("100.0"),
                                    "Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=200"));

    cout << "done3" << endl;
    ueMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                "Bounds", RectangleValue(Rectangle(0, 1000, 0, 1000)), "Speed", StringValue("20"));
                                */

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
        //++otherPort;

        PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
        //PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort));
        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));
        serverApps.Add(ulPacketSinkHelper.Install(remoteHost));
        //serverApps.Add(packetSinkHelper.Install(ueNodes.Get(u)));

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

    //Custom Trace Sinks
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));

    Config::Connect("/NodeList/*/DeviceList/*/LteUePhy/ReportCurrentCellRsrpSinr",
                    MakeCallback(&ReportCurrentCellRsrpSinr));

    //schedule an event every 0.1 as a controller
    //Simulator::Schedule(Seconds(0.1), &handler, enbLteDevs, ueLteDevs);

    Simulator::Stop(Seconds(simTime));

    //set up a timer to measure the performance


    cout << "Starting Simulation" << endl;
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
