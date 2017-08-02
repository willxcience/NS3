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


void handler(NetDeviceContainer enbLteDevs, NetDeviceContainer ueLteDevs, NetDeviceContainer ueLteDevs2)
{
    //Ptr<MobilityModel> mobility = ueLteDevs.Get(1)->GetNode()->GetObject<MobilityModel>();
    //Vector3D pos = mobility->GetPosition();
    
    //from 0 to 3
    Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy();
    double currTxPower = enbPhy->GetTxPower();

    
    cout << enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy()->GetTxPower() << "db" << endl;
    
    enbPhy->SetTxPower(40.0);
    

    
    cout << "IMSI: " << UEs[0].imsi << " SINR: " << UEs[0].sinr << " RSRP: " << UEs[0].rsrp << endl;
    cout << "IMSI: " << UEs[1].imsi << " SINR: " << UEs[1].sinr << " RSRP: " << UEs[1].rsrp << endl;
    
    /* */
    
    //schedule next control
    Simulator::Schedule(Seconds(1.0), &handler, enbLteDevs, ueLteDevs, ueLteDevs2);
    cout << endl;
}


//Trace Sink
static void ReportCurrentCellRsrpSinr(string context, uint16_t cellId, uint16_t rnti, double rsrp, double sinr)
{
    int id = getNodeNumber(context) - 2;
    
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




int main (int argc, char *argv[])
{
  double enbDist = 100.0;
  double radius = 50.0;
  uint32_t numUes = 1;
  double simTime = 10.0;

  CommandLine cmd;
  cmd.AddValue ("enbDist", "distance between the two eNBs", enbDist);
  cmd.AddValue ("radius", "the radius of the disc where UEs are placed around an eNB", radius);
  cmd.AddValue ("numUes", "how many UEs are attached to each eNB", numUes);
  cmd.AddValue ("simTime", "Total duration of the simulation (in seconds)", simTime);
  cmd.Parse (argc, argv);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line
  cmd.Parse (argc, argv);

  // determine the string tag that identifies this simulation run
  // this tag is then appended to all filenames

  IntegerValue runValue;
  GlobalValue::GetValueByName ("RngRun", runValue);

  std::ostringstream tag;
  tag  << "_enbDist" << std::setw (3) << std::setfill ('0') << std::fixed << std::setprecision (0) << enbDist
       << "_radius"  << std::setw (3) << std::setfill ('0') << std::fixed << std::setprecision (0) << radius
       << "_numUes"  << std::setw (3) << std::setfill ('0')  << numUes
       << "_rngRun"  << std::setw (3) << std::setfill ('0')  << runValue.Get () ;

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisSpectrumPropagationLossModel"));

  // Create Nodes: eNodeB and UE
  NodeContainer enbNodes;
  NodeContainer ueNodes1, ueNodes2;
  enbNodes.Create (2);
  ueNodes1.Create (numUes);
  ueNodes2.Create (numUes);

  // Position of eNBs
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (enbDist, 0.0, 0.0));
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (positionAlloc);
  enbMobility.Install (enbNodes);

  // Position of UEs attached to eNB 1
  MobilityHelper ue1mobility;
  ue1mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                    "X", DoubleValue (0.0),
                                    "Y", DoubleValue (0.0),
                                    "rho", DoubleValue (radius));
  ue1mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  ue1mobility.Install (ueNodes1);

  // Position of UEs attached to eNB 2
  MobilityHelper ue2mobility;
  ue2mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                    "X", DoubleValue (enbDist),
                                    "Y", DoubleValue (0.0),
                                    "rho", DoubleValue (radius));
  ue2mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  ue2mobility.Install (ueNodes2);



  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs1;
  NetDeviceContainer ueDevs2;
  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueDevs1 = lteHelper->InstallUeDevice (ueNodes1);
  ueDevs2 = lteHelper->InstallUeDevice (ueNodes2);

  // Attach UEs to a eNB
  lteHelper->Attach (ueDevs1, enbDevs.Get (0));
  lteHelper->Attach (ueDevs2, enbDevs.Get (1));

  // Activate a data radio bearer each UE
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  lteHelper->ActivateDataRadioBearer (ueDevs1, bearer);
  lteHelper->ActivateDataRadioBearer (ueDevs2, bearer);

    
    //Custom Trace Sinks for measuring Handover
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteUePhy/ReportCurrentCellRsrpSinr",
                    MakeCallback(&ReportCurrentCellRsrpSinr));
    
    
  Simulator::Schedule(Seconds(1.0), &handler, enbDevs, ueDevs1, ueDevs2);
    
  Simulator::Stop (Seconds (simTime));

  // Insert RLC Performance Calculator
  std::string dlOutFname = "DlRlcStats";
  dlOutFname.append (tag.str ());
  std::string ulOutFname = "UlRlcStats";
  ulOutFname.append (tag.str ());

  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
