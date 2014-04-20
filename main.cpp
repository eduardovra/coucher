#include <iostream>

#include <Music/Music.h>
#include "ANR.h"
#include "CustomMainForm.h"

using namespace std;

int main (int argc, char *argv[])
{
	QApplication app(argc, argv);
	CustomMainForm win;

	Music::SetSamplingRate(44100);

	new ANR();
	anr().init();

	anr().m_capture_thread.autoDetectTransport();
	//anr().m_capture_thread.selectTransport("SOUNDFILE");

	anr().start();

	win.show();

	return app.exec();
}
