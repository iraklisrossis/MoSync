#include "config_platform.h"
#include "helpers/helpers.h"
#include "NativeUI.h"
#include "Cascade.h"
#include "ThreadPoolImpl.h"
#include "bb10err.h"
#include "event.h"
#include "MemStream.h"
#include "Syscall.h"

#include <hash_map>
#include <assert.h>
#include <unistd.h>
#include <limits.h>

#include <bb/cascades/Application>
#include <bb/cascades/Button>
#include <bb/cascades/Page>
#include <bb/cascades/TitleBar>
#include <bb/cascades/WebView>
#include <bb/AbstractBpsEventHandler>
#include <bb/device/DisplayInfo>

#include <bps/event.h>

#include "slots.h"
#include "../build/moc_slots.h.cpp"

#define APP (*Application::instance())

using namespace bb::cascades;
using namespace std;

enum WidgetType {
	eScreen,
	eWebView,
	eButton,
};

enum WidgetFunction {
	EVENT_CODE_WIDGET_CREATE,
	EVENT_CODE_WIDGET_ADD_CHILD,
	EVENT_CODE_WIDGET_SET_PROPERTY,
	EVENT_CODE_WIDGET_SCREEN_SHOW,
};

struct Widget {
	UIObject* o;
	WidgetType type;
	Handler* handler;
};

static int nuiFunction(int code, const bps_event_payload_t&);

static hash_map<MAWidgetHandle, Widget> sWidgets;
static MAWidgetHandle sNextWidgetHandle = 1;
static int sNuiEventDomain;
static int sNuiEventChannel;
static int sNuiReturnValue;
static MoSyncSemaphore sNuiSem;

class BpsEventHandler : public bb::AbstractBpsEventHandler {
public:
	BpsEventHandler() {
		subscribe(sNuiEventDomain);
	}
	virtual ~BpsEventHandler() {
		unsubscribe(sNuiEventDomain);
	}
	virtual void event(bps_event_t* e) {
		assert(bps_event_get_domain(e) == sNuiEventDomain);
		//LOG("Got Nui event %i.\n", bps_event_get_code(e));
		sNuiReturnValue = nuiFunction(bps_event_get_code(e), *bps_event_get_payload(e));
		sNuiSem.post();
	}
};


static int nuiCreate(const bps_event_payload_t& payload);
static int nuiAddChild(const bps_event_payload_t& payload);
static int nuiSetProperty(const bps_event_payload_t& payload);
static int nuiScreenShow(const bps_event_payload_t& payload);

int nuiFunction(int code, const bps_event_payload_t& payload) {
	switch(code) {
	case EVENT_CODE_WIDGET_CREATE: return nuiCreate(payload);
	case EVENT_CODE_WIDGET_ADD_CHILD: return nuiAddChild(payload);
	case EVENT_CODE_WIDGET_SET_PROPERTY: return nuiSetProperty(payload);
	case EVENT_CODE_WIDGET_SCREEN_SHOW: return nuiScreenShow(payload);
	default:
		DEBIG_PHAT_ERROR;
	}
}


int runCascadeApp(int argc, char** argv, int (*func)(void*)) {
	Application app(argc, argv);

	sNuiEventDomain = bps_register_domain();
	sNuiEventChannel = bps_channel_get_active();
	BpsEventHandler b;

	MoSyncThread t;
	t.start(func, NULL);

	return app.exec();
}

static const Widget& getWidget(MAWidgetHandle handle) {
	auto itr = sWidgets.find(handle);
	DEBUG_ASSERT(itr != sWidgets.end());
	return itr->second;
}

MAWidgetHandle maWidgetCreate(const char* widgetType) {
	LOG("maWidgetCreate(%s);\n", widgetType);
	bps_event_t* be;
	bps_event_payload_t payload = { (uintptr_t)widgetType, 0, 0 };
	BPSERR(bps_event_create(&be, sNuiEventDomain, EVENT_CODE_WIDGET_CREATE, &payload, NULL));
	BPSERR(bps_channel_push_event(sNuiEventChannel, be));
	sNuiSem.wait();
	return sNuiReturnValue;
}

static int nuiCreate(const bps_event_payload_t& payload) {
	const char* widgetType = (char*)payload.data1;
	UIObject* u = NULL;
	WidgetType t;
	Handler* h = NULL;
	if(strcmp(widgetType, MAW_SCREEN) == 0) {
		Page* page = Page::create();
		u = page;
		t = eScreen;
	}
	if(strcmp(widgetType, MAW_WEB_VIEW) == 0) {
		WebView* w = WebView::create();
		h = new Handler(sNextWidgetHandle);
		u = w;
		t = eWebView;
		assert(QObject::connect(w, SIGNAL(messageReceived(const QVariantMap&)), h, SLOT(messageReceived(const QVariantMap&))));
	}

	if(!u) {
		DEBIG_PHAT_ERROR;
	}
	LOG("Widget created: %i\n", sNextWidgetHandle);
	sWidgets[sNextWidgetHandle] = { u, t, h };
	return sNextWidgetHandle++;
}

struct HookMessageInfo : Functor {
	int type;
	MAWidgetHandle h;
	Base::MemStream* stream;
};

static void postHookMessage(const Functor&);

