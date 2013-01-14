#include "sound.h"

/////////////////////////////////////////////////////////////////////////////////
///
/// Neogeo Pocket Sound system
///
/////////////////////////////////////////////////////////////////////////////////
unsigned int	ngpRunning;
int sndCycles = 0;

void soundStep(int cycles)
{
	sndCycles+= cycles;
}

// Initialize the NGPC sound engine (Z80)
void ngpSoundStart()
{
	ngpRunning = 1;	// ?
#ifdef DRZ80
    Z80_Reset();
#else
	z80Init();
	z80SetRunning(1);
#endif
}

/// Execute all gained cycles (divided by 2)
void ngpSoundExecute()
{
#ifdef DRZ80
    int toRun = sndCycles/2;
    if(ngpRunning)
    {
        Z80_Execute(toRun);
    }
    //timer_add_cycles(toRun);
    sndCycles -= toRun;
#else
	int		elapsed;
	while(sndCycles > 0)
	{
		elapsed = z80Step();
		sndCycles-= (2*elapsed);
		//timer_add_cycles(elapsed);
	}
#endif
}

/// Switch sound system off
void ngpSoundOff() {
	ngpRunning = 0;
#ifdef DRZ80

#else
	z80SetRunning(0);
#endif
}

// Generate interrupt to ngp sound system
void ngpSoundInterrupt() {
	if (ngpRunning)
	{
#ifdef DRZ80
        Z80_Cause_Interrupt(0x100);//Z80_IRQ_INT???
#else
		z80Interrupt(0);
#endif
	}
}
