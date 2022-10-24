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

#ifndef INCLUDED_LORA_RTS_CTS_CADDETECT_H
#define INCLUDED_LORA_RTS_CTS_CADDETECT_H

#include <lora_rts_cts/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace lora_rts_cts {
    enum cad_state_t {
      S_CAD_RESET,
      S_CAD_DETECT,
      S_CAD_OUT
    };
    /*!
     * \brief <+description of block+>
     * \ingroup lora_rts_cts
     *
     */
    class LORA_RTS_CTS_API cadDetect : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<cadDetect> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of lora_rts_cts::cadDetect.
       *
       * To avoid accidental use of raw pointers, lora_rts_cts::cadDetect's
       * constructor is in a private implementation
       * class. lora_rts_cts::cadDetect::make is the public interface for
       * creating new instances.
       */
      static sptr make(uint8_t sf, uint32_t sampRate,uint32_t bw);
    };

  } // namespace lora_rts_cts
} // namespace gr

#endif /* INCLUDED_LORA_RTS_CTS_CADDETECT_H */

