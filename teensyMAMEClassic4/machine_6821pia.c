/**********************************************************************

	Motorola 6821 PIA interface and emulation

	This function emulates all the functionality of up to 4 M6821
	peripheral interface adapters.

**********************************************************************/

#include <string.h>
#include <stdio.h>
#include "machine/6821pia.h"

#ifdef macintosh
#undef printf
#endif

/******************* internal PIA data structure *******************/

struct pia6821
{
	unsigned char in_a;
	unsigned char in_ca1;
	unsigned char in_ca2;
	unsigned char out_a;
	unsigned char out_ca2;
	unsigned char ddr_a;
	unsigned char ctl_a;
	unsigned char irq_a1;
	unsigned char irq_a2;

	unsigned char in_b;
	unsigned char in_cb1;
	unsigned char in_cb2;
	unsigned char out_b;
	unsigned char out_cb2;
	unsigned char ddr_b;
	unsigned char ctl_b;
	unsigned char irq_b1;
	unsigned char irq_b2;

	int (*in_a_func)(int offset);
	int (*in_b_func)(int offset);
	int (*in_ca1_func)(int offset);
	int (*in_ca2_func)(int offset);
	int (*in_cb1_func)(int offset);
	int (*in_cb2_func)(int offset);
	void (*out_a_func)(int offset, int val);
	void (*out_b_func)(int offset, int val);
	void (*out_ca2_func)(int offset, int val);
	void (*out_cb2_func)(int offset, int val);
	void (*irq_a_func)(void);
	void (*irq_b_func)(void);
};


/******************* convenince macros and defines *******************/

#define PIA_IRQ1				0x80
#define PIA_IRQ2				0x40

#define IRQ1_ENABLED(c)       (c & 0x01)
#define IRQ1_DISABLED(c)      (!(c & 0x01))
#define C1_LOW_TO_HIGH(c)     (c & 0x02)
#define C1_HIGH_TO_LOW(c)     (!(c & 0x02))
#define OUTPUT_SELECTED(c)    (c & 0x04)
#define DDR_SELECTED(c)       (!(c & 0x04))
#define IRQ2_ENABLED(c)       (c & 0x08)
#define IRQ2_DISABLED(c)      (!(c & 0x08))
#define STROBE_E_RESET(c)     (c & 0x08)
#define STROBE_C1_RESET(c)    (!(c & 0x08))
#define SET_C2(c)             (c & 0x08)
#define RESET_C2(c)           (!(c & 0x08))
#define C2_LOW_TO_HIGH(c)     (c & 0x10)
#define C2_HIGH_TO_LOW(c)     (!(c & 0x10))
#define C2_SET_MODE(c)        (c & 0x10)
#define C2_STROBE_MODE(c)     (!(c & 0x10))
#define C2_OUTPUT(c)          (c & 0x20)
#define C2_INPUT(c)           (!(c & 0x20))




/******************* static variables *******************/

static struct pia6821 pia[MAX_PIA];
static int pia_offsets[8];



/******************* startup *******************/

int pia_startup (struct pia6821_interface *intf)
{
	int i;

	memset (pia, 0, sizeof (pia));
	for (i = 0; i < intf->num; i++)
	{
		pia[i].in_a_func = intf->in_a_func[i];
		pia[i].in_ca1_func = intf->in_ca1_func[i];
		pia[i].in_ca2_func = intf->in_ca2_func[i];
		pia[i].out_a_func = intf->out_a_func[i];
		pia[i].out_ca2_func = intf->out_ca2_func[i];
		pia[i].irq_a_func = intf->irq_a_func[i];

		pia[i].in_b_func = intf->in_b_func[i];
		pia[i].in_cb1_func = intf->in_cb1_func[i];
		pia[i].in_cb2_func = intf->in_cb2_func[i];
		pia[i].out_b_func = intf->out_b_func[i];
		pia[i].out_cb2_func = intf->out_cb2_func[i];
		pia[i].irq_b_func = intf->irq_b_func[i];
	}
	memcpy (pia_offsets, intf->offsets, sizeof (pia_offsets));

	return 1;
}


/******************* shutdown *******************/

