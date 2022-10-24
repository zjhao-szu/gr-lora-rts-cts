/* -*- c++ -*- */
/*
 * Copyright 2022 zjhao.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_LORA_RTS_CTS_RTSSENDER_IMPL_H
#define INCLUDED_LORA_RTS_CTS_RTSSENDER_IMPL_H

#include <lora_rts_cts/RTSSender.h>
#include <gnuradio/fft/fft.h>
#include <vector>
#include <string>
#include <queue>
namespace gr {
  namespace lora_rts_cts {

    class RTSSender_impl : public RTSSender
    {
    private:
		//切换自动机状态
		RTS_Sender_State m_state;
		pmt::pmt_t m_receive_userdata_port; 
		pmt::pmt_t m_out_userdata;
		pmt::pmt_t m_out_RTS;

		//存储当前用户要发送的数据
		std::queue<std::string> strs;
		void receiveUserData(pmt::pmt_t msg);
		
		//普通且非功能性数据定义
		uint32_t m_sf;
		uint32_t m_sampRate;
		uint32_t m_bw;
		uint32_t m_oversample_rate;
		
		uint32_t m_number_of_bins;
		uint32_t m_samp_pre_symbol;
		uint32_t m_fft_size;
	
		std::vector<gr_complex> m_upchirp,m_downchirp;

		//CAD功能
		uint32_t m_preamble_drift_max;
		gr::fft::fft_complex * m_fft;
		std::vector<uint32_t>  m_argmaxHistory;
		int m_cad_count; //cad检测次数，超过上限结束转入发送RTS
		bool m_cad_detect;
		bool detectCad();
		unsigned int pmod(int x,unsigned int n);
		uint32_t argmax(float * fft_result,float * max_value);
		uint32_t searchFFTPeek(const lv_32fc_t * fft_result,float * max_value,float * fft_res_mag);
		bool CADDetect_MinBin();

		//休眠
		uint32_t sleepTime;
		//模拟class A

		//模拟class B

		//模拟class C


    public:
      RTSSender_impl();
      ~RTSSender_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace lora_rts_cts
} // namespace gr

#endif /* INCLUDED_LORA_RTS_CTS_RTSSENDER_IMPL_H */

