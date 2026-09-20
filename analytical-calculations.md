# NS-3 Bottleneck Queue Simulation

This project implements an **M/M/1/K bottleneck queue** in NS-3 and compares the simulation results with analytical calculations.

## Parameters

| Parameter                |       Value |
| ------------------------ | ----------: |
| Link capacity            |      1 Mbps |
| Link latency             |      0.1 ms |
| Buffer size              |  50 packets |
| Average packet size      |   200 Bytes |
| Traffic model            |     Poisson |
| Packet size distribution | Exponential |
| Link utilization         |         0.9 |

## Objectives

* Calculate the average queueing length.
* Calculate the packet loss probability.
* Calculate the probability that buffer occupancy is greater than 20 packets.
* Verify the analytical results using NS-3 simulation.

## Project Files

* [`poisson-bottleneck.cc`](poisson-bottleneck.cc) — NS-3 simulation code
* [`analytical-calculations.md`](analytical-calculations.md) — Analytical calculations
* `results/` — Simulation parameters and results

## Simulation Parameters

![Simulation Parameters](results/parameters.png)

## Results Comparison

![Results Comparison](results/results-comparison.png)

## Run the Simulation

```bash
./ns3 build
./ns3 run poisson-bottleneck
```

## Author

**Saqib Hussain**
BS Information Technology, Ghazi University, Dera Ghazi Khan, Pakistan

GitHub: [Saki882](https://github.com/Saki882)
