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

#ifndef INCLUDED_LORA_RTS_CTS_CTSSENDER_IMPL_H
#define INCLUDED_LORA_RTS_CTS_CTSSENDER_IMPL_H

#include <lora_rts_cts/CTSSender.h>
#include <gnuradio/fft/fft.h>
namespace gr {
  namespace lora_rts_cts {

    class CTSSender_impl : public CTSSender
    {
     private:
      // Nothing to declare in this block.
      gr::fft::fft_complex  * m_fft;
      uint32_t m_fft_size;
      CTS_Sender_State m_state;


      //普通且非功能性数据定义
      uint32_t m_sf;
      uint32_t m_sampRate;
      uint32_t m_bw;
      uint32_t m_oversample_rate;

      uint32_t m_number_of_bins;
      uint32_t m_samp_pre_symbol;
      uint32_t m_fft_size;

      std::vector<gr_complex> m_upchirp,m_downchirp;
      std::vector<pair<int,int>> m_nodeIds;
      uint32_t m_nodeId;
      uint32_t m_duration;
      // Class_Type m_classType;
     public:
      CTSSender_impl();
      ~CTSSender_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace lora_rts_cts
} // namespace gr

#endif /* INCLUDED_LORA_RTS_CTS_CTSSENDER_IMPL_H */

