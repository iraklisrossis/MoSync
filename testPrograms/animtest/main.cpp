#include <ma.h>

MAExtent scrSize;

static void eventLoop()
{
	int animLoc = 0;
	MAEvent event;
	BOOL appRunning = TRUE;
	while (appRunning)
	{
		if ((animLoc + 100) >= EXTENT_X(scrSize))
		{
			maSetColor(0x000000);
			maFillRect(animLoc, 300, 100, 100);
			animLoc = 0;
		}
		maSetColor(0x000000);
		maFillRect(animLoc, 300, 100, 100);
		maSetColor(0x0000ff);
		animLoc = animLoc + 1;
		maFillRect(animLoc, 300, 100, 100);
		maUpdateScreen();

		maWait(10);
		while (maGetEvent(&event))
		{
			if (EVENT_TYPE_CLOSE == event.type)
			{
				appRunning = FALSE;
				break;
			}
			else if (EVENT_TYPE_KEY_PRESSED == event.type)
			{
				if (event.key == MAK_BACK)
				{
					appRunning = FALSE;
					break;
				}
			}
		}
	}
}

int MAMain()
{
	maSetDrawTarget(HANDLE_SCREEN);
	scrSize = maGetScrSize();
	eventLoop();
	return 0;
}
