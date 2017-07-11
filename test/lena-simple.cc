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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/config-store.h"
#include <ns3/buildings-helper.h>
//#include "ns3/gtk-config-store.h"

#define D 500

using namespace ns3;

int main(int argc, char *argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // to save a template default attribute file run it like this:
    // ./waf --command-template="%s --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Save --ns3::ConfigStore::FileFormat=RawText" --run src/lte/examples/lena-first-sim
    //
    // to load a previously created default attribute file
    // ./waf --command-template="%s --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Load --ns3::ConfigStore::FileFormat=RawText" --run src/lte/examples/lena-first-sim

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // Parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

    // Uncomment to enable logging
    //  lteHelper->EnableLogComponents ();

    // Create Nodes: eNodeB and UE
    NodeContainer enbNodes;
    enbNodes.Create(19);

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
    mobility.SetFreq(748);
    mobility.Install(enbNodes);

    NodeContainer ueNodes;
    ueNodes.Create(380);

    Ptr<ListPositionAllocator> uePositions = CreateObject<ListPositionAllocator>();
    MobilityHelper ueMobility;
    ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    srand(time(NULL));

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

    //Set up LTE helper
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

    // Create Devices and install them in the Nodes
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;
    enbDevs = lteHelper->InstallEnbDevice(enbNodes); //!
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));








    // Attach a UE to a eNB
    lteHelper->Attach(ueDevs, enbDevs.Get(0));

    // Activate a data radio bearer
    enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs, bearer);
    lteHelper->EnableTraces();

    Simulator::Stop(Seconds(1.05));

    Simulator::Run();

    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    Simulator::Destroy();
    return 0;
}
