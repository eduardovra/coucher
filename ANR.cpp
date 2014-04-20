// Copyright 2005 "Gilles Degottex"

// This file is part of "midingsolo"

// "midingsolo" is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// "midingsolo" is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "ANR.h"

#include <iostream>
using namespace std;

ANR::ANR()
: m_is_running(false)
, m_capture_thread("Midingsolo")
, m_quantizer()
, m_midistr("Midingsolo", 60)
, m_algo_multicorr(NULL)
, m_algo_autocorr(NULL)
, m_algo_bubble(NULL)
, m_algo_current(NULL)
, m_transform_current(NULL)
#ifdef FANR_OUTPUT_MIDI
#endif
{
//	cerr << "ANR::ANR" << endl;

	m_most_recent_note = 0;

	m_midi_enabled = true;
	m_std_enabled = true;
	m_std_anglo_names = LOCAL_ANGLO;
	m_std_transpose = false;

	m_notes_history.resize(m_quantizer.getNbChannels());

	m_old_running_time = 0;
	m_quantizer.addListener(this);
#ifdef FANR_OUTPUT_MIDI
	cerr << "ALSA midi client built " << m_midistr.getAlsaMidiID() << ":" << m_midistr.getPort() << endl;
#endif

	m_refresh_time_timer.start();


//	cerr << "/ANR::ANR" << endl;
}

void ANR::start()
{
//	cerr << "ANR::start" << endl;

	m_capture_thread.startCapture();
	m_time.start();
	m_refresh_time_timer.start();
	m_is_running = true;
	
//	cerr << "/ANR::start" << endl;
}

void ANR::pause(bool toggled)
{
//	cerr << "ANR::pause " << toggled << endl;

	if(toggled && m_is_running)
	{
		m_old_running_time = m_time.elapsed();
		m_is_running = false;
	}
	else if(!toggled && !m_is_running)
	{
		m_capture_thread.lock();
		m_capture_thread.m_values.clear();
		m_capture_thread.unlock();

		m_forgotten_time += m_time.elapsed() - m_old_running_time;

		m_is_running = true;
	}

//	cerr << "/ANR::pause " << toggled << endl;
}

void ANR::init()
{
//	cerr << "ANR::init" << endl;

//	if(GetSamplingRate()<=0)	return;

	if(m_algo_multicorr!=NULL)		delete m_algo_multicorr;
	cerr << "building MultiCorr Algorithm " << flush;
	m_algo_multicorr = new MultiCorrelationAlgo(1, 2.0);
	cerr << "\tok" << endl;

	if(m_algo_autocorr!=NULL)		delete m_algo_autocorr;
	cerr << "building AutoCorr Algorithm " <<  flush;
	m_algo_autocorr = new AutocorrelationAlgo(0.1);
	cerr << "\tok" << endl;

//	if(m_algo_bubble!=NULL)			delete m_algo_bubble;
//	cerr << "building Bubble Algorithm " <<  flush;
//	m_algo_bubble = new BubbleAlgo();
//	cerr << "\tok" << endl;

	m_algo_current = m_algo_multicorr;
//	m_algo_current = m_algo_bubble;
	m_transform_current = m_algo_multicorr;

//	cerr << "/ANR::init" << endl;
}