void pia_shutdown (void)
{
}


/******************* CPU interface for PIA read *******************/

int pia_read (int which, int offset)
{
	struct pia6821 *p = pia + which;
	int val = 0;

	switch (pia_offsets[offset & 7])
	{
		/******************* port A output/DDR read *******************/
		case 0:

			/* read output register */
			if (OUTPUT_SELECTED (p->ctl_a))
			{
				/* update the input */
				if (p->in_a_func) p->in_a = p->in_a_func (0);

				/* combine input and output values */
				val = (p->out_a & p->ddr_a) + (p->in_a & ~p->ddr_a);

				/* IRQ flags implicitly cleared by a read */
				p->irq_a1 = p->irq_a2 = 0;

				/* CA2 is configured as output and in read strobe mode */
				if (C2_OUTPUT (p->ctl_a) && C2_STROBE_MODE (p->ctl_a))
				{
					/* this will cause a transition low; call the output function if we're currently high */
					if (p->out_ca2)
						if (p->out_ca2_func) p->out_ca2_func (0, 0);
					p->out_ca2 = 0;

					/* if the CA2 strobe is cleared by the E, reset it right away */
					if (STROBE_E_RESET (p->ctl_a))
					{
						if (p->out_ca2_func) p->out_ca2_func (0, 1);
						p->out_ca2 = 1;
					}
				}
			}

			/* read DDR register */
			else
				val = p->ddr_a;
			break;

		/******************* port B output/DDR read *******************/
		case 1:

			/* read output register */
			if (OUTPUT_SELECTED (p->ctl_b))
			{
				/* update the input */
				if (p->in_b_func) p->in_b = p->in_b_func (0);

				/* combine input and output values */
				val = (p->out_b & p->ddr_b) + (p->in_b & ~p->ddr_b);

				/* IRQ flags implicitly cleared by a read */
				p->irq_b1 = p->irq_b2 = 0;
			}

			/* read DDR register */
			else
				val = p->ddr_b;
			break;

		/******************* port A control read *******************/
		case 2:

			/* Update CA1 & CA2 if callback exists, these in turn may update IRQ's */
			if (p->in_ca1_func) pia_set_input_ca1(which, p->in_ca1_func (0));
			if (p->in_ca2_func) pia_set_input_ca2(which, p->in_ca2_func (0));

			/* read control register */
			val = p->ctl_a;

			/* set the IRQ flags if we have pending IRQs */
			if (p->irq_a1) val |= PIA_IRQ1;
			if (p->irq_a2 && C2_INPUT (p->ctl_a)) val |= PIA_IRQ2;
			break;

		/******************* port B control read *******************/
		case 3:

			/* Update CB1 & CB2 if callback exists, these in turn may update IRQ's */
			if (p->in_cb1_func) pia_set_input_cb1(which, p->in_cb1_func (0));
			if (p->in_cb2_func) pia_set_input_cb2(which, p->in_cb2_func (0));

			/* read control register */
			val = p->ctl_b;

			/* set the IRQ flags if we have pending IRQs */
			if (p->irq_b1) val |= PIA_IRQ1;
			if (p->irq_b2 && C2_INPUT (p->ctl_b)) val |= PIA_IRQ2;
			break;
	}

	return val;
}


/******************* CPU interface for PIA write *******************/

