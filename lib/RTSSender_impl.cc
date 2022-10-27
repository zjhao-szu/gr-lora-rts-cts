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

#define SEND_MODULE SEND_MODULE_PACKET
#define SEND_MODULE_PACKET 0
#define SEND_MODULE_RTS 1

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
		m_receiveLoRaDecodeMessage = pmt::mp("DecodeMessage");
		m_out_userdata = pmt::mp("UserDataOut");
		m_out_RTS = pmt::mp("RTSOUT");

		message_port_register_in(m_receiveLoRaDecodeMessage);
		message_port_register_in(m_receive_userdata_port);
		message_port_register_out(m_out_userdata);
		message_port_register_out(m_out_RTS);
		
		set_msg_handler(m_receiveLoRaDecodeMessage,boost::bind(&RTSSender_impl::receiveDecodeMessage,this,_1));
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
    RTSSender_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
	  ninput_items_required[0] = noutput_items * m_samples_per_symbol;
    }
	/********************************************************************
	 * 接收用户需要发送的数据包
	 * 
	********************************************************************/
	void
	RTSSender_impl::receiveUserData(pmt::pmt_t msg){
		std::cout<<pmt::symbol_to_string(msg)<<std::endl;
		strs.push(pmt::symbol_to_string(msg));
		m_state =  S_RTS_CAD;
	}
	
	void
	RTSSender_impl::receiveDecodeMessage(pmt::pmt_t msg){
		//only state is receiving, resolve the message
		if(m_state == S_RTS_RECEIVE1 || m_state == S_RTS_RECEIVE2 || m_state == S_RTS_RECEIVE_Slot || m_state == S_RTS_RECEIVE_CLass_C){
			std::string str = pmt::symbol_to_string(msg);
			std::cout<<str<<std::endl;
			if(str.find("RTS",0) != std::string::npos){
				int indexID = str.find("NodeID",0);
				int indexColon = str.find(":",indexID);
				int indexComma = str.find(",",indexID);
				int winNodeID = std::stoi(str.substr(indexcolon+1,indexComma));
				if(winNodeID == m_nodeId){
					m_state = S_RTS_Send_Data;
				}else if(m_state == S_RTS_RECEIVE1 && winNodeID != m_nodeId){
					m_state = S_RTS_TO_RECEIVE2;
				}else if(m_state == S_RTS_RECEIVE2 && winNodeID != m_nodeId){
					m_state = S_RTS_SLEEP;
				}
			}else{
				//doing other work
				messageDebugPrint(msg);
			};
		}
    }

	/********************************************************************
	 * CAD功能检测
	 * 
	********************************************************************/
	uint32_t 
    RTSSender_impl::argmax(float * fft_result,float * max_value){
        float mag = abs(fft_result[0]);
        float max_val at * fft_res_mag){
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
	/********************************************************************
	 * 发送RTS数据包-基于不同的发送方式
	 * 
	********************************************************************/
	void
	RTSSender_impl::sendRTSByPacket(){
		uint32_t totalLen = 0;
		for(int i =0;i < n;i++){
			totalLen += strs[i].length();
		}
		std:string msg = "Type:RTS,ID:"+ m_nodeId + ",duration:"+ totalLen;
		pmt::pmt_t msg = pmt::string_to_symbol(msg);
		message_port_pub(m_out_userdata,msg);
	}
	//TODO.....
	void
	RTSSender_impl::sendRTSBySerialization(){
		
	}
	/********************************************************************
	 * Debug模式
	 * 
	********************************************************************/
	void
	RTSSender_impl::messageDebugPrint(const pmt::pmt_t &msg){
		std::stringstream sout;
		if (pmt::is_pdu(msg)){
			const auto& meta = pmt::car(msg);
			const auto& vector = pmt::cdr(msg);


			sout << "***** PDU DEBUG PRINT *****"<<std::endl
				 << pmt::write_string(meta) << std::endl;
			size_t len = pmt::blob_length(vector);
			sout << "pdu length = " << len << " bytes" << std::endl
                 << "pdu vector contents = " << std::endl;
            size_t offset(0);
            const uint8_t* d =
                (const uint8_t*)pmt::uniform_vector_elements(vector, offset);
            for (size_t i = 0; i < len; i += 16) {
                sout << std::hex << std::setw(4) << std::setfill('0')
                     << static_cast<unsigned int>(i) << ": ";
                for (size_t j = i; j < std::min(i + 16, len); j++) {
                    sout << std::hex << std::setw(2) << std::setfill('0')
                         << static_cast<unsigned int>(d[j]) << " ";
                }
                sout << std::endl;
            }
		}  else {
        	sout << "******* MESSAGE DEBUG PRINT ********" << std::endl
             << pmt::write_string(msg) << std::endl;
		}

		sout << "************************************" << std::endl;
		std::cout << sout.str();
	}

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
				m_argmaxHistory.clear();
				m_cad_count = 5;
				m_cad_detect = false;
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
					//SLEEP 会等待二进制退避算法结束休眠
					//reset 则会重置缓冲区的数据，但是我们还没发送数据，所以这里状态不能切换至reset状态
					m_state = S_RTS_SLEEP;
				}
				
				if(m_cad_count == 0 && m_cad_detect==false){
					m_state = S_RTS_send_RTS;
				}
				break;
			}
			case S_RTS_send_RTS:{
				#if SEND_MODULE == SEND_MODULE_PACKET
					sendRTSByPacket();
				else //TODO
					sendRTSBySerialization();
				#endif
				break;
			}
			case S_RTS_SLEEP:{
				boost::this_thread::sleep(boost::posix_time::microseconds(static_cast<long>(m_before_receive1_ms)));
				break;
			}
			case S_RTS_RECEIVE1:{
				
				 
				break;
			}
			case S_RTS_RECEIVE2:{
				
				break;
			}
			case S_RTS_Send_Data:{
				break;
			}
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

