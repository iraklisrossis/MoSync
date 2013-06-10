#include "config_platform.h"
#include "helpers/helpers.h"
#include "NativeUI.h"
#include "Cascade.h"
#include "ThreadPoolImpl.h"
#include "bb10err.h"
#include "event.h"
#include "MemStream.h"
#include "Syscall.h"
#include "bbutil.h"

#include <hash_map>
#include <assert.h>
#include <unistd.h>
#include <limits.h>

#include <bb/cascades/AbsoluteLayout>
#include <bb/cascades/Application>
#include <bb/cascades/Button>
#include <bb/cascades/CheckBox>
#include <bb/cascades/Container>
#include <bb/cascades/ForeignWindowControl>
#include <bb/cascades/ImageButton>
#include <bb/cascades/ImageView>
#include <bb/cascades/Label>
#include <bb/cascades/ListView>
#include <bb/cascades/NavigationPane>
#include <bb/cascades/Page>
#include <bb/cascades/StackLayout>
#include <bb/cascades/StandardListItem>
#include <bb/cascades/TextField>
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
	eNull,
	ePage,
	eWebView,
	eButton,
	eLayout,
	eLabel,
	eTextField,
	eForeignWindowControl,
	eImageButton,
	eNavigationPane,
	eListView,
	eStandardListItem,
	eAbsoluteLayout,
	eCheckBox,
	eImageView,
};

