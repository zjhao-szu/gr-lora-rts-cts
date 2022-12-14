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
#include "CTSSender_impl.h"

#define SLOTINTERVALTIME 1000
#define BEACONINTERVALTIME 1500

namespace gr {
  namespace lora_rts_cts {

    CTSSender::sptr
    CTSSender::make(uint32_t sf,uint32_t bw,uint32_t sampRate,int classType)
    {
      return gnuradio::get_initial_sptr
        (new CTSSender_impl(sf,bw,sampRate,classType));
    }


    /*
     * The private constructor
     */
    CTSSender_impl::CTSSender_impl(uint32_t sf,uint32_t bw,uint32_t sampRate,int classType)
      : gr::block("CTSSender",
              gr::io_signature::make(0,0, 0),
              gr::io_signature::make(0, 0, 0)),
              m_sf(sf),
              m_bw(bw),
              m_sampRate(sampRate)
              
    {
      //自动机状态切换
      m_state  = S_CTS_Reset;

      //port选择
      m_receive_data_port = pmt::mp("receiveData");
      m_receive_RTS_port = pmt::mp("receiveRTS");
      m_out_data_port = pmt::mp("OutData");
      m_out_CTS_port = pmt::mp("OutCTS");

      message_port_register_in(m_receive_data_port);
      message_port_register_in(m_receive_RTS_port);
      message_port_register_out(m_out_data_port);
      message_port_register_out(m_out_CTS_port);

      set_msg_handler(m_receive_data_port,boost::bind(&CTSSender_impl::receiveDataSolve,this,_1));
      set_msg_handler(m_receive_RTS_port,boost::bind(&CTSSender_impl::receiveRTSSolve,this,_1));
      
      //网关节点初始状态
      m_isconnected = false;
      m_NodeIdConnected = -1;
      m_classType = CTS_ClassA;
      m_durationConnected = 0;

      //ping slot 时间间隔
      m_slotIntervalTime = SLOTINTERVALTIME;
      m_beaconIntervalTime = BEACONINTERVALTIME;
      m_waitTime = 10;
      
      if(classType == 0){
        m_classType = CTS_ClassA;
      }else if(classType == 1){
        m_classType = CTS_ClassB;
      }else if(classType == 2){
        m_classType = CTS_ClassC;
      } 
    }

    /*
     * Our virtual destructor.
     */
    CTSSender_impl::~CTSSender_impl()
    {
    }
    /***********************************************************************
     *  该部分接口用于处理接收端口传入的数据-包括RTS数据包 以及 用户 
     * 
     ***********************************************************************/
    void
    CTSSender_impl::receiveRTSSolve(const pmt::pmt_t msg){ //根据接收模块接收的数据打印数据
      uint32_t nodeToken = pmt::to_double(msg);
      m_nodeIdDurations.push(stD::make_pair<int,int>((nodeToken+1)/2),0);
    }
    void
    CTSSender_impl::receiveDataSolve(pmt::pmt_t msg){ //根据USRP模块接收的信号，就地解析数据
      //do nothing
      std::cout<<"接收到了用户信息:"<<std::endl;
      std::cout<<pmt::symbol_to_string(msg)<<std::endl;

      

      if(m_state != S_CTS_Receive) return;
      std::string str = pmt::symbol_to_string(msg);
      //message 格式：“Type:RTS,NodeID:10,duration:1000”
      if(str.find("RTS") != std::string::npos){
        
        int indexId = str.find("NodeID",0);
        int indexColon = str.find(":",indexId);
        int indexComma = str.find(",",indexId);
        int winNodeID = std::stoi(str.substr(indexColon+1,indexComma-indexColon));
        
        indexId = str.find("duration",0);
        indexColon = str.find(":",indexId);
        int duration = std::stoi(str.substr(indexColon+1));
        std::cout<<"收到来自节点NodeId:"<<winNodeID<<" duration: "<<duration<<std::endl;
        m_nodeIdDurations.push(std::make_pair(winNodeID,duration));
        if(m_classType == CTS_ClassA ){
          m_state = S_CTS_Send_CTS_ClassA;
        }else if(m_classType == CTS_ClassB){
          m_state = S_CTS_Send_CTS_ClassB;
        }else if(m_classType == CTS_ClassC){
          m_state = S_CTS_Send_CTS_ClassC;
        }
      }else{
        std::cout<<"************user data message******************"<<std::endl;
        std::cout<<str<<std::endl;
      }
    }


    /**
     * @brief 
     * 
     * @param noutput_items 
     * @param ninput_items_required 
     */


    void
    CTSSender_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
      ninput_items_required[0] = 0;
    }

    int
    CTSSender_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      unsigned int *out = (unsigned int *) output_items[0];
      
      if(m_classType == CTS_ClassB && m_start){ //首次立马发送beacon
        m_state = S_CTS_Beacon;
        m_start = false;
      }else if(m_classType == CTS_ClassB && !m_start && m_beaconIntervalTime == 0){
        //到达beacon发送时间，发送beacon
        m_state = S_CTS_Beacon;
      }else if(m_classType == CTS_ClassB && !m_start && m_slotIntervalTime == 0){
        m_state = S_CTS_Send_CTS_ClassB;
      }


      if(m_classType == CTS_ClassB ){
        --m_beaconIntervalTime;
        --m_slotIntervalTime;
        //TODO::优先发送beacon还是CTS
        if(m_beaconIntervalTime == 0){
          m_state = S_CTS_Beacon;
        }
        if(m_slotIntervalTime == 0){
          m_state = S_CTS_Send_CTS_ClassB;
        }
      }

