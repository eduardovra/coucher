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


#include "CaptureThread.h"

#include <cassert>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <list>
using namespace std;
#include <QtCore/qdatetime.h>

CaptureThread::CaptureThread(const QString& name)
{
	m_current_impl = NULL;

	m_alive = true;
	m_in_run = false;
	m_capturing = false;

	m_loop = false;
	m_pause = false;

	m_name = name;
#ifdef CAPTURE_SOUNDFILE
	m_impls.push_back(new CaptureThreadImplSoundFile(this));
#endif
#ifdef CAPTURE_JACK
	m_impls.push_back(new CaptureThreadImplJACK(this));
#endif
#ifdef CAPTURE_ALSA
	m_impls.push_back(new CaptureThreadImplALSA(this));
#endif

	listTransports();
}

void CaptureThread::autoDetectTransport()
{
	bool was_capturing = isCapturing();
	if(was_capturing)
		stopCapture();

	QString old_name;
	if(m_current_impl!=NULL)
		old_name = m_current_impl->m_name;

	cerr << "CaptureThread: INFO: Auto detecting a working transport ... " << flush;

	CaptureThreadImpl* impl = NULL;
	for(list<CaptureThreadImpl*>::iterator it=m_impls.begin(); impl==NULL && it!=m_impls.end(); it++)
		if((*it)->is_available())
			impl = *it;

	if(impl!=NULL)
	{
		m_current_impl = impl;

		cerr << "using " << m_current_impl->m_name.toStdString() << endl;

		if(m_current_impl->m_name!=old_name)
			emit(transportChanged(m_current_impl->m_name));

		if(was_capturing)
			startCapture();
	}
	else
	{
		cerr << "no transport working !" << endl;

		if(old_name!="")
			emit(transportChanged(""));
	}
}
void CaptureThread::selectTransport(const QString& name)
{
	if(name==getCurrentTransport())	return;

	bool was_capturing = isCapturing();
	if(was_capturing)
		stopCapture();

	QString old_name;
	if(m_current_impl!=NULL)
		old_name = m_current_impl->m_name;

	CaptureThreadImpl* impl = NULL;
	for(list<CaptureThreadImpl*>::iterator it=m_impls.begin(); impl==NULL && it!=m_impls.end(); it++)
		if((*it)->m_name==name)
			impl = *it;

	if(impl==NULL)
	{
		cerr << "CaptureThread: ERROR: unknown transport '" << name.toStdString() << "'" << endl;
		throw QString("CaptureThread: unknown transport '")+name+"'";
	}

	m_current_impl = impl;

	if(m_current_impl->m_name!=old_name)
		emit(transportChanged(m_current_impl->m_name));

	if(was_capturing)
		startCapture();
}
list<QString> CaptureThread::getTransports() const
{
	list<QString> trsps;

	for(list<CaptureThreadImpl*>::const_iterator it=m_impls.begin(); it!=m_impls.end(); it++)
		trsps.push_back((*it)->m_name);

	return trsps;
}
void CaptureThread::listTransports()
{
	cerr << "CaptureThread: INFO: Built in transports" << endl;
	for(list<CaptureThreadImpl*>::iterator it=m_impls.begin(); it!=m_impls.end(); it++)
		cerr << "CaptureThread: INFO:	" << (*it)->m_name.toStdString() << "   " << (*it)->getStatus().toStdString() << endl;
//		cerr << "CaptureThread: INFO:   " << (*it)->m_name.toStdString() << "   " << (*it)->getStatus() << endl;
}
QString CaptureThread::getCurrentTransport() const
{
	if(m_current_impl==NULL)
		return "";

	return m_current_impl->m_name;
}
QString CaptureThread::getCurrentTransportDescr() const
{
	if(m_current_impl==NULL)
		return "";

	return m_current_impl->m_descr;
}
QString CaptureThread::getFormatDescr() const
{
	if(m_current_impl==NULL)
		return "";

	// TODO
}

void CaptureThread::emitError(const QString& error)
{
	emit(errorRaised(error));
}

