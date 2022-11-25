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

#ifndef INCLUDED_LORA_RTS_CTS_CTSSENDTOKEN_IMPL_H
#define INCLUDED_LORA_RTS_CTS_CTSSENDTOKEN_IMPL_H

#include <lora_rts_cts/CTSSendToken.h>

namespace gr {
  namespace lora_rts_cts {

    class CTSSendToken_impl : public CTSSendToken
    {
     private:
      uint8_t m_sf;
      uint32_t m_samp_rate;
      uint32_t m_bw;
      uint32_t m_number_of_bins;
      uint32_t m_samples_per_symbol;
      std::vector<uint16_t> m_sync_words;    
      int m_oversample_rate;
      std::vector<gr_complex> m_upchirp;
      std::vector<gr_complex> m_downchirp;

      uint8_t m_up; //preamble 长度
      uint32_t preamb_symb_cnt; //前导码symbol

      uint32_t  m_CTSToken;  
      pmt::pmt_t m_receiveCTSTokenPort;

      CTS_TOKEN_STATE m_state;
      void receiveCTSToken(pmt::pmt_t msg);
      void sendCTSToken();
     public:
      CTSSendToken_impl(uint8_t sf, uint32_t samp_rate, uint32_t bw, std::vector<uint16_t> sync_words);
      ~CTSSendToken_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace lora_rts_cts
} // namespace gr

#endif /* INCLUDED_LORA_RTS_CTS_CTSSENDTOKEN_IMPL_H */
