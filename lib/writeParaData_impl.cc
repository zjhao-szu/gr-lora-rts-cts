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
#include "writeParaData_impl.h"

namespace gr {
  namespace lora_rts_cts {

    writeParaData::sptr
    writeParaData::make()
    {
      return gnuradio::get_initial_sptr
        (new writeParaData_impl());
    }


    /*
     * The private constructor
     */
    writeParaData_impl::writeParaData_impl()
      : gr::block("writeParaData",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(0, 0, 0))，
        m_nodeId(nodeId)
    {
      m_state = 0;
      m_sendSuccess = false;
      m_difsTime = 0;
      m_RTSTime = 0;
      m_receiveTime = 0;
      m_readBytes = 0;
      m_sendFrameCnt = 0;
      m_StartTime = 0;
      m_endTime = 0;
      m_phyTotalTime = 0;
      std::string filename =  "MYMAC_record"+std::to_string(m_nodeId)+".txt";
      out_file.open(filename, std::ios::out | std::ios::trunc);

      out_file<<"NODEID\tSTATE\tsendSuccess\tdifsTime\tRTSTime\treceiveTime\tRealBytes\tSendFrameCnt\tStartTime\tEndTime\tPhyTotalTime\t\n";
    }

    /*
     * Our virtual destructor.
     */
    writeParaData_impl::~writeParaData_impl()
    {
    }

    void
    writeParaData_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }
    void
    writeParaData_impl::receiveMAC(){
      pmt::pmt_t not_found;
      pmt::pmt_t state = pmt::dict_ref(msg,pmt::intern("State"),not_found);
      pmt::pmt_t sendSuccess = pmt::dict_ref(msg,pmt::intern("sendSuccess"),not_found);;
      pmt::pmt_t difsTime = pmt::dict_ref(msg,pmt::intern("difsTime"),not_found);;
      pmt::pmt_t RTSTime = pmt::dict_ref(msg,pmt::intern("RTSTime"),not_found);;
      pmt::pmt_t receiveTime = pmt::dict_ref(msg,pmt::intern("receiveTime"),not_found);;
   

      m_state = pmt::to_uint64(state);
      m_sendSuccess = pmt::to_bool(sendSuccess);

      m_difsTime = pmt::to_uint64(difsTime);
      m_RTSTime = pmt::to_double(RTSTime);
      m_receiveTime = pmt::to_double(receiveTime);
      
      if(!m_sendSuccess){
        writeData();
        reset();
      }
    }
    void
    writeParaData_impl::receivePHY(){
      pmt::pmt_t not_found;
      pmt::pmt_t realBytes = pmt::dict_ref(msg,pmt::intern("realBytes"),not_found);;
      pmt::pmt_t sendFrameCnt = pmt::dict_ref(msg,pmt::intern("sendBytes"),not_found) ;
      pmt::pmt_t StartTime = pmt::dict_ref(msg,pmt::intern("StartTime"),not_found) ;
      pmt::pmt_t endTime = pmt::dict_ref(msg,pmt::intern("endTime"),not_found) ;
      pmt::pmt_t phyTotalTime = pmt::dict_ref(msg,pmt::intern("phyTotalTime"),not_found);

      m_realBytes = pmt::to_uint64(realBytes);
      m_sendFrameCnt = pmt::to_uint64(sendFrameCnt);
      m_StartTime = pmt::to_double(StartTime);
      m_endTime = pmt::to_double(endTime);
      m_phyTotalTime = pmt::to_double(phyTotalTime); 
    
      wirteData();
      reset();
    }
    void writeParaData_impl::wirteData(){
      out_file<<m_nodeId<<"\t"<<m_state<<"\t"<<m_sendSuccess<<"\t"<<m_difsTime<<"\t"<<m_RTSTime<<"\t"<<m_receiveTime<<"\t"<<m_realBytes<<"\t"<<m_sendFrameCnt<<"\t"<<m_StartTime<<"\t"<<m_endTime<<"\t"<<m_phyTotalTime<<"\n";
    };

    void  writeParaData_impl::reset(){
      m_state = 0;
      m_sendSuccess = false;
      m_difsTime = 0;
      m_RTSTime = 0;
      m_receiveTime = 0;
      m_readBytes = 0;
      m_sendFrameCnt = 0;
      m_StartTime = 0;
      m_endTime = 0;
      m_phyTotalTime = 0;
    }
    int
    writeParaData_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];

      // Do <+signal processing+>
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace lora_rts_cts */
} /* namespace gr */

