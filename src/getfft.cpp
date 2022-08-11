#include <stdio.h>
#include <complex>
#include "hls_fft.h"


const char FFT_INPUT_WIDTH                     = 16;
const char FFT_OUTPUT_WIDTH                    = FFT_INPUT_WIDTH;
const char FFT_CONFIG_WIDTH                    = 24;
const char FFT_NFFT_MAX                        = 10;
const int  FFT_LENGTH                          = 1 << FFT_NFFT_MAX;

struct config1 : hls::ip_fft::params_t {
	static const unsigned ordering_opt = hls::ip_fft::natural_order;
	static const unsigned config_width = FFT_CONFIG_WIDTH;
	static const bool has_nfft = true;
	static const unsigned max_nfft = FFT_NFFT_MAX;
};

typedef hls::ip_fft::config_t<config1> config_t;
typedef hls::ip_fft::status_t<config1> status_t;
typedef ap_fixed<FFT_INPUT_WIDTH,1> data_in_t;
typedef ap_fixed<FFT_OUTPUT_WIDTH,FFT_OUTPUT_WIDTH-FFT_INPUT_WIDTH+1> data_out_t;
typedef std::complex<data_in_t> cmpxDataIn;
typedef std::complex<data_out_t> cmpxDataOut;

const int max = 1 << FFT_INPUT_WIDTH; // might not work for > 32 bits!
const int max_half_minus_one = (max/2)-1;
const double sc = ldexp(1.0, FFT_INPUT_WIDTH-1);

void access_src(unsigned short *pSrc, cmpxDataIn in[FFT_LENGTH], int len)
{
	double input_data_re;
	unsigned short cachemem[128], d;

	for(int i=0;i<len;)
	{
#pragma HLS pipeline
#pragma HLS loop_tripcount min=1 max=16
		int m = ((len-i)>128) ? 128 : len-i;
		memcpy(cachemem, &pSrc[i], m*sizeof(unsigned short));
		for(int k=0;k<m;k++)
		{
#pragma HLS loop_tripcount min=1 max=128
			d = cachemem[k];
			input_data_re = (d > max_half_minus_one) ? (d-(max-1))/sc : d/sc;
			in[i+k] = cmpxDataIn(input_data_re, 0);
		}
		i += m;
	}
}

void access_dst(unsigned short *pDst, cmpxDataOut out[FFT_LENGTH], int len)
{
	unsigned short cachemem[128];

	for(int i=0;i<len;)
	{
#pragma HLS pipeline
#pragma HLS loop_tripcount min=1 max=16
		int m = ((len-i)>128) ? 128 : len-i;
		for(int k=0;k<m;k++)
		{
#pragma HLS loop_tripcount min=1 max=128
			float An = sqrt(out[i+k].real().to_float()*out[i+k].real().to_float()+out[i+k].imag().to_float()*out[i+k].imag().to_float());
			An = An * 4;
			cachemem[k] = An*sc;
		}
		cachemem[0] = cachemem[0] >> 1;
		memcpy(&pDst[i], cachemem, m*sizeof(unsigned short));
		i += m;
	}
}

void getfft(int nfft, unsigned short *pSrc, unsigned short *pDst)
{
#pragma HLS INTERFACE s_axilite port=nfft
#pragma HLS INTERFACE m_axi port=pSrc offset=slave
#pragma HLS INTERFACE m_axi port=pDst offset=slave
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS dataflow

	config_t fft_config;
	status_t fft_status;
	cmpxDataIn xn[FFT_LENGTH];
	cmpxDataOut xk[FFT_LENGTH];

	fft_config.setDir(0);
	fft_config.setSch(0x2AB);
	fft_config.setNfft(nfft);
	access_src(pSrc, xn, 1<<nfft);
	hls::fft<config1>(xn, xk, &fft_status, &fft_config);
	access_dst(pDst, xk, 1<<nfft);
}
