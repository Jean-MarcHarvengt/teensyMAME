/*
 *	Emulation for the NEC PD4990A.
 *
 *	The PD4990A is a serial I/O Calendar & Clock IC used in the
 *      NEO GEO and probably a couple of other machines.
 */

/* Set the data in the chip to Monday 09/09/73 00:00:00     */
/* If you ever read this Leejanne, you know what I mean :-) */
static int seconds=0x00;	/* BCD */
static int minutes=0x00;	/* BCD */
static int hours=0x00;		/* BCD */
static int days=0x09;		/* BCD */
static int month=0x09;		/* Hexadecimal form */
static int year=0x73;		/* BCD */
static int weekday=0x01;	/* BCD */

static int retraces=0;		/* Assumes 60 retraces a second */
static int coinflip=0;		/* Pulses a bit in order to simulate */
				/* test output */
static int outputbit=0;

void addretrace (void) {
	coinflip ^= 1;
	retraces++;
	if (retraces == 60) {
		retraces = 0;
		seconds++;
		if ((seconds & 0x0f) == 10) {
			seconds &= 0xf0;
			seconds += 0x10;
			if ( (seconds & 0xf0) == 0xA0) {
				seconds &= 0x0f;
				minutes++;
				if ( (minutes & 0x0f) == 10) {
					minutes &= 0xf0;
					minutes += 0x10;
					if ((minutes & 0xf0) == 0xa0) {
						minutes &= 0x0f;
						hours++;
						if ( (hours & 0x0f) == 10 ){
							hours &= 0xf0;
							hours += 0x10;
							if ((hours & 0xf0) ==  3) {
								hours &= 0x0f;
							}
						}
					}
				}
			}
		}
	}
}

int read_4990_testbit(void) {
	return (coinflip);
}

void write_4990_control(int data) {
	static int bitno=0;

	switch (data) {
		case 0x00:	/* Register hold, do nothing */
				break;

		case 0x04:	/* Start afresh with shifting */
				bitno=0;
				break;

		case 0x02:	/* shift one position */
				bitno++;
				switch(bitno) {
					case 0x00:
					case 0x01:
					case 0x02:
					case 0x03:
					case 0x04:
					case 0x05:
					case 0x06:
					case 0x07: outputbit=(seconds >> bitno) & 0x01;
						   break;
					case 0x08:
					case 0x09:
					case 0x0a:
					case 0x0b:
					case 0x0c:
					case 0x0d:
					case 0x0e:
					case 0x0f: outputbit=(minutes >> (bitno-8)) & 0x01;
						   break;
					case 0x10:
					case 0x11:
					case 0x12:
					case 0x13:
					case 0x14:
					case 0x15:
					case 0x16:
					case 0x17: outputbit=(hours >> (bitno-16)) & 0x01;
						   break;
					case 0x18:
					case 0x19:
					case 0x1a:
					case 0x1b:
					case 0x1c:
					case 0x1d:
					case 0x1e:
					case 0x1f: outputbit=(days >> (bitno-24)) & 0x01;
						   break;
					case 0x20:
					case 0x21:
					case 0x22:
					case 0x23: outputbit=(weekday >> (bitno-28)) & 0x01;
						   break;
					case 0x24:
					case 0x25:
					case 0x26:
					case 0x27: outputbit=(month >> (bitno-32)) & 0x01;
						   break;
					case 0x28:
					case 0x29:
					case 0x2a:
					case 0x2b:
					case 0x2c:
					case 0x2d:
					case 0x2e:
					case 0x2f: outputbit=(year >> (bitno-40)) & 0x01;
						   break;

				}
		default:	/* Unhandled value */
				break;
	}
}
