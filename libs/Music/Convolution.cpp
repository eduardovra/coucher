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


#include "Convolution.h"
#include <iostream>
using namespace std;
#include <CppAddons/Math.h>
#include "Music.h"

namespace Music
{
	Convolution::Convolution(double latency_factor, double gauss_factor, double ht)
	: m_ht(ht)
	, m_freq(h2f(m_ht))
	, m_latency_factor(latency_factor)
	, m_duration(m_latency_factor/m_freq)
	, m_wave(int(m_duration*GetSamplingRate()))
	{
//		cerr << "Convolution::Convolution " << ht << endl;
		double c = - 2.0*Math::Pi * m_freq / GetSamplingRate();

		double u = Usefull(Win_Sinc(gauss_factor));

		for(size_t j=0; j<m_wave.size(); j++)
			m_wave[j] = exp(complex<double>(0.0, c*j))*double(2.0/m_wave.size()) * win_sinc(j/double(m_wave.size()), gauss_factor)/u;
		// 		m_wave[j] = exp(complex<double>(0.0, c*j))*double(2.0*Math::Pi/m_wave.size()) * win_sinc(j/double(m_wave.size()), win_factor)/u;
	}

	void Convolution::apply(const deque<double>& buff, int start)
	{
		if(buff.size()-start < m_wave.size())
			m_formant = complex<double>(0.0,0.0);
		else
		{
			m_formant = complex<double>(0.0,0.0);

			for(size_t i=0; i<m_wave.size(); i++)
				m_formant += m_wave[i]*buff[i+start];
		}
	}

#if 0
	DataMultiplierConvolution::DataMultiplierConvolution(double AFreq, int dataBySecond, double rep, double win_factor, int h)
	: m_rep(rep)
	, m_freq(h2f(h, AFreq))
	, m_length(size_t((1.0/m_freq)*dataBySecond))
	, m_wave(size_t(m_rep*m_length))
	{
		double c = - 2.0*Math::Pi * m_freq / dataBySecond;

		//	double u = Usefull(Win_Gauss(win_factor));
		double u = Usefull(Win_Sinc(win_factor));

		//	cout << "Convolution::Convolution " << m_wave.size() << endl;

		for(size_t j=0; j<m_wave.size(); j++)
			m_wave[j] = exp(complex<double>(0.0, c*j))*double(2.0*Math::Pi/m_wave.size()) * win_sinc(j/double(m_wave.size()), win_factor)/u;

		m_fwd_plan = rfftw_create_plan(m_wave.size(), FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE | FFTW_OUT_OF_PLACE | FFTW_USE_WISDOM);
		m_bck_plan = rfftw_create_plan(m_wave.size(), FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE | FFTW_OUT_OF_PLACE | FFTW_USE_WISDOM);
		m_in = new fftw_real[m_wave.size()];
		m_out = new fftw_real[m_wave.size()];
	}

	void DataMultiplierConvolution::receive(const deque<double>& buff, int start)
	{
		if(buff.size()-start < m_length)	return;

		//	for(int i=0; i<m_length; i++)
		//		m_in[i] = m_queue[i];
		//	rfftw_one(m_fwd_plan, m_in, m_out);
		//	for(int i=0; i<m_fourier_FFT.size(); i++)
		//		m_fourier_FFT[i] = (m_out[i+skip]*m_out[i+skip] + m_out[m_queue.size()-(i+skip)]*m_out[m_queue.size()-(i+skip)])/100;

		m_trans = complex<double>(0.0,0.0);

		for(size_t i=0; i<m_wave.size(); i++)
			m_trans += m_wave[i]*buff[i+start];
	}

	DataMultiplierConvolution::~DataMultiplierConvolution()
	{
		rfftw_destroy_plan(m_fwd_plan);
		rfftw_destroy_plan(m_bck_plan);

		delete m_in;
		delete m_out;
	}
#endif
}

