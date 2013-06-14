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
#include <bb/cascades/QListDataModel>
#include <bb/cascades/StackLayout>
#include <bb/cascades/StandardListItem>
#include <bb/cascades/TextField>
#include <bb/cascades/TitleBar>
#include <bb/cascades/WebView>
#include <bb/AbstractBpsEventHandler>
#include <bb/Image>
#include <bb/ImageData>
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
	eListItemUninitialized,
	eListItemStandard,
	eListItemContainer,
	eAbsoluteLayout,
	eCheckBox,
	eImageView,
};

enum WidgetFunction {
	EVENT_CODE_WIDGET_CREATE,
	EVENT_CODE_WIDGET_ADD_CHILD,
	EVENT_CODE_WIDGET_SET_PROPERTY,
	EVENT_CODE_WIDGET_GET_PROPERTY,
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

class MyListItemProvider : public ListItemProvider {
public:
	VisualNode* createItem(ListView* list, const QString& type) {
		// type is ignored; our listViews have only one type.
		return new Container();
	}

	void updateItem(ListView* list, VisualNode* listItem, const QString& type,
		const QVariantList& indexPath, const QVariant& data)
	{
		Container* con = (Container*)listItem;
		con->add(data.value<Control*>());
	}
};
typedef QListDataModel<Control*> MyListDataModel;

static MyListItemProvider sMyListItemProvider;

static int nuiCreate(const bps_event_payload_t& payload);
static int nuiAddChild(const bps_event_payload_t& payload);
static int nuiSetProperty(const bps_event_payload_t& payload);
static int nuiGetProperty(const bps_event_payload_t& payload);
static int nuiScreenShow(const bps_event_payload_t& payload);
static int nuiStackScreenPush(const bps_event_payload_t& payload);
static int nuiStackScreenPop(const bps_event_payload_t& payload);

static int nuiFunction(int code, const bps_event_payload_t& payload) {
	switch(code) {
	case EVENT_CODE_WIDGET_CREATE: return nuiCreate(payload);
	case EVENT_CODE_WIDGET_ADD_CHILD: return nuiAddChild(payload);
	case EVENT_CODE_WIDGET_SET_PROPERTY: return nuiSetProperty(payload);
	case EVENT_CODE_WIDGET_GET_PROPERTY: return nuiGetProperty(payload);
	case EVENT_CODE_WIDGET_SCREEN_SHOW: return nuiScreenShow(payload);
	case EVENT_CODE_STACK_SCREEN_PUSH: return nuiStackScreenPush(payload);
	case EVENT_CODE_STACK_SCREEN_POP: return nuiStackScreenPop(payload);
	default:
		DEBIG_PHAT_ERROR;
	}
}

// NOT thread safe. Call only from core thread.
static void postWidgetEvent(MAWidgetEventData* ed) {
	MAEventNative event;
	event.type = EVENT_TYPE_WIDGET;
	event.data = ed;
	putEvent(event);
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

static Widget& getWidget(MAWidgetHandle handle) {
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
		DEBUG_ASSERT(QObject::connect(w, SIGNAL(messageReceived(const QVariantMap&)), h, SLOT(webViewMessageReceived(const QVariantMap&))));
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
		Container* c = new Container();
		c->setLayout(s);
		u = c;
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
		//ForeignWindowControl* f = new ForeignWindowControl();
		ForeignWindowControl* f = ForeignWindowControl::create()
			.windowId("MoSyncGLView")
			.preferredSize(300, 300);

		h = new Handler(sNextWidgetHandle);
		DEBUG_ASSERT(QObject::connect(f, SIGNAL(windowAttached(screen_window_t, const QString&, const QString&)),
			h, SLOT(windowAttached(screen_window_t, const QString&, const QString&))));

#if 0	// doesn't work.
		screen_window_t sw = f->windowHandle();
#else
		screen_context_t screen_ctx;
    ERRNO(screen_create_context(&screen_ctx, SCREEN_APPLICATION_CONTEXT));

		screen_window_t sw;
		ERRNO(screen_create_window_type(&sw, screen_ctx, SCREEN_CHILD_WINDOW));

		// I hear this is useful for performance.
		int z = 1;
		screen_set_window_property_iv(sw, SCREEN_PROPERTY_ZORDER, &z);

		int vis = 1;
    ERRNO(screen_set_window_property_iv(sw, SCREEN_PROPERTY_VISIBLE, &vis));

		int format = SCREEN_FORMAT_RGBA8888;
		ERRNO(screen_set_window_property_iv(sw, SCREEN_PROPERTY_FORMAT, &format));

		QByteArray groupArr = f->windowGroup().toAscii();
		ERRNO(screen_join_window_group(sw, groupArr.constData()));

		// This causes windowAttached().
		ERRNO(screen_set_window_property_cv(sw, SCREEN_PROPERTY_ID_STRING, strlen("MoSyncGLView"), "MoSyncGLView"));
#endif
		DEBUG_ASSERT(bbutil_init_egl(sw, 0) == EXIT_SUCCESS);
#if 0
		// binding does not seem to be automated.
		f->bindToWindow(sw, f->windowGroup(), f->windowId());
		DEBUG_ASSERT(f->isBoundToWindow());

		MAWidgetEventData* ed = new MAWidgetEventData;
		ed->eventType = MAW_EVENT_GL_VIEW_READY;
		ed->widgetHandle = sNextWidgetHandle;
		postWidgetEvent(ed);
#endif

		u = f;
		t = eForeignWindowControl;
	}
	else
	if(strcmp(widgetType, MAW_BUTTON) == 0) {
		Button* b = new Button();
		h = new Handler(sNextWidgetHandle);
		DEBUG_ASSERT(QObject::connect(b, SIGNAL(clicked()), h, SLOT(buttonClicked())));
		u = b;
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
		NavigationPane* np = NavigationPane::create();
		h = new Handler(sNextWidgetHandle);
		DEBUG_ASSERT(QObject::connect(np, SIGNAL(popTransitionEnded(bb::cascades::Page*)), h, SLOT(popTransitionEnded(bb::cascades::Page*))));
		u = np;
		t = eNavigationPane;
	}
	else
	if(strcmp(widgetType, MAW_LIST_VIEW) == 0) {
		ListView* lv = new ListView();
		lv->setDataModel(new MyListDataModel());
		lv->setListItemProvider(&sMyListItemProvider);
		h = new Handler(sNextWidgetHandle);
		DEBUG_ASSERT(QObject::connect(lv, SIGNAL(triggered(QVariantList)), h, SLOT(listViewTriggered(QVariantList))));
		u = lv;
		t = eListView;
	}
	else
	if(strcmp(widgetType, MAW_LIST_VIEW_ITEM) == 0) {
		// MoSync ListViewItems can have children.
		// BB10 ones can't.
		// Therefore, we have this container, which will pe populated with one widget,
		// once it's decided if the ListViewItem will have children or not.
		// Attempting to reverse the decision will cause a Panic.
		u = new Container();
		t = eListItemUninitialized;
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
	u->setProperty("MoSync_widget_handle", sNextWidgetHandle);
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

void Handler::webViewMessageReceived(const QVariantMap& map) {
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

	postWidgetEvent(ed);
}


struct ButtonEventInfo : Functor {
	MAWidgetHandle h;
};
static void postButtonClickedEvent(const Functor& arg) {
	const ButtonEventInfo& bei = (const ButtonEventInfo&)arg;
	MAWidgetEventData *ed = new MAWidgetEventData;
	ed->eventType = MAW_EVENT_CLICKED;
	ed->widgetHandle = bei.h;
	postWidgetEvent(ed);
}
void Handler::buttonClicked() {
	ButtonEventInfo* bei = (ButtonEventInfo*)malloc(sizeof(ButtonEventInfo));
	bei->func = postButtonClickedEvent;
	bei->h = mHandle;
	executeInCoreThread(bei);
}


struct ListViewItemEventInfo : Functor {
	MAWidgetHandle h;
	int index;
};
static void postListViewItemClickedEvent(const Functor& arg) {
	const ListViewItemEventInfo& lviei = (const ListViewItemEventInfo&)arg;
	MAWidgetEventData *ed = new MAWidgetEventData;
	ed->eventType = MAW_EVENT_ITEM_CLICKED;
	ed->widgetHandle = lviei.h;
	ed->listItemIndex = lviei.index;
	postWidgetEvent(ed);
}
void Handler::listViewTriggered(QVariantList indexPath) {
	ListViewItemEventInfo* lviei = (ListViewItemEventInfo*)malloc(sizeof(ListViewItemEventInfo));
	lviei->func = postListViewItemClickedEvent;
	lviei->h = mHandle;
	lviei->index = indexPath[0].toInt();
	LOG("listViewTriggered(handle %i, index %i)\n", lviei->h, lviei->index);
	executeInCoreThread(lviei);
}


struct StackScreenPoppedEventInfo : Functor {
	MAWidgetHandle h;
	MAWidgetHandle from;
	MAWidgetHandle to;
};
static void postStackScreenPoppedEvent(const Functor& arg) {
	const StackScreenPoppedEventInfo& ei = (const StackScreenPoppedEventInfo&)arg;
	MAWidgetEventData *ed = new MAWidgetEventData;
	ed->eventType = MAW_EVENT_STACK_SCREEN_POPPED;
	ed->widgetHandle = ei.h;
	ed->fromScreen = ei.from;
	ed->toScreen = ei.to;
	postWidgetEvent(ed);
}
void Handler::popTransitionEnded(Page* page) {
	StackScreenPoppedEventInfo* ei = (StackScreenPoppedEventInfo*)malloc(sizeof(StackScreenPoppedEventInfo));
	ei->func = postStackScreenPoppedEvent;
	ei->h = mHandle;
	ei->from = page->property("MoSync_widget_handle").toInt();
	NavigationPane* np((NavigationPane*)getWidget(mHandle).o);
	ei->to = np->top()->property("MoSync_widget_handle").toInt();
	LOG("popTransitionEnded(stack %i, from %i, to %i)\n", ei->h, ei->from, ei->to);
	executeInCoreThread(ei);
}


struct GlViewEventInfo : Functor {
	MAWidgetHandle h;
};
static void postGlViewEvent(const Functor& arg) {
	const GlViewEventInfo& ei = (const GlViewEventInfo&)arg;
	MAWidgetEventData *ed = new MAWidgetEventData;
	ed->eventType = MAW_EVENT_GL_VIEW_READY;
	ed->widgetHandle = ei.h;
	postWidgetEvent(ed);
}
void Handler::windowAttached(screen_window_t, const QString&, const QString&) {
	LOG("windowAttached!\n");
	GlViewEventInfo* ei = (GlViewEventInfo*)malloc(sizeof(GlViewEventInfo));
	ei->func = postGlViewEvent;
	ei->h = mHandle;
	executeInCoreThread(ei);
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
	Widget& p = getWidget(parentHandle);
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
	case eListItemUninitialized:
		{
			Container* con = (Container*)p.o;
			DEBUG_ASSERT(con->count() == 0);
			con->add((Control*)c.o);
			p.type = eListItemContainer;
			return MAW_RES_OK;
		}
	case eListItemStandard:
		{
			Container* con = (Container*)p.o;
			DEBUG_ASSERT(con->count() >= 1);
			con->add((Control*)c.o);
			return MAW_RES_OK;
		}
	case eListView:
		{
			ListView* lv = (ListView*)p.o;
			MyListDataModel* dm = (MyListDataModel*)lv->dataModel();
			dm->append((Control*)c.o);
			return MAW_RES_OK;
		}
	default:
		LOG("type %i\n", p.type);
		DEBIG_PHAT_ERROR;
		//return MAW_RES_INVALID_LAYOUT;
	}
}

int maWidgetInsertChild(MAWidgetHandle parentHandle, MAWidgetHandle childHandle, int index) {
	LOG("maWidgetInsertChild(%i, %i, %i);\n", parentHandle, childHandle, index);
	DEBIG_PHAT_ERROR;
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

static Color ColorFromRgbString(const char* value) {
	// convert 6-digit hex color to uint.
	DEBUG_ASSERT((strlen(value) == 6));
	uint argb;
	char* end;
	argb = strtoul(value, &end, 16);
	DEBUG_ASSERT(end - value == 6);

	// non-transparent.
	argb |= 0xff000000;

	return Color::fromARGB(argb);
}

static float floatFromString(const char* value) {
	char* end;
	float f = strtof(value, &end);
	DEBUG_ASSERT((int)strlen(value) == end - value);
	return f;
}

static int intFromString(const char* value) {
	char* end;
	int i = strtol(value, &end, 10);
	DEBUG_ASSERT((int)strlen(value) == end - value);
	return i;
}

static bool boolFromString(const char* value) {
	if(strcmp(value, "true") == 0) {
		return true;
	} else if(strcmp(value, "false") == 0) {
		return false;
	} else {
		DEBIG_PHAT_ERROR;
	}
}

static int nuiSetProperty(const bps_event_payload_t& payload) {
	MAWidgetHandle handle = payload.data1;
	const char* key = (char*)payload.data2;
	const char* value = (char*)payload.data3;
	Widget& w = getWidget(handle);

	// Some properties are unavailable for all widgets.
	if(strcmp(key, MAW_WIDGET_HEIGHT) == 0) {
		return MAW_RES_FEATURE_NOT_AVAILABLE;
	}
	if(strcmp(key, MAW_WIDGET_WIDTH) == 0) {
		return MAW_RES_FEATURE_NOT_AVAILABLE;
	}

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
			}
			if(strcmp(key, MAW_LABEL_FONT_COLOR) == 0) {
				l->textStyle()->setColor(ColorFromRgbString(value));
				return MAW_RES_OK;
			}
			if(strcmp(key, MAW_LABEL_FONT_SIZE) == 0) {
				l->textStyle()->setFontSize(FontSize::PointValue);
				l->textStyle()->setFontSizeValue(floatFromString(value));
				return MAW_RES_OK;
			}
			if(strcmp(key, MAW_WIDGET_BACKGROUND_COLOR) == 0) {
				return MAW_RES_FEATURE_NOT_AVAILABLE;
			}
			if(strcmp(key, MAW_LABEL_TEXT_VERTICAL_ALIGNMENT) == 0) {
				// TODO: parse value
				l->setVerticalAlignment(VerticalAlignment::Center);
				return MAW_RES_OK;
			}
			if(strcmp(key, MAW_LABEL_TEXT_HORIZONTAL_ALIGNMENT) == 0) {
				// TODO: parse value
				l->setHorizontalAlignment(HorizontalAlignment::Center);
				return MAW_RES_OK;
			}
			if(strcmp(key, MAW_LABEL_MAX_NUMBER_OF_LINES) == 0) {
				if(strcmp(value, "0") == 0) {
					l->setMultiline(true);
					return MAW_RES_OK;
				}
				if(strcmp(value, "1") == 0) {
					l->setMultiline(false);
					return MAW_RES_OK;
				}
				return MAW_RES_FEATURE_NOT_AVAILABLE;
			}
		}
		break;
	case eButton:
		{
			Button* b = (Button*)w.o;
			if(strcmp(key, MAW_BUTTON_TEXT) == 0) {
				b->setText(value);
				return MAW_RES_OK;
			}
			if(strcmp(key, MAW_BUTTON_TEXT_HORIZONTAL_ALIGNMENT) == 0) {
				return MAW_RES_FEATURE_NOT_AVAILABLE;
			}
			if(strcmp(key, MAW_BUTTON_TEXT_VERTICAL_ALIGNMENT) == 0) {
				return MAW_RES_FEATURE_NOT_AVAILABLE;
			}
		}
		break;
	case eListItemUninitialized:
		{
			Container* con = (Container*)w.o;
			if(strcmp(key, MAW_LIST_VIEW_ITEM_TEXT) == 0) {
				DEBUG_ASSERT(con->count() == 0);
				StandardListItem* sli = new StandardListItem();
				sli->setTitle(value);
				((StackLayout*)con->layout())->setOrientation(LayoutOrientation::LeftToRight);
				con->add(sli);
				w.type = eListItemStandard;
				return MAW_RES_OK;
			}
			DEBIG_PHAT_ERROR;
		}
		break;
	case eListItemStandard:
		{
			if(strcmp(key, MAW_LIST_VIEW_ITEM_ACCESSORY_TYPE) == 0) {
				return MAW_RES_FEATURE_NOT_AVAILABLE;
			}
		}
		break;
	case eLayout:
		{
			Container* con = (Container*)w.o;
			if(strcmp(key, MAW_HORIZONTAL_LAYOUT_CHILD_VERTICAL_ALIGNMENT) == 0) {
				// TODO: actually parse the value.
				con->setVerticalAlignment(VerticalAlignment::Center);
				return MAW_RES_OK;
			}
			if(strcmp(key, MAW_HORIZONTAL_LAYOUT_CHILD_HORIZONTAL_ALIGNMENT) == 0) {
				// TODO: actually parse the value.
				con->setHorizontalAlignment(HorizontalAlignment::Center);
				return MAW_RES_OK;
			}
			if(strcmp(key, MAW_WIDGET_BACKGROUND_COLOR) == 0) {
				con->setBackground(ColorFromRgbString(value));
				return MAW_RES_OK;
			}
			if(strcmp(key, MAW_VERTICAL_LAYOUT_SCROLLABLE) == 0) {
				// TODO: needs a new ScrollView object.
				return MAW_RES_FEATURE_NOT_AVAILABLE;
			}
		}
		break;
	case eTextField:
		{
			TextField* tf((TextField*)w.o);
			if(strcmp(key, MAW_EDIT_BOX_TEXT) == 0) {
				tf->setText(value);
				return MAW_RES_OK;
			}
			if(strcmp(key, MAW_EDIT_BOX_SHOW_KEYBOARD) == 0) {
				if(boolFromString(value)) {
					tf->requestFocus();
					return MAW_RES_OK;
				} else {
					return MAW_RES_FEATURE_NOT_AVAILABLE;
				}
			}
			if(strcmp(key, MAW_EDIT_BOX_INPUT_FLAG) == 0) {
				return MAW_RES_FEATURE_NOT_AVAILABLE;
			}
			if(strcmp(key, MAW_EDIT_BOX_PLACEHOLDER) == 0) {
				tf->setHintText(value);
				return MAW_RES_OK;
			}
		}
		break;
	case ePage:
		{
			Page* p((Page*)w.o);
			if(strcmp(key, MAW_SCREEN_TITLE) == 0) {
				TitleBar* tb(p->titleBar());
				if(!tb)
					tb = new TitleBar();
				tb->setTitle(value);
				return MAW_RES_OK;
			}
		}
		break;
	case eImageButton:
		{
			ImageButton* ib((ImageButton*)w.o);
			if(strcmp(key, MAW_IMAGE_BUTTON_IMAGE) == 0) {
				Base::Image* img = SYSCALL_THIS->resources.get_RT_IMAGE(intFromString(value));
				DEBUG_ASSERT(img->pixelFormat == Base::Image::PIXELFORMAT_ARGB8888);
				ib->setDefaultImage(bb::ImageData::fromPixels(img->data, bb::PixelFormat::RGBX, img->width, img->height, img->pitch));
				return MAW_RES_OK;
			}
		}
		break;
	case eImageView:
		{
			ImageView* iv((ImageView*)w.o);
			if(strcmp(key, MAW_IMAGE_BUTTON_IMAGE) == 0) {
				Base::Image* img = SYSCALL_THIS->resources.get_RT_IMAGE(intFromString(value));
				DEBUG_ASSERT(img->pixelFormat == Base::Image::PIXELFORMAT_ARGB8888);
				iv->setImage(bb::ImageData::fromPixels(img->data, bb::PixelFormat::RGBX, img->width, img->height, img->pitch));
				return MAW_RES_OK;
			}
		}
		break;
	case eCheckBox:
		{
			CheckBox* cb((CheckBox*)w.o);
			if(strcmp(key, MAW_CHECK_BOX_CHECKED) == 0) {
				cb->setChecked(boolFromString(value));
				return MAW_RES_OK;
			}
		}
		break;
	case eListView:
		{
			if(strcmp(key, MAW_LIST_VIEW_REQUEST_FOCUS) == 0) {
				return MAW_RES_FEATURE_NOT_AVAILABLE;
			}
		}
		break;
	case eForeignWindowControl:
		{
			if(strcmp(key, MAW_GL_VIEW_BIND) == 0) {
				return MAW_RES_FEATURE_NOT_AVAILABLE;
			}
#if 0
			if(strcmp(key, MAW_WIDGET_WIDTH) == 0) {
				return MAW_RES_FEATURE_NOT_AVAILABLE;
			}
			if(strcmp(key, MAW_WIDGET_HEIGHT) == 0) {
				return MAW_RES_FEATURE_NOT_AVAILABLE;
			}
#endif
		}
		break;
	default:
		break;
	}
	// catch unknown props.
	// return MAW_RES_FEATURE_NOT_AVAILABLE explicitly for a prop to avoid this error.
	DEBIG_PHAT_ERROR;
}


