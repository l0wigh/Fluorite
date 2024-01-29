#include <stdlib.h>
#include <string.h>

#define	BAR_ENABLED		True
// TOP | BOTTOM
#define POSITION		TOP
#define BAR_HEIGHT		50
// GAPS is applied on left and right
#define BAR_GAPS		0
// POS_GAP is based on the position. If on top, the bar will be lower
#define	BAR_POS_GAP		0
#define BAR_BACKGROUND	0x1e1e1e
#define BAR_FOREGROUND	"#de4a2c"
#define BAR_TRANSPARENT False
#define BAR_REFRESH		1

// The size should be the same
static const char bar_font[] = "VictorMono NF:size=20";
#define BAR_FONT_SIZE	20

typedef struct
{
	char name[255];
	char command[255];
	/* unsigned long foreground; */
	/* unsigned long background; */
} basic_module;

static const basic_module clock_module = {
	.name =  "  ",
	.command = "date | cut -d ' ' -f 5",
	/* .foreground = 0xffffff, */
	/* .background = 0x1e1e1e, */
};

static const basic_module monbrightness_module = {
	.name =  "󰖚  ",
	.command = "brightnessctl | grep Current | cut -d ' ' -f 4 | sed 's/(//' | sed 's/)//'",
	/* .foreground = 0xffffff, */
	/* .background = 0x1e1e1e, */
};

#define BAR_MODULE_COUNT		2
#define BAR_MODULE_SEPARATOR	" ┇ "
// Use this to fix margin issues that can happen with nerdfont or just to add right margin
#define BAR_MODULE_OFFSET		100

static const basic_module user_modules[] = {
	monbrightness_module,
	clock_module,
};
