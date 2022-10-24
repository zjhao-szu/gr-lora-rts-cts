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

#ifndef INCLUDED_LORA_RTS_CTS_CADDETECT_IMPL_H
#define INCLUDED_LORA_RTS_CTS_CADDETECT_IMPL_H

#include "lora_rts_cts/cadDetect.h"
#include "gnuradio/fft/fft.h"
#include <volk/volk.h>
#include <vector>
#include <cmath>

namespace gr {
  namespace lora_rts_cts {

    class cadDetect_impl : public cadDetect
    {
     private:
      pmt::pmt_t m_cad_port;
      cad_state_t m_state;
      
      uint8_t m_sf;
      uint32_t  m_samp_rate;
      uint32_t m_bw;
      uint32_t m_number_of_bins;  
      int m_oversample_rate;
      uint32_t m_samples_per_symbol;
      uint32_t m_fft_size;

      std::vector<gr_complex> m_upchirp;
      std::vector<gr_complex> m_downchirp;

      bool m_cad_result;

      /**/
      gr::fft::fft_complex  * m_fft;
      bool CADDetect_Correlation(std::vector<gr_complex> inputs);
    

      std::vector<uint32_t> m_argmaxHistory;
      bool CADDetect_MinBin();
      uint32_t searchFFTPeek(const lv_32fc_t *fft_result,float * max_value,float * fft_res_mag);
      uint32_t argmax(float * fft_result,float * max_value);
      unsigned int pmod(int x,unsigned int n);
      unsigned int m_preamble_drift_max; 

     public:
      cadDetect_impl(uint8_t sf,uint32_t sameRate,uint32_t bw);
      ~cadDetect_impl();
      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace lora_rts_cts
} // namespace gr

#endif /* INCLUDED_LORA_RTS_CTS_CADDETECT_IMPL_H */

