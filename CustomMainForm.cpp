
#include <iostream>
#include "ANR.h"
#include "CustomMainForm.h"

using namespace std;

CustomMainForm::CustomMainForm (QWidget *parent) : QMainWindow (parent)
{
	cerr << "Window" << endl;
	m_timer_refresh = new QTimer(this);
	connect((QObject*) m_timer_refresh, SIGNAL(timeout()),
			(QObject*) this, SLOT(refresh()));
	m_timer_refresh->start(1000);
}

void CustomMainForm::fill_buffer (void)
{

}

void CustomMainForm::refresh()
{
	if (anr().m_capture_thread.isCapturing())
		fill_buffer();

	if (m_incoming_data)
		anr().recognize();

	cerr << "refresh()" << endl;
}