void Handler::messageReceived(const QVariantMap& map) {
#if 0
	LOG("messageReceived. keys:\n");
	auto keys = map.keys();
	for(auto itr=keys.begin(); itr!=keys.end(); ++itr) {
		LOG("%s\n", itr->toLocal8Bit().data());
	}
	LOG("url: %s\n", map["url"].toUrl().toString().toLocal8Bit().data());
#endif
	LOG("data: %s\n", map["data"].toString().toLocal8Bit().data());

	// extract UTF-8 data, store it in a data object.
	QByteArray a(map["data"].toString().toUtf8());
	Base::MemStream* s = new Base::MemStream(a.length());
	memcpy(s->ptr(), a.data(), a.length());

	// we need to construct a data object,
	// but that can only be done in the core thread.
	HookMessageInfo* hmi = (HookMessageInfo*)malloc(sizeof(HookMessageInfo));
	hmi->func = postHookMessage;
	hmi->h = mHandle;
	hmi->type = MAW_CONSTANT_MESSAGE;
	hmi->stream = s;
	executeInCoreThread(hmi);
}

// construct and push event
static void postHookMessage(const Functor& arg) {
	const HookMessageInfo& hmi = (const HookMessageInfo&)arg;

	MAHandle data = SYSCALL_THIS->resources.create_RT_PLACEHOLDER();
	int res = SYSCALL_THIS->resources.add_RT_BINARY(data, hmi.stream);
	DEBUG_ASSERT(res == RES_OK);

	MAWidgetEventData *ed = new MAWidgetEventData;
	ed->eventType = MAW_EVENT_WEB_VIEW_HOOK_INVOKED;
	ed->widgetHandle = hmi.h;
	ed->hookType = MAW_CONSTANT_MESSAGE;
	ed->urlData = data;

	MAEventNative event;
	event.type = EVENT_TYPE_WIDGET;
	event.data = ed;
	putEvent(event);
}

int maWidgetDestroy(MAWidgetHandle handle) {
	DEBIG_PHAT_ERROR;
}

int maWidgetAddChild(MAWidgetHandle parentHandle, MAWidgetHandle childHandle) {
	LOG("maWidgetAddChild(%i, %i);\n", parentHandle, childHandle);
	bps_event_t* be;
	bps_event_payload_t payload = { parentHandle, childHandle, 0 };
	BPSERR(bps_event_create(&be, sNuiEventDomain, EVENT_CODE_WIDGET_ADD_CHILD, &payload, NULL));
	BPSERR(bps_channel_push_event(sNuiEventChannel, be));
	sNuiSem.wait();
	return sNuiReturnValue;
}

static int nuiAddChild(const bps_event_payload_t& payload) {
	MAWidgetHandle parentHandle = payload.data1;
	MAWidgetHandle childHandle = payload.data2;
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
	bps_event_t* be;
	bps_event_payload_t payload = { screenHandle, 0, 0 };
	BPSERR(bps_event_create(&be, sNuiEventDomain, EVENT_CODE_WIDGET_SCREEN_SHOW, &payload, NULL));
	BPSERR(bps_channel_push_event(sNuiEventChannel, be));
	sNuiSem.wait();
	return sNuiReturnValue;
}

static int nuiScreenShow(const bps_event_payload_t& payload) {
	MAWidgetHandle screenHandle = payload.data1;
	const Widget& s = getWidget(screenHandle);
	DEBUG_ASSERT(s.type == eScreen);
	APP.setScene((Page*)s.o);
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
	bps_event_t* be;
	bps_event_payload_t payload = { handle, (uintptr_t)key, (uintptr_t)value };
	BPSERR(bps_event_create(&be, sNuiEventDomain, EVENT_CODE_WIDGET_SET_PROPERTY, &payload, NULL));
	BPSERR(bps_channel_push_event(sNuiEventChannel, be));
	sNuiSem.wait();
	return sNuiReturnValue;
}

#if 0
static bool begin_with(const char* what, const char* with) {
	return strncmp(what, with, strlen(with)) == 0;
}
#endif

static bool include(const char* what, const char* inc) {
	return strstr(what, inc) != NULL;
}

static int nuiSetProperty(const bps_event_payload_t& payload) {
	MAWidgetHandle handle = payload.data1;
	const char* key = (char*)payload.data2;
	const char* value = (char*)payload.data3;
	const Widget& w = getWidget(handle);
	switch(w.type) {
	case eWebView:
		{
			WebView* ww = (WebView*)w.o;
			if(strcmp(key, MAW_WEB_VIEW_URL) == 0) {
				/*static const char js[] = "javascript:";
				if(begin_with(value, js)) {
					LOG("evaluateJavaScript(\"%s\")\n", value + strlen(js));
					ww->evaluateJavaScript(value + strlen(js));
				} else*/
				if(include(value, ":")) {
					ww->setUrl(QUrl(value));
				} else {
					static const QString p("file://"), d("/data/");
					char cwd[PATH_MAX];
					NULL_ERRNO(getcwd(cwd, PATH_MAX));

					//LOG("URL: %s\n", cwd);
					ww->setUrl(QUrl(p + cwd + d + value));
				}
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

MAExtent getScrSize() {
	bb::device::DisplayInfo display;
	return EXTENT(display.pixelSize().width(), display.pixelSize().height());
}
