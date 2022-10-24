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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>

#include "RTSSender_impl.h"

namespace gr {
  namespace lora_rts_cts {

    RTSSender::sptr
    RTSSender::make()
    {
      return gnuradio::get_initial_sptr
        (new RTSSender_impl());
    }


    /*
     * The private constructor
     */
    RTSSender_impl::RTSSender_impl()
      : gr::block("RTSSender",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0))
    {
		//注册并添加端口
		m_receive_userdata_port = pmt::mp("UserDataIn");
		m_out_userdata = pmt::mp("UserDataOut");
		m_out_RTS = pmt::mp("RTSOUT");

		message_port_register_in(m_receive_userdata_port);
		message_port_register_out(m_out_userdata);

		set_msg_handler(m_receive_userdata_port,boost::bind(&RTSSender_impl::receiveUserData,this,_1));
		m_state = S_RTS_RESET;

		//初始化普通需要参数
		m_number_of_bins = (uint32_t)(1u << m_sf); //根据sf决定每个sysbol 占用多少个bins
		m_oversample_rate = m_sampRate / m_bw; //根据 采样率 和 带宽 决定比例
		m_samples_per_symbol = (uint32_t) (m_number_of_bins * m_oversample_rate); 
		m_fft_size = m_samples_per_symbol; //fft大小一般跟symbol采样率一致
		m_preamble_drift_max = 1;//bin值相差容忍的最大误差
		m_fft = new fft::fft_complex(m_fft_size,true,1);

		m_upchirp.resize(m_samples_per_symbol);
        m_downchirp.resize(m_samples_per_symbol);
        for(int i = 0; i < m_samples_per_symbol;i++){
            double phase = M_PI / m_oversample_rate * (i - i*i / (float) m_samples_per_symbol);
            m_downchirp[i] = gr_complex(std::polar(1.0,phase));
            m_upchirp[i] = gr_complex(std::polar(1.0,-1*phase));
        }

		//初始化CAD功能
		m_cad_count = 5;
		m_cad_detect = false;
	}

	
    /*
     * Our virtual destructor.
     */
    RTSSender_impl::~RTSSender_impl()
    {
    
	}

	void
	RTSSender_impl::receiveUserData(pmt::pmt_t msg){
		std::cout<<pmt::symbol_to_string(msg)<<std::endl;
		strs.push(pmt::symbol_to_string(msg));
		m_state =  S_RTS_CAD;
	}
    void
    RTSSender_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
	  ninput_items_required[0] = noutput_items * m_samples_per_symbol;
    }

	uint32_t 
    RTSSender_impl::argmax(float * fft_result,float * max_value){
        float mag = abs(fft_result[0]);
        float max_val = mag;
        uint32_t max_index = 0;
        for(int i = 0; i < m_number_of_bins;i++){
            mag = abs(fft_result[i]);
            if(mag > max_val){
                max_index = i;
                max_val = mag;
            }
        }
        *max_value = max_val;
        return max_index;
    }

	uint32_t 
    RTSSender_impl::searchFFTPeek(const lv_32fc_t *fft_result,float * max_value,float * fft_res_mag){
        volk_32fc_magnitude_32f(fft_res_mag, fft_result, m_fft_size);
        uint32_t max_idx = argmax(fft_res_mag, max_value);
        return max_idx;
    }

	bool 
    RTSSender_impl::CADDetect_MinBin(){
        float m_preamble_idx = m_argmaxHistory[0];
        if(m_preamble_idx == 0) return false;
        // Check for discontinuities that exceed some tolerance
        bool preamble_found = true;
        int count = 4;
        for (int i = 1; i < m_argmaxHistory.size(); i++)
        {  
        //   std::cout<<"firstIdx"<<m_preamble_idx<<" "<<"historyIdx"<<m_argmaxHistory[i]<<std::endl;
          unsigned int dis = pmod(int(m_preamble_idx) - int(m_argmaxHistory[i]), m_fft_size);
          if (dis <= m_preamble_drift_max)
          {
            count--;
          }
        }
        return count > 0 ? false : true;
    };

    int
    RTSSender_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      	const gr_complex *in = (const gr_complex *) input_items[0];
    	unsigned int *out = (unsigned int *) output_items[0];
		unsigned int num_consumed = m_samples_per_symbol;
        uint32_t maxIndex = 0;
        float maxValue = 0;

		
		
		switch (m_state)
		{
			case S_RTS_RESET:{
				while(!strs.empty()){
					strs.pop();
				}
				break;
			}
			case S_RTS_RECEIVE_DATA:{
				//接收用户数据并切换rts-cad状态
				break;
			}
			case S_RTS_CAD:{
				gr_complex * signalBlockBuffer =  (gr_complex *) volk_malloc(m_fft_size * sizeof(gr_complex),volk_get_alignment());
				gr_complex * outbuf =  (gr_complex *) volk_malloc(m_fft_size * sizeof(gr_complex),volk_get_alignment());
				float * fft_res_mag = (float*)volk_malloc(m_fft_size*sizeof(float), volk_get_alignment());

				volk_32fc_x2_multiply_32fc(signalBlockBuffer, in, &m_downchirp[0], m_samples_per_symbol);
				
				memset(m_fft->get_inbuf(), 0,m_fft_size*sizeof(gr_complex));
				memcpy(m_fft->get_inbuf(),&signalBlockBuffer[0],m_samples_per_symbol*sizeof(gr_complex));
				m_fft->execute();
				
				maxIndex = searchFFTPeek(m_fft->get_outbuf(),&maxValue,fft_res_mag);
				if(maxValue >= 0.1){
					// std::cout<<"max value is"<<maxValue<<std::endl;
					m_argmaxHistory.insert(m_argmaxHistory.begin(),maxIndex);
				}else{
					// std::cout<<"while noise is"<<maxValue<<std::endl;
				}

				if(m_argmaxHistory.size() > REQUIRE_PREAMBLE_NUMBER){
					m_argmaxHistory.pop_back();
				}
				if(m_argmaxHistory.size() >= REQUIRE_PREAMBLE_NUMBER){

					m_cad_detect = CADDetect_MinBin();
					//todo
					//两种方案 
					//一种是连续，只删除一个
					//如果采用第一种方案的方式，只需要将下面代码注释就可以
					//一种是不连续 清空历史记录
					m_argmaxHistory.clear();
					m_cad_count -= 1;
					
				}
				if(m_cad_detect){
					m_state = S_RTS_RESET;
				}
				if(m_cad_count == 0 && m_cad_detect==false){
					m_state = S_RTS_send_RTS;
				}
				break;
			}
			// case S_RTS_RESET:{
			// 	break;
			// }
			// case S_RTS_RESET:{
			// 	break;
			// }
			// case S_RTS_RESET:{
			// 	break;
			// }
			// case S_RTS_RESET:{
			// 	break;
			// }
			// case S_RTS_RESET:{
			// 	break;
			// }
			// case S_RTS_RESET:{
			// 	break;
			// }
			default:
				break;
		}
	

      // Do <+signal processing+>
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace lora_rts_cts */
} /* namespace gr */

