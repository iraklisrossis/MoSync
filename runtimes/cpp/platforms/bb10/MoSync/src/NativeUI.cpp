#include "config_platform.h"
#include "helpers/helpers.h"
#include "NativeUI.h"

#include <hash_map>

#include <bb/cascades/Application>
#include <bb/cascades/Button>
#include <bb/cascades/Page>
#include <bb/cascades/TitleBar>
#include <bb/cascades/WebView>

using namespace bb::cascades;
using namespace std;

enum WidgetType {
	eScreen,
	eWebView,
	eButton,
};

struct Widget {
	UIObject* o;
	WidgetType type;
};

static hash_map<MAWidgetHandle, Widget> sWidgets;
static MAWidgetHandle sNextWidgetHandle = 1;

static int argc = 0;
static Application sApp(argc, NULL);

static const Widget& getWidget(MAWidgetHandle handle) {
	auto itr = sWidgets.find(handle);
	DEBUG_ASSERT(itr != sWidgets.end());
	return itr->second;
}

MAWidgetHandle maWidgetCreate(const char* widgetType) {
	LOG("maWidgetCreate(%s);\n", widgetType);
	UIObject* u = NULL;
	WidgetType t;
	if(strcmp(widgetType, MAW_SCREEN) == 0) {
		Page* page = Page::create();
		u = page;
		t = eScreen;
#if 0
		TitleBar* modifyBar = TitleBar::create().title("Modify");
    page->setTitleBar(modifyBar);

		page = Page::create()
        .content(Button::create("Hello, World!"));
    sApp.setScene(page);
		sApp.exec();
#endif
	}
	if(strcmp(widgetType, MAW_WEB_VIEW) == 0) {
#if 1
		u = WebView::create();
		t = eWebView;
#else
		u = Button::create().text("Simple button");
		t = eButton;
#endif
	}

	if(!u) {
		DEBIG_PHAT_ERROR;
	}
	LOG("Widget created: %i\n", sNextWidgetHandle);
	sWidgets[sNextWidgetHandle] = { u, t };
	return sNextWidgetHandle++;
}

int maWidgetDestroy(MAWidgetHandle handle) {
	DEBIG_PHAT_ERROR;
}

int maWidgetAddChild(MAWidgetHandle parentHandle, MAHandle childHandle) {
	LOG("maWidgetAddChild(%i, %i);\n", parentHandle, childHandle);
	const Widget& p = getWidget(parentHandle);
	const Widget& c = getWidget(childHandle);
	if(p.type == eScreen) {
		Page* page = (Page*)p.o;
		page->setContent((Control*)c.o);	//HACK
	} else {
		return MAW_RES_INVALID_LAYOUT;
	}
	return MAW_RES_OK;
}

int maWidgetInsertChild(MAWidgetHandle parentHandle, MAWidgetHandle childHandle, int index) {
	LOG("maWidgetInsertChild(%i, %i, %i);\n", parentHandle, childHandle, index);
	return MAW_RES_OK;
}

int maWidgetRemoveChild(MAWidgetHandle childHandle) {
	DEBIG_PHAT_ERROR;
}

int maWidgetModalDialogShow(MAWidgetHandle dialogHandle) {
	DEBIG_PHAT_ERROR;
}

int maWidgetModalDialogHide(MAWidgetHandle dialogHandle) {
	DEBIG_PHAT_ERROR;
}

int maWidgetScreenShow(MAWidgetHandle screenHandle) {
	LOG("maWidgetScreenShow(%i);\n", screenHandle);
	const Widget& s = getWidget(screenHandle);
	DEBUG_ASSERT(s.type == eScreen);
	sApp.setScene((Page*)s.o);
	sApp.exec();
	return MAW_RES_OK;
}

int maWidgetStackScreenPush(MAWidgetHandle stackScreenHandle, MAWidgetHandle screenHandle) {
	DEBIG_PHAT_ERROR;
}

int maWidgetStackScreenPop(MAWidgetHandle stackScreenHandle) {
	DEBIG_PHAT_ERROR;
}

int maWidgetSetProperty(MAWidgetHandle handle, const char* key, const char* value) {
	LOG("maWidgetSetProperty(%i, %s, %s);\n", handle, key, value);
	const Widget& w = getWidget(handle);
	switch(w.type) {
	case eWebView:
		{
			WebView* ww = (WebView*)w.o;
			if(strcmp(key, MAW_WEB_VIEW_URL) == 0) {
				ww->setUrl(QUrl(value));
				return MAW_RES_OK;
			}
		}
		break;
	default:
		break;
	}
	return MAW_RES_INVALID_PROPERTY_NAME;
}

int maWidgetGetProperty(MAWidgetHandle handle, const char* key, char* value, int maxSize) {
	LOG("maWidgetGetProperty(%i, %s);\n", handle, key);
	DEBIG_PHAT_ERROR;
}

int maWidgetScreenAddOptionsMenuItem(MAWidgetHandle widget, const char* title,
	const char* iconHandle, int iconPredefined)
{
	DEBIG_PHAT_ERROR;
}