void ANR::recognize()
{
	m_refresh_time = m_refresh_time_timer.elapsed();
	m_refresh_time_timer.start();

	cerr << "ANR::recognize " << m_refresh_time << ":" << m_nb_new_data << " (" << ((m_queue.empty())?0.0:m_queue[0]) << ")" << endl;

	vector<bool> playing(GetNbSemitones());
	for(size_t i=0; i<playing.size(); i++)
		playing[i] = false;

	m_algo_current->apply(m_queue);

	if(getCurrentAlgorithm()->hasNoteRecognized())
		cerr << m_algo_current->getFondamentalWaveLength() << " " << f2h(GetSamplingRate()/m_algo_current->getFondamentalWaveLength()) << endl;

	//cerr << "hasNoteRecognized " << getCurrentAlgorithm()->hasNoteRecognized() << " (" << getCurrentAlgorithm()->getFondamentalNote() << ")" << endl;

	if(getCurrentAlgorithm()->hasNoteRecognized())
		playing[int(getCurrentAlgorithm()->getFondamentalNote()-GetSemitoneMin())] = true;

	m_quantizer.quantize(playing, GetSemitoneMin());

	// get some real time stats
	double current_time = getTime();
	m_recognition_stats.push_front(recon_stat(current_time, m_refresh_time, m_quantizer.getMinStoredRecon(), m_capture_thread.getNbPendingData()));

	for(size_t i=0; i<playing.size(); i++)
	{
		if(playing[i])
		{
			int ih = i+GetSemitoneMin()-m_quantizer.getSemitoneMin();
			if(!m_notes_history[ih].empty() && !m_notes_history[ih].front()->finished)
			{
				NoteDescription* note = m_notes_history[ih].front();

				note->reliability *= note->quantizer_use;
				note->addReconsStats(m_recognition_stats.front());
				note->reliability += m_quantizer.m_channels[ih].reliability;
				note->quantizer_use++;
				note->reliability /= note->quantizer_use;
			}
		}
	}

	// keep only one second of stats
	while(!m_recognition_stats.empty() && current_time-m_recognition_stats.back().time > 1000)
		m_recognition_stats.pop_back();

	cerr << "(" << m_quantizer.getMinStoredRecon() << ")" << endl;

	double min_refresh=1000, max_refresh=0;
	m_min_used_recon=1000000;
	m_min_pending_data=1000000;
	m_max_pending_data=0;
	m_avg_refresh = 0;
	for(size_t i=0; i<m_recognition_stats.size(); i++)
	{
		cerr << "(" << m_recognition_stats[i].refresh << " " << m_recognition_stats[i].used_recon << ")" << endl;
		min_refresh = min(min_refresh, m_recognition_stats[i].refresh);
		max_refresh = max(max_refresh, m_recognition_stats[i].refresh);
		m_avg_refresh += int(m_recognition_stats[i].refresh);
		m_min_used_recon = min(m_min_used_recon, m_recognition_stats[i].used_recon);
		m_min_pending_data = min(m_min_pending_data, m_recognition_stats[i].pending_data);
		m_max_pending_data = max(m_max_pending_data, m_recognition_stats[i].pending_data);
	}
	m_avg_refresh /= m_recognition_stats.size();
	m_refresh_variation = max_refresh - min_refresh;
}

void ANR::noteStarted(int tag, int ht, double dt)
{
	m_most_recent_note = tag;

#ifdef FANR_OUTPUT_MIDI
	if(m_midi_enabled)
		m_midistr << (m_last_note=note_on(m_midistr.getA3Index()+ht)) << drain;
#endif

	int ih = ht-m_quantizer.getSemitoneMin();
	if(ih<0 || ih>=int(m_notes_history.size()))
		cerr << "ANR::noteStarted " << ht << " out of range" << endl;
	else
	{
		double current_time = getTime()+dt;

		NoteDescription* note = new NoteDescription(tag, ht, current_time, 0);
		m_notes_history[ih].push_front(note);
		m_notes_tag2descr.insert(make_pair(tag, note));

		for(size_t i=0; i<m_recognition_stats.size() && m_recognition_stats[i].time>current_time; i++)
			m_notes_history[ih].front()->addReconsStats(m_recognition_stats[i]);
	}

	cout << "ANR::noteStarted " << ht << " " << tag << endl;
}
void ANR::noteFinished(int tag, int ht, double dt)
{
#ifdef FANR_OUTPUT_MIDI
	if(m_midi_enabled)
		m_midistr << note_off(m_last_note) << drain;
#endif

	int ih = ht-m_quantizer.getSemitoneMin();
	if(ih<0 || ih>=int(m_notes_history.size()))
		cerr << "ANR::noteFinished " << ht << " out of range" << endl;
	else
	{
		if(!m_notes_history[ih].front()->finished)
		{
			m_notes_history[ih].front()->duration = getTime()+dt - m_notes_history[ih].front()->start_time;
			m_notes_history[ih].front()->finished = true;
		}
		else
			cerr << "ANR::noteFinished try to finished an already finished note ?!? ("<<ht<<")"<<endl;
	}

		cout << "ANR::noteFinished " << ht << " " << tag << endl;
}
void ANR::notePlayed(int ht, double duration, double dt)
{
#ifdef FANR_OUTPUT_STDOUT
	if(m_std_enabled)
	{
		NotesName names = LOCAL_ANGLO;
		if(!m_std_anglo_names)	names = GetNotesName();
		int ton = 0;
		if(m_std_transpose)		ton = GetTonality();
		cout << h2n(ht, names, ton) << endl;
	}
#endif
	cout << "GLGraph::notePlayed " << ht << " duration=" << duration << " at time=" << dt << endl;
}

ANR::~ANR()
{
}

