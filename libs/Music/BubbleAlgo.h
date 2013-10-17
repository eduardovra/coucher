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


#ifndef _BubbleAlgo_h_
#define _BubbleAlgo_h_

#include <vector>
#include <deque>
#include <map>
#include <list>
#include <complex>
using namespace std;
#include "Algorithm.h"
#include "Correlation.h"
	
namespace Music
{
	class BubbleAlgo : public Algorithm
	{
		typedef double Type;

		size_t m_wave_length;
		size_t m_min_length;
		size_t m_max_length;
		Type m_error_threshold;
		Type m_conv_threshold;

		struct Bubble
		{
			size_t s;
			size_t count;
			size_t length;
			Type score;
			Type err;
			complex<double> conv;
			size_t n;
			size_t f;
			bool todrop;
			size_t gcd_count;
			Bubble(size_t as=0) : s(as), count(0), score(0.0), err(0.0), conv(complex<double>(0.0,0.0)), n(0), todrop(false), gcd_count(0) {}
		};

		vector<Bubble> m_bubbles;
		vector< vector<complex<double> > > m_waves;
		vector< vector<double> > m_waves_best;
		map<Type,list<size_t> > m_sort;
		map<Type,list<size_t> > m_best_set;

		Type diff(const deque<double>& buff, size_t i, size_t j)	{return abs(buff[i] - buff[j]);}

		void setMinMaxLength(size_t min_length, size_t max_length)
										{m_min_length=min_length; m_max_length=max_length;}

	  protected:
		void init();
		virtual void AFreqChanged()							{init();}
		virtual void samplingRateChanged()					{init();}
		virtual void semitoneBoundsChanged()				{init();}

	  public:
		BubbleAlgo();

		virtual int getSampleAlgoLatency() const		{return 2*m_waves.back().size();}

		void apply(const deque<double>& buff);

		virtual bool hasNoteRecognized() const			{return m_wave_length!=0;}
		virtual int getFondamentalWaveLength() const	{return m_wave_length;}
		void setErrorThreshold(double threshold)		{m_error_threshold = threshold;}
		void setConvolutionThreshold(double threshold)	{m_conv_threshold = threshold;}

		virtual ~BubbleAlgo() {}
	};
}

#endif