void pia_write (int which, int offset, int data)
{
	struct pia6821 *p = pia + which;

	switch (pia_offsets[offset & 7])
	{
		/******************* port A output/DDR write *******************/
		case 0:

			/* write output register */
			if (OUTPUT_SELECTED (p->ctl_a))
			{
				/* update the output value */
				p->out_a = data & p->ddr_a;

				/* send it to the output function */
				if (p->out_a_func) p->out_a_func (0, p->out_a);
			}

			/* write DDR register */
			else
				p->ddr_a = data;
			break;

		/******************* port B output/DDR write *******************/
		case 1:

			/* write output register */
			if (OUTPUT_SELECTED (p->ctl_b))
			{
				/* update the output value */
				p->out_b = data & p->ddr_b;

				/* send it to the output function */
				if (p->out_b_func) p->out_b_func (0, p->out_b);

				/* CB2 is configured as output and in write strobe mode */
				if (C2_OUTPUT (p->ctl_b) && C2_STROBE_MODE (p->ctl_b))
				{
					/* this will cause a transition low; call the output function if we're currently high */
					if (p->out_cb2)
						if (p->out_cb2_func) p->out_cb2_func (0, 0);
					p->out_cb2 = 0;

					/* if the CB2 strobe is cleared by the E, reset it right away */
					if (STROBE_E_RESET (p->ctl_b))
					{
						if (p->out_cb2_func) p->out_cb2_func (0, 1);
						p->out_cb2 = 1;
					}
				}
			}

			/* write DDR register */
			else
				p->ddr_b = data;
			break;

		/******************* port A control write *******************/
		case 2:

			/* CA2 is configured as output and in set/reset mode */
			/* 10/22/98 - MAB/FMP - any C2_OUTPUT should affect CA2 */
//			if (C2_OUTPUT (data) && C2_SET_MODE (data))
			if (C2_OUTPUT (data))
			{
				/* determine the new value */
				int temp = SET_C2 (data) ? 1 : 0;

				/* if this creates a transition, call the CA2 output function */
				if (p->out_ca2 ^ temp)
					if (p->out_ca2_func) p->out_ca2_func (0, temp);

				/* set the new value */
				p->out_ca2 = temp;
			}

			/* update the control register */
			p->ctl_a = data;
			break;

		/******************* port B control write *******************/
		case 3:

			/* CB2 is configured as output and in set/reset mode */
			/* 10/22/98 - MAB/FMP - any C2_OUTPUT should affect CB2 */
//			if (C2_OUTPUT (data) && C2_SET_MODE (data))
			if (C2_OUTPUT (data))
			{
				/* determine the new value */
				int temp = SET_C2 (data) ? 1 : 0;

				/* if this creates a transition, call the CA2 output function */
				if (p->out_cb2 ^ temp)
					if (p->out_cb2_func) p->out_cb2_func (0, temp);

				/* set the new value */
				p->out_cb2 = temp;
			}

			/* update the control register */
			p->ctl_b = data;
			break;
	}
}


/******************* interface setting PIA port A input *******************/

void pia_set_input_a (int which, int data)
{
	struct pia6821 *p = pia + which;

	/* set the input, what could be easier? */
	p->in_a = data;
}



/******************* interface setting PIA port CA1 input *******************/

void pia_set_input_ca1 (int which, int data)
{
	struct pia6821 *p = pia + which;

	/* limit the data to 0 or 1 */
	data = data ? 1 : 0;

	/* the new state has caused a transition */
	if (p->in_ca1 ^ data)
	{
		/* handle the active transition */
		if ((data && C1_LOW_TO_HIGH (p->ctl_a)) || (!data && C1_HIGH_TO_LOW (p->ctl_a)))
		{
			/* mark the IRQ */
			p->irq_a1 = 1;

			/* call the IRQ function if enabled */
			if (IRQ1_ENABLED (p->ctl_a))
				if (p->irq_a_func) p->irq_a_func ();

			/* CA2 is configured as output and in read strobe mode and cleared by a CA1 transition */
			if (C2_OUTPUT (p->ctl_a) && C2_STROBE_MODE (p->ctl_a) && STROBE_C1_RESET (p->ctl_a))
			{
				/* call the CA2 output function */
				if (!p->out_ca2)
					if (p->out_ca2_func) p->out_ca2_func (0, 1);

				/* clear CA2 */
				p->out_ca2 = 1;
			}
		}
	}

	/* set the new value for CA1 */
	p->in_ca1 = data;
}



/******************* interface setting PIA port CA2 input *******************/

