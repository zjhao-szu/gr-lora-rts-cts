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
#define REQUIRE_PREAMBLE_NUMBER 8
#define BEFORE_RECEIVE2
#define BEFORE_RECEIVE1 
#define RECEIVE_WINDOW_COUNT
namespace gr {
  namespace lora_rts_cts {

    RTSSender::sptr
    RTSSender::make(uint32_t sf,uint32_t bw,int classType,uint32_t NodeId)
    {
      return gnuradio::get_initial_sptr
        (new RTSSender_impl(sf,bw,classType,NodeId));
    }


    /*
     * The private constructor
     */
	/**
	 * TODO:: 
	 * params:: 
	 * sf --
	 * bw --
	 * ClassType --
	 * NodeId --
	 * 
	*/
    RTSSender_impl::RTSSender_impl(uint32_t sf,uint32_t bw,int classType,uint32_t NodeId)
      : gr::block("RTSSender",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0)),
		m_sf(sf),
		m_bw(bw),
		m_nodeId(NodeId)
    {
		//注册并添加端口
		m_receive_userdata_port = pmt::mp("UserDataIn");
		m_receiveLoRaDecodeMessage = pmt::mp("DecodeMessage");
		m_out_userdata = pmt::mp("UserDataOut");
		m_out_RTS = pmt::mp("RTSOUT");
		m_receiveCTSMessage = pmt::mp("receiveCTSMessage");
		
		message_port_register_in(m_receiveLoRaDecodeMessage);
		message_port_register_in(m_receive_userdata_port);
		message_port_register_in(m_receiveCTSMessage);
		message_port_register_out(m_out_userdata);
		message_port_register_out(m_out_RTS);
		

		set_msg_handler(m_receiveLoRaDecodeMessage,boost::bind(&RTSSender_impl::receiveDecodeMessage,this,_1));
		set_msg_handler(m_receive_userdata_port,boost::bind(&RTSSender_impl::receiveUserData,this,_1));
		set_msg_handler(m_receiveCTSMessage,boost::bind(&RTSSender_impl::receiveCTSMsg,this,_1));
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

		//class A\B\C windows params;

		//class A
		m_before_receive1_ms = 100;
		m_before_receive2_ms = 100;
		m_receive1_window_count = 1000;
		m_receive2_window_count = 1000;
		//class B
		m_slotReceive_window_count = 1000;

		if(classType == 0){
			m_classType = M_RTS_CLASSA;
		}else if(classType == 1){
			m_classType = M_RTS_CLASSB;
		}else if(classType == 2){
			m_classType = M_RTS_CLASSC;
		}

		m_outParaPort = pmt::intern("paraOut");
		message_port_register_out(m_outParaPort);
		m_DIFSTime = 0;
		m_receiveTime = 0;
		m_CTSToken = m_nodeId + 1;
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
		// if(m_state == S_RTS_RECEIVE_DATA || m_state == S_RTS_RECEIVE_Class_C){
			std::cout<<pmt::symbol_to_string(msg)<<std::endl;
			m_userDatas.push_back(pmt::symbol_to_string(msg));
			std::cout<<"检测到用户输入：："<<m_userDatas.back()<<std::endl;
			if(m_state == S_RTS_RECEIVE_DATA || m_state == S_RTS_RECEIVE_Class_C){
				std::cout<<"当前状态为静默状态,可以直接发送信息,状态切换为CAD状态!!"<<std::endl;
				m_state =  S_RTS_CAD;
			}else{
				std::cout<<"当前状态为忙碌状态，缓存需要发送的信息，状态不进行切换!!!"<<std::endl;
			} 			
		// }
		
	}