struct WGP {
	MAWidgetHandle handle;
	const char* key;
	char* value;
	int maxSize;
};

int maWidgetGetProperty(MAWidgetHandle handle, const char* key, char* value, int maxSize) {
	LOG("maWidgetGetProperty(%i, %s);\n", handle, key);
	// Since run is synchronous, it's safe to pass a pointer to a local.
	WGP wgp = { handle, key, value, maxSize };
	int res = run(EVENT_CODE_WIDGET_GET_PROPERTY, (uintptr_t)&wgp);
	if(res <= 0)
		LOG("got %i\n", res);
	else
		LOG("got \"%s\"\n", value);
	return res;
}

static int nuiGetProperty(const bps_event_payload_t& payload) {
	const WGP& wgp(*(WGP*)payload.data1);
	Widget& w = getWidget(wgp.handle);
	switch(w.type) {
	case eTextField:
		{
			TextField* tf((TextField*)w.o);
			if(strcmp(wgp.key, MAW_EDIT_BOX_TEXT) == 0) {
				QByteArray bytes(tf->text().toUtf8());
				// bytes.size() == strlen(bytes.data())
				if(wgp.maxSize <= bytes.size())
					return MAW_RES_INVALID_STRING_BUFFER_SIZE;
				memcpy(wgp.value, bytes.data(), bytes.size());
				wgp.value[bytes.size()] = 0;
				return bytes.size();
			}
		}
		break;
	case eForeignWindowControl:
		{
			ForeignWindowControl* f = (ForeignWindowControl*)w.o;
			if(strcmp(wgp.key, MAW_WIDGET_WIDTH) == 0) {
				int size[2];
				ERRNO(screen_get_window_property_iv(f->windowHandle(), SCREEN_PROPERTY_SIZE, size));
				int res = snprintf(wgp.value, wgp.maxSize, "%i", size[0]);
				return res;
			}
			if(strcmp(wgp.key, MAW_WIDGET_HEIGHT) == 0) {
				int size[2];
				ERRNO(screen_get_window_property_iv(f->windowHandle(), SCREEN_PROPERTY_SIZE, size));
				int res = snprintf(wgp.value, wgp.maxSize, "%i", size[1]);
				return res;
			}
		}
		break;
	default:
		break;
	}
	// catch unknown props.
	// return MAW_RES_INVALID_PROPERTY_NAME explicitly for a prop to avoid this error.
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
