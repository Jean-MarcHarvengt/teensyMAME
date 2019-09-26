#include "driver.h"

extern struct MachineDriver pacman_driver;
extern struct MachineDriver mspacman_driver;
extern struct MachineDriver crush_driver;
extern struct MachineDriver pengo_driver;
extern struct MachineDriver ladybug_driver;


struct MachineDriver *drivers[] =
{
	&pacman_driver,
	&mspacman_driver,
	&crush_driver,
	&pengo_driver,
	&ladybug_driver,
	0	/* end of array */
};
