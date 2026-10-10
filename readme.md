# NS-3 Bottleneck Queue Simulation: M/M/1/K Validation

An NS-3 simulation of a finite-buffer bottleneck link driven by Poisson arrivals and exponentially distributed packet sizes. The simulated queue occupancy, loss, and waiting-time statistics are validated against the closed-form **M/M/1/K** queueing model.

---

## Table of Contents

1. [Problem Statement](#1-problem-statement)
2. [System Model](#2-system-model)
3. [Parameters](#3-parameters)
4. [Analytical Solution](#4-analytical-solution)
5. [Simulation Design](#5-simulation-design)
6. [Running the Simulation](#6-running-the-simulation)
7. [Results](#7-results)
8. [Validation Checks](#8-validation-checks)
9. [Discussion](#9-discussion)
10. [Limitations and Future Work](#10-limitations-and-future-work)
11. [Project Structure](#11-project-structure)
12. [References](#12-references)
13. [Author](#13-author)

---

## 1. Problem Statement

Two nodes are connected by a point-to-point link that acts as a bottleneck. Packets arrive at the sending node according to a Poisson process, and their sizes are exponentially distributed with a mean of 200 bytes. The link has the following characteristics:

- **Capacity:** 1 Mbps
- **Propagation delay:** 0.1 ms
- **Buffer size:** 50 packets (the total number of packets in the system, including the one being transmitted)
- **Link utilisation:** ρ = 0.9

Because the buffer is finite, some arriving packets are dropped when the buffer is full. The analysis focuses on the following questions:

1. What is the average number of packets in the system and the average number waiting in the queue?
2. What is the probability that an arriving packet is dropped?
3. What is the probability that the system holds more than 20 packets, and that more than 20 packets are waiting?

The analytical answers are obtained from the M/M/1/K model. They are then verified with an independent NS-3 discrete-event simulation.

---

## 2. System Model

```
 Poisson arrivals (λ)          Bottleneck link (μ, K = 50)
 ────────────────────►  [ queue (49 waiting) | server ]  ────► Node 1
                         ◄──── drop if full ────┘
```

Each packet is treated as a customer in an M/M/1/K queue:

| Queueing component | Realisation in this project |
|---|---|
| **M** (arrivals) | Poisson process: exponential inter-arrival times |
| **M** (service) | Exponentially distributed packet sizes, which give exponential transmission times on a fixed-rate link |
| **1** (server) | One transmitter on the bottleneck link |
| **K** (capacity) | 50 packets in total: 49 waiting in the device queue and 1 in service |

---

## 3. Parameters

| Parameter | Symbol | Value |
|---|---|---|
| Link capacity | C | 1 Mbps |
| Propagation delay | d | 0.1 ms |
| System capacity | K | 50 packets (49 waiting + 1 in service) |
| Mean packet size | 1/μ (in bytes) | 200 bytes |
| Packet size distribution | | Exponential |
| Arrival process | | Poisson |
| Service rate | μ = C / (8 · 200) | 625 packets/s |
| Arrival rate | λ = ρ · μ | 562.5 packets/s |
| Offered load | ρ = λ / μ | 0.9 |
| Simulated time | | 3000 s (50 s warm-up discarded, 2950 s measured) |
| RNG seed / run | | 12345 / run 1 |

> **Note on K:** K = 50 counts every packet in the system. The NS-3 device queue is therefore configured for K − 1 = 49 waiting packets, because the packet being transmitted has already been removed from the queue.

> **Note on ρ:** ρ = λ/μ is the *offered* load. The *carried* load is ρ(1 − p_K) because some packets are dropped. The measured utilisation is therefore slightly below 0.9 (see [Section 8](#8-validation-checks)).

---

## 4. Analytical Solution

For an M/M/1/K queue with ρ ≠ 1, the steady-state probability of n packets in the system is

$$
p_n = \frac{(1-\rho)\,\rho^{n}}{1-\rho^{K+1}}, \qquad 0 \le n \le K
$$

From these probabilities the following metrics are derived:

| Metric | Formula |
|---|---|
| Average packets in system | $L = \sum_{n=0}^{K} n\,p_n$ |
| Average packets waiting | $L_q = \sum_{n=1}^{K} (n-1)\,p_n = L - (1 - p_0)$ |
| Packet loss probability (blocking) | $P_{\text{loss}} = p_K$ |
| P(system occupancy > 20) | $\sum_{n=21}^{K} p_n$ |
| P(waiting packets > 20) | $\sum_{n=22}^{K} p_n$ |

Substituting ρ = 0.9 and K = 50 gives the analytical values reported in [Section 7](#7-results).

Full derivations are in [`analytical-calculations.md`](analytical-calculations.md).

---

## 5. Simulation Design

The simulation is written in C++ against the NS-3 API and is located in [`poisson-bottleneck.cc`](poisson-bottleneck.cc).

**Topology.** Two nodes are connected by a single `PointToPointNetDevice` link. The sending device uses a `DropTailQueue` limited to K − 1 packets. Node 0 generates traffic and Node 1 receives it.

**Traffic generation.**
- Inter-arrival times are drawn from an exponential distribution with mean 1/λ, which produces a Poisson arrival process.
- Packet sizes are drawn from an exponential distribution with mean 200 bytes. The on-wire size includes the 2-byte PPP header.

**Measurement.**
- **Time-weighted occupancy.** The time spent in each state n is accumulated whenever the number in system changes. This gives time-average probabilities.
- **Loss.** Drops are counted at the moment a packet is refused by the full queue.
- **Warm-up.** The first 50 s are discarded so that the statistics reflect steady state.

**PASTA.** Because arrivals are Poisson, the Poisson Arrivals See Time Averages property implies that the distribution seen by arriving packets equals the time-average distribution. The program records what each arrival sees and compares the two.

**Little's Law.** The program checks that L − L_q equals the effective utilisation.

---

## 6. Running the Simulation

### Requirements

- NS-3 (ns-3-dev recommended)
- A C++17-capable compiler
- Linux or macOS (tested on Ubuntu)

### Steps

```bash
# 1. Copy the simulation into the NS-3 scratch directory
cp poisson-bottleneck.cc ~/ns-3-dev/scratch/

# 2. Build NS-3
cd ~/ns-3-dev
./ns3 build

# 3. Run the default experiment (K = 50, ρ = 0.9)
./ns3 run scratch/poisson-bottleneck
```

### Command-Line Options

| Option | Default | Description |
|---|---|---|
| `--K` | 50 | Maximum packets in system (queue + server) |
| `--rho` | 0.9 | Offered load ρ = λ/μ |
| `--simTime` | 3000 | Total simulated time (s) |
| `--warmup` | 50 | Warm-up period to discard (s) |
| `--run` | 1 | RNG run number, for independent replications |

Example with a different load and an independent replication:

```bash
./ns3 run "scratch/poisson-bottleneck --rho=0.8 --run=2"
```

### Terminal Output: Simulation Parameters

The program first prints the configuration it is running with:

![Simulation parameters as printed by the program](results/parameters.png)

---

## 7. Results

The 2950 s measurement window (after the 50 s warm-up) produced results that closely match the analytical M/M/1/K values.

### Terminal Output: Packet Counts and M/M/1/K Comparison

The program then prints the packet counts and the simulation-versus-analytical comparison:

![Packet counts and M/M/1/K results comparison from the terminal](results/results-comparison.png)

### Summary Table

| Metric | Analytical | NS-3 Simulation | Relative Difference |
|---|---:|---:|---:|
| Average packets in system, L | 8.7623 | 8.7321 | −0.3% |
| Average packets waiting, L_q | 7.8628 | 7.8328 | −0.4% |
| Packet loss probability, p_K | 5.18 × 10⁻⁴ | 4.91 × 10⁻⁴ | −5.2% |
| P(system occupancy > 20) | 0.1053 | 0.1030 | −2.1% |
| P(waiting packets > 20) | 0.0943 | 0.0921 | −2.4% |

**Packet counts (measurement window):**

| Quantity | Value |
|---|---:|
| Packets offered | 1,658,802 |
| Packets accepted | 1,657,988 |
| Packets dropped | 814 |
| Measured utilisation | 0.89929 (theory: 0.89953) |

---

## 8. Validation Checks

| Check | Result | Interpretation |
|---|---|---|
| **Occupancy vs. analytical model** | All five metrics agree within 5% | The simulation reproduces the M/M/1/K model. |
| **PASTA** | P(arrival sees N > 20) = 0.102848; time average = 0.103045 | Arrivals see the same distribution as the time average, as PASTA predicts for Poisson arrivals. |
| **Little's Law** | L − L_q = 0.899290 = measured utilisation | The internal bookkeeping is consistent. |
| **Carried load** | Measured 0.89929 vs. theory ρ(1 − p_K) = 0.89953 | Throughput is consistent with the blocking probability. |

---

## 9. Discussion

**Why is the loss-probability difference larger?** The loss probability is about 5 × 10⁻⁴, so only roughly one packet in 2000 is dropped. A single run with 814 observed drops has a standard error of about √859 ≈ 29 drops, against an expected count of about 859 from the analytical value. The observed difference is about 1.5 standard errors, so it is consistent with random variation. Rare events need longer runs or more replications to estimate precisely.

**Why do the occupancy metrics agree so closely?** Occupancy is a time average over millions of arrivals and departures, so its estimator converges quickly. The relative error in L and L_q is below 0.5%.

**Why is the measured utilisation below 0.9?** The offered load is ρ = λ/μ = 0.9, but dropped packets never use the link. The carried load is ρ(1 − p_K) ≈ 0.8995, which matches the measured value.

**Practical implication.** At ρ = 0.9 with a 50-packet buffer, the link is busy about 90% of the time and the queue is usually long. Under these conditions, queueing delay dominates the end-to-end latency, and the loss probability is small only because the buffer is large relative to the offered load.

---

## 10. Limitations and Future Work

- **Single replication.** Results come from one RNG run (seed 12345, run 1). Future work should run multiple independent replications and report confidence intervals, for example with a 95% interval across 10 runs.
- **Single bottleneck.** The model has one queue and one link. Extending it to multi-hop topologies or TCP traffic would test the model under more realistic conditions.
- **Fixed buffer size.** Varying K (for example 10, 50, 100) would show how buffer size trades loss against queueing delay.
- **Sweep over load.** Running ρ from 0.5 to 1.2 would show how the blocking probability grows as the link saturates.

---

## 11. Project Structure

```text
.
├── README.md                      # This file
├── poisson-bottleneck.cc          # NS-3 simulation source code
├── analytical-calculations.md     # Derivation of M/M/1/K metrics
└── results/
    ├── parameters.png             # Terminal output: simulation parameters
    └── results-comparison.png     # Terminal output: packet counts and M/M/1/K comparison
```

---

## 12. References

1. L. Kleinrock, *Queueing Systems, Volume 1: Theory*, Wiley, 1975. (M/M/1/K derivation and Little's Law.)
2. R. W. Wolff, "Poisson Arrivals See Time Averages," *Operations Research*, 30(2), 1982. (PASTA.)
3. J. D. C. Little, "A Proof for the Queuing Formula: L = λW," *Operations Research*, 9(3), 1961.

---

## 13. Author

**Saqib Hussain**
BS Information Technology, Ghazi University, Dera Ghazi Khan, Pakistan
Research interests: wireless and mobile networks (5G/6G, IoT), network performance modelling with NS-3, protocol optimisation, dynamic resource allocation

- GitHub: [@Saki882](https://github.com/Saki882)
- LinkedIn: [saqib-hussain-it](https://linkedin.com/in/saqib-hussain-it)
- Email: saqibhussainit882@gmail.com
