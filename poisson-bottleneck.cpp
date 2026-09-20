CPP CODE
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
using namespace ns3;
NS_LOG_COMPONENT_DEFINE("Mm1kSimulation");
//
===========================================================
=================
// GLOBAL STATE
//
===========================================================
=================
// PPP header = 2 bytes added by PointToPointNetDevice
static const uint32_t PPP_HEADER = 2;
static Ptr<PointToPointNetDevice> g_txDev;
static Ptr<ExponentialRandomVariable> g_interArrival;
static Ptr<ExponentialRandomVariable> g_pktSize;
static uint32_t g_K = 50; // max packets in the system
static uint32_t g_inSystem = 0; // packets currently in the system
static bool g_measuring = false; // false during warm-up
static double g_lastEventTime = 0.0; // for time-weighted statistics
static std::vector<double> g_timeAtN; // time spent with n in system
static std::vector<uint64_t> g_seenByArrival; // state seen by arrivals
static uint64_t g_arrivals = 0;
static uint64_t g_drops = 0;
static uint64_t g_departures = 0;
static uint64_t g_received = 0;
static double g_measureStart = 0.0;
//
===========================================================
=================
// TIME-WEIGHTED BOOKKEEPING
// Must be called BEFORE g_inSystem changes
//
===========================================================
=================
static void
Accumulate()
{
 double now = Simulator::Now().GetSeconds();
 if (g_measuring)
 {
 g_timeAtN[g_inSystem] += now - g_lastEventTime;
 }
 g_lastEventTime = now;
}
static void
StartMeasurement()
{
 g_lastEventTime = Simulator::Now().GetSeconds();
 g_measureStart = g_lastEventTime;
 std::fill(g_timeAtN.begin(), g_timeAtN.end(), 0.0);
 std::fill(g_seenByArrival.begin(), g_seenByArrival.end(), 0);
 g_arrivals = 0;
 g_drops = 0;
 g_departures = 0;
 g_received = 0;
 g_measuring = true;
}
//
===========================================================
=================
// TRAFFIC SOURCE
// Poisson arrivals + exponential on-wire packet size
//
===========================================================
=================
static void
SendPacket()
{
 Accumulate();
 uint32_t seen = g_inSystem; // state seen by this arrival (PASTA)
 // On-wire size (payload + PPP header) ~ Exp(mean 200 B), min 3 bytes
 double s = g_pktSize->GetValue();
 uint32_t wireSize = std::max<uint32_t>(PPP_HEADER + 1,
 (uint32_t)std::lround(s));
 Ptr<Packet> p = Create<Packet>(wireSize - PPP_HEADER);
 // 0x0800 = IPv4 protocol number for the PPP header
 bool accepted = g_txDev->Send(p, g_txDev->GetBroadcast(), 0x0800);
 if (g_measuring)
 {
 g_arrivals++;
 g_seenByArrival[seen]++;
 if (!accepted)
 {
 g_drops++;
 }
 }
 if (accepted)
 {
 g_inSystem++;
 }
 Simulator::Schedule(Seconds(g_interArrival->GetValue()), &SendPacket);
}
//
===========================================================
=================
// DEPARTURE / RECEPTION TRACES
//
===========================================================
=================
static void
OnTxEnd(Ptr<const Packet> /* p */)
{
 Accumulate();
 NS_ASSERT(g_inSystem > 0);
 g_inSystem--;
 if (g_measuring)
 {
 g_departures++;
 }
}
static void
OnRx(Ptr<const Packet> /* p */)
{
 if (g_measuring)
 {
 g_received++;
 }
}
//
===========================================================
=================
// ANALYTICAL M/M/1/K
//
===========================================================
=================
struct Analytical
{
 double L; // mean number in system
 double Lq; // mean number waiting (excluding one in service)
 double pLoss; // blocking probability p_K
 double pSysGt20; // P(N > 20) N = packets in system
 double pQGt20; // P(Nq > 20) Nq = packets waiting
};
static Analytical
ComputeAnalytical(double rho, uint32_t K)
{
 std::vector<double> p(K + 1);
 double sum = 0.0;
 for (uint32_t n = 0; n <= K; n++)
 {
 p[n] = std::pow(rho, n);
 sum += p[n];
 }
 for (uint32_t n = 0; n <= K; n++)
 {
 p[n] /= sum;
 }
 Analytical a{0, 0, 0, 0, 0};
 for (uint32_t n = 0; n <= K; n++)
 {
 a.L += n * p[n];
 a.Lq += (n > 0 ? n - 1 : 0) * p[n];
 if (n > 20) a.pSysGt20 += p[n];
 if (n > 21) a.pQGt20 += p[n];
 }
 a.pLoss = p[K];
 return a;
}
//
===========================================================
=================
// MAIN
//
===========================================================
=================
int
main(int argc, char *argv[])
{
 // ----------------------------------------------------------------
 // Parameters
 // ----------------------------------------------------------------
 double linkRateBps = 1e6; // 1 Mbps
 std::string delay = "0.1ms"; // propagation delay
 double meanPktBytes = 200.0; // mean on-wire packet size
 double rho = 0.9; // utilisation
 double simTime = 3000.0; // simulated seconds
 double warmup = 50.0; // warm-up seconds discarded
 uint32_t run = 1; // RNG run number
 CommandLine cmd(__FILE__);
 cmd.AddValue("K", "Max packets in system (queue + one in service)",
g_K);
 cmd.AddValue("rho", "Link utilisation", rho);
 cmd.AddValue("simTime", "Simulated time in seconds", simTime);
 cmd.AddValue("warmup", "Warm-up time in seconds", warmup);
 cmd.AddValue("run", "RNG run number (different independent runs)", run);
 cmd.Parse(argc, argv);
 RngSeedManager::SetSeed(12345);
 RngSeedManager::SetRun(run);
 g_timeAtN.assign(g_K + 1, 0.0);
 g_seenByArrival.assign(g_K + 1, 0);
 // ----------------------------------------------------------------
 // Rates
 // ----------------------------------------------------------------
 double mu = linkRateBps / (meanPktBytes * 8.0); // packets/s served
 double lambda = rho * mu; // packets/s arriving
 // ----------------------------------------------------------------
 // Topology: 2 nodes connected by a single bottleneck link
 //
 // NOTE: The device queue holds only the packets WAITING.
 // The packet being transmitted has already been dequeued.
 // So "K in system" -> K-1 in the device queue.
 // ----------------------------------------------------------------
 NodeContainer nodes;
 nodes.Create(2);
 std::ostringstream qs;
 qs << (g_K - 1) << "p";
 PointToPointHelper p2p;
 p2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate(linkRateBps)));
 p2p.SetChannelAttribute("Delay", StringValue(delay));
 p2p.SetQueue("ns3::DropTailQueue<Packet>",
 "MaxSize",
 StringValue(qs.str()));
 NetDeviceContainer devs = p2p.Install(nodes);
 g_txDev = DynamicCast<PointToPointNetDevice>(devs.Get(0));
 Ptr<PointToPointNetDevice> rxDev =
