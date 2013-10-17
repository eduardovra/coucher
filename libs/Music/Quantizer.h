// Copyright 2005 "Gilles Degottex"

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


#ifndef _QUANTIZER_H_
#define _QUANTIZER_H_

#include <string>
#include <deque>
#include <vector>
#include <iostream>
using namespace std;
#include <QtCore/qdatetime.h>
#include <Music/Music.h>
using namespace Music;
#include <CppAddons/Observer.h>

struct QuantizerListener : Listener<QuantizerListener>
{
	virtual void noteStarted(int tag, int ht, double dt)		{todo("noteStarted");}
	virtual void noteFinished(int tag, int ht, double dt)		{todo("noteFinished");}
	virtual void notePlayed(int ht, double duration, double dt)	{todo("notePlayed");}
};

/*!
  a function object for merging small note events into note events with a duration.
  - Fill small holes where the note should appears
  - Ignore notes which are too small
  */
class Quantizer : public Talker<QuantizerListener>
{
	QTime m_time;

	//! in millis
	float m_tolerance;
	float m_min_density;

	struct State{
		double time;
		bool play;
		State(double t, bool p) : time(t), play(p) {}
	};
	struct Channel{
		deque<State> old_states;
		enum{QC_NOTHING, QC_STARTING, QC_PLAYING} state;
		QTime lag;
		QTime duration;
		double reliability;
		int last_tag;
		Channel() : state(QC_NOTHING) {}
	};
	int m_min_ht;

  public:
	Quantizer(float tolerance=1, float min_density=0.5);

	double getTolerance()							{return m_tolerance;}
	void setTolerance(float tolerance)				{m_tolerance=tolerance;}
	double getMinDensity()							{return m_min_density;}
	void setMinDensity(float min_density)			{m_min_density=min_density;}

	void quantize(const vector<bool> hts, int min_ht);
	void update(int ht);

	int m_min_stored_recon;
	int getMinStoredRecon(){return m_min_stored_recon;}

	int getNbChannels()								{return m_channels.size();}
	int getSemitoneMin()									{return m_min_ht;}
	vector<Channel> m_channels;

	void cutAll();

	double getLatency()								{return m_tolerance;}

	virtual ~Quantizer(){}
};

#endif // _QUANTIZER_H_

