#ifndef _CustomMainForm_h_
#define _CustomMainForm_h_

#include <QtCore/qtimer.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qapplication.h>

class CustomMainForm : public QMainWindow
{
	Q_OBJECT
public:
	CustomMainForm (QWidget *parent=0);

private:
	QTimer* m_timer_refresh;
	bool m_incoming_data;
	void fill_buffer (void);

private slots:
	void refresh();
};

#endif // _CustomMainForm_h_