void CaptureThread::emitSamplingRateChanged()
{
	if(m_current_impl->m_sampling_rate>0)
		emit(samplingRateChanged(m_current_impl->m_sampling_rate));
}

void CaptureThread::startCapture()
{
	if(m_current_impl==NULL)	return;

	if(!isRunning())
		start();

	m_loop = true;
}
void CaptureThread::stopCapture()
{
	//	cerr << "CaptureThread::stopCapture" << endl;

	m_loop = false;

	while(m_in_run)
		msleep(10);

	//	cerr << "/CaptureThread::stopCapture" << endl;
}

void CaptureThread::toggleCapture(bool run)
{
	if(run && !m_capturing)	startCapture();
	if(!run && m_capturing)	stopCapture();
}

void CaptureThread::reset()
{
	stopCapture();
	startCapture();
}

void CaptureThread::togglePause(bool pause)
{
	m_pause = pause;
}

int CaptureThread::getSamplingRate() const
{
	if(m_current_impl==NULL)	return SAMPLING_RATE_UNKNOWN;

	return m_current_impl->m_sampling_rate;
}
void CaptureThread::setSamplingRate(int rate)
{
	if(m_current_impl!=NULL)
		m_current_impl->setSamplingRate(rate);
}

void CaptureThread::setPortName(const QString& name)
{
	assert(name!="");

	if(m_current_impl==NULL)
	{
		cerr << "CaptureThread: setPortName: ERROR: select a transport first" << endl;
		return;
	}

	if(name!=m_current_impl->m_port_name)
	{
		m_current_impl->m_port_name = name;
		if(isCapturing())
		{
			stopCapture();
			startCapture();
		}

		emit(portNameChanged(m_current_impl->m_port_name));
	}
}
void CaptureThread::setSource(const QString& name)
{
	if(m_current_impl==NULL)
	{
		cerr << "CaptureThread: setPortName: ERROR: select a transport first" << endl;
		return;
	}

	if(name!=m_current_impl->m_source)
	{
		m_current_impl->m_source = name;
		if(isCapturing())
		{
			stopCapture();
			startCapture();
		}

		emit(sourceChanged(m_current_impl->m_source));
	}
}

CaptureThread::~CaptureThread()
{
	m_alive = false;

	stopCapture();

	while(isRunning())
		msleep(10);
}

void CaptureThread::run()
{
	cerr << "CaptureThread: INFO: capture thread entered" << endl;

	while(m_alive)
	{
		while(m_alive && !m_loop)
			msleep(10);

		m_in_run = true;

		try
		{
			cerr << "CaptureThread: INFO: capture thread running" << endl;

			m_current_impl->capture_init();

			m_capturing = true;
			emit(captureStarted());
			emit(captureToggled(true));

			m_current_impl->capture_loop();

			m_capturing = false;
			emit(captureStoped());
			emit(captureToggled(false));
		}
		catch(QString error)
		{
			m_loop = false;
//			cerr << "CaptureThread: ERROR: " << error << endl;
			emit(errorRaised(error));
		}

		m_current_impl->capture_finished();

		m_in_run = false;

		cerr << "CaptureThread: INFO: capture thread stop running" << endl;
	}

	cerr << "CaptureThread: INFO: capture thread exited" << endl;
}

// -------------------------------- implementation ------------------------------

CaptureThreadImpl::CaptureThreadImpl(CaptureThread* capture_thread, const QString& name, const QString& descr)
: m_capture_thread(capture_thread)
{
	m_name = name;
	m_descr = descr;
	m_status = "";

	m_sampling_rate = CaptureThread::SAMPLING_RATE_UNKNOWN;
	m_port_name = "input";
	m_source = "";
}

QString CaptureThreadImpl::getStatus()
{
	if(m_status=="")
		is_available();

	return m_status;
}

// ------------------------------ ALSA implementation ----------------------------
#ifdef CAPTURE_ALSA

#define ALSA_BUFF_SIZE 1024

void alsa_error_handler(const char *file, int line, const char *function, int err, const char *fmt, ...)
{
	cerr << "alsa_error_handler: " << file << ":" << line << " " << function << " err=" << err << endl;
}

