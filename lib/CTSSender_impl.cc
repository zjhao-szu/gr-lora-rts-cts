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

namespace gr {
  namespace lora_rts_cts {

    CTSSender::sptr
    CTSSender::make(uint32_t sf,uint32_t bw,uint32_t sampRate)
    {
      return gnuradio::get_initial_sptr
        (new CTSSender_impl(sf,bw,sampRate));
    }


    /*
     * The private constructor
     */
    CTSSender_impl::CTSSender_impl(uint32_t sf,uint32_t bw,uint32_t sampRate)
      : gr::block("CTSSender",
              gr::io_signature::make(1,1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0))
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
    CTSSender_impl::receiveRTSSolve(pmt::pmt_t msg){ //根据接收模块接收的数据打印数据

    }
    void
    CTSSender_impl::receiveDataSolve(){ //根据USRP模块接收的信号，就地解析数据

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
      


      // Do <+signal processing+>
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace lora_rts_cts */
} /* namespace gr */

