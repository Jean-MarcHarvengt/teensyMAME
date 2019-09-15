static unsigned EA;

static unsigned EA_000(void) { cycle_count-=7; EA=DefaultBase(DS)+(WORD)(regs.w[BX]+regs.w[SI]); return EA; }
static unsigned EA_001(void) { cycle_count-=8; EA=DefaultBase(DS)+(WORD)(regs.w[BX]+regs.w[DI]); return EA; }
static unsigned EA_002(void) { cycle_count-=8; EA=DefaultBase(SS)+(WORD)(regs.w[BP]+regs.w[SI]); return EA; }
static unsigned EA_003(void) { cycle_count-=7; EA=DefaultBase(SS)+(WORD)(regs.w[BP]+regs.w[DI]); return EA; }
static unsigned EA_004(void) { cycle_count-=5; EA=DefaultBase(DS)+regs.w[SI]; return EA; }
static unsigned EA_005(void) { cycle_count-=5; EA=DefaultBase(DS)+regs.w[DI]; return EA; }
static unsigned EA_006(void) { cycle_count-=6; EA=FETCH; EA+=FETCH<<8; EA+=DefaultBase(DS); return EA; }
static unsigned EA_007(void) { cycle_count-=5; EA=DefaultBase(DS)+regs.w[BX]; return EA; }

static unsigned EA_100(void) { cycle_count-=11; EA=DefaultBase(DS)+(WORD)(regs.w[BX]+regs.w[SI]+(INT8)FETCH); return EA; }
static unsigned EA_101(void) { cycle_count-=12; EA=DefaultBase(DS)+(WORD)(regs.w[BX]+regs.w[DI]+(INT8)FETCH); return EA; }
static unsigned EA_102(void) { cycle_count-=12; EA=DefaultBase(SS)+(WORD)(regs.w[BP]+regs.w[SI]+(INT8)FETCH); return EA; }
static unsigned EA_103(void) { cycle_count-=11; EA=DefaultBase(SS)+(WORD)(regs.w[BP]+regs.w[DI]+(INT8)FETCH); return EA; }
static unsigned EA_104(void) { cycle_count-=9; EA=DefaultBase(DS)+(WORD)(regs.w[SI]+(INT8)FETCH); return EA; }
static unsigned EA_105(void) { cycle_count-=9; EA=DefaultBase(DS)+(WORD)(regs.w[DI]+(INT8)FETCH); return EA; }
static unsigned EA_106(void) { cycle_count-=9; EA=DefaultBase(SS)+(WORD)(regs.w[BP]+(INT8)FETCH); return EA; }
static unsigned EA_107(void) { cycle_count-=9; EA=DefaultBase(DS)+(WORD)(regs.w[BX]+(INT8)FETCH); return EA; }

static unsigned EA_200(void) { cycle_count-=11; EA=FETCH; EA+=FETCH<<8; EA+=regs.w[BX]+regs.w[SI]; EA=DefaultBase(DS)+(WORD)EA; return EA; }
static unsigned EA_201(void) { cycle_count-=12; EA=FETCH; EA+=FETCH<<8; EA+=regs.w[BX]+regs.w[DI]; EA=DefaultBase(DS)+(WORD)EA; return EA; }
static unsigned EA_202(void) { cycle_count-=12; EA=FETCH; EA+=FETCH<<8; EA+=regs.w[BP]+regs.w[SI]; EA=DefaultBase(SS)+(WORD)EA; return EA; }
static unsigned EA_203(void) { cycle_count-=11; EA=FETCH; EA+=FETCH<<8; EA+=regs.w[BP]+regs.w[DI]; EA=DefaultBase(SS)+(WORD)EA; return EA; }
static unsigned EA_204(void) { cycle_count-=9; EA=FETCH; EA+=FETCH<<8; EA+=regs.w[SI]; EA=DefaultBase(DS)+(WORD)EA; return EA; }
static unsigned EA_205(void) { cycle_count-=9; EA=FETCH; EA+=FETCH<<8; EA+=regs.w[DI]; EA=DefaultBase(DS)+(WORD)EA; return EA; }
static unsigned EA_206(void) { cycle_count-=9; EA=FETCH; EA+=FETCH<<8; EA+=regs.w[BP]; EA=DefaultBase(SS)+(WORD)EA; return EA; }
static unsigned EA_207(void) { cycle_count-=9; EA=FETCH; EA+=FETCH<<8; EA+=regs.w[BX]; EA=DefaultBase(DS)+(WORD)EA; return EA; }

static unsigned (*GetEA[192])(void)={
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,

	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,

	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207
};
