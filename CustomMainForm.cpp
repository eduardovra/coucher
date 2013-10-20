
#include <iostream>
#include "ANR.h"
#include "CustomMainForm.h"

using namespace std;

CustomMainForm::CustomMainForm (QWidget *parent) : QMainWindow (parent)
{
	m_timer_refresh = new QTimer(this);
	connect((QObject*) m_timer_refresh, SIGNAL(timeout()),
			(QObject*) this, SLOT(refresh()));
	m_timer_refresh->start(1000);
}

void CustomMainForm::fill_buffer (void)
{
	m_incoming_data = false;

	anr().m_capture_thread.lock();

	while(!anr().m_capture_thread.m_values.empty())
	{
		m_incoming_data = true;
		anr().m_queue.push_front(anr().m_capture_thread.m_values.back());
		anr().m_capture_thread.m_values.pop_back();
	}

	anr().m_capture_thread.unlock();
}

void CustomMainForm::refresh()
{
	if (anr().m_capture_thread.isCapturing())
		fill_buffer();

	if (m_incoming_data)
		anr().recognize();

	//cerr << "CustomMainForm::refresh()" << endl;
}
