# RDTSC Input Scheduler

## Overview

This is a submodule of MIDI++, which schedules note events for the auto-player via the RDTSCP instruction, it uses busy wait for exact number of nanoseconds while compensating for overhead and jitter caused by high-level api's like SendInput with near nanosecond accuracy. Note that this is still subject to OS thread prempting so error rates may vary from system to system.

---

## Integration

### Files

- **RDTSCInputScheduler.asm**  
  Contains the a small x64 MASM routine `RDTSCInputSchedulerWait` that busy-waits for a given delay.

- **RDTSCInputScheduler.h**  
  The header file declaring the globals and the function prototype for integration into projects.

- **RDTSCInputScheduler.cpp**  
  A demo showing how to calibrate the scheduler and schedule input events (simulating a simple series of events).  

### Steps to Integrate

1. **Add Files to Your Project:**  
   - Include `RDTSCInputScheduler.asm` in your project.
   - Add `RDTSCInputScheduler.h` to your include directories.
   - Integrate the source code from `RDTSCInputScheduler.cpp` (or reference its functions) into your project.

2. **Assemble the ASM File:**  
   Using the Visual Studio Developer Command Prompt, run:
   ```bash
   ml64 /c /FoRDTSCInputScheduler.obj RDTSCInputScheduler.asm
   ```
   This will produce an object file (`RDTSCInputScheduler.obj`).
   
  (*Make sure u set PATH to ml64.exe location*)

4. **Link the Object File:**  
   Add `RDTSCInputScheduler.obj` to your project’s linker inputs and specify it there.

5. **Define Globals:**  
   Ensure that the globals (`gCyclesPerNanosecond`, `gSyscallOverheadCycles`, and `gJitterCompensationCycles`) are defined in exactly one source file. The demo in `RDTSCInputScheduler.cpp` shows how to do this.

6. **Calibration:**  
   Before scheduling input events, call the provided calibration routine (see `InitializeTimingAndCalibrations()` in `RDTSCInputScheduler.cpp`). This measures your CPU’s TSC frequency and calculates:
   - **Cycles per Nanosecond**
   - **Syscall Overhead (cycles)**
   - **Jitter Compensation (cycles)**
   
   These values are used by the scheduler to determine the correct busy-wait time.

7. **Scheduling Input Events:**  
   Use `RDTSCInputSchedulerWait(delay_in_ns)` to delay execution by the specified number of nanoseconds. Once the delay elapses, trigger your input event (e.g., send a key press using `SendInput`).

---

## How It Works

1. **Timing Calculation:**  
   The routine multiplies the desired delay (in nanoseconds) by the number of CPU cycles per nanosecond (calibrated beforehand) to calculate the total cycles required for the delay.

2. **Overhead Compensation:**  
   It subtracts the measured overhead (from system calls and jitter) from the total cycles to improve accuracy.

3. **Busy-Wait Loop:**  
   The routine reads the current time stamp (TSC) using the `RDTSCP` instruction, then loops until the TSC reaches the target value. This guarantees very precise delays at the cost of using one CPU core fully.

4. **Integration Example:**  
   The demo simulates a short event burst by scheduling multiple input events (key presses/releases) with precise delays between them. The calibration report is printed, showing the average TSC frequency, cycles per nanosecond, and overhead values. It also calculates and displays the error rate between the requested and actual delay etc.

---

## Example Output

```
Initializing RDTSC Input Scheduler...

=== Timing Calibration Results ===
Parameter                          Value
--------------------------------------------------
Average TSC Frequency (Hz)         2.91832e+09
Cycles per Nanosecond              3
Syscall Overhead (cycles)          2144
Jitter Compensation (cycles)       0 (usually negligble) 
==================================================

Delaying for 1000000 ns before MIDI events...
Requested delay: 1000000 ns, actual: 1030700 ns, error rate: 3.07%

Starting event simulation...
Sending note A...
Sending note S...
Sending note D...
Sending note F...
Event simulation complete.
```

---

## Troubleshooting

- **Undefined Symbols:**  
  If you see errors regarding `RDTSCInputSchedulerWait`, ensure that your ASM file is correctly assembled and linked, and that your header is included

- **Calibration Issues:**  
  If the reported TSC frequency or error rates seem off, verify that your hardware supports the `RDTSCP` instruction and that your system timer is functioning correctly (this could also be affected by CPU power profile changes)

- **High CPU Usage:**  
  Remember, the busy-wait approach occupies one CPU core completely during its delay. For less critical timing, consider mixing in a sleep period (hybrid sleep approach). 

---


