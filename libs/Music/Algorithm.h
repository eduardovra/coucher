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


#ifndef _Algorithm_h_
#define _Algorithm_h_

#include <list>
#include <vector>
#include <deque>
#include <iostream>
using namespace std;
#include "Music.h"

namespace Music
{
	class Algorithm : public SettingsListener
	{
		static list<Algorithm*> s_algos;

	  protected:
		double m_volume_treshold;
		double m_volume_max;

		virtual void samplingRateChanged()					{cerr<<__FILE__<<":"<<__LINE__<<" Algorithm::samplingRateChanged Not Yet Implemented"<<endl;}
		virtual void AFreqChanged()							{cerr<<__FILE__<<":"<<__LINE__<<" Algorithm::AFreqChanged Not Yet Implemented"<<endl;}
		virtual void semitoneBoundsChanged()				{cerr<<__FILE__<<":"<<__LINE__<<" Algorithm::semitoneBoundsChanged Not Yet Implemented"<<endl;}

	  public:
		Algorithm(double volume_treshold);

		inline double getVolumeTreshold()					{return m_volume_treshold;}
		inline void setVolumeTreshold(double t)				{m_volume_treshold=t;}
		inline double getVolumeMax()						{return m_volume_max;}

		virtual int getSampleAlgoLatency() const =0;
		virtual double getAlgoLatency() const				{return double(getSampleAlgoLatency())/GetSamplingRate();}

		virtual void apply(const deque<double>& buff)=0;
		virtual bool hasNoteRecognized() const =0;
		virtual int getFondamentalWaveLength() const		{return int(GetSamplingRate()/getFondamentalFreq());}
		virtual double getFondamentalFreq() const			{return double(GetSamplingRate())/getFondamentalWaveLength();}
		virtual double getFondamentalNote() const			{return f2hf(getFondamentalFreq());}

		virtual ~Algorithm();
	};

	class Transform : public Algorithm
	{
	  protected:
		double m_components_treshold;

		vector< complex<double> > m_formants;
		vector<double> m_components;
		double m_components_max;
		int m_first_fond;
		vector<bool> m_is_fondamental;

	  public:

		inline double getComponentsTreshold()				{return m_components_treshold;}
		inline void setComponentsTreshold(double t)			{m_components_treshold=t;}

		Transform(double volume_treshold, double components_treshold, int size);
		Transform(double volume_treshold, double components_treshold);

		size_t size() const									{return m_components.size();}

		const vector< complex<double> >& getFormants()		{return m_formants;}
		const vector<double>& getComponents()				{return m_components;}
		double getComponentsMax()							{return m_components_max;}
		virtual bool hasNoteRecognized() const				{return m_first_fond!=-1;}
		int getFondamentalRelativeNote() const				{return m_first_fond;}
		virtual double getFondamentalNote() const			{return (hasNoteRecognized())?m_first_fond+GetSemitoneMin():Music::UNDEFINED_SEMITONE;}
		bool isFondamentalNote(size_t i)					{return m_is_fondamental[i];}

		virtual int getFondamentalWaveLength() const	{return int(GetSamplingRate()/h2f(getFondamentalNote()));}

		virtual ~Transform(){}
	};
}

#endif // _Algorithms_h_

