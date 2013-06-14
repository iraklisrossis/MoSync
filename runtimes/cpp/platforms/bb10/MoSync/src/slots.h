#ifndef SLOTS_H
#define SLOTS_H

#include <bb/AbstractBpsEventHandler>
#include <bb/cascades/Page>

class Handler : public QObject {
	Q_OBJECT
public:
	Handler(MAWidgetHandle h) : mHandle(h) {
	}
public slots:
	void buttonClicked();
	void listViewTriggered(QVariantList);
	void popTransitionEnded(bb::cascades::Page*);
	void windowAttached(screen_window_t, const QString&, const QString&);
	void webViewMessageReceived(const QVariantMap&);
private:
	const MAWidgetHandle mHandle;
};

#endif	//SLOTS_H
