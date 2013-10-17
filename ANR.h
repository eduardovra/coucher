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



#ifndef _ANR_h_
#define _ANR_h_

//#include "config.h"

#include <deque>
#include <map>
#include <QtCore/qdatetime.h>
#include <CppAddons/Singleton.h>
#ifdef FANR_OUTPUT_MIDI
#include <Music/omidistream.h>
using namespace Music;
#endif
#include <Music/TimeAnalysis.h>
#include <Music/MultiCorrelationAlgo.h>
#include <Music/AutocorrelationAlgo.h>
#include <Music/BubbleAlgo.h>
#include <Music/Quantizer.h>
using namespace Music;

#include "CaptureThread.h"

class ANR : public Singleton<ANR>, public QuantizerListener
{
  public:
	ANR();

	// Capture
	QTime m_time;
	bool m_is_running;
	float m_old_running_time;
	float m_forgotten_time;

	QTime m_refresh_time_timer;	//! time between each refresh (ie. each recognition computation) in milis
	double m_refresh_time;

	CaptureThread m_capture_thread;
	void start();
	void pause(bool toggled);
	bool isRunning()				{return m_is_running;}
	double getTime()				{return (m_is_running)?m_time.elapsed():m_old_running_time;} // return running time in miliseconds
	float getForgottenTime()		{return m_forgotten_time;}
	deque<double> m_queue;
	int m_nb_new_data;

	// Algos
	MultiCorrelationAlgo* m_algo_multicorr;
	AutocorrelationAlgo* m_algo_autocorr;
	BubbleAlgo* m_algo_bubble;
	Algorithm* m_algo_current;

	Transform* m_transform_current;

	Algorithm* getCurrentAlgorithm()			{return m_algo_current;}
	Transform* getCurrentTransform()			{return m_transform_current;}

	// Params
	void init();

	double getRefreshTime()				{return (isRunning())?m_refresh_time:0.0;}

	// Recognition
	void recognize();

	int m_most_recent_note;	// in tag

	virtual void noteStarted(int tag, int ht, double dt);
	virtual void noteFinished(int tag, int ht, double dt);
	virtual void notePlayed(int ht, double duration, double dt);

	// Statistics
	struct recon_stat {
		double time;
		double refresh;
		int used_recon;
		int pending_data;
		recon_stat(double t, double r, int u, int p) : time(t), refresh(r), used_recon(u), pending_data(p) {}
	};
	deque<recon_stat> m_recognition_stats;
	double m_refresh_variation;
	int m_min_used_recon;
	int m_min_pending_data;
	int m_max_pending_data;
	int m_avg_refresh;
	
	Quantizer m_quantizer;

	// result
	// note description
	struct NoteDescription
	{
		//! unique id
		int tag;
		//! semi-tone
		int ht;
		//! time in millis
		float start_time;
		//! duration in millis
		float duration;

		int quantizer_use;
		float reliability;
		
		//! true if the note finished
		bool finished;

		deque<recon_stat> used_recons;

		void addReconsStats(recon_stat& rec_stat)
		{
			used_recons.push_front(rec_stat);
		}

		NoteDescription(int id, int h, double st, double d)
		: tag(id), ht(h), start_time(st), duration(d), quantizer_use(0), reliability(0.0), finished(false) {}
	};
	vector<deque<NoteDescription*> > m_notes_history;
	map<int,NoteDescription*> m_notes_tag2descr;

	// Output

#ifdef FANR_OUTPUT_MIDI
	bool m_midi_enabled;
	note_on m_last_note;
	// the midi output stream
	omidistream m_midistr;
#endif
	bool m_std_enabled;
	bool m_std_anglo_names;
	bool m_std_transpose;

	virtual ~ANR();
};

inline ANR& anr(){return ANR::getInstance();}

#endif //_ANR_h_

