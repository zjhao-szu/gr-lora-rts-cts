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
    CTSSender::make(uint32_t sf,uint32_t bw,uint32_t sampRate,Class_Type class)
    {
      return gnuradio::get_initial_sptr
        (new CTSSender_impl(sf,bw,sampRate,class));
    }


    /*
     * The private constructor
     */
    CTSSender_impl::CTSSender_impl(uint32_t sf,uint32_t bw,uint32_t sampRate,Class_Type class)
      : gr::block("CTSSender",
              gr::io_signature::make(1,1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0)),
              m_sf(sf),
              m_bw(bw),
              m_sampRate(sampRate),
              m_classType(class)
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
      m_watiTime = 10;
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
      std::string str = pmt::symbol_to_string(msg);
      //message 格式：“Type:RTS,NodeID:10,duration:1000”
      if(str.find("RTS") != std::string::npos){
        
        int indexId = str.find("NodeID",0);
        int indexColon = str.find(":",indexId);
        int indexComma = str.find(",",indexId);
        int winNodeID = std::stoi(str.substr(indexColon+1,indexComma-indexColon));
        
        int indexId = str.find("duration",0);
        int indexColon = str.find(":",indexId);
        int duration = std::stoi(str.substr(indexColon+1));

        m_nodeIdDurations.push_back(make_pair<int,int>(winNodeID,duration));
        if(m_classType == CTS_ClassA ){
          m_state = S_CTS_Send_CTS_CLassA;
        }else if(m_classType == CTS_ClassB){
          m_state = S_CTS_Send_CTS_ClassB;
        }else if(m_classType == CTS_ClassC){
          m_state = S_CTS_Send_CTS_ClassC;
        }
      }else{
        std::cout<<"************user data message******************"<<std::endl;
        std::cout<<str<<endl;
      }
    }
    void
    CTSSender_impl::receiveDataSolve(){ //根据USRP模块接收的信号，就地解析数据
      //do nothing
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
      ninput_items_required[0] = noutput_items * m_samples_per_symbol;
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
        m_state = s_CTS_Beacon;
        m_start = false;
      }else if(m_classType == CTS_ClassB && !m_start && m_beaconIntervalTime == 0){
        //到达beacon发送时间，发送beacon
        m_state = S_CTS_Beacon;
      }
      if(m_classType == CTS_ClassB ){
        --m_beaconIntervalTime;
        --m_slotIntervalTime;
        //TODO::优先发送beacon还是CTS
        if(m_beaconIntervalTime == 0){
          m_state = S_CTS_Beacon;
        }
        if(m_slotInrervalTime == 0){
          m_state = S_CTS_Send_CTS_ClassB;
        }
      }
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
        m_waitTime = 10;
        m_NodeIdConnected = -1;
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
        if(m_isconnected){
          //可以选择发或者可以选择发送当前链接的节点
          winNodeId = m_NodeIdConnected;
          winDuration = m_durationConnected;
        }else{
          winNodeId = m_nodeIdDurations.front().first;
          winDuration = m_nodeIdDurations.front().second;
        }
        //class A采用先来先服务策略
        message = "type:CTS,NodeId:"+to_string(winNodeId)+"Duration:"+to_string(winDuration);
        pmt::pmt_t pmtmsg = pmt::string_to_symbol(message);
        message_port_pub(pmtmsg);
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
            std::string message = "type:CTS,NodeId:"+to_string(winNodeId)+"Duration:"+to_string(winDuration);
            pmt::pmt_t pmtmsg = pmt::string_to_symbol(message);
            message_port_pub(pmtmsg);
          }
          m_state = S_CTS_Receive;
          m_slotIntervalTime = SLOTINTERVALTIME;
          while(!m_nodeIdDurations.empty()){
            m_nodeIdDurations.pop();
          }
        } else{
          m_slotIntervalTime--;
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
        message = "type:CTS,NodeId:"+to_string(winNodeId)+"Duration:"+to_string(winDuration);
        pmt::pmt_t pmtmsg = pmt::string_to_symbol(message);
        message_port_pub(pmtmsg);
        m_state = S_CTS_Receive;
        break;
      }
      case S_CTS_Beacon:{
        //发送beacon
        std::string message = "type:Beacon,IntervalTime:"+to_string(m_beaconIntervalTime)+",slotTime:"+to_string(m_slotIntervalTime);
        pmt::pmt_t msgpmt = pmt::string_to_symbol(message);
        message_port_pub(pmtmsg);
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
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace lora_rts_cts */
} /* namespace gr */