	std::unordered_map<std::string,std::string>
	RTSSender_impl::parseMessage(std::string str){
		std::unordered_map<std::string,std::string> msgMap;
		int indexColon = str.find(":");
		int indexComma = str.find(",");
		std::string type = str.substr(indexColon+1,indexComma);
		msgMap.insert(make_pair("Type",type));
		if(type == "Beacon"){
			indexColon = str.find(":",indexComma);
			indexComma = str.find(",",indexColon);
			std::string beaconIntervalTime = str.substr(indexColon+1,indexComma);
			msgMap.emplace("IntervalTime",beaconIntervalTime);
			indexColon = str.find(":",indexComma);
			std::string slotTime = str.substr(indexColon+1,str.length() - indexColon - 1);
			msgMap.emplace("slotTime",slotTime);
		}else if(type == "RTS" || type == "CTS"){
			indexColon = str.find(":",indexComma);
			indexComma = str.find(",",indexColon);
			std::string beaconIntervalTime = str.substr(indexColon+1,indexComma);
			msgMap.emplace("NodeId",beaconIntervalTime);
			indexColon = str.find(":",indexComma);
			std::string slotTime = str.substr(indexColon+1,str.length() - indexColon - 1);
			msgMap.emplace("Duration",slotTime);
		}
		return msgMap;
	}
	void
	RTSSender_impl::receiveDecodeMessage(pmt::pmt_t msg){
		//only state is receiving, resolve the message
		if(m_state == S_RTS_RECEIVE1 || m_state == S_RTS_RECEIVE2 || m_state == S_RTS_RECEIVE_Slot || m_state == S_RTS_RECEIVE_Class_C){
			std::string str = pmt::symbol_to_string(msg);
			if(str.find("Type") == std::string::npos){
				messageDebugPrint(msg);
			}else{
				std::cout<<str<<std::endl;
				std::unordered_map<std::string,std::string> mesMap = parseMessage(str);
				// mesMap["Type"] == "CTS"
				std::string type = mesMap["Type"];
				if(strcmp(type.c_str(),"CTS") == 0){
					m_ReceiveNodeId = std::stoi(mesMap["NodeId"]);	
					m_ReceiveDuration = std::stoi(mesMap["Duration"]);
					std::cout<<"NodeID:"<<m_nodeId<<" receive CTS数据包!!!"<<std::endl;
					std::cout<<"Receive winNode ID is"<<m_ReceiveNodeId<<" Duration:"<<m_ReceiveDuration<<std::endl;
					if(m_ReceiveNodeId == m_nodeId){
						std::cout<<"m_NodeId: "<<m_nodeId<<" 收到CTS数据包并且该节点为获胜节点!!!"<<std::endl;
						m_state = S_RTS_Send_Data;
					}else if(m_ReceiveNodeId != m_nodeId){
						std::cout<<"m_NodeId: "<<m_nodeId<<" 收到CTS数据包,获胜节点为: "<<m_ReceiveNodeId<<std::endl;
						m_state = S_RTS_SLEEP;
					}
				}else if(strcmp(type.c_str(),"RTS") == 0){
					m_ReceiveNodeId = std::stoi(mesMap["NodeId"]);	
					m_ReceiveDuration = std::stoi(mesMap["Duration"]);
					std::cout<<"NodeID:"<<m_nodeId<<" receive RTS数据包!!!"<<std::endl;
					std::cout<<"Receive winNode ID is "<<m_ReceiveNodeId<<" Duration: "<<m_ReceiveDuration<<std::endl;
					if(m_ReceiveNodeId == m_nodeId){
						std::cout<<"m_NodeId: "<<m_nodeId<<" 收到自身RTS,放弃ing!!!"<<std::endl;
					}else if(m_ReceiveNodeId != m_nodeId){
						std::cout<<"m_NodeId: "<<m_nodeId<<" 收到RTS数据包,通信节点为: "<<m_ReceiveNodeId<<std::endl;
						m_state = S_RTS_SLEEP;
					}
				}else if(strcmp(type.c_str(),"Beacon") == 0){
					m_beacon_Interval_Window = std::stoi(mesMap["IntervalTime"]);
					m_slotReceive_window_count = std::stoi(mesMap["slotTime"]);
				}else{
					messageDebugPrint(msg);
				}
			} 
		} 
    }
	//根据RTS结果决定切换状态
	void 
	RTSSender_impl::receiveCTSMsg(pmt::pmt_t msg){
		if(m_state !=S_RTS_TO_RECEIVE1 || m_state != S_RTS_RECEIVE2 || m_state != S_RTS_RECEIVE_Slot || m_state != S_RTS_RECEIVE_Class_C  ){
			return ;
		} 
		uint32_t CTSToken = pmt::to_uint64(msg);
		if(CTSToken == m_CTSToken){
			m_state = S_RTS_Send_Data;
			gettimeofday(&m_endReceiveTime,NULL);
			
			m_receiveTime += getTimeval(m_startReceiveTime,m_endReceiveTime);
		}else{
			m_state = S_RTS_Sleep;
			gettimeofday(&m_endReceiveTime,NULL);
     		m_receiveTime += getTimeval(m_startReceiveTime,m_endReceiveTime);
			if(m_state == S_RTS_RECEIVE2){
				m_paraState = 4;
			}else if(m_state == S_RTS_RECEIVE1){
				m_paraState = 3;
			}
			sendParaMsg();
		}
	}	

