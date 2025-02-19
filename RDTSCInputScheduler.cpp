//
// Mohamed Maatallah <hotelsmaatallahrecemail@gmail.com>
// Tue Feb 18 2025
// RDTSCInputScheduler.cpp - Demo for RDTSC Input Scheduler

#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include "RDTSCPInputScheduler.h"

// calibration of sendinput overhead
static unsigned __int64 calibrateSyscallOverheadBatched() {
    const int bs = 10, its = 50;
    unsigned __int64 tot = 0;
    for (int i = 0; i < its; i++) {
        unsigned __int64 s = __rdtsc();
        for (int b = 0; b < bs; b++)
            SendInput(0, nullptr, 0);
        unsigned __int64 e = __rdtsc();
        tot += (e - s);
    }
    return tot / (its * bs);
}

// lazy calibration of jitter compensation
static unsigned __int64 calibrateJitterCompensationBatched() {
    const int bs = 10, its = 50;
    const unsigned __int64 wt = 200; // ns
    unsigned __int64 totOver = 0, totCalls = its * bs;
    for (int i = 0; i < its; i++) {
        unsigned __int64 s = __rdtsc();
        for (int b = 0; b < bs; b++)
            RDTSCInputSchedulerWait(wt);
        unsigned __int64 e = __rdtsc();
        unsigned __int64 measured = e - s;
        unsigned __int64 expected = wt * gCyclesPerNanosecond * bs;
        if (measured > expected)
            totOver += (measured - expected);
    }
    return totOver / totCalls;
}

// calibrate timing globals and print a report
void InitializeTimingAndCalibrations() {
    const int its = 3;
    double totFreq = 0.0;
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) {
        MessageBoxA(nullptr, "QPF failed", "Error", MB_OK);
        return;
    }
    for (int i = 0; i < its; i++) {
        LARGE_INTEGER sp = { 0 }, ep = { 0 };
        QueryPerformanceCounter(&sp);
        unsigned __int64 tStart = __rdtsc();
        Sleep(50);
        unsigned __int64 tEnd = __rdtsc();
        QueryPerformanceCounter(&ep);
        double sec = double(ep.QuadPart - sp.QuadPart) / freq.QuadPart;
        double tscFreq = double(tEnd - tStart) / sec;
        totFreq += tscFreq;
    }
    double avgFreq = totFreq / its;
    double cps = avgFreq / 1e9;
    gCyclesPerNanosecond = (unsigned __int64)(cps + 0.5);
    gSyscallOverheadCycles = calibrateSyscallOverheadBatched();
    gJitterCompensationCycles = calibrateJitterCompensationBatched();

    std::cout << "\n=== Timing Calibration Results ===\n";
    std::cout << std::left << std::setw(35) << "Parameter" << std::setw(25) << "Value" << "\n";
    std::cout << "--------------------------------------------------\n";
    std::cout << std::left << std::setw(35) << "Avg TSC Frequency (Hz)" << std::setw(25) << avgFreq << "\n";
    std::cout << std::left << std::setw(35) << "Cycles per Nanosecond" << std::setw(25) << gCyclesPerNanosecond << "\n";
    std::cout << std::left << std::setw(35) << "Syscall Overhead (cycles)" << std::setw(25) << gSyscallOverheadCycles << "\n";
    std::cout << std::left << std::setw(35) << "Jitter Compensation (cycles)" << std::setw(25) << gJitterCompensationCycles << "\n";
    std::cout << "==================================================\n\n";
}

// simulate a key press and release 
void SimulateKey(WORD vk) {
    INPUT inp[2] = {};
    inp[0].type = INPUT_KEYBOARD;
    inp[0].ki.wVk = vk;
    inp[1].type = INPUT_KEYBOARD;
    inp[1].ki.wVk = vk;
    inp[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inp, sizeof(INPUT));
}

// MIDI event structure
struct MidiEvent {
    char note;
    unsigned __int64 delayNs;
    int velocity;
};

// simulate a series of events with variations
void SimulateMIDIEvents() {
    std::vector<MidiEvent> events = {
        {'A', 300000ULL, 100},
        {'W', 200000ULL, 110},
        {'S', 500000ULL, 120},
        {'E', 250000ULL,  90},
        {'D', 400000ULL, 105},
        {'F', 350000ULL, 115},
        {'T', 300000ULL,  95},
        {'G', 450000ULL, 100},
        {'Y', 400000ULL, 110},
        {'H', 550000ULL, 105},
        {'U', 300000ULL, 115},
        {'J', 500000ULL, 100},
        {'K', 600000ULL, 120},
        {'L', 450000ULL,  95}
    };

    unsigned __int64 totReq = 0, totAct = 0;
    std::cout << "Simulating events...\n";
    for (const auto& evt : events) {
        totReq += evt.delayNs;
        auto st = std::chrono::high_resolution_clock::now();
        RDTSCInputSchedulerWait(evt.delayNs);
        auto en = std::chrono::high_resolution_clock::now();
        auto actual = std::chrono::duration_cast<std::chrono::nanoseconds>(en - st).count();
        totAct += actual;
        std::cout << "Note " << evt.note << " (vel " << evt.velocity << ") - req: "
            << evt.delayNs << " ns, act: " << actual << " ns\n";
        SimulateKey(evt.note);
    }
    double errRate = ((double)totAct - totReq) / totReq * 100.0;
    std::cout << "\n=== MIDI Scheduling Report ===\n";
    std::cout << "Total Requested Delay: " << totReq << " ns\n";
    std::cout << "Total Actual Delay:    " << totAct << " ns\n";
    std::cout << "Overall Error Rate:      " << std::fixed << std::setprecision(2)
        << errRate << "%\n";
    std::cout << "===============================\n";
}

int main() {
    std::cout << "initializing RDTSC Input Scheduler (MIDI++ core)...\n";
    InitializeTimingAndCalibrations();

    unsigned __int64 initDelay = 1000000ULL; // 1 ms
    std::cout << "delaying for " << initDelay << " ns before MIDI events...\n";
    auto st = std::chrono::high_resolution_clock::now();
    RDTSCInputSchedulerWait(initDelay);
    auto en = std::chrono::high_resolution_clock::now();
    auto initActual = std::chrono::duration_cast<std::chrono::nanoseconds>(en - st).count();
    double initError = ((double)initActual - initDelay) / initDelay * 100.0;
    std::cout << "init delay: req " << initDelay << " ns, act " << initActual
        << " ns, error " << std::fixed << std::setprecision(2) << initError << "%\n\n";

    SimulateMIDIEvents();

    std::cout << "\nPress Enter to exit...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    return 0;
}
