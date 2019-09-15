/**********************************************************************

	Motorola 6821 PIA interface and emulation

	This function emulates all the functionality of up to 4 M6821
	peripheral interface adapters.

**********************************************************************/


#define MAX_PIA 8

#define PIA_NOP -1
#define PIA_DDRA 0
#define PIA_DDRB 1
#define PIA_CTLA 2
#define PIA_CTLB 3

struct pia6821_interface
{
	int num;
	int offsets[8];

	int (*in_a_func[MAX_PIA])(int offset);
	int (*in_ca1_func[MAX_PIA])(int offset);
	int (*in_ca2_func[MAX_PIA])(int offset);
	int (*in_b_func[MAX_PIA])(int offset);
	int (*in_cb1_func[MAX_PIA])(int offset);
	int (*in_cb2_func[MAX_PIA])(int offset);
	void (*out_a_func[MAX_PIA])(int offset, int val);
	void (*out_b_func[MAX_PIA])(int offset, int val);
	void (*out_ca2_func[MAX_PIA])(int offset, int val);
	void (*out_cb2_func[MAX_PIA])(int offset, int val);
	void (*irq_a_func[MAX_PIA])(void);
	void (*irq_b_func[MAX_PIA])(void);
};
typedef struct pia6821_interface pia6821_interface;


int pia_startup (struct pia6821_interface *intf);
void pia_shutdown (void);
int pia_read (int which, int offset);
void pia_write (int which, int offset, int data);
void pia_set_input_a (int which, int data);
void pia_set_input_ca1 (int which, int data);
void pia_set_input_ca2 (int which, int data);
void pia_set_input_b (int which, int data);
void pia_set_input_cb1 (int which, int data);
void pia_set_input_cb2 (int which, int data);

/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

int pia_1_r (int offset);
int pia_2_r (int offset);
int pia_3_r (int offset);
int pia_4_r (int offset);
int pia_5_r (int offset);
int pia_6_r (int offset);
int pia_7_r (int offset);
int pia_8_r (int offset);

void pia_1_w (int offset, int data);
void pia_2_w (int offset, int data);
void pia_3_w (int offset, int data);
void pia_4_w (int offset, int data);
void pia_5_w (int offset, int data);
void pia_6_w (int offset, int data);
void pia_7_w (int offset, int data);
void pia_8_w (int offset, int data);

/******************* 8-bit A/B port interfaces *******************/

void pia_1_porta_w (int offset, int data);
void pia_2_porta_w (int offset, int data);
void pia_3_porta_w (int offset, int data);
void pia_4_porta_w (int offset, int data);
void pia_5_porta_w (int offset, int data);
void pia_6_porta_w (int offset, int data);
void pia_7_porta_w (int offset, int data);
void pia_8_porta_w (int offset, int data);

void pia_1_portb_w (int offset, int data);
void pia_2_portb_w (int offset, int data);
void pia_3_portb_w (int offset, int data);
void pia_4_portb_w (int offset, int data);
void pia_5_portb_w (int offset, int data);
void pia_6_portb_w (int offset, int data);
void pia_7_portb_w (int offset, int data);
void pia_8_portb_w (int offset, int data);

int pia_1_porta_r (int offset);
int pia_2_porta_r (int offset);
int pia_3_porta_r (int offset);
int pia_4_porta_r (int offset);
int pia_5_porta_r (int offset);
int pia_6_porta_r (int offset);
int pia_7_porta_r (int offset);
int pia_8_porta_r (int offset);

int pia_1_portb_r (int offset);
int pia_2_portb_r (int offset);
int pia_3_portb_r (int offset);
int pia_4_portb_r (int offset);
int pia_5_portb_r (int offset);
int pia_6_portb_r (int offset);
int pia_7_portb_r (int offset);
int pia_8_portb_r (int offset);

/******************* 1-bit CA1/CA2/CB1/CB2 port interfaces *******************/

void pia_1_ca1_w (int offset, int data);
void pia_2_ca1_w (int offset, int data);
void pia_3_ca1_w (int offset, int data);
void pia_4_ca1_w (int offset, int data);
void pia_5_ca1_w (int offset, int data);
void pia_6_ca1_w (int offset, int data);
void pia_7_ca1_w (int offset, int data);
void pia_8_ca1_w (int offset, int data);
void pia_1_ca2_w (int offset, int data);
void pia_2_ca2_w (int offset, int data);
void pia_3_ca2_w (int offset, int data);
void pia_4_ca2_w (int offset, int data);
void pia_5_ca2_w (int offset, int data);
void pia_6_ca2_w (int offset, int data);
void pia_7_ca2_w (int offset, int data);
void pia_8_ca2_w (int offset, int data);

void pia_1_cb1_w (int offset, int data);
void pia_2_cb1_w (int offset, int data);
void pia_3_cb1_w (int offset, int data);
void pia_4_cb1_w (int offset, int data);
void pia_5_cb1_w (int offset, int data);
void pia_6_cb1_w (int offset, int data);
void pia_7_cb1_w (int offset, int data);
void pia_8_cb1_w (int offset, int data);
void pia_1_cb2_w (int offset, int data);
void pia_2_cb2_w (int offset, int data);
void pia_3_cb2_w (int offset, int data);
void pia_4_cb2_w (int offset, int data);
void pia_5_cb2_w (int offset, int data);
void pia_6_cb2_w (int offset, int data);
void pia_7_cb2_w (int offset, int data);
void pia_8_cb2_w (int offset, int data);

int pia_1_ca1_r (int offset);
int pia_2_ca1_r (int offset);
int pia_3_ca1_r (int offset);
int pia_4_ca1_r (int offset);
int pia_5_ca1_r (int offset);
int pia_6_ca1_r (int offset);
int pia_7_ca1_r (int offset);
int pia_8_ca1_r (int offset);
int pia_1_ca2_r (int offset);
int pia_2_ca2_r (int offset);
int pia_3_ca2_r (int offset);
int pia_4_ca2_r (int offset);
int pia_5_ca2_r (int offset);
int pia_6_ca2_r (int offset);
int pia_7_ca2_r (int offset);
int pia_8_ca2_r (int offset);

int pia_1_cb1_r (int offset);
int pia_2_cb1_r (int offset);
int pia_3_cb1_r (int offset);
int pia_4_cb1_r (int offset);
int pia_5_cb1_r (int offset);
int pia_6_cb1_r (int offset);
int pia_7_cb1_r (int offset);
int pia_8_cb1_r (int offset);
int pia_1_cb2_r (int offset);
int pia_2_cb2_r (int offset);
int pia_3_cb2_r (int offset);
int pia_4_cb2_r (int offset);
int pia_5_cb2_r (int offset);
int pia_6_cb2_r (int offset);
int pia_7_cb2_r (int offset);
int pia_8_cb2_r (int offset);