enum WidgetFunction {
	EVENT_CODE_WIDGET_CREATE,
	EVENT_CODE_WIDGET_ADD_CHILD,
	EVENT_CODE_WIDGET_SET_PROPERTY,
	EVENT_CODE_WIDGET_SCREEN_SHOW,
	EVENT_CODE_STACK_SCREEN_PUSH,
	EVENT_CODE_STACK_SCREEN_POP,
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
static int nuiStackScreenPush(const bps_event_payload_t& payload);
static int nuiStackScreenPop(const bps_event_payload_t& payload);

int nuiFunction(int code, const bps_event_payload_t& payload) {
	switch(code) {
	case EVENT_CODE_WIDGET_CREATE: return nuiCreate(payload);
	case EVENT_CODE_WIDGET_ADD_CHILD: return nuiAddChild(payload);
	case EVENT_CODE_WIDGET_SET_PROPERTY: return nuiSetProperty(payload);
	case EVENT_CODE_WIDGET_SCREEN_SHOW: return nuiScreenShow(payload);
	case EVENT_CODE_STACK_SCREEN_PUSH: return nuiStackScreenPush(payload);
	case EVENT_CODE_STACK_SCREEN_POP: return nuiStackScreenPop(payload);
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

// run on UI thread.
static int run(WidgetFunction f, uintptr_t data1=0, uintptr_t data2=0, uintptr_t data3=0) {
	bps_event_t* be;
	bps_event_payload_t payload = { data1, data2, data3 };
	BPSERR(bps_event_create(&be, sNuiEventDomain, f, &payload, NULL));
	BPSERR(bps_channel_push_event(sNuiEventChannel, be));
	sNuiSem.wait();
	return sNuiReturnValue;
}

MAWidgetHandle maWidgetCreate(const char* widgetType) {
	LOG("maWidgetCreate(%s);\n", widgetType);
	return run(EVENT_CODE_WIDGET_CREATE, (uintptr_t)widgetType);
}

static int nuiCreate(const bps_event_payload_t& payload) {
	const char* widgetType = (char*)payload.data1;
	UIObject* u = NULL;
	WidgetType t = eNull;
	Handler* h = NULL;
	if(strcmp(widgetType, MAW_SCREEN) == 0) {
		Page* page = Page::create();
		u = page;
		t = ePage;
	}
	else
	if(strcmp(widgetType, MAW_WEB_VIEW) == 0) {
		WebView* w = WebView::create();
		h = new Handler(sNextWidgetHandle);
		u = w;
		t = eWebView;
		assert(QObject::connect(w, SIGNAL(messageReceived(const QVariantMap&)), h, SLOT(messageReceived(const QVariantMap&))));
	}
	else
	if(strcmp(widgetType, MAW_VERTICAL_LAYOUT) == 0) {
		// Cheaper than create().
		StackLayout* s = new StackLayout();
		// This is the default, but let's set it anyway to be clear and sure.
		s->setOrientation(LayoutOrientation::TopToBottom);
		Container* c = new Container();
		c->setLayout(s);
		u = c;
		t = eLayout;
	}
	else
	if(strcmp(widgetType, MAW_HORIZONTAL_LAYOUT) == 0) {
		StackLayout* s = new StackLayout();
		s->setOrientation(LayoutOrientation::LeftToRight);
		u = s;
		t = eLayout;
	}
	else
	if(strcmp(widgetType, MAW_LABEL) == 0) {
		u = new Label();
		t = eLabel;
	}
	else
	if(strcmp(widgetType, MAW_EDIT_BOX) == 0) {
		u = new TextField();
		t = eTextField;
	}
	else
	if(strcmp(widgetType, MAW_GL_VIEW) == 0) {
		ForeignWindowControl* f = new ForeignWindowControl();
#if 0	// doesn't work.
		screen_window_t sw = f->windowHandle();
#else
		screen_context_t screen_ctx;
    ERRNO(screen_create_context(&screen_ctx, SCREEN_APPLICATION_CONTEXT));

		screen_window_t sw;
		ERRNO(screen_create_window(&sw, screen_ctx));

		QByteArray groupArr = f->windowGroup().toAscii();
		ERRNO(screen_join_window_group(sw, groupArr.constData()));

		int format = SCREEN_FORMAT_RGBA8888;
		ERRNO(screen_set_window_property_iv(sw, SCREEN_PROPERTY_FORMAT, &format));
#endif
		bbutil_init_egl(sw, 0);

		// I hear this is useful for performance.
		int z = 1;
		screen_set_window_property_iv(sw, SCREEN_PROPERTY_ZORDER, &z);

		u = f;
		t = eForeignWindowControl;
	}
	else
	if(strcmp(widgetType, MAW_BUTTON) == 0) {
		u = new Button();
		t = eButton;
	}
	else
	if(strcmp(widgetType, MAW_IMAGE) == 0) {
		u = new ImageView();
		t = eImageView;
	}
	else
	if(strcmp(widgetType, MAW_IMAGE_BUTTON) == 0) {
		u = new ImageButton();
		t = eImageButton;
	}
	else
	if(strcmp(widgetType, MAW_STACK_SCREEN) == 0) {
		u = NavigationPane::create();
		t = eNavigationPane;
	}
	else
	if(strcmp(widgetType, MAW_LIST_VIEW) == 0) {
		u = new ListView();
		t = eListView;
	}
	else
	if(strcmp(widgetType, MAW_LIST_VIEW_ITEM) == 0) {
		u = new StandardListItem();
		t = eStandardListItem;
	}
	else
	if(strcmp(widgetType, MAW_RELATIVE_LAYOUT) == 0) {
		Container* c = new Container();
		c->setLayout(new AbsoluteLayout());
		u = c;
		t = eAbsoluteLayout;
	}
	else
	if(strcmp(widgetType, MAW_CHECK_BOX) == 0) {
		u = new CheckBox();
		t = eCheckBox;
	}

	if(!u) {
		DEBIG_PHAT_ERROR;
	}
	DEBUG_ASSERT(t != eNull);
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
	ed->hookType = hmi.type;
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
	return run(EVENT_CODE_WIDGET_ADD_CHILD, parentHandle, childHandle);
}

static int nuiAddChild(const bps_event_payload_t& payload) {
	MAWidgetHandle parentHandle = payload.data1;
	MAWidgetHandle childHandle = payload.data2;
	const Widget& p = getWidget(parentHandle);
	const Widget& c = getWidget(childHandle);
	switch(p.type) {
	case ePage:
		{
			Page* page = (Page*)p.o;
			page->setContent((Control*)c.o);	//HACK
			return MAW_RES_OK;
		}
	case eLayout:
	case eAbsoluteLayout:
		{
			Container* con = (Container*)p.o;
			con->add((Control*)c.o);
			return MAW_RES_OK;
		}
	default:
		DEBIG_PHAT_ERROR;
		//return MAW_RES_INVALID_LAYOUT;
	}
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
	return run(EVENT_CODE_WIDGET_SCREEN_SHOW, screenHandle);
}

static int nuiScreenShow(const bps_event_payload_t& payload) {
	MAWidgetHandle screenHandle = payload.data1;
	const Widget& s = getWidget(screenHandle);
	AbstractPane* p;
	switch(s.type) {
	case ePage: p = (Page*)s.o; break;
	case eNavigationPane: p = (NavigationPane*)s.o; break;
	default: DEBIG_PHAT_ERROR;
	}
	APP.setScene(p);
	return MAW_RES_OK;
}

int maWidgetScreenShowWithTransition(MAWidgetHandle screenHandle,
	MAWScreenTransitionType screenTransitionType, int screenTransitionDuration)
{
	DEBIG_PHAT_ERROR;
}

int maWidgetStackScreenPush(MAWidgetHandle stackScreenHandle, MAWidgetHandle screenHandle) {
	LOG("maWidgetStackScreenPush(%i, %i);\n", stackScreenHandle, screenHandle);
	return run(EVENT_CODE_STACK_SCREEN_PUSH, stackScreenHandle, screenHandle);
}

static int nuiStackScreenPush(const bps_event_payload_t& payload) {
	const Widget& ss = getWidget(payload.data1);
	const Widget& s = getWidget(payload.data2);
	DEBUG_ASSERT(ss.type == eNavigationPane);
	DEBUG_ASSERT(s.type == ePage);
	((NavigationPane*)ss.o)->push((Page*)s.o);
	return MAW_RES_OK;
}

int maWidgetStackScreenPop(MAWidgetHandle stackScreenHandle) {
	LOG("maWidgetStackScreenPop(%i);\n", stackScreenHandle);
	return run(EVENT_CODE_STACK_SCREEN_POP, stackScreenHandle);
}

static int nuiStackScreenPop(const bps_event_payload_t& payload) {
	const Widget& ss = getWidget(payload.data1);
	DEBUG_ASSERT(ss.type == eNavigationPane);
	((NavigationPane*)ss.o)->pop();
	return MAW_RES_OK;
}

int maWidgetSetProperty(MAWidgetHandle handle, const char* key, const char* value) {
	LOG("maWidgetSetProperty(%i, %s, %s);\n", handle, key, value);
	return run(EVENT_CODE_WIDGET_SET_PROPERTY, handle, (uintptr_t)key, (uintptr_t)value);
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
	case eLabel:
		{
			Label* l = (Label*)w.o;
			if(strcmp(key, MAW_LABEL_TEXT) == 0) {
				l->setText(value);
				return MAW_RES_OK;
			} else if(strcmp(key, MAW_LABEL_FONT_COLOR) == 0) {
				// convert 6-digit hex color to uint.
				DEBUG_ASSERT((strlen(value) == 6));
				uint argb;
				char* end;
				argb = strtoul(value, &end, 16);
				DEBUG_ASSERT(end - value == 6);

				// non-transparent.
				argb |= 0xff000000;

				l->textStyle()->setColor(Color::fromARGB(argb));
				return MAW_RES_OK;
			}
		}
		break;
	case eButton:
		{
			Button* b = (Button*)w.o;
			if(strcmp(key, MAW_LABEL_TEXT) == 0) {
				b->setText(value);
				return MAW_RES_OK;
			}
		}
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