void pia_set_input_ca2 (int which, int data)
{
	struct pia6821 *p = pia + which;

	/* limit the data to 0 or 1 */
	data = data ? 1 : 0;

	/* CA2 is in input mode */
	if (C2_INPUT (p->ctl_a))
	{
		/* the new state has caused a transition */
		if (p->in_ca2 ^ data)
		{
			/* handle the active transition */
			if ((data && C2_LOW_TO_HIGH (p->ctl_a)) || (!data && C2_HIGH_TO_LOW (p->ctl_a)))
			{
				/* mark the IRQ */
				p->irq_a2 = 1;

				/* call the IRQ function if enabled */
				if (IRQ2_ENABLED (p->ctl_a))
					if (p->irq_a_func) p->irq_a_func ();
			}
		}
	}

	/* set the new value for CA2 */
	p->in_ca2 = data;
}



/******************* interface setting PIA port B input *******************/

void pia_set_input_b (int which, int data)
{
	struct pia6821 *p = pia + which;

	/* set the input, what could be easier? */
	p->in_b = data;
}



/******************* interface setting PIA port CB1 input *******************/

void pia_set_input_cb1 (int which, int data)
{
	struct pia6821 *p = pia + which;

	/* limit the data to 0 or 1 */
	data = data ? 1 : 0;

	/* the new state has caused a transition */
	if (p->in_cb1 ^ data)
	{
		/* handle the active transition */
		if ((data && C1_LOW_TO_HIGH (p->ctl_b)) || (!data && C1_HIGH_TO_LOW (p->ctl_b)))
		{
			/* mark the IRQ */
			p->irq_b1 = 1;

			/* call the IRQ function if enabled */
			if (IRQ1_ENABLED (p->ctl_b))
				if (p->irq_b_func) p->irq_b_func ();

			/* CB2 is configured as output and in write strobe mode and cleared by a CA1 transition */
			if (C2_OUTPUT (p->ctl_b) && C2_STROBE_MODE (p->ctl_b) && STROBE_C1_RESET (p->ctl_b))
			{
				/* the IRQ1 flag must have also been cleared */
				if (!p->irq_b1)
				{
					/* call the CB2 output function */
					if (!p->out_cb2)
						if (p->out_cb2_func) p->out_cb2_func (0, 1);

					/* clear CB2 */
					p->out_cb2 = 1;
				}
			}
		}
	}

	/* set the new value for CB1 */
	p->in_cb1 = data;
}



/******************* interface setting PIA port CB2 input *******************/

void pia_set_input_cb2 (int which, int data)
{
	struct pia6821 *p = pia + which;

	/* limit the data to 0 or 1 */
	data = data ? 1 : 0;

	/* CB2 is in input mode */
	if (C2_INPUT (p->ctl_b))
	{
		/* the new state has caused a transition */
		if (p->in_cb2 ^ data)
		{
			/* handle the active transition */
			if ((data && C2_LOW_TO_HIGH (p->ctl_b)) || (!data && C2_HIGH_TO_LOW (p->ctl_b)))
			{
				/* mark the IRQ */
				p->irq_b2 = 1;

				/* call the IRQ function if enabled */
				if (IRQ2_ENABLED (p->ctl_b))
					if (p->irq_b_func) p->irq_b_func ();
			}
		}
	}

	/* set the new value for CA2 */
	p->in_cb2 = data;
}



/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

int pia_1_r (int offset) { return pia_read (0, offset); }
int pia_2_r (int offset) { return pia_read (1, offset); }
int pia_3_r (int offset) { return pia_read (2, offset); }
int pia_4_r (int offset) { return pia_read (3, offset); }
int pia_5_r (int offset) { return pia_read (4, offset); }
int pia_6_r (int offset) { return pia_read (5, offset); }
int pia_7_r (int offset) { return pia_read (6, offset); }
int pia_8_r (int offset) { return pia_read (7, offset); }

void pia_1_w (int offset, int data) { pia_write (0, offset, data); }
void pia_2_w (int offset, int data) { pia_write (1, offset, data); }
void pia_3_w (int offset, int data) { pia_write (2, offset, data); }
void pia_4_w (int offset, int data) { pia_write (3, offset, data); }
void pia_5_w (int offset, int data) { pia_write (4, offset, data); }
void pia_6_w (int offset, int data) { pia_write (5, offset, data); }
void pia_7_w (int offset, int data) { pia_write (6, offset, data); }
void pia_8_w (int offset, int data) { pia_write (7, offset, data); }

