#ifndef SLOTS_H
#define SLOTS_H

#include <bb/AbstractBpsEventHandler>

class Handler : public QObject {
	Q_OBJECT
public:
	Handler(MAWidgetHandle h) : mHandle(h) {
	}
public slots:
	void messageReceived(const QVariantMap&);
private:
	const MAWidgetHandle mHandle;
};

#endif	//SLOTS_H
