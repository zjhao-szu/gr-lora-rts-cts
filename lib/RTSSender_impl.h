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
#include <volk/volk.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <string.h>
#include <time.h>
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
		pmt::pmt_t m_receiveLoRaDecodeMessage;
		pmt::pmt_t m_receiveCTSMessage;

		//存储当前用户要发送的数据
		std::vector<std::string> m_userDatas;
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

		RTS_Class_Type	 m_classType;
		uint32_t m_nodeId;
		uint32_t m_duration;
		
		//接收NodeID、Duation
		uint32_t m_ReceiveNodeId;
		uint32_t m_ReceiveDuration;

		//CAD功能
		uint32_t m_preamble_drift_max;
		gr::fft::fft_complex * m_fft;
		std::vector<uint32_t>  m_argmaxHistory;
		uint32_t m_samples_per_symbol;
		int m_cad_count; //cad检测次数，超过上限结束转入发送RTS
		bool m_cad_detect;
		bool detectCad();
		unsigned int pmod(int x,unsigned int n);
		uint32_t argmax(float * fft_result,float * max_value);
		uint32_t searchFFTPeek(const lv_32fc_t * fft_result,float * max_value,float * fft_res_mag);
		bool CADDetect_MinBin();

		//发送RTS，基于两种模式
		void receiveDecodeMessage(pmt::pmt_t msg);
		void receiveCTSMsg(pmt::pmt_t msg);
		//模式一：RTS、CTS利用数据包发送
		void sendRTSByPacket();
		//模式二：RTS/CTS序列化发送
		void sendRTSBySerialization();
		

		//休眠


		
		void receiveDecodeMessage(pmt::pmt_t msg);  
		std::unordered_map<std::string,std::string> parseMessage(std::string msgString);
		
		//模拟class A
		uint32_t m_before_receive1_ms;
		uint32_t m_before_receive2_ms;
		uint32_t m_receive1_window_count;
		uint32_t m_receive2_window_count;
		
		//模拟class B
		bool m_firstSendBeacon;
		uint32_t m_slotReceive_window_count;
		uint32_t m_beacon_Interval_Window;
		
		//模拟class C
		

		//发送数据
		bool m_SendTotalData;
		void sendData();
		void sendTotalData();

		//debug
		void  messageDebugPrint(const pmt::pmt_t &msg);

		uint32_t m_CTSToken;

		//记录实验数据
		pmt::pmt_t m_outParaPort;
		uint32_t m_paraState; //用于记录阶段 1：DIFS 2：RTS 3：Receive1 4：Receive2 4：sendData
		struct timeval m_startTime,m_endTime;
		double m_totalTime;//DIFS时间 + SIFS时间 + RTS 时间 + Receive接收窗口时间
		//记录DIFS时间
		
		struct timeval m_startDIFSTime,m_endDIFSTime;
		double m_DIFSTime;
		//记录RTS时间
		
		//记录接收窗口时间
		struct timeval m_startReceiveTime,m_endReceiveTime;
		double m_receiveTime;
		double getTimeval(const struct timeval startTime,const struct timeval endTime);
		void sendParaMsg();
		
    public:
      RTSSender_impl(uint32_t sf,uint32_t bw,int classType,uint32_t NodeId);
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

