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


#ifndef _Instrument_h_
#define _Instrument_h_

#include <assert.h>
#include <vector>
#include <iostream>
#include <fstream>
using namespace std;
#include <CppAddons/Math.h>
using namespace Math;
#include "Music.h"

namespace Music
{

#define MAX_NB_FREQ 24
#define MAX_NB_HARM 7
#define MAX_RDM_HT -12
#define MIN_RDM_HT 12

//! an abstract instrument
struct Instrument
{
	virtual vector< Math::polar<double> >* getFreqs(){return NULL;}

	virtual double gen_value(int k, double phase=0.0)=0;

	virtual ~Instrument(){}
};

//! an harmonic instrument
/*!
 * une fondamentale choisie dans un interval et des composantes harmoniques
 */
struct HarmInstrument : Instrument
{
	int m_dataBySecond;
	double m_time;
	vector< Math::polar<double> > m_freqs;
	int m_base_ht;
	double m_base_freq;

	HarmInstrument(int dataBySecond, double AFreq, int nb_harm=int((rand()/RAND_MAX)*MAX_NB_HARM), int min_rdm_ht=-12, int max_rdm_ht=12)
	: m_dataBySecond(dataBySecond)
	, m_time(0.0)
	, m_freqs(nb_harm+1)
	, m_base_ht(int(min_rdm_ht+(rand()/RAND_MAX)*(max_rdm_ht-min_rdm_ht+1)))
	, m_base_freq(h2f(m_base_ht, AFreq))
	{
		for(size_t i=0; i<m_freqs.size(); i++)
			m_freqs[i] = Math::polar<double>(Random::s_random.nextDouble(), 2*Math::Pi*Random::s_random.nextDouble());

		// s'il n'y a pas de composantes, ça ne sert à rien de choisir une amplitude au hazard pour la fondamentale
		if(m_freqs.size()==1)
			m_freqs[0].mod = 1.0;

		// la fondamentale doit avoir une amplitude minimale
		m_freqs[0].mod = m_freqs[0].mod*0.8 + 0.2;
	}

	HarmInstrument(int dataBySecond, vector< Math::polar<double> >& freqs, double base_freq)
	: m_dataBySecond(dataBySecond)
	, m_time(0.0)
	, m_freqs(freqs)
	, m_base_freq(base_freq)
	{
	}

	virtual vector< Math::polar<double> >* getFreqs(){return &m_freqs;}

	virtual double gen_value(int k, double phase=0.0)
	{
		double value = 0.0;

		for(size_t i=0; i<m_freqs.size(); i++)
			value += m_freqs[i].mod * sin(h2f(k, m_base_freq)*(i+1)*m_time+m_freqs[i].arg + phase);

		m_time += 2*Math::Pi/m_dataBySecond;

		return value;
	}
};

inline ostream& operator<<(ostream& out, const HarmInstrument& instr)
{
	cout << "HarmInstrument (mod:arg)" << endl;
	for(size_t i=0; i<instr.m_freqs.size(); i++)
		out << "\t" << instr.m_base_freq*(i+1) << " (" << instr.m_freqs[i].mod << ":" << instr.m_freqs[i].arg << ")" << endl;

	return out;
}

/* basé sur un motif pré-enregistré
*/
struct WaveInstrument : Instrument
{
	vector<double> m_data;

	double m_time;
	
	int m_dataBySecond;
	double m_AFreq;

	WaveInstrument(int dataBySecond, double AFreq, const string& file_name)
	: m_time(0.0)
	, m_dataBySecond(dataBySecond)
	, m_AFreq(AFreq)
	{
		ifstream file(file_name.c_str());

		assert(file.is_open());

		double index;
		double value;
		file >> index;
		file >> value;
		while(!file.eof())
		{
			m_data.push_back(value);

			file >> index;
			file >> value;
		}
	}

	WaveInstrument(int dataBySecond, double AFreq, const deque<double>& data)
	: m_data(data.size())
	, m_time(0.0)
	, m_dataBySecond(dataBySecond)
	, m_AFreq(AFreq)
	{
		for(size_t i=0; i<data.size(); i++)
			m_data[i] = data[i];
	}