/******************* 8-bit A/B port interfaces *******************/

void pia_1_porta_w (int offset, int data) { pia_set_input_a (0, data); }
void pia_2_porta_w (int offset, int data) { pia_set_input_a (1, data); }
void pia_3_porta_w (int offset, int data) { pia_set_input_a (2, data); }
void pia_4_porta_w (int offset, int data) { pia_set_input_a (3, data); }
void pia_5_porta_w (int offset, int data) { pia_set_input_a (4, data); }
void pia_6_porta_w (int offset, int data) { pia_set_input_a (5, data); }
void pia_7_porta_w (int offset, int data) { pia_set_input_a (6, data); }
void pia_8_porta_w (int offset, int data) { pia_set_input_a (7, data); }

void pia_1_portb_w (int offset, int data) { pia_set_input_b (0, data); }
void pia_2_portb_w (int offset, int data) { pia_set_input_b (1, data); }
void pia_3_portb_w (int offset, int data) { pia_set_input_b (2, data); }
void pia_4_portb_w (int offset, int data) { pia_set_input_b (3, data); }
void pia_5_portb_w (int offset, int data) { pia_set_input_b (4, data); }
void pia_6_portb_w (int offset, int data) { pia_set_input_b (5, data); }
void pia_7_portb_w (int offset, int data) { pia_set_input_b (6, data); }
void pia_8_portb_w (int offset, int data) { pia_set_input_b (7, data); }

int pia_1_porta_r (int offset) { return pia[0].in_a; }
int pia_2_porta_r (int offset) { return pia[1].in_a; }
int pia_3_porta_r (int offset) { return pia[2].in_a; }
int pia_4_porta_r (int offset) { return pia[3].in_a; }
int pia_5_porta_r (int offset) { return pia[4].in_a; }
int pia_6_porta_r (int offset) { return pia[5].in_a; }
int pia_7_porta_r (int offset) { return pia[6].in_a; }
int pia_8_porta_r (int offset) { return pia[7].in_a; }

int pia_1_portb_r (int offset) { return pia[0].in_b; }
int pia_2_portb_r (int offset) { return pia[1].in_b; }
int pia_3_portb_r (int offset) { return pia[2].in_b; }
int pia_4_portb_r (int offset) { return pia[3].in_b; }
int pia_5_portb_r (int offset) { return pia[4].in_b; }
int pia_6_portb_r (int offset) { return pia[5].in_b; }
int pia_7_portb_r (int offset) { return pia[6].in_b; }
int pia_8_portb_r (int offset) { return pia[7].in_b; }

/******************* 1-bit CA1/CA2/CB1/CB2 port interfaces *******************/

void pia_1_ca1_w (int offset, int data) { pia_set_input_ca1 (0, data); }
void pia_2_ca1_w (int offset, int data) { pia_set_input_ca1 (1, data); }
void pia_3_ca1_w (int offset, int data) { pia_set_input_ca1 (2, data); }
void pia_4_ca1_w (int offset, int data) { pia_set_input_ca1 (3, data); }
void pia_5_ca1_w (int offset, int data) { pia_set_input_ca1 (4, data); }
void pia_6_ca1_w (int offset, int data) { pia_set_input_ca1 (5, data); }
void pia_7_ca1_w (int offset, int data) { pia_set_input_ca1 (6, data); }
void pia_8_ca1_w (int offset, int data) { pia_set_input_ca1 (7, data); }
void pia_1_ca2_w (int offset, int data) { pia_set_input_ca2 (0, data); }
void pia_2_ca2_w (int offset, int data) { pia_set_input_ca2 (1, data); }
void pia_3_ca2_w (int offset, int data) { pia_set_input_ca2 (2, data); }
void pia_4_ca2_w (int offset, int data) { pia_set_input_ca2 (3, data); }
void pia_5_ca2_w (int offset, int data) { pia_set_input_ca2 (4, data); }
void pia_6_ca2_w (int offset, int data) { pia_set_input_ca2 (5, data); }
void pia_7_ca2_w (int offset, int data) { pia_set_input_ca2 (6, data); }
void pia_8_ca2_w (int offset, int data) { pia_set_input_ca2 (7, data); }