      //会预留一段时间m_waitTime
      //如果没连上
      if(m_isconnected){
        if(m_waitTime <= 0){
          m_durationConnected--;
        }else{
          m_waitTime--;
        }
        if(m_durationConnected <= 0){
          m_state = S_CTS_Reset;
          m_isconnected = false;
        }
      }
      switch (m_state)
      {

      case S_CTS_Reset:{
        while(!m_nodeIdDurations.empty()){
          m_nodeIdDurations.pop();
        }
        m_state = S_CTS_Receive;
        m_durationConnected = -1;
        m_waitTime = 10; //
        m_NodeIdConnected = -1;
        m_isconnected = false;
        break;
      }

      case S_CTS_Send_Data:{
        //ToDo send message
        
        break;
      }

      case S_CTS_Receive:{
        //TODO：一直接收来自外部的数据
        
        break;
      }
      
      case S_CTS_Send_CTS_ClassA:{
        std::string message = "";
        int winNodeId = 0;
        int winDuration = 0;
        //当前网关节点是否正在通信，如果是则返回相应的正在沟通的CTS
        
        std::cout<<"=====Class A采用先来先服务策略==="<<std::endl;
        if(m_isconnected){
          //可以选择发或者可以选择发送当前链接的节点
          winNodeId = m_NodeIdConnected;
          winDuration = m_durationConnected;
          std::cout<<"当前网关正在通信,与节点:"<<winNodeId<<" duration:"<<winDuration<<std::endl;
        }else{
          winNodeId = m_nodeIdDurations.front().first;
          winDuration = m_nodeIdDurations.front().second;
          std::cout<<"当前网关未与人通信,获胜节点为:"<<winNodeId<<" duration:"<<winDuration<<std::endl;
        }
        
  
        //class A采用先来先服务策略
        //采用data数据
        // message = "type:CTS,NodeId:"+std::to_string(winNodeId)+"Duration:"+std::to_string(winDuration);
        // pmt::pmt_t pmtmsg = pmt::string_to_symbol(message);
        // message_port_pub(m_out_data_port,pmtmsg);

        //采用downchirp
        pmt::pmt_t CTSToken = pmt::from_uint64(winNodeId);
        message_port_pub(m_out_CTS_port,CTSToken);
        m_state = S_CTS_Receive;
        break;
      }

      case S_CTS_Send_CTS_ClassB:{
        //class B 在beacon窗口发送消息
        if(m_slotIntervalTime == 0){
          if(m_nodeIdDurations.size() != 0){
             //竞争策略选择 先来先服务
             
            int winNodeId = m_nodeIdDurations.front().first;
            int winDuration = m_nodeIdDurations.front().second;
            std::cout<<"=====Class B采用先来先服务策略==="<<std::endl;
            
            std::cout<<"当前网关未与人通信,获胜节点为:"<<winNodeId<<" duration:"<<winDuration<<std::endl;
            //采用data数据
            // std::string message = "type:CTS,NodeId:"+std::to_string(winNodeId)+",Duration:"+std::to_string(winDuration);
            // pmt::pmt_t pmtmsg = pmt::string_to_symbol(message);
            // message_port_pub(m_out_data_port,pmtmsg);
            //采用downchirp
            pmt::pmt_t CTSToken = pmt::from_uint64(winNodeId);
            message_port_pub(m_out_CTS_port,CTSToken);

          }
          m_state = S_CTS_Receive;
          m_slotIntervalTime = SLOTINTERVALTIME;
          while(!m_nodeIdDurations.empty()){
            m_nodeIdDurations.pop();
          }
        } else{
          m_state = S_CTS_Receive;
        }

        break;
      }

      case S_CTS_Send_CTS_ClassC:{
        //实现逻辑和A类一致，这里写出来是为了表示逻辑的清晰性

        std::string message = "";
        int winNodeId = 0;
        int winDuration = 0;
        //当前网关节点是否正在通信，如果是则返回相应的正在沟通的CTS
        if(m_isconnected){
          //可以选择发或者可以选择发送当前链接的节点
          winNodeId = m_NodeIdConnected;
          winDuration = m_durationConnected;
        }else{
          winNodeId = m_nodeIdDurations.front().first;
          winDuration = m_nodeIdDurations.front().second;
        }
        //class A采用先来先服务策略
        // message = "Type:CTS,NodeId:"+std::to_string(winNodeId)+",Duration:"+std::to_string(winDuration);
        // pmt::pmt_t pmtmsg = pmt::string_to_symbol(message);
        // message_port_pub(m_out_data_port,pmtmsg);
        //采用downchirp
        pmt::pmt_t CTSToken = pmt::from_uint64(winNodeId);
        message_port_pub(m_out_CTS_port,CTSToken);

        m_state = S_CTS_Receive;
        break;
      }

      case S_CTS_Beacon:{
        //发送beacon
        std::string message = "Type:Beacon,IntervalTime:"+std::to_string(m_beaconIntervalTime)+",slotTime:"+std::to_string(m_slotIntervalTime);
        pmt::pmt_t msgpmt = pmt::string_to_symbol(message);
        message_port_pub(m_out_data_port,msgpmt);
        m_state = S_CTS_Receive;
        m_beaconIntervalTime = BEACONINTERVALTIME;
        break;
      }  
      default:
        
        break;
      }



      // Do <+signal processing+>
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (0);

      // Tell runtime system how many output items we produced.
      return 0;
    }

  } /* namespace lora_rts_cts */
} /* namespace gr */

