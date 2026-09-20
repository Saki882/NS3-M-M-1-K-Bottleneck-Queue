# NS-3 Bottleneck Queue Simulation (M/M/1/K)

An NS-3 simulation of a single bottleneck link with **Poisson arrivals** and **exponentially distributed packet sizes**, compared with the analytical **M/M/1/K** queueing model.

## Summary

Two nodes are connected by a 1 Mbps bottleneck link with a system capacity of 50 packets and link utilisation of 0.9. The analytical results are compared with NS-3 simulation results.

| Metric                    |      Analytical | NS-3 Simulation |
| ------------------------- | --------------: | --------------: |
| Average packets in system |      **8.7623** |          8.7321 |
| Average waiting packets   |      **7.8628** |          7.8328 |
| Packet loss probability   | **5.18 × 10⁻⁴** |     4.91 × 10⁻⁴ |
| P(system occupancy > 20)  |      **0.1053** |          0.1030 |
| P(waiting packets > 20)   |      **0.0943** |          0.0921 |

The simulation results are close to the analytical results. The simulation also includes PASTA and Little's Law consistency checks.

## Problem Statement

Two nodes are connected with a link having:

* Capacity: 1 Mbps
* Propagation delay: 0.1 ms
* System capacity: 50 packets
* Poisson packet arrivals
* Exponential packet-size distribution
* Mean packet size: 200 Bytes
* Link utilisation: 0.9

The objectives are to calculate the average queueing length, packet loss probability, and probability of occupancy greater than 20 packets, and then verify the analytical results using NS-3.

## Parameters

| Parameter                |           Value |
| ------------------------ | --------------: |
| Link capacity            |          1 Mbps |
| Propagation delay        |          0.1 ms |
| System capacity (K)      |      50 packets |
| Mean packet size         |       200 Bytes |
| Packet size distribution |     Exponential |
| Arrival process          |         Poisson |
| Link utilisation (ρ)     |             0.9 |
| Service rate (μ)         |   625 packets/s |
| Arrival rate (λ)         | 562.5 packets/s |

![Simulation Parameters](results/parameters.png)

> **Note:** K = 50 represents the total number of packets in the system (waiting + one in service). Therefore, the NS-3 device queue is configured for 49 waiting packets.

## Analytical Solution

The system is modeled as an **M/M/1/K** finite-capacity queue with ρ = λ/μ = 0.9 and K = 50.

The analytical calculations for:

* Average packets in the system
* Average waiting packets
* Packet loss probability
* P(system occupancy > 20)
* P(waiting packets > 20)

are provided in [`analytical-calculations.md`](analytical-calculations.md).

## NS-3 Implementation

The simulation uses two nodes connected through a point-to-point bottleneck link.

The implementation uses:

* Poisson arrival process
* Exponentially distributed packet sizes
* 1 Mbps link capacity
* 0.1 ms propagation delay
* 49-packet device queue + 1 packet in service
* 50-second warm-up period
* Time-weighted queue occupancy measurements

PASTA is used to compare the probability seen by arriving packets with the time-average occupancy.

## How to Run

Place `poisson-bottleneck.cc` in the `scratch/` directory of your NS-3 installation.

```bash
cd ~/ns-3-dev
./ns3 build
./ns3 run scratch/poisson-bottleneck
```

The simulation supports parameters such as `--simTime`, `--warmup`, `--run`, `--K`, and `--rho`.

## Results

![Results Comparison](results/results-comparison.png)

The 3000-second simulation produced results close to the analytical M/M/1/K values.

| Metric                    |  Analytical |  Simulation | Difference |
| ------------------------- | ----------: | ----------: | ---------: |
| Average packets in system |      8.7623 |      8.7321 |      −0.3% |
| Average waiting packets   |      7.8628 |      7.8328 |      −0.4% |
| Packet loss probability   | 5.18 × 10⁻⁴ | 4.91 × 10⁻⁴ |      −5.2% |
| P(system occupancy > 20)  |      0.1053 |      0.1030 |      −2.1% |
| P(waiting packets > 20)   |      0.0943 |      0.0921 |      −2.4% |

The difference in packet loss probability is larger because packet loss is a relatively rare event and therefore has greater statistical variation in a single simulation run.

## Project Files

```text
.
├── README.md
├── poisson-bottleneck.cc
├── analytical-calculations.md
└── results/
    ├── parameters.png
    └── results-comparison.png
```

* [`poisson-bottleneck.cc`](poisson-bottleneck.cc) — NS-3 simulation source code
* [`analytical-calculations.md`](analytical-calculations.md) — Analytical calculations
* `results/parameters.png` — Simulation parameters
* `results/results-comparison.png` — Analytical and simulation results comparison

## Author

**Saqib Hussain**

BS Information Technology, Ghazi University, Dera Ghazi Khan, Pakistan

GitHub: [@Saki882](https://github.com/Saki882)