	/********************************************************************
	 * CAD功能检测
	 * 
	********************************************************************/
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
    RTSSender_impl::searchFFTPeek(const gr_complex *fft_result,float * max_value,float * fft_res_mag){
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
		int totalLen = 0;
		for(int i = 0;i < m_userDatas.size();i++){
			totalLen += m_userDatas[i].length();
		}
		std::string msgString("Type:RTS,NodeId:" + std::to_string(m_nodeId) + ",Duration:" + std::to_string(totalLen));
		pmt::pmt_t msg = pmt::string_to_symbol(msgString);
		message_port_pub(m_out_userdata,msg);
		std::cout<<"RTS packet 发送结束"<<std::endl;
	}
	//TODO.....
	void
	RTSSender_impl::sendRTSBySerialization(){
		pmt::pmt_t msg = pmt::mp("rtsSend");
		message_port_pub(m_out_RTS);
	}
	/*******************************************************************
	 * 发送数据
	********************************************************************/
	void
	RTSSender_impl::sendTotalData(){
		std::cout<<"开始发送数据"<<std::endl;
		int totalLen  = 0;
		//这里是每一条消息都有一个数据包发送，其实也可以合并所有的数据包一起发送
		//两种方案后续都可以考虑
		for(auto & data : m_userDatas){
			std::cout<<"message is :"<<data<<std::endl;
			totalLen += data.length();
			pmt::pmt_t datamsg = pmt::string_to_symbol(data);
			message_port_pub(m_out_userdata,datamsg);
		}
		std::cout<<"全部数据发送完全，总共发送数据: "<<totalLen<<" 个字符数据"<<std::endl;
	}
	void 
	RTSSender_impl::sendData(){
		
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

	double
    RTSSender_impl::getTimeval(const struct timeval startTime,const struct timeval endTime){
      double stime =  startTime.tv_sec * 1000 + startTime.tv_usec / 1000;
      double etime = endTime.tv_sec * 1000 + endTime.tv_usec/1000;
      return etime - stime;
    }

	void sendParaMsg(){
		pmt::pmt_t msgDict = pmt::make_dict();
		if(m_state !- 5){
			pmt::dict_add(msgDict,pmt::intern("sendSuccess"),pmt::from_bool(false));
		}else{
			pmt::dict_add(msgDict,pmt::intern("sendSuccess"),pmt::from_bool(true));
		}
		pmt::dict_add(msgDict,pmt::intern("DIFSTIME"),pmt::from_double(m_DIFSTime));
		pmt::dict_add(msgDict,pmt::intern("RECEIVETIME"),pmt::from_double(m_receiveTime));
		pmt::dict_add(msgDict,pmt::intern("PARATIME"),pmt::from_uint64(PARATIME));
		pmt::dict_add(msgDict,pmt::intern("NOID"),pmt::from_uint64(m_nodeId));

		message_port_pub(m_outParaPort,msgDict);
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
	
		if(m_classType == M_RTS_CLASSB && m_beacon_Interval_Window > 0 ){
			m_beacon_Interval_Window--;
			if(m_beacon_Interval_Window == 0){
				m_state = S_RTS_BEACON;
			}
		}else if(m_classType == M_RTS_CLASSB && m_slotReceive_window_count > 0){
			//compute left how much time about next slot;
			m_slotReceive_window_count--;
			if(m_slotReceive_window_count == 0){
				m_state = S_RTS_RECEIVE_Slot;
			} 
		}
		
		
		switch (m_state)
		{
			case S_RTS_RESET: {
				m_userDatas.clear();
				m_argmaxHistory.clear();
				m_cad_count = 8;
				m_cad_detect = false;
				m_receive1_window_count = 1000;
				m_receive2_window_count = 1000;
				m_slotReceive_window_count = 1000;
				m_state = S_RTS_RECEIVE_DATA;
				
				break;
			}
			case S_RTS_RECEIVE_DATA: {
				//接收用户数据并切换rts-cad状态
				std::cout<<"等待用户传入数据......"<<std::endl;
				break;
			}
			case S_RTS_CAD: {
				if(m_cad_count == 8){
					gettimeofday(&m_startDIFSTime,NULL);
				}
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
					m_cad_detect = true;
					m_cad_count -= 1;
				}else{
					// std::cout<<"while noise is"<<maxValue<<std::endl;
					m_cad_count -= 1;
				}

				if(m_cad_detect){
					std::cout<<"CAD DETECTED!!!! Node Need to Sleep!!!!"<<std::endl;
					//SLEEP 会等待二进制退避算法结束休眠
					//reset 则会重置缓冲区的数据，但是我们还没发送数据，所以这里状态不能切换至reset状态
					m_state = S_RTS_SLEEP;
					gettimeofday(&m_endDIFSTime,NULL);
					m_DIFSTime = getTimeval(m_startDIFSTime,m_endDIFSTime);
					m_paraState = 1;
					sendParaMsg();
				}
				
				if(m_cad_count == 0 && m_cad_detect == false){
					std::cout<<"==============CAD Detect count is 0 && 没有CAD 被检测============"<<std::endl;
					std::cout<<"==============进入发送RTS阶段===================================="<<std::endl;
					m_state = S_RTS_send_RTS;
					gettimeofday(&m_endDIFSTime,NULL);
					m_DIFSTime = getTimeval(m_startDIFSTime,m_endDIFSTime);
				}
				break;
			}
			case S_RTS_send_RTS: {
				// #if SEND_MODULE == SEND_MODULE_PACKET
				// 	sendRTSByPacket();
				
				// #else //TODO
				sendRTSBySerialization();
				// #endif
				m_before_receive1_ms = 100;
				if(m_classType == M_RTS_CLASSC){
					m_state = S_RTS_RECEIVE_Class_C;
				}else{
					m_state = S_RTS_RECEIVE1;
				}
				
				break;
			}
			case S_RTS_SLEEP: {
				boost::this_thread::sleep(boost::posix_time::microseconds(static_cast<long>(m_before_receive1_ms)));
				break;
			}
			case S_RTS_TO_RECEIVE1:{
				m_before_receive1_ms--;
				if(m_before_receive1_ms == 0){
					std::cout<<"正在打开接收窗口...... Before_receive"<<m_before_receive1_ms<<std::endl;
					m_state = S_RTS_RECEIVE1;
				}
				break;
			}
			//class A window 1
			case S_RTS_RECEIVE1: {
				if(m_receive1_window_count == 1000){
					gettimeofday(&m_startReceiveTime,NULL);
				}
				m_receive1_window_count--;
				std::cout<<"正在等待数据传入。。。。"<<std::endl;
				if(m_receive1_window_count == 0){
					std::cout<<"接收数据结束..进入等待第二窗口打开"<<std::endl;
					m_state = S_RTS_TO_RECEIVE2;
					m_before_receive2_ms = 100; 
					gettimeofday(&m_endReceiveTime,NULL);
					m_receiveTime += getTimeval(m_startReceiveTime,m_endReceiveTime);
				}
				break;
			}
			case S_RTS_TO_RECEIVE2:{
				m_before_receive2_ms--;
				if(m_before_receive2_ms == 0){
					std::cout<<"正在打开接收窗口2"<<std::endl;
					m_state = S_RTS_RECEIVE2;
				}
				break;
			}
			//class A window 2
			case S_RTS_RECEIVE2: {
				if(m_receive2_window_count == 1000){
					gettimeofday(&m_startReceiveTime,NULL);
				}
				m_receive2_window_count--;
				std::cout<<"正在等待数据传入。。。。"<<std::endl;
				if(m_receive2_window_count == 0){
					std::cout<<"接收数据结束..重置参数，等待用户传输数据......"<<std::endl;
					m_state = S_RTS_RESET;
					gettimeofday(&m_endReceiveTime,NULL);
					m_receiveTime += getTimeval(m_startReceiveTime,m_endReceiveTime);
					m_paraState = 4;
					sendParaMsg();

				}	
				break;
			}
			//class B slot window
			case S_RTS_RECEIVE_Slot: {
				m_slotReceive_window_count--;
				if(m_slotReceive_window_count == 0){
					std::cout<<"class B slot,接收窗口减一......"<<std::endl;
					m_state = S_RTS_RESET;
				}
				break;
			}
			//class C window Open all
			case S_RTS_RECEIVE_Class_C:{
				
				break;
			}
			case S_RTS_Send_Data:{
				sendTotalData();
				m_paraState = 5;
				sendParaMsg();
				m_state = S_RTS_RESET;
				break;
			}
			case S_RTS_BEACON:{
				
				break;
			}
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