void pia_1_cb1_w (int offset, int data) { pia_set_input_cb1 (0, data); }
void pia_2_cb1_w (int offset, int data) { pia_set_input_cb1 (1, data); }
void pia_3_cb1_w (int offset, int data) { pia_set_input_cb1 (2, data); }
void pia_4_cb1_w (int offset, int data) { pia_set_input_cb1 (3, data); }
void pia_5_cb1_w (int offset, int data) { pia_set_input_cb1 (4, data); }
void pia_6_cb1_w (int offset, int data) { pia_set_input_cb1 (5, data); }
void pia_7_cb1_w (int offset, int data) { pia_set_input_cb1 (6, data); }
void pia_8_cb1_w (int offset, int data) { pia_set_input_cb1 (7, data); }
void pia_1_cb2_w (int offset, int data) { pia_set_input_cb2 (0, data); }
void pia_2_cb2_w (int offset, int data) { pia_set_input_cb2 (1, data); }
void pia_3_cb2_w (int offset, int data) { pia_set_input_cb2 (2, data); }
void pia_4_cb2_w (int offset, int data) { pia_set_input_cb2 (3, data); }
void pia_5_cb2_w (int offset, int data) { pia_set_input_cb2 (4, data); }
void pia_6_cb2_w (int offset, int data) { pia_set_input_cb2 (5, data); }
void pia_7_cb2_w (int offset, int data) { pia_set_input_cb2 (6, data); }
void pia_8_cb2_w (int offset, int data) { pia_set_input_cb2 (7, data); }

int pia_1_ca1_r (int offset) { return pia[0].in_ca1; }
int pia_2_ca1_r (int offset) { return pia[1].in_ca1; }
int pia_3_ca1_r (int offset) { return pia[2].in_ca1; }
int pia_4_ca1_r (int offset) { return pia[3].in_ca1; }
int pia_5_ca1_r (int offset) { return pia[4].in_ca1; }
int pia_6_ca1_r (int offset) { return pia[5].in_ca1; }
int pia_7_ca1_r (int offset) { return pia[6].in_ca1; }
int pia_8_ca1_r (int offset) { return pia[7].in_ca1; }
int pia_1_ca2_r (int offset) { return pia[0].in_ca2; }
int pia_2_ca2_r (int offset) { return pia[1].in_ca2; }
int pia_3_ca2_r (int offset) { return pia[2].in_ca2; }
int pia_4_ca2_r (int offset) { return pia[3].in_ca2; }
int pia_5_ca2_r (int offset) { return pia[4].in_ca2; }
int pia_6_ca2_r (int offset) { return pia[5].in_ca2; }
int pia_7_ca2_r (int offset) { return pia[6].in_ca2; }
int pia_8_ca2_r (int offset) { return pia[7].in_ca2; }

int pia_1_cb1_r (int offset) { return pia[0].in_cb1; }
int pia_2_cb1_r (int offset) { return pia[1].in_cb1; }
int pia_3_cb1_r (int offset) { return pia[2].in_cb1; }
int pia_4_cb1_r (int offset) { return pia[3].in_cb1; }
int pia_5_cb1_r (int offset) { return pia[4].in_cb1; }
int pia_6_cb1_r (int offset) { return pia[5].in_cb1; }
int pia_7_cb1_r (int offset) { return pia[6].in_cb1; }
int pia_8_cb1_r (int offset) { return pia[7].in_cb1; }
int pia_1_cb2_r (int offset) { return pia[0].in_cb2; }
int pia_2_cb2_r (int offset) { return pia[1].in_cb2; }
int pia_3_cb2_r (int offset) { return pia[2].in_cb2; }
int pia_4_cb2_r (int offset) { return pia[3].in_cb2; }
int pia_5_cb2_r (int offset) { return pia[4].in_cb2; }
int pia_6_cb2_r (int offset) { return pia[5].in_cb2; }
int pia_7_cb2_r (int offset) { return pia[6].in_cb2; }
int pia_8_cb2_r (int offset) { return pia[7].in_cb2; }
