# Energy-Adaptive Vector Search for Intermittent Computing

An **energy-aware, reliable approximate nearest neighbor (ANN) search system** designed for intermittently-powered, hardware-constrained embedded devices. The system dynamically trades off search accuracy and cost against real-time energy availability, ensuring forward progress and reliability even under fluctuating, energy-harvesting power conditions.

## Overview

This repository implements a graph-based ANN search engine that runs entirely on a resource-constrained microcontroller with external flash storage, targeting **intermittent computing** scenarios where power is unstable or harvested (e.g. from solar, RF, or vibration sources). Rather than assuming a stable power budget, the system continuously adapts its search configuration — trading precision for energy savings — to avoid wasting computation and losing progress across power failures.

## Hardware Platform

| Component | Role |
|---|---|
| **TI MSP430FR5994** | Ultra-low-power MCU, serves as the search engine (compute + control) |
| **Micron MT29F1G01ABAFDWB (SPI NAND)** | External non-volatile storage for large-scale vector data and graph index structures |
| **FRAM (on-chip)** | Fast, non-volatile memory used for double-buffered checkpointing |
| **External power supply + capacitor circuit** | Emulates fluctuating / harvested energy conditions for demo and evaluation |

## Key Technical Highlights

### 1. Graph-Based ANN Search on Constrained Hardware
Implements a DiskANN/Vamana-style graph search with product-quantized (PQ) vector compression, running directly on a microcontroller with kilobytes of RAM — with the full vector dataset and graph index stored on external SPI NAND flash rather than in on-chip memory.

### 2. Energy-Adaptive Search Configuration
A gear-switching algorithm dynamically selects between different search configurations (e.g. PQ code size, search depth) based on the current energy state, preventing wasted computation and lost progress when energy is scarce, while maximizing recall when energy is abundant.

### 3. Double-Buffered FRAM Checkpointing
A double-buffer checkpoint/restore mechanism persists search progress to FRAM, enabling the system to resume interrupted queries after a power loss instead of restarting from scratch — a core requirement for reliable intermittent computing.

### 4. DMA-Driven SPI I/O with Compute/IO Overlap
SPI transfers to/from the external NAND flash are offloaded to DMA, freeing the CPU from busy-loop polling. This decouples compute and I/O during graph search, allowing distance computation and flash I/O to overlap in the pipeline.
- **Result: 1.7x speedup** in overall workflow time vs. a naive blocking I/O implementation.

### 5. LEA-Accelerated Distance Computation
Leverages the MSP430FR5994's Low-Energy Accelerator (LEA) to batch-compute distances between the query vector and candidate vectors, rather than relying on naive CPU-only computation.
- **Result: 5.9x speedup** in distance computation vs. naive CPU implementation.

### 6. Low-Power Sleep via RTC-Gated Wakeup
When the current energy budget is insufficient to sustain a full ("dry") search run, the system enters **LPM3.5** low-power mode and uses the RTC timer to schedule a future wakeup — minimizing energy drain while waiting for conditions to improve.

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    MSP430FR5994 (MCU)                    │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐  │
│  │ Energy Monitor│──▶│ Gear-Switch  │──▶│ Graph Search │  │
│  │   (ADC/RTC)   │   │  Controller  │   │   + PQ Dist. │  │
│  └──────────────┘   └──────┬───────┘   └──────┬───────┘  │
│                             │                   │         │
│                      ┌──────▼───────┐    ┌──────▼──────┐  │
│                      │ FRAM Double- │    │ LEA Distance│  │
│                      │  Buffer CKPT │    │ Accelerator │  │
│                      └──────────────┘    └─────────────┘  │
│                             │                   │         │
│                      ┌──────▼───────────────────▼──────┐  │
│                      │      DMA-Driven SPI Engine      │  │
│                      └───────────────┬──────────────────┘ │
└──────────────────────────────────────┼─────────────────────┘
                                        │ SPI
                              ┌─────────▼──────────┐
                              │  MT29F1G01ABAFDWB   │
                              │  (Vector + Index    │
                              │   Storage on NAND)  │
                              └─────────────────────┘
```

## Performance Summary

| Optimization | Metric | Improvement |
|---|---|---|
| DMA-driven SPI + compute/IO overlap | Workflow completion time | **1.7x** faster |
| LEA-accelerated distance computation | Distance computation time | **5.9x** faster |
| Energy-adaptive gear-switching | Query completion under fluctuating power | Maintains progress vs. fixed-configuration baselines |
| RTC-gated LPM3.5 sleep | Idle-state energy consumption | Reduced vs. active polling |

## Motivation

Energy-harvesting and battery-free embedded devices are increasingly deployed for edge AI and sensing applications, but their power supply is inherently unstable. Traditional ANN search implementations assume continuous power and a fixed accuracy/latency budget — assumptions that break down under intermittent power. This project explores how a search system can remain **reliable** (no lost progress) and **energy-proportional** (accuracy scales with available energy) under these harsh, realistic power conditions.
