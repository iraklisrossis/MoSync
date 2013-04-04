#include "helpers/cpp_defs.h"
#include "helpers/CPP_IX_WIDGET.h"

MAWidgetHandle maWidgetCreate(const char* widgetType);
int maWidgetDestroy(MAWidgetHandle handle);
int maWidgetAddChild(MAWidgetHandle parentHandle, MAHandle childHandle);
int maWidgetInsertChild(MAWidgetHandle parentHandle, MAWidgetHandle childHandle, int index);
int maWidgetRemoveChild(MAWidgetHandle childHandle);
int maWidgetModalDialogShow(MAWidgetHandle dialogHandle);
int maWidgetModalDialogHide(MAWidgetHandle dialogHandle);
int maWidgetScreenShow(MAWidgetHandle screenHandle);
int maWidgetScreenShowWithTransition(MAWidgetHandle screenHandle, MAWScreenTransitionType screenTransitionType, int screenTransitionDuration);
int maWidgetStackScreenPush(MAWidgetHandle stackScreenHandle, MAWidgetHandle screenHandle);
int maWidgetStackScreenPop(MAWidgetHandle stackScreenHandle);
int maWidgetSetProperty(MAWidgetHandle handle, const char* property, const char* value);
int maWidgetGetProperty(MAWidgetHandle handle, const char* property, char* value, int maxSize);
int maWidgetScreenAddOptionsMenuItem(MAWidgetHandle widget, const char*  title,
	const char* iconHandle, int iconPredefined);

MAExtent getScrSize();