	WaveInstrument(int dataBySecond, double AFreq, const vector<double>& data)
	: m_data(data)
	, m_time(0.0)
	, m_dataBySecond(dataBySecond)
	, m_AFreq(AFreq)
	{
	}

	virtual double gen_value(int k, double phase=0.0)
	{
		double f = h2f(k, m_AFreq);

		int i = int(f*(m_time+phase)*m_data.size()/(2*Math::Pi))%m_data.size();

		m_time += 2*Math::Pi/m_dataBySecond;

		return m_data[i];
	}
};

/* faux instrument: les composantes devraient toujours être des harmoniques
*/
struct FreqInstrument : Instrument
{
	static double get_freq(double lowest_freq, int i, double AFreq)
	{
		return h2f(int(f2h(lowest_freq, AFreq)+i), AFreq);
	}

	int m_dataBySecond;
	double m_AFreq;
	double m_time;
	vector< Math::polar<double> > m_freqs;
	double m_lowest_freq;

	virtual vector< Math::polar<double> >* getFreqs(){return &m_freqs;}

	FreqInstrument(int dataBySecond, double AFreq, double lowest_freq, int nb_freq=int((rand()/RAND_MAX)*MAX_NB_FREQ))
	: m_dataBySecond(dataBySecond)
	, m_AFreq(AFreq)
	, m_time(0.0)
	, m_freqs(nb_freq)
	, m_lowest_freq(lowest_freq)
	{
		//		cout << "FreqInstrument::FreqInstrument" << endl;

		for(size_t i=0; i<m_freqs.size(); i++)
			m_freqs[i] = Math::polar<double>((rand()/RAND_MAX), Math::Pi*(rand()/RAND_MAX));

		// s'il n'y a pas de composantes, ça ne sert à rien de choisir une amplitude au hazard pour la fondamentale
		if(m_freqs.size()==1)
			m_freqs[0].mod = 1.0;

		m_freqs[1].mod = 0.0;		// TEMP

		// la fondamentale doit avoir une amplitude minimale
		m_freqs[0].mod = m_freqs[0].mod*0.8 + 0.2;

		// rajoute un bruit sur la vitesse
		double upper_freq = h2f(1, m_lowest_freq);
		double lower_freq = h2f(-1, m_lowest_freq);
		cout << "m_lowest_freq=" << m_lowest_freq << " upper_freq="<<upper_freq<<" lower_freq="<<lower_freq;
		m_lowest_freq += (rand()/RAND_MAX)*(upper_freq-lower_freq)/16 - (m_lowest_freq-lower_freq)/16;
		cout << " => " << m_lowest_freq << endl;

		//		cout << "/FreqInstrument::FreqInstrument" << endl;
	}

	FreqInstrument(int dataBySecond, double AFreq, vector< Math::polar<double> >& freqs, double lowest_freq)
	: m_dataBySecond(dataBySecond)
	, m_AFreq(AFreq)
	, m_time(0.0)
	, m_freqs(freqs)
	, m_lowest_freq(lowest_freq)
	{
	}

	virtual double gen_value(int k, double phase=0.0)
	{
		//		cout << "FreqInstrument::gen_value" << endl;

		double value = 0.0;

		for(size_t i=0; i<m_freqs.size(); i++)
			value += m_freqs[i].mod * sin(get_freq(m_lowest_freq,i+k,m_AFreq)*m_time + m_freqs[i].arg);

		m_time += 2*Math::Pi/m_dataBySecond;

		return value;
	}
};

inline ostream& operator<<(ostream& out, const FreqInstrument& instr)
{
	cout << "FreqInstrument (mod:arg)" << endl;
	for(size_t i=0; i<instr.m_freqs.size(); i++)
		out << "\t" << FreqInstrument::get_freq(instr.m_lowest_freq,i,instr.m_AFreq) << " (" << instr.m_freqs[i].mod << ":" << instr.m_freqs[i].arg << ")" << endl;

	return out;
}

}

#endif // _Instrument_h_