DynamicCast<PointToPointNetDevice>(devs.Get(1));
 g_txDev->TraceConnectWithoutContext("PhyTxEnd",
MakeCallback(&OnTxEnd));
 rxDev ->TraceConnectWithoutContext("MacRx", MakeCallback(&OnRx));
 // ----------------------------------------------------------------
 // Random variables
 // ----------------------------------------------------------------
 g_interArrival = CreateObject<ExponentialRandomVariable>();
 g_interArrival->SetAttribute("Mean", DoubleValue(1.0 / lambda));
 g_pktSize = CreateObject<ExponentialRandomVariable>();
 g_pktSize->SetAttribute("Mean", DoubleValue(meanPktBytes));
 // ----------------------------------------------------------------
 // Schedule and run
 // ----------------------------------------------------------------
 Simulator::Schedule(Seconds(0.0), &SendPacket);
 Simulator::Schedule(Seconds(warmup), &StartMeasurement);
 Simulator::Stop(Seconds(simTime));
 std::cout << "\nRunning simulation ("
 << simTime << " s, warm-up " << warmup
 << " s, run " << run << ") ...\n";
 Simulator::Run();
 Accumulate(); // close the last interval
 double T = Simulator::Now().GetSeconds() - g_measureStart;
 Simulator::Destroy();
 // ----------------------------------------------------------------
 // Simulation results
 // ----------------------------------------------------------------
 double L = 0.0, Lq = 0.0, pSysGt20 = 0.0, pQGt20 = 0.0;
 for (uint32_t n = 0; n <= g_K; n++)
 {
 double frac = g_timeAtN[n] / T;
 L += n * frac;
 Lq += (n > 0 ? n - 1 : 0) * frac;
 if (n > 20) pSysGt20 += frac;
 if (n > 21) pQGt20 += frac;
 }
 double pLoss = g_arrivals ? (double)g_drops / g_arrivals : 0.0;
 Analytical a = ComputeAnalytical(rho, g_K);
 // ----------------------------------------------------------------
 // Pretty output
 // ----------------------------------------------------------------
 std::cout << std::fixed << std::setprecision(6);
 std::cout <<
