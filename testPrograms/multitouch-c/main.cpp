#include <ma.h>
#include <mavsprintf.h>

int fingers[5][2];

static void eventLoop()
{
	int i;
	MAEvent event;
	BOOL isRunning = TRUE;
	int screenSize = maGetScrSize();

	while (isRunning)
	{
		maSetColor(0x000000);
		maFillRect(0, 0, EXTENT_X(screenSize), EXTENT_Y(screenSize));
		for (i = 0; i < 5 ; ++i)
		{
			if (fingers[i][0] != -1)
			{
				char strPos[16];
				sprintf(strPos, "%i", i);
				maSetColor(0xff0000);
				maFillRect(fingers[i][0] - 50, fingers[i][1] - 50, 100, 100);
				maSetColor(0xffffff);
				maDrawText(fingers[i][0] - 50, fingers[i][1] - 50, strPos);
			}
		}
		maUpdateScreen();

		maWait(0);
		while (maGetEvent(&event))
		{
			if (EVENT_TYPE_CLOSE == event.type)
			{
				isRunning = FALSE;
				break;
			}
			else if (EVENT_TYPE_KEY_PRESSED == event.type)
			{
				if (event.key == MAK_BACK)
				{
					isRunning = FALSE;
					break;
				}
			}
			else if (EVENT_TYPE_POINTER_PRESSED == event.type)
			{
				if (event.touchId < 5)
				{
					fingers[event.touchId][0] = event.point.x;
					fingers[event.touchId][1] = event.point.y;
				}
			}
			else if (EVENT_TYPE_POINTER_RELEASED == event.type)
			{
				if (event.touchId < 5)
				{
					fingers[event.touchId][0] = -1;
				}
			}
			else if (EVENT_TYPE_POINTER_DRAGGED == event.type)
			{
				if (event.touchId < 5)
				{
					fingers[event.touchId][0] = event.point.x;
					fingers[event.touchId][1] = event.point.y;
				}
			}
		}
	}
}

int MAMain()
{
	int i;
	for (i = 0; i < 5; i++) fingers[i][0] = -1;
	eventLoop();
	return 0;
}