CaptureThreadImplALSA::CaptureThreadImplALSA(CaptureThread* capture_thread)
: CaptureThreadImpl(capture_thread, "ALSA", "Advanced Linux Sound Architecture")
{
	m_alsa_capture_handle = NULL;
	m_alsa_hw_params = NULL;
	m_alsa_buffer = NULL;
	m_format = SND_PCM_FORMAT_UNKNOWN;

	m_source = "hw:0";

	snd_lib_error_set_handler(alsa_error_handler);
}

bool CaptureThreadImplALSA::is_available()
{
	if(m_alsa_capture_handle==NULL)
	{
		try
		{
			int err = -1;
			if((err=snd_pcm_open(&m_alsa_capture_handle, m_source.toLatin1(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0)
			{
				if(err==-19)	// TODO risks of changes for the error code
					throw QString("invalid source '")+m_source+"'";
				else if(err==-16)
					throw QString("device '")+m_source+"' busy";
				else
					throw QString("cannot open pcm: ")+QString(snd_strerror(err));
			}
		}
		catch(QString error)
		{
			m_alsa_capture_handle = NULL;

			m_status = "unavailable ("+error+")";

			return false;
		}

		if(m_alsa_capture_handle!=NULL)
		{
			snd_pcm_close(m_alsa_capture_handle);
			m_alsa_capture_handle = NULL;
		}
	}

	m_status = "available";

	cerr << "CaptureThread: INFO: ALSA seems available" << endl;

	return true;
}

void CaptureThreadImplALSA::set_params()
{
//	cerr << "ALSA: Recognized sample formats are" << endl;
//	for (int k = 0; k < SND_PCM_FORMAT_LAST; ++(unsigned long) k) {
//		const char *s = snd_pcm_format_name((snd_pcm_format_t)k);
//		if (s)	cerr << s << endl;
//	}
	int err=0;

	if(m_source=="")
		throw QString("ALSA: set the source first");
	if((err=snd_pcm_open(&m_alsa_capture_handle, m_source.toLatin1(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0)
	{
		//					cerr << "err=" << err << ":" << snd_strerror(err) << endl;

		if(err==-19)	// TODO risks of changes for the error code
			throw QString("ALSA: Invalid Source '")+m_source+"'";
		else if(err==-16)
			throw QString("ALSA: Device '")+m_source+"' busy";
		else
			throw QString("ALSA: Cannot open pcm: ")+QString(snd_strerror(err));
	}

	snd_pcm_hw_params_alloca(&m_alsa_hw_params);

	if((err=snd_pcm_hw_params_any(m_alsa_capture_handle, m_alsa_hw_params)) < 0)
		throw QString("ALSA: cannot initialize hardware parameter structure (")+QString(snd_strerror(err))+")";

	if((err=snd_pcm_hw_params_set_access(m_alsa_capture_handle, m_alsa_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
		throw QString("ALSA: cannot set access type (")+QString(snd_strerror(err))+")";

	// Formats
	if(m_format==-1)
	{
		list<snd_pcm_format_t> formats;
		formats.push_back(SND_PCM_FORMAT_S16);	formats.push_back(SND_PCM_FORMAT_U16);
		formats.push_back(SND_PCM_FORMAT_S8);	formats.push_back(SND_PCM_FORMAT_U8);

		err = -1;
		while(err<0)
		{
			if(formats.empty())
				throw QString("ALSA: cannot set any format (")+QString(snd_strerror(err))+")";

			m_format = formats.front();
			cerr << "CaptureThread: INFO: ALSA: try to set format to " << snd_pcm_format_description(m_format) << flush;
			err=snd_pcm_hw_params_set_format(m_alsa_capture_handle, m_alsa_hw_params, m_format);

			if(err<0)	cerr << " failed" << endl;
			else		cerr << " success" << endl;

			formats.pop_front();
		}
	}
	else
	{
		if((err=snd_pcm_hw_params_set_format(m_alsa_capture_handle, m_alsa_hw_params, m_format))<0)
		{
			QString err_msg = QString("ALSA: cannot set format (")+QString(snd_strerror(err))+")";
			cerr << "CaptureThread: ERROR: " << err_msg.toStdString() << endl;
		}
	}

	// Channel count
	unsigned int channel_count = 1;
	if((err=snd_pcm_hw_params_set_channels_near(m_alsa_capture_handle, m_alsa_hw_params, &channel_count)) < 0)
	{
		QString err_msg = QString("ALSA: cannot set channel count (")+QString(snd_strerror(err))+")";
		cerr << "CaptureThread: ERROR: " << err_msg.toStdString() << endl;
	}

	if(channel_count!=1)
		cerr << "CaptureThread: ERROR: ALSA: channel count is not set to one !" << endl;

	if(m_sampling_rate==CaptureThread::SAMPLING_RATE_MAX)
	{
		list<int> sampling_rates;
		sampling_rates.push_front(8000);	sampling_rates.push_front(11025);	sampling_rates.push_front(16000);
		sampling_rates.push_front(22050);	sampling_rates.push_front(24000);	sampling_rates.push_front(32000);
		sampling_rates.push_front(44100);	sampling_rates.push_front(48000);	sampling_rates.push_front(44100);
		sampling_rates.push_front(48000);	sampling_rates.push_front(96000);

		err = -1;
		while(err<0)
		{
			if(sampling_rates.empty())
				throw QString("ALSA: cannot set any sample rate (")+QString(snd_strerror(err))+")";

			m_sampling_rate = sampling_rates.front();
			cerr << "CaptureThread: INFO: ALSA: try to set sampling rate to " << m_sampling_rate << flush;
			unsigned int rrate = m_sampling_rate;
			err = snd_pcm_hw_params_set_rate(m_alsa_capture_handle, m_alsa_hw_params, rrate, 0);

			if(err<0)	cerr << " failed" << endl;
			else		cerr << " success" << endl;

			sampling_rates.pop_front();
		}
	}
	else
	{
		int err, dir;
		unsigned int rrate = m_sampling_rate;
		if((err = snd_pcm_hw_params_set_rate_near(m_alsa_capture_handle, m_alsa_hw_params, &rrate, &dir))<0)
			throw QString("ALSA: cannot set sampling rate (")+QString(snd_strerror(err))+")";
		m_sampling_rate = rrate;
	}

	if((err=snd_pcm_hw_params(m_alsa_capture_handle, m_alsa_hw_params)) < 0)
		throw QString("ALSA: cannot set parameters (")+QString(snd_strerror(err))+")";
}

void CaptureThreadImplALSA::setSamplingRate(int value)
{
	assert(value>=0);

	int old_sampling_rate = m_sampling_rate;

	if(m_sampling_rate!=value)
	{
		if(m_capture_thread->isCapturing())
		{
			m_capture_thread->stopCapture();
			m_sampling_rate = value;
			m_capture_thread->startCapture();
		}
		else
		{
			m_sampling_rate = value;

			try
			{
				set_params();
			}
			catch(QString error)
			{
				cerr << "CaptureThread: ERROR: " << error.toStdString() << endl;
				m_capture_thread->emitError(error);
			}

			// it was just for test
			capture_finished();
		}

		if(m_sampling_rate!=old_sampling_rate)
			m_capture_thread->emitSamplingRateChanged();
	}
}

void CaptureThreadImplALSA::capture_init()
{
	set_params();

	snd_pcm_nonblock(m_alsa_capture_handle, 0);

	m_alsa_buffer = new signed short[ALSA_BUFF_SIZE];

	int err=0;

	if((err=snd_pcm_prepare(m_alsa_capture_handle)) < 0)
		throw QString("ALSA: cannot prepare audio interface for use (")+QString(snd_strerror(err))+")";
}
void CaptureThreadImplALSA::capture_loop()
{
	double value;

	int format_size = snd_pcm_format_width(m_format) / 8;
	bool format_signed = snd_pcm_format_signed(m_format);

	while(m_capture_thread->m_loop)
	{
		int ret_val = snd_pcm_readi(m_alsa_capture_handle, m_alsa_buffer, ALSA_BUFF_SIZE);
		if(ret_val<0)
		{
			cerr << "CaptureThread: WARNING: ALSA: " << snd_strerror(ret_val) << endl;
			while((ret_val = snd_pcm_prepare(m_alsa_capture_handle)) < 0)
			{
				m_capture_thread->msleep(1000);
				cerr << QString("ALSA: cannot prepare audio interface (").toStdString()+QString(snd_strerror(ret_val)).toStdString()+")" << endl;
//				throw QString("ALSA: cannot prepare audio interface (")+QString(snd_strerror(ret_val))+")";
			}
		}
		else
		{
			if(!m_capture_thread->m_pause)
			{
				m_capture_thread->m_lock.lock();

				for(int i=0; i<ret_val; i++)
				{
					if(format_size==2)
					{
						if(format_signed)	value = (signed short)(m_alsa_buffer[i])/32768.0;
						else				value = 2*(unsigned short)(m_alsa_buffer[i])/65536.0 - 1;
					}
					else
					{
						if(format_signed)	value = (signed char)(m_alsa_buffer[i])/128.0;
						else				value = 2*(unsigned char)(m_alsa_buffer[i])/256.0 - 1;
					}
					m_capture_thread->m_values.push_front(value);
				}

				m_capture_thread->m_lock.unlock();

				m_capture_thread->m_packet_size = ret_val;
			}
		}
	}
}
void CaptureThreadImplALSA::capture_finished()
{
	if(m_alsa_buffer!=NULL)
	{
		delete m_alsa_buffer;
		m_alsa_buffer = NULL;
	}

	if(m_alsa_capture_handle!=NULL)
	{
		snd_pcm_hw_free(m_alsa_capture_handle);
		snd_pcm_close(m_alsa_capture_handle);
		m_alsa_capture_handle = NULL;
	}
}

#endif

// ------------------------------ JACK implementation ----------------------------
#ifdef CAPTURE_JACK
CaptureThreadImplJACK::CaptureThreadImplJACK(CaptureThread* capture_thread)
: CaptureThreadImpl(capture_thread, "JACK", "Jack Audio Connection Kit")
{
	m_jack_client = NULL;
	m_jack_port = NULL;
	/*	try
		{
		m_jack_client = jack_client_new(m_capture_thread->m_name.latin1());
		if(m_jack_client==NULL)
		throw QString("JACK: cannot create client and so cannot get server sampling rate");

		m_capture_thread->m_sampling_rate = jack_get_sample_rate(m_jack_client);
	//		emit(samplingRateChanged(m_sampling_rate));		// TODO emit in the ctor => crash ?
	}
	catch(QString error)
	{
	cerr << "CaptureThread: ERROR: " << error << endl;
	//		emit(errorRaised(error));	// TODO emit in the ctor => crash ?
	}
	capture_finished();
	*/
}

bool CaptureThreadImplJACK::is_available()
{
	if(m_jack_client==NULL)
	{
		try
		{
			m_jack_client = jack_client_open((m_capture_thread->m_name+"_test").toLatin1(), (jack_options_t) 0, NULL);
			if(m_jack_client==NULL)
				throw QString("unknown reason");
		}
		catch(QString error)
		{
			m_jack_client = NULL;
			m_status = "unavailable";
			return false;
		}
		capture_finished();
	}

	m_status = "available";

	return true;
}

void CaptureThreadImplJACK::setSamplingRate(int value)
{
	cerr << "CaptureThread: ERROR: JACK: setSamplingRate not available with JACK ! change the JACK server sampling rate instead" << endl;
}

void CaptureThreadImplJACK::JackShutdown(void* arg){((CaptureThreadImplJACK*)arg)->jackShutdown();}
void CaptureThreadImplJACK::jackShutdown()
{
	m_jack_client = NULL;

	m_capture_thread->emitError("JACK: server shutdown !");

	m_capture_thread->m_loop = false;
}

int CaptureThreadImplJACK::JackSampleRate(jack_nframes_t nframes, void* arg){return ((CaptureThreadImplJACK*)arg)->jackSampleRate(nframes);}
int CaptureThreadImplJACK::jackSampleRate(jack_nframes_t nframes)
{
	if(m_sampling_rate!=int(nframes))
	{
		m_sampling_rate = nframes;
		m_capture_thread->emitSamplingRateChanged();
	}

	return 0;
}

//int g_frames = 0;
//bool g_count = false;

int CaptureThreadImplJACK::JackProcess(jack_nframes_t nframes, void* arg){return ((CaptureThreadImplJACK*)arg)->jackProcess(nframes);}
int CaptureThreadImplJACK::jackProcess(jack_nframes_t nframes)
{
	if(m_capture_thread->m_pause)	return 0;

	//cerr << "jackProcess" << endl;

	jack_default_audio_sample_t* in = (jack_default_audio_sample_t*) jack_port_get_buffer(m_jack_port, nframes);

	m_capture_thread->m_lock.lock();

	for(jack_nframes_t i=0; i<nframes; i++)
		m_capture_thread->m_values.push_front(in[i]);

	m_capture_thread->m_lock.unlock();

	m_capture_thread->m_packet_size = nframes;

	//	if(g_count)		g_frames += nframes;

	return 0;
}

void CaptureThreadImplJACK::capture_init()
{
	m_jack_client = jack_client_open(m_capture_thread->m_name.toLatin1(), (jack_options_t)0, NULL);
	if(m_jack_client==NULL)
		throw QString("JACK: cannot create client, JACK deamon is running ?");
	jack_set_process_callback(m_jack_client, JackProcess, (void*)this);
	jack_on_shutdown(m_jack_client, JackShutdown, (void*)this);
	//jack_set_error_function(jack_error_callback);
	jack_set_sample_rate_callback(m_jack_client, JackSampleRate, (void*)this);

	int err=0;
	if((err=jack_activate(m_jack_client))!=0)
		throw QString("JACK: cannot activate client");

	m_jack_port = jack_port_register(m_jack_client, m_port_name.toLatin1(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,0);

	if(m_source!="")
		if((err=jack_connect(m_jack_client, m_source.toLatin1(), (m_capture_thread->m_name+":"+m_port_name).toLatin1()))!=0)
			m_capture_thread->emitError(QString("JACK: Invalid source '")+m_source+"'");

	int old_sampling_rate = m_sampling_rate;
	m_sampling_rate = jack_get_sample_rate(m_jack_client);
	if(m_sampling_rate!=old_sampling_rate)
		m_capture_thread->emitSamplingRateChanged();
}
void CaptureThreadImplJACK::capture_loop()
{
	//	QTime time;
	//	time.start();
	//	int t = 0;

	while(m_capture_thread->m_loop)
	{
		//		if(!g_count && time.elapsed()>=3000)
		//		{
		//			t = time.elapsed();
		//			g_count = true;
		//		}
		//		if(g_count)
		//		{
		//			cerr << 1000.0*g_frames/float(time.elapsed()-t) << endl;
		//		}
		m_capture_thread->msleep(33);
	}
}
void CaptureThreadImplJACK::capture_finished()
{
	if(m_jack_client!=NULL)
	{
		jack_client_close(m_jack_client);
		m_jack_client = NULL;
	}
}

#endif

// ------------------------------ WAVE implementation ----------------------------

#ifdef CAPTURE_SOUNDFILE

CaptureThreadImplSoundFile::CaptureThreadImplSoundFile(CaptureThread* capture_thread)
: CaptureThreadImpl(capture_thread, "SOUNDFILE", "libsndfile")
{
	//
}

bool CaptureThreadImplSoundFile::is_available()
{
	m_status = "available";

	cerr << "CaptureThread: INFO: SoundFile seems available" << endl;

	return true;
}

void CaptureThreadImplSoundFile::setSamplingRate(int value)
{

}

void CaptureThreadImplSoundFile::capture_init()
{

}

void CaptureThreadImplSoundFile::capture_loop()
{

}

void CaptureThreadImplSoundFile::capture_finished()
{
	
}

#endif
