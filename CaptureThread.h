// Copyright 2004 "Gilles Degottex"

// This file is part of "Music"

// "Music" is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
// 
// "Music" is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#ifndef _CaptureThread_h_
#define _CaptureThread_h_

#include <deque>
#include <list>
using namespace std;
#include <QtCore/qobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>
	
class CaptureThread;

// ----------------------- the implementations ----------------------

class CaptureThreadImpl
{
	friend class CaptureThread;

  protected:
	CaptureThread* m_capture_thread;
	QString m_name;
	QString m_descr;

	int m_sampling_rate;
	QString m_port_name;
	QString m_source;
	QString m_status;

  public:
	CaptureThreadImpl(CaptureThread* capture_thread, const QString& name, const QString& descr);

	QString getStatus();

	virtual void setSamplingRate(int rate)=0;

	virtual void capture_init()=0;
	virtual void capture_loop()=0;
	virtual void capture_finished()=0;

	virtual bool is_available()=0;

	virtual ~CaptureThreadImpl(){}
};

// ---------------------- the ALSA implementation ---------------------

#ifdef CAPTURE_ALSA
#include <alsa/asoundlib.h>
class CaptureThreadImplALSA : public CaptureThreadImpl
{
	snd_pcm_t* m_alsa_capture_handle;
	snd_pcm_hw_params_t* m_alsa_hw_params;
	signed short* m_alsa_buffer;
	snd_pcm_format_t m_format;

	void set_params();

  public:
	CaptureThreadImplALSA(CaptureThread* capture_thread);
	
	virtual void setSamplingRate(int rate);

	virtual void capture_init();
	virtual void capture_loop();
	virtual void capture_finished();

	virtual bool is_available();
};
#endif

// ---------------------- the JACK implementation ---------------------

#ifdef CAPTURE_JACK
#include <jack/jack.h>
class CaptureThreadImplJACK : public CaptureThreadImpl
{
	static int JackProcess(jack_nframes_t nframes, void* arg);
	static void JackShutdown(void* arg);
	static int JackSampleRate(jack_nframes_t nframes, void* arg);

	jack_client_t* m_jack_client;
	jack_port_t* m_jack_port;
	int jackSampleRate(jack_nframes_t nframes);
	int jackProcess(jack_nframes_t nframes);
	void jackShutdown();

  public:
	CaptureThreadImplJACK(CaptureThread* capture_thread);

	virtual void setSamplingRate(int rate);

	virtual void capture_init();
	virtual void capture_loop();
	virtual void capture_finished();

	virtual bool is_available();
};
#endif

// ---------------------- the SoundFile implementation ---------------------

#ifdef CAPTURE_SOUNDFILE
//#include <alsa/asoundlib.h>
class CaptureThreadImplSoundFile : public CaptureThreadImpl
{
//	snd_pcm_t* m_alsa_capture_handle;
//	snd_pcm_hw_params_t* m_alsa_hw_params;
//	signed short* m_alsa_buffer;
//	snd_pcm_format_t m_format;

  public:
	CaptureThreadImplSoundFile(CaptureThread* capture_thread);
	
	virtual void setSamplingRate(int rate);

	virtual void capture_init();
	virtual void capture_loop();
	virtual void capture_finished();

	virtual bool is_available();
};
#endif

// --------------------- the real accessible thread -------------------------

class CaptureThread : public QThread
{
	Q_OBJECT

#ifdef CAPTURE_ALSA
	friend class CaptureThreadImplALSA;
#endif
#ifdef CAPTURE_JACK
	friend class CaptureThreadImplJACK;
#endif

	list<CaptureThreadImpl*> m_impls;
	CaptureThreadImpl* m_current_impl;

	void emitError(const QString& error);
	void emitSamplingRateChanged();

	bool m_capturing;

	int m_packet_size;
	QString m_name;

	virtual void run();

	// control
	volatile bool m_loop;
	volatile bool m_pause;

	// view
	volatile bool m_alive;
	volatile bool m_in_run;

	QMutex m_lock;

  public:

	deque<double> m_values;

	enum {SAMPLING_RATE_UNKNOWN=-1, SAMPLING_RATE_MAX=0};

	CaptureThread(const QString& name="bastard_thread");

	void lock()						{m_lock.lock();}
	void unlock()					{m_lock.unlock();}

	bool isCapturing() const						{return m_capturing;}
	int getSamplingRate() const;
	int getPacketSize() const						{return m_packet_size;}
	int getNbPendingData() const					{return m_values.size();}
	QString getCurrentTransport() const;
	QString getCurrentTransportDescr() const;
	QString getFormatDescr() const;
	list<QString> getTransports() const;
	void listTransports();

	virtual ~CaptureThread();

  signals:
	void samplingRateChanged(int sampling_rate);
	void portNameChanged(const QString& name);
	void sourceChanged(const QString& src);
	void transportChanged(const QString& name);
	void captureStarted();
	void captureStoped();
	void captureToggled(bool run);
	void errorRaised(const QString& error);

  public slots:
	//! auto detect a working transport
	void autoDetectTransport();
	//! select a specific transport
	void selectTransport(const QString& name);
	//! reset capture (stop/start)
	void reset();
	//! start capture
	void startCapture();
	//! stop capture
	void stopCapture();
	//! set capture status
	void toggleCapture(bool run);
	//! set pause status
	/*! keep capture system connected, but throw away all incoming data
	 */
	void togglePause(bool pause);

	//! set the sampling rate
	/*! not always available, depending on the implementation 
	 * (reset the capture system !)
	 */
	void setSamplingRate(int value);
	//! set the port name
	/*! unused for ALSA, 'input' by default for JACK 
	 * (reset the capture system !)
	 */
	void setPortName(const QString& name);
	//! the source name
	/*! 'hw:0' for example for ALSA, something like alsa_pcm:capture_1 for JACK 
	 * (reset the capture system !)
	 */
	void setSource(const QString& src);
};

#endif // _CaptureThread_h_

