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


#ifndef _MultiCorrelationAlgo_h_
#define _MultiCorrelationAlgo_h_

#include <vector>
#include <deque>
using namespace std;
#include "Algorithm.h"
#include "Correlation.h"

namespace Music
{
	//! Compute error for each possible freqs with correlation algorithm
	class MultiCorrelationAlgo : public Transform
	{
		double m_latency_factor;
		double m_test_complexity;
		int m_max_harm;

	  protected:
		void init();
		virtual void AFreqChanged()							{init();}
		virtual void samplingRateChanged()					{init();}
		virtual void semitoneBoundsChanged()				{init();}

	  public:
		//! correlation filters
		vector< Correlation* > m_corrs;

		bool is_minima(int ih);

	  public:
		double m_pitch_tolerance;

		void setLatencyFactor(double latency_factor);
		double getLatencyFactor()							{return m_latency_factor;}
		void setTestComplexity(double test_complexity)		{m_test_complexity = test_complexity;}
		double getTestComplexity()							{return m_test_complexity;}

		virtual int getSampleAlgoLatency() const			{return int((getAlgoLatency()/1000.0)*GetSamplingRate());}
		//! in millis
		virtual double getAlgoLatency() const	{return 1000.0*(max(double((m_max_harm+1)*m_corrs[0]->m_s), (m_latency_factor+1)*m_corrs[0]->m_s))/GetSamplingRate();}
//		virtual double getAlgoLatency()	{return 1000.0*((m_latency_factor-1)*m_corrs[0]->m_smax + m_corrs[0]->m_latency_factor*m_corrs[0]->m_smax+m_corrs[0]->m_smax)/getSamplingRate();}

		//! unique ctor
		/*!
		 * \param AFreq frequency of A3 (440.0)
		 * \param sampling_rate wave capture sampling rate (11khz;44khz)
		 * \param latency_factor latency factor for statistical purpose [1;oo[
		 * \param pitch_tolerance tolareted pitch variation between the analysed semi-tone ]0;0.5]
		 * \param seek_factor seek factor [1;oo[
		 * \param min_ht minimal analysed semi-tone (-48;+48)
		 * \param max_ht maximal analysed semi-tone ]min_ht;+48)
		 */
		MultiCorrelationAlgo(int latency_factor, double test_complexity);

		//! overwrited compute fonction
		virtual void apply(const deque<double>& buff);
		
		virtual int getFondamentalWaveLength() const;

		virtual ~MultiCorrelationAlgo();
	};
}

#endif

