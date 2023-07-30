enum class TimeSlot{
    TOTAL,
    FRONTEND,
    MIDEND,
    BACKEND,
    EXEC_BC,
    COUNT,
};

struct Timer{
    f64 start;
    f64 time;
};

char *timeSlotNames[] = {
    "total",
    "frontend",
    "midend",
    "backend",
    "executing_bytecodes",
};

void dumpTimers(Timer *t){
    const u32 dots = 30;
    for(u32 x=0; x<(u16)TimeSlot::COUNT; x+=1){
	char * slotName = timeSlotNames[x];
	printf("%s", slotName);
	u32 j = dots-strlen(slotName);
	while(j != 0){
	    printf(".");
	    j -= 1;
	};
	printf("%f\n", t[x].time);
    };
};

#if(PLAT_WIN)
#include "windowsOS.cc"
#elif(PLAT_LIN)
#include "linuxOS.cc"
#endif
