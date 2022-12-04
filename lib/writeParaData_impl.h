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

#ifndef INCLUDED_LORA_RTS_CTS_WRITEPARADATA_IMPL_H
#define INCLUDED_LORA_RTS_CTS_WRITEPARADATA_IMPL_H

#include <lora_rts_cts/writeParaData.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
namespace gr {

  namespace lora_rts_cts {

    class writeParaData_impl : public writeParaData
    {
     private:
     pmt::pmt_t m_macInPort,m_RTSInPort;
     std::ofstream out_file;
     
     uint32_t m_nodeId;
     uint32_t m_state;//1: choose channel 2:DIFS  3:BO  4:UPDATE  5:TX
     bool m_sendSuccess;
     double m_difsTime;
     double m_RTSTime;
     double m_receiveTime;

    uint32_t m_realBytes;
    uint32_t m_sendFrameCnt;
    double m_StartTime;
    double m_endTime;
    double m_phyTotalTime;    

    void receiveMAC(pmt::pmt_t msg);
    void receivePHY(pmt::pmt_t msg);
    void wirteData();

     public:
      writeParaData_impl();
      ~writeParaData_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace lora_rts_cts
} // namespace gr

#endif /* INCLUDED_LORA_RTS_CTS_WRITEPARADATA_IMPL_H */

