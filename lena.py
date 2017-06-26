# This is simple example of lte from ns-3-model-library
import ns.config_store
import ns.core
import ns.network
import ns.mobility
import ns.lte
import ns.buildings
import ns.applications
import ns.internet
import ns.point_to_point

ns.core.GlobalValue("nBlocks","Number of femtocell blocks",
ns.core.UintegerValue(1), ns.core.MakeUintegerChecker("uint32_t"))