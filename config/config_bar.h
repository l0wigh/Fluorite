#include <stdlib.h>
#include <string.h>

#define	BAR_ENABLED				True
// TOP | BOTTOM
#define POSITION				TOP
#define BAR_HEIGHT				35
// GAPS is applied on left and right
#define BAR_GAPS				0
// POS_GAP is based on the position. If on top, the bar will be lower
#define	BAR_POS_GAP				0
#define BAR_BACKGROUND			0x1e1e1e
#define BAR_DEFAULT_FOREGROUND	"#ffffff"
#define BAR_ACCENT_FOREGROUND	"#35e5dc"
#define BAR_TRANSPARENT			False
#define BAR_REFRESH				1
#define BAR_TITLE				"Fluorite 0.2"
#define BAR_TEXT_GAP			10

// The size should be the same
static const char bar_font[] = "VictorMono NF:size=18";
#define BAR_FONT_SIZE	18

static const char *custom_workspaces[10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
#define BAR_WORKSPACE_GAP	10

typedef struct
{
	char name[255];
	char command[255];
	/* unsigned long foreground; */
	/* unsigned long background; */
} basic_module;

static const basic_module clock_module = {
	/* .name =  "  ", */
	.name =  "",
	.command = "date | cut -d ' ' -f 5 | rev | sed 's/:/-/' | rev | cut -d '-' -f 1 | tr -d \\\\n",
	/* .foreground = 0xffffff, */
	/* .background = 0x1e1e1e, */
};

static const basic_module monbrightness_module = {
	.name =  "󰖚  ",
	.command = "brightnessctl | grep Current | cut -d ' ' -f 4 | sed 's/(//' | sed 's/)//' | tr -d \\\\n",
	/* .foreground = 0xffffff, */
	/* .background = 0x1e1e1e, */
};

static const basic_module battery_module = {
	.name =  "",
	.command = "/home/thomas/projects/x11windows/Fluorite/scripts/bat_fluorite.sh",
	/* .foreground = 0xffffff, */
	/* .background = 0x1e1e1e, */
};

static const basic_module volume_module = {
	.name =  "  ",
	.command = "/home/thomas/projects/x11windows/Fluorite/scripts/vol_fluorite.sh",
	/* .foreground = 0xffffff, */
	/* .background = 0x1e1e1e, */
};

static const basic_module wifi_module = {
	.name =  "",
	.command = "/home/thomas/projects/x11windows/Fluorite/scripts/wifi_fluorite.sh",
	/* .foreground = 0xffffff, */
	/* .background = 0x1e1e1e, */
};

#define BAR_MODULE_COUNT		5
#define BAR_MODULE_SEPARATOR	"  "
// Use this to fix margin issues that can happen with nerdfont or just to add right margin
#define BAR_MODULE_OFFSET		290

static const basic_module user_modules[] = {
	wifi_module,
	monbrightness_module,
	volume_module,
	battery_module,
	clock_module,
};
