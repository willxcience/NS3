import sys
import ns.core
import ns.config_store
import ns.lte
import ns.applications
import ns.internet
import ns.network
import ns.mobility
import ns.point_to_point

# set all the defaults
ns.core.Config.SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled",
                          ns.core.BooleanValue(True))

# Data Error Model is most related to the interference
ns.core.Config.SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled",
                          ns.core.BooleanValue(True))

# Radio link
ns.core.Config.SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping",
                          ns.core.EnumValue(2))
ns.core.Config.SetDefault("ns3::LteUePhy::EnableUplinkPowerControl",
                          ns.core.BooleanValue(False))

# use PDCCH
ns.core.Config.SetDefault("ns3::LteHelper::UsePdschForCqiGeneration",
                          ns.core.BooleanValue(False))

ns.core.Config.SetDefault("ns3::LteEnbPhy::TxPower", ns.core.DoubleValue(30.0))
ns.core.Config.SetDefault("ns3::LteUePhy::TxPower", ns.core.DoubleValue(10.0))

lte = ns.lte.LteHelper()
epc = ns.lte.PointToPointEpcHelper()
lte.SetEpcHelper(epc)

lte.SetAttribute("PathlossModel",
                 ns.core.StringValue("ns3::FriisSpectrumPropagationLossModel"))

lte.SetHandoverAlgorithmType("ns3::NoOpHandoverAlgorithm")

pgw = epc.GetPgwNode()
remoteHostContainer = ns.network.NodeContainer()
remoteHostContainer.Create(1)
remoteHost = remoteHostContainer.Get(0)
internet = ns.internet.InternetStackHelper()
internet.Install(remoteHostContainer)

p2ph = ns.point_to_point.PointToPointHelper()
p2ph.SetDeviceAttribute(
    "DataRate", ns.network.DataRateValue(ns.network.DataRate("100Gb/s")))
p2ph.SetDeviceAttribute("Mtu", ns.core.UintegerValue(1500))
p2ph.SetChannelAttribute("Delay", ns.core.TimeValue(ns.core.Seconds(0.010)))
internetDevices = p2ph.Install(pgw, remoteHost)
ipv4h = ns.internet.Ipv4AddressHelper()
ipv4h.SetBase(
    ns.network.Ipv4Address("1.0.0.0"), ns.network.Ipv4Mask("255.0.0.0"))
internetIpIfaces = ipv4h.Assign(internetDevices)

# interface 0 is localhost, 1 is the p2p device
remoteHostAddr = internetIpIfaces.GetAddress(1)
ipv4RoutingHelper = ns.internet.Ipv4StaticRoutingHelper()
remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(
    remoteHost.GetObject(ns.internet.Ipv4.GetTypeId()))
remoteHostStaticRouting.AddNetworkRouteTo(
    ns.network.Ipv4Address("7.0.0.0"), ns.network.Ipv4Mask("255.0.0.0"), 1)
