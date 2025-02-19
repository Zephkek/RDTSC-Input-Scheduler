#ifndef RDTSC_INPUT_SCHEDULER_H
#define RDTSC_INPUT_SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif
	extern "C" void RDTSCInputSchedulerWait(unsigned __int64 nanoseconds);

	// Global vars
	unsigned __int64 gCyclesPerNanosecond = 0;
	unsigned __int64 gSyscallOverheadCycles = 0;
	unsigned __int64 gJitterCompensationCycles = 0;

#ifdef __cplusplus
}
#endif

#endif // RDTSC_INPUT_SCHEDULER_H