"\n=========================================================
=======\n";
 std::cout << " SIMULATION PARAMETERS\n";
 std::cout <<
"==========================================================
======\n";
 std::cout << " Service rate (mu) : " << mu << " packets/sec\n";
 std::cout << " Arrival rate (lambda) : " << lambda << " packets/sec\n";
 std::cout << " Utilisation (rho) : " << rho << "\n";
 std::cout << " System capacity (K) : " << g_K << " packets\n";
 std::cout << " Mean packet size : " << meanPktBytes << " Bytes\n";
 std::cout << " Link rate : " << linkRateBps / 1e6 << " Mbps\n";
 std::cout << " Link delay : " << delay << "\n";
 std::cout << " Simulation time : " << simTime << " seconds\n";
 std::cout << " Warm-up time : " << warmup << " seconds\n";
 std::cout << " RNG run : " << run << "\n";
 std::cout <<
"\n=========================================================
=======\n";
 std::cout << " PACKET COUNTS\n";
 std::cout <<
"==========================================================
======\n";
 std::cout << " Packets offered : " << g_arrivals << "\n";
 std::cout << " Packets accepted : " << g_arrivals - g_drops << "\n";
 std::cout << " Packets dropped : " << g_drops << "\n";
 std::cout << " Packets received at node 1 : " << g_received << "\n";
 std::cout << " Measured utilisation : " << (1.0 - g_timeAtN[0] / T)
 << " (theory: " << rho * (1.0 - a.pLoss) << ")\n";
 std::cout <<
"\n=========================================================
=======\n";
 std::cout << " M/M/1/K RESULTS COMPARISON\n";
 std::cout <<
"==========================================================
======\n";
 std::cout << std::left
 << std::setw(46) << " Metric"
 << std::setw(16) << " Simulation"
 << std::setw(16) << " Analytical" << "\n";
 std::cout << "----------------------------------------------------------------\n";
 std::cout << std::setw(46) << " Avg packets in system (queue + server)"
 << std::setw(16) << L
 << std::setw(16) << a.L << "\n";
 std::cout << std::setw(46) << " Avg queue length (waiting packets)"
 << std::setw(16) << Lq
 << std::setw(16) << a.Lq << "\n";
 std::cout << std::setw(46) << " Packet loss probability"
 << std::setw(16) << pLoss
 << std::setw(16) << a.pLoss << "\n";
 std::cout << std::setw(46) << " P(N > 20) [packets in system]"
 << std::setw(16) << pSysGt20
 << std::setw(16) << a.pSysGt20 << "\n";
 std::cout << std::setw(46) << " P(Nq > 20) [waiting packets]"
 << std::setw(16) << pQGt20
 << std::setw(16) << a.pQGt20 << "\n";
 std::cout <<
"==========================================================
======\n";
 // ----------------------------------------------------------------
 // PASTA check
 // ----------------------------------------------------------------
 uint64_t seenGt20 = 0;
 for (uint32_t n = 21; n <= g_K; n++)
 {
 seenGt20 += g_seenByArrival[n];
 }
 std::cout << "\n PASTA check:"
 << " P(arrival sees N > 20) = "
 << (g_arrivals ? (double)seenGt20 / g_arrivals : 0.0)
 << " (time avg = " << pSysGt20 << ")\n";
 std::cout << " Little's Law check:"
 << " L - Lq = " << (L - Lq)
 << " (rho_eff = " << (1.0 - g_timeAtN[0] / T) << ")\n";
 std::cout <<
"==========================================================
======\n\n";
 return 0;
}