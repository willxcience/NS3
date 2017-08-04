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
 *         Nicola Baldo <nbaldo@cttc.es>
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

#include <iomanip>
#include <string>

#define D 500
#define OFFSET 200

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

UserEquipment UEs[2];

uint16_t getNodeNumber(string context)
{
    uint32_t pos = context.find('/', 10);
    return atoi(context.substr(10, pos - 10).c_str());
}

void handler(NetDeviceContainer enbLteDevs, NetDeviceContainer ueLteDevs)
{
    //Ptr<MobilityModel> mobility = ueLteDevs.Get(1)->GetNode()->GetObject<MobilityModel>();
    //Vector3D pos = mobility->GetPosition();

    //from 0 to 3
    Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy();
    double currTxPower = enbPhy->GetTxPower();

    cout << enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy()->GetTxPower() << "db" << endl;
    cout << ueLteDevs.Get(0)->GetObject<LteUeNetDevice>()->GetPhy()->GetTxPower() << "dbm" << endl;

    enbPhy->SetTxPower(35.0);

    cout << "IMSI: " << UEs[0].imsi << " SINR: " << UEs[0].sinr << " RSRP: " << UEs[0].rsrp << endl;
    cout << "IMSI: " << UEs[1].imsi << " SINR: " << UEs[1].sinr << " RSRP: " << UEs[1].rsrp << endl;
    cout << "IMSI: " << UEs[2].imsi << " SINR: " << UEs[2].sinr << " RSRP: " << UEs[2].rsrp << endl;
    cout << "IMSI: " << UEs[3].imsi << " SINR: " << UEs[3].sinr << " RSRP: " << UEs[3].rsrp << endl;

    /* */

    //schedule next control
    Simulator::Schedule(Seconds(1.0), &handler, enbLteDevs, ueLteDevs);
    cout << endl;
}

//Trace Sink
static void ReportCurrentCellRsrpSinr(string context, uint16_t cellId, uint16_t rnti, double rsrp, double sinr)
{
    int id = getNodeNumber(context) - 4;

    //cout << "CellId " << cellId << " SINR: " << sinr << endl;
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

int main(int argc, char *argv[])
{
    double enbDist = 100.0;
    double radius = 50.0;
    uint32_t numUes = 1;
    double simTime = 10.0;

    CommandLine cmd;
    cmd.AddValue("enbDist", "distance between the two eNBs", enbDist);
    cmd.AddValue("radius", "the radius of the disc where UEs are placed around an eNB", radius);
    cmd.AddValue("numUes", "how many UEs are attached to each eNB", numUes);
    cmd.AddValue("simTime", "Total duration of the simulation (in seconds)", simTime);
    cmd.Parse(argc, argv);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

    // determine the string tag that identifies this simulation run
    // this tag is then appended to all filenames

    IntegerValue runValue;
    GlobalValue::GetValueByName("RngRun", runValue);
    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue(false));

    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(30.0));
    //Config::SetDefault("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue(false));

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisSpectrumPropagationLossModel"));

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
    uePositions->Add(Vector(D - OFFSET, D - OFFSET, 10));
    uePositions->Add(Vector(D + OFFSET, D - OFFSET, 10));
    uePositions->Add(Vector(D - OFFSET, D + OFFSET, 10));
    uePositions->Add(Vector(D + OFFSET, D + OFFSET, 10));
    ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    ueMobility.SetPositionAllocator(uePositions);
    ueMobility.Install(ueNodes);

    /***Next Part***/
    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Attach UEs to a eNB
    lteHelper->Attach(ueLteDevs.Get(0), enbLteDevs.Get(0));
    lteHelper->Attach(ueLteDevs.Get(1), enbLteDevs.Get(1));
    lteHelper->Attach(ueLteDevs.Get(2), enbLteDevs.Get(2));
    lteHelper->Attach(ueLteDevs.Get(3), enbLteDevs.Get(3));

    // Activate a data radio bearer each UE
    enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueLteDevs.Get(0), bearer);
    lteHelper->ActivateDataRadioBearer(ueLteDevs.Get(1), bearer);
    lteHelper->ActivateDataRadioBearer(ueLteDevs.Get(2), bearer);
    lteHelper->ActivateDataRadioBearer(ueLteDevs.Get(3), bearer);

    //Custom Trace Sinks for measuring Handover
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteUePhy/ReportCurrentCellRsrpSinr",
                    MakeCallback(&ReportCurrentCellRsrpSinr));

    Simulator::Schedule(Seconds(1.0), &handler, enbLteDevs, ueLteDevs);

    Simulator::Stop(Seconds(simTime));

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
