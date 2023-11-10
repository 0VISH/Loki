enum class TimeSlot{
    TOTAL,
    FRONTEND,
    MIDEND,
    BACKEND,
    COUNT,
};

char *timeSlotNames[] = {
    "total",
    "frontend",
    "midend",
    "backend",
    "executing_bytecodes",
};

f64 times[(u16)TimeSlot::COUNT];

void dumpTimers(f64 *t){
    printf("\n");
    const u32 dots = 30;
    for(u32 x=0; x<(u16)TimeSlot::COUNT; x+=1){
	char * slotName = timeSlotNames[x];
	printf("%s", slotName);
	u32 j = dots-strlen(slotName);
	while(j != 0){
	    printf(".");
	    j -= 1;
	};
	printf("%f\n", t[x]);
    };
};
