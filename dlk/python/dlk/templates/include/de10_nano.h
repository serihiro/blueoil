/* Copyright 2018 The Blueoil Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#pragma once
#include "global.h"
#include "memdriver.h"
#include "time_measurement.h"
#include <cassert>

namespace de10_nano {

class QconvWithKn2row {
public:
  QconvWithKn2row()
      : start(IP_CSR_ADDR + 0x08, 1, sizeof(T_UINT)),
        done(IP_CSR_ADDR + 0x18, 1, sizeof(T_UINT)),
        in_data_reg(IP_CSR_ADDR + 0x20, 1, sizeof(T_UINT)),
        out_data_reg(IP_CSR_ADDR + 0x28, 1, sizeof(T_UINT)),
        k_data_reg(IP_CSR_ADDR + 0x30, 1, sizeof(T_UINT)),
        out_data_partial_reg(IP_CSR_ADDR + 0x38, 1, sizeof(T_UINT)),
        in_w_reg(IP_CSR_ADDR + 0x40, 1, sizeof(T_UINT)),
        in_h_reg(IP_CSR_ADDR + 0x48, 1, sizeof(T_UINT)),
        in_c_reg(IP_CSR_ADDR + 0x50, 1, sizeof(T_UINT)),
        out_w_reg(IP_CSR_ADDR + 0x58, 1, sizeof(T_UINT)),
        out_h_reg(IP_CSR_ADDR + 0x60, 1, sizeof(T_UINT)),
        out_c_reg(IP_CSR_ADDR + 0x68, 1, sizeof(T_UINT)),
        k_w_reg(IP_CSR_ADDR + 0x70, 1, sizeof(T_UINT)),
        k_h_reg(IP_CSR_ADDR + 0x78, 1, sizeof(T_UINT)),
        pad_reg(IP_CSR_ADDR + 0x80, 1, sizeof(T_UINT)),

        th_start(TH_IP_CSR_ADDR + 0x08, 1, sizeof(T_UINT)),
        th_done(TH_IP_CSR_ADDR + 0x18, 1, sizeof(T_UINT)),
        th_in_data_reg(TH_IP_CSR_ADDR + 0x20, 1, sizeof(T_UINT)),
        th_out_data_reg(TH_IP_CSR_ADDR + 0x28, 1, sizeof(T_UINT)),
        th_data_reg(TH_IP_CSR_ADDR + 0x30, 1, sizeof(T_UINT)),
        th_out_w_reg(TH_IP_CSR_ADDR + 0x38, 1, sizeof(T_UINT)),
        th_out_h_reg(TH_IP_CSR_ADDR + 0x40, 1, sizeof(T_UINT)),
        th_out_c_reg(TH_IP_CSR_ADDR + 0x48, 1, sizeof(T_UINT)) {}

  void run_qconv_with_kn2row(unsigned long in_data_addr,
                             unsigned long out_data_addr,
                             unsigned long k_data_addr, unsigned in_w,
                             unsigned in_h, unsigned in_c, unsigned out_w,
                             unsigned out_h, unsigned out_c, unsigned k_w,
                             unsigned k_h, unsigned pad) {
    in_data_reg.Write(in_data_addr);
    out_data_reg.Write(out_data_addr);
    k_data_reg.Write(k_data_addr);
    out_data_partial_reg.Write(out_data_addr);

    in_w_reg.Write(in_w);
    in_h_reg.Write(in_h);
    in_c_reg.Write(in_c);
    out_w_reg.Write(out_w);
    out_h_reg.Write(out_h);
    out_c_reg.Write(out_c);
    k_h_reg.Write(k_h);
    k_w_reg.Write(k_w);
    pad_reg.Write(pad);

    start.Write(0x1);

    volatile T_UINT done_flag = 0;
    while (!(done_flag & 0x2)) {
      done.Read(done_flag);
    }
  }

  void run_apply_thresholds(unsigned long in_data_addr,
                            unsigned long out_data_addr,
                            unsigned long th_data_addr, unsigned out_w,
                            unsigned out_h, unsigned out_c) {
    th_in_data_reg.Write(in_data_addr);
    th_out_data_reg.Write(out_data_addr);
    th_data_reg.Write(th_data_addr);

    th_out_w_reg.Write(out_w);
    th_out_h_reg.Write(out_h);
    th_out_c_reg.Write(out_c);

    th_start.Write(0x1);

    volatile T_UINT done_flag = 0;
    while (!(done_flag & 0x2)) {
      th_done.Read(done_flag);
    }
  }

private:
  MappedMem start;
  MappedMem done;
  MappedMem in_data_reg;
  MappedMem out_data_reg;
  MappedMem k_data_reg;
  MappedMem out_data_partial_reg;
  MappedMem in_w_reg;
  MappedMem in_h_reg;
  MappedMem in_c_reg;
  MappedMem out_w_reg;
  MappedMem out_h_reg;
  MappedMem out_c_reg;
  MappedMem k_w_reg;
  MappedMem k_h_reg;
  MappedMem pad_reg;

  MappedMem th_start;
  MappedMem th_done;
  MappedMem th_in_data_reg;
  MappedMem th_out_data_reg;
  MappedMem th_data_reg;
  MappedMem th_out_w_reg;
  MappedMem th_out_h_reg;
  MappedMem th_out_c_reg;
};

void qconv_with_kn2row(unsigned long input_addr, unsigned long output_addr,
                       const QUANTIZED_PACKED_KERNEL k_data_packed[], BIN_CONV_OUTPUT th_data[],
                       unsigned in_w, unsigned in_h, unsigned in_c_by_word,
                       unsigned nbits_in_data, unsigned out_w, unsigned out_h,
                       unsigned out_c, unsigned k_w, unsigned k_h, unsigned pad,
                       unsigned stride) {
  assert((k_h == 1 && k_w == 1) || (k_h == 3 && k_w == 3));

  const unsigned k_size = k_h * k_w * in_c_by_word * out_c;
  static QconvWithKn2row qwq;

  MappedMem k_data_mem(KERNEL_ADDR, k_size, sizeof(T_UINT));
  k_data_mem.Write(k_data_packed, k_size);

  if (th_data == nullptr) {
    qwq.run_qconv_with_kn2row(input_addr, output_addr, KERNEL_ADDR, in_w, in_h,
                              in_c_by_word, out_w, out_h, out_c, k_w, k_h, pad);
  } else { // with threshold skipping
    const unsigned num_th = NUM_OF_A2W1_THRESHOLD;
    const unsigned th_size = out_c * num_th;

    MappedMem th_data_mem(THRESHOLD_ADDR, th_size, sizeof(BIN_CONV_OUTPUT));
    th_data_mem.Write(th_data, th_size);

    qwq.run_qconv_with_kn2row(input_addr, OUTPUT1_ADDR, KERNEL_ADDR, in_w, in_h,
                              in_c_by_word, out_w, out_h, out_c, k_w, k_h, pad);

    qwq.run_apply_thresholds(OUTPUT1_ADDR, output_addr, THRESHOLD_ADDR, out_w,
                             out_h, out_c);
  }
}

class QconvKn2rowTiling {
public:
  QconvKn2rowTiling()
      : start(IP_CSR_ADDR + 0x08, 1, sizeof(T_UINT)),
        done(IP_CSR_ADDR + 0x18, 1, sizeof(T_UINT)),
        in_data_reg(IP_CSR_ADDR + 0x20, 1, sizeof(T_UINT)),
        out_data_reg(IP_CSR_ADDR + 0x28, 1, sizeof(T_UINT)),
        k_data_reg(IP_CSR_ADDR + 0x30, 1, sizeof(T_UINT)),
        th_data_reg(IP_CSR_ADDR + 0x38, 1, sizeof(T_UINT)),
        in_w_reg(IP_CSR_ADDR + 0x40, 1, sizeof(T_UINT)),
        in_h_reg(IP_CSR_ADDR + 0x48, 1, sizeof(T_UINT)),
        in_c_reg(IP_CSR_ADDR + 0x50, 1, sizeof(T_UINT)),
        out_w_reg(IP_CSR_ADDR + 0x58, 1, sizeof(T_UINT)),
        out_h_reg(IP_CSR_ADDR + 0x60, 1, sizeof(T_UINT)),
        out_c_reg(IP_CSR_ADDR + 0x68, 1, sizeof(T_UINT)),
        k_w_reg(IP_CSR_ADDR + 0x70, 1, sizeof(T_UINT)),
        k_h_reg(IP_CSR_ADDR + 0x78, 1, sizeof(T_UINT)),
        pad_reg(IP_CSR_ADDR + 0x80, 1, sizeof(T_UINT)),
        use_threshold_reg(IP_CSR_ADDR + 0x88, 1, sizeof(T_UINT)) {}

  void run(unsigned long in_data_addr, unsigned long out_data_addr,
           unsigned long k_data_addr, unsigned long th_data_addr, unsigned in_w,
           unsigned in_h, unsigned in_c, unsigned out_w, unsigned out_h,
           unsigned out_c, unsigned k_w, unsigned k_h, unsigned pad,
           unsigned use_threshold) {
    in_data_reg.Write(in_data_addr);
    out_data_reg.Write(out_data_addr);
    k_data_reg.Write(k_data_addr);
    th_data_reg.Write(th_data_addr);

    in_w_reg.Write(in_w);
    in_h_reg.Write(in_h);
    in_c_reg.Write(in_c);
    out_w_reg.Write(out_w);
    out_h_reg.Write(out_h);
    out_c_reg.Write(out_c);
    k_h_reg.Write(k_h);
    k_w_reg.Write(k_w);
    pad_reg.Write(pad);
    use_threshold_reg.Write(use_threshold);

    start.Write(0x1);

    volatile T_UINT done_flag = 0;
    while (!(done_flag & 0x2)) {
      done.Read(done_flag);
    }
  }

private:
  MappedMem start;
  MappedMem done;
  MappedMem in_data_reg;
  MappedMem out_data_reg;
  MappedMem k_data_reg;
  MappedMem th_data_reg;
  MappedMem in_w_reg;
  MappedMem in_h_reg;
  MappedMem in_c_reg;
  MappedMem out_w_reg;
  MappedMem out_h_reg;
  MappedMem out_c_reg;
  MappedMem k_w_reg;
  MappedMem k_h_reg;
  MappedMem pad_reg;
  MappedMem use_threshold_reg;
};

void qconv_kn2row_tiling(unsigned long input_addr, unsigned long output_addr,
                         const QUANTIZED_PACKED_KERNEL k_data_packed[],
                         BIN_CONV_OUTPUT th_data[], unsigned in_w,
                         unsigned in_h, unsigned in_c_by_word,
                         unsigned nbits_in_data, unsigned out_w, unsigned out_h,
                         unsigned out_c, unsigned k_w, unsigned k_h,
                         unsigned pad, unsigned stride) {
  assert((k_h == 1 && k_w == 1) || (k_h == 3 && k_w == 3));

  const unsigned in_size = in_h * in_w * in_c_by_word * nbits_in_data;
  const unsigned out_size = out_h * out_w * out_c;
  const unsigned k_size = k_h * k_w * in_c_by_word * out_c;

  static QconvKn2rowTiling qkt;
  MappedMem k_data_mem(KERNEL_ADDR, k_size, sizeof(QUANTIZED_PACKED_KERNEL));
  k_data_mem.Write(k_data_packed, k_size);
  unsigned use_threshold = (th_data != NULL) ? 1 : 0;

  if (use_threshold == 1) {
    const unsigned th_size = out_c * NUM_OF_A2W1_THRESHOLD;
    MappedMem th_data_mem(THRESHOLD_ADDR, th_size, sizeof(BIN_CONV_OUTPUT));
    th_data_mem.Write(th_data, th_size);

    qkt.run(input_addr, output_addr, KERNEL_ADDR, THRESHOLD_ADDR, in_w, in_h,
            in_c_by_word, out_w, out_h, out_c, k_w, k_h, pad, use_threshold);
  } else {
    qkt.run(input_addr, output_addr, KERNEL_ADDR, THRESHOLD_ADDR, in_w, in_h,
            in_c_by_word, out_w, out_h, out_c, k_w, k_h, pad, use_threshold);
  }
}

//
// TCA
//
uint8_t* mapPhysicalMemory(size_t base, size_t size) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC, 0);
    if (fd == -1)
        throw std::system_error(errno, std::generic_category());
    int rw = PROT_READ | PROT_WRITE;
    auto* mapped_base = reinterpret_cast<uint8_t*>(mmap(nullptr, size, rw, MAP_SHARED, fd, base));
    if (mapped_base == MAP_FAILED)
        throw std::system_error(errno, std::generic_category());
    return mapped_base;
}

struct Csr {
    static constexpr uint32_t start = 0;
    static constexpr uint32_t admaInputAddress = 1;
    static constexpr uint32_t admaInputHCount = 2;
    static constexpr uint32_t admaInputWCount = 3;
    static constexpr uint32_t admaInputCCount = 4;
    static constexpr uint32_t admaTopTileH = 5;
    static constexpr uint32_t admaMiddleTileH = 6;
    static constexpr uint32_t admaBottomTileH = 7;
    static constexpr uint32_t admaLeftTileW = 8;
    static constexpr uint32_t admaMiddleTileW = 9;
    static constexpr uint32_t admaRightTileW = 10;
    static constexpr uint32_t admaLeftRowToRowDistance = 11;
    static constexpr uint32_t admaMiddleRowToRowDistance = 12;
    static constexpr uint32_t admaRightRowToRowDistance = 13;
    static constexpr uint32_t admaLeftStep = 14;
    static constexpr uint32_t admaMiddleStep = 15;
    static constexpr uint32_t admaTopRowDistance = 16;
    static constexpr uint32_t admaMidRowDistance = 17;
    static constexpr uint32_t admaInputSpace = 18;
    static constexpr uint32_t admaTopBottomLeftPad = 19;
    static constexpr uint32_t admaTopBottomMiddlePad = 20;
    static constexpr uint32_t admaTopBottomRightPad = 21;
    static constexpr uint32_t admaSidePad = 22;
    static constexpr uint32_t wdmaStartAddress = 23;
    static constexpr uint32_t wdmaOutputHCount = 24;
    static constexpr uint32_t wdmaOutputWCount = 25;
    static constexpr uint32_t wdmaKernelBlockCount = 26;
    static constexpr uint32_t fdmaOutputAddress = 27;
    static constexpr uint32_t fdmaOutputHCount = 28;
    static constexpr uint32_t fdmaOutputWCount = 29;
    static constexpr uint32_t fdmaOutputCCount = 30;
    static constexpr uint32_t fdmaRegularTileH = 31;
    static constexpr uint32_t fdmaLastTileH = 32;
    static constexpr uint32_t fdmaRegularTileW = 33;
    static constexpr uint32_t fdmaLastTileW = 34;
    static constexpr uint32_t fdmaRegularRowToRowDistance = 35;
    static constexpr uint32_t fdmaLastRowToRowDistance = 36;
    static constexpr uint32_t fdmaOutputSpace = 37;
    static constexpr uint32_t fdmaRowDistance = 38;
    static constexpr uint32_t a2fInputCCount = 39;
    static constexpr uint32_t a2fKernelVCount = 40;
    static constexpr uint32_t a2fKernelHCount = 41;
    static constexpr uint32_t a2fTileStep = 42;
    static constexpr uint32_t a2fTileGap = 43;
    static constexpr uint32_t a2fOutputHCount = 44;
    static constexpr uint32_t a2fOutputWCount = 45;
    static constexpr uint32_t a2fRegularTileH = 46;
    static constexpr uint32_t a2fLastTileH = 47;
    static constexpr uint32_t a2fRegularTileW = 48;
    static constexpr uint32_t a2fLastTileW = 49;
    static constexpr uint32_t qdmaStartAddress = 50;
    static constexpr uint32_t bnqEnable = 51;

    static constexpr uint32_t statusRegister = 52;
};

struct Parameters {
    uint32_t admaInputAddress;
    uint32_t admaInputHCount;
    uint32_t admaInputWCount;
    uint32_t admaInputCCount;
    uint32_t admaTopTileH;
    uint32_t admaMiddleTileH;
    uint32_t admaBottomTileH;
    uint32_t admaLeftTileW;
    uint32_t admaMiddleTileW;
    uint32_t admaRightTileW;
    uint32_t admaLeftRowToRowDistance;
    uint32_t admaMiddleRowToRowDistance;
    uint32_t admaRightRowToRowDistance;
    uint32_t admaLeftStep;
    uint32_t admaMiddleStep;
    uint32_t admaTopRowDistance;
    uint32_t admaMidRowDistance;
    uint32_t admaInputSpace;
    uint32_t admaTopBottomLeftPad;
    uint32_t admaTopBottomMiddlePad;
    uint32_t admaTopBottomRightPad;
    uint32_t admaSidePad;
    uint32_t wdmaStartAddress;
    uint32_t wdmaOutputHCount;
    uint32_t wdmaOutputWCount;
    uint32_t wdmaKernelBlockCount;
    uint32_t fdmaOutputAddress;
    uint32_t fdmaOutputHCount;
    uint32_t fdmaOutputWCount;
    uint32_t fdmaOutputCCount;
    uint32_t fdmaRegularTileH;
    uint32_t fdmaLastTileH;
    uint32_t fdmaRegularTileW;
    uint32_t fdmaLastTileW;
    uint32_t fdmaRegularRowToRowDistance;
    uint32_t fdmaLastRowToRowDistance;
    uint32_t fdmaOutputSpace;
    uint32_t fdmaRowDistance;
    uint32_t a2fInputCCount;
    uint32_t a2fKernelVCount;
    uint32_t a2fKernelHCount;
    uint32_t a2fTileStep;
    uint32_t a2fTileGap;
    uint32_t a2fOutputHCount;
    uint32_t a2fOutputWCount;
    uint32_t a2fRegularTileH;
    uint32_t a2fLastTileH;
    uint32_t a2fRegularTileW;
    uint32_t a2fLastTileW;
    uint32_t qdmaStartAddress;
    uint32_t bnqEnable;
};

Parameters calcParameters(uint32_t inputHeight, uint32_t inputWidth, uint32_t inputChannels, uint32_t inputTileWidth, uint32_t inputTileHeight,
    uint32_t outputChannels, uint32_t kernelHeight, uint32_t kernelWidth, uint32_t inputAddress, uint32_t kernelAddress, uint32_t thresholdAddress, uint32_t outputAddress, bool enable_bnq) {

  auto divRoundUp = [](uint32_t x, uint32_t y) {
    return (x + y - 1) / y;
  };

  constexpr uint32_t maxBurst = 32;
  constexpr uint32_t b = 32;

  assert((kernelHeight == 3 && kernelWidth == 3) || (kernelHeight == 1 && kernelWidth == 1));

  uint32_t pad = (kernelHeight == 1) ? 0 : 1;
  uint32_t dep = kernelHeight - 1;

  auto outputHeight = inputHeight + 2 * pad - dep;
  auto outputWidth = inputWidth + 2 * pad - dep;

  auto outputTileHeight = inputTileHeight - dep;
  auto outputTileWidth = inputTileWidth - dep;

  assert(inputTileHeight > dep && inputTileWidth > dep);

  auto hCount = divRoundUp(outputHeight, outputTileHeight);
  auto wCount = divRoundUp(outputWidth, outputTileWidth);

  // ADMA Parameters
  Parameters p;
  p.admaInputAddress = inputAddress;
  p.admaInputHCount = hCount;
  p.admaInputWCount = wCount;

  p.admaInputCCount = divRoundUp(inputChannels, b);

  p.admaTopTileH = (hCount == 1) ? inputHeight : (inputTileHeight - pad);
  p.admaMiddleTileH = inputTileHeight;
  p.admaBottomTileH = inputHeight + pad - (hCount - 1)  * (inputTileHeight - dep);

  p.admaLeftTileW = (wCount == 1) ? inputWidth : (inputTileWidth - pad);
  p.admaMiddleTileW = inputTileWidth;
  p.admaRightTileW = (wCount == 1) ? inputWidth : inputWidth + pad - (wCount - 1) * (inputTileWidth - dep);

  p.admaLeftRowToRowDistance = inputWidth - p.admaLeftTileW + ((p.admaLeftTileW % maxBurst == 0) ? maxBurst : p.admaLeftTileW % maxBurst);
  p.admaMiddleRowToRowDistance = inputWidth - p.admaMiddleTileW + ((p.admaMiddleTileW % maxBurst == 0) ? maxBurst : p.admaMiddleTileW % maxBurst);
  p.admaRightRowToRowDistance = inputWidth - p.admaRightTileW + ((p.admaRightTileW % maxBurst == 0) ? maxBurst : p.admaRightTileW % maxBurst);

  p.admaLeftStep = p.admaLeftTileW - dep;
  p.admaMiddleStep = p.admaMiddleTileW - dep;

  p.admaTopRowDistance = inputWidth * (p.admaTopTileH - dep) - inputWidth + p.admaRightTileW;
  p.admaMidRowDistance = inputWidth * (p.admaMiddleTileH - dep) - inputWidth + p.admaRightTileW;

  p.admaInputSpace = inputHeight * inputWidth;

  p.admaTopBottomLeftPad = (p.admaLeftTileW + pad) * pad;
  p.admaTopBottomMiddlePad = p.admaMiddleTileW * pad;
  p.admaTopBottomRightPad = (p.admaRightTileW + pad) * pad;
  p.admaSidePad = (wCount == 1) ? (2 * pad) : pad;

  // WDMA Parameters
  p.wdmaStartAddress = kernelAddress;
  p.wdmaOutputHCount = hCount;
  p.wdmaOutputWCount = wCount;
  p.wdmaKernelBlockCount = divRoundUp(outputChannels, b) * divRoundUp(inputChannels, b) * kernelHeight * kernelWidth;

  // FDMA Parameters
  //bool enableBnq = true;
  const uint32_t dataWidth = (enable_bnq) ? b * 2 : b * 16;
  const uint32_t avalonDataWidth = (enable_bnq) ? b * 2 : 128;
  auto bytesPerElement = dataWidth / 8;
  auto wordsPerElement = dataWidth / avalonDataWidth;

  auto fdmaRowToRowDistance = [&](uint32_t outputWidth, uint32_t tileWidth) {
    return (outputWidth - tileWidth) * wordsPerElement + 1;
  };

  p.fdmaOutputAddress = outputAddress;

  p.fdmaOutputHCount = hCount;
  p.fdmaOutputWCount = wCount;
  p.fdmaOutputCCount = divRoundUp(outputChannels, b);

  p.fdmaRegularTileH = outputTileHeight;
  p.fdmaLastTileH = outputHeight - (hCount - 1)  * outputTileHeight;

  p.fdmaRegularTileW = outputTileWidth;
  p.fdmaLastTileW = outputWidth - (wCount - 1)  * outputTileWidth;

  p.fdmaRegularRowToRowDistance = fdmaRowToRowDistance(outputWidth, p.fdmaRegularTileW);
  p.fdmaLastRowToRowDistance = fdmaRowToRowDistance(outputWidth, p.fdmaLastTileW);

  p.fdmaOutputSpace = outputHeight * outputWidth;
  p.fdmaRowDistance = outputWidth * p.fdmaRegularTileH - outputWidth + p.fdmaLastTileW;

  // A2F Parameters
  p.a2fInputCCount = p.admaInputCCount;
  p.a2fKernelVCount = kernelHeight;
  p.a2fKernelHCount = kernelWidth;

  if (kernelHeight == 1) {
    p.a2fTileStep = 1u;
    p.a2fTileGap = 1u;
  }
  else {
    // TODO: 3x3 stride one assumed here
    p.a2fTileStep = 1u;
    p.a2fTileGap = 3u;
  }

  p.a2fOutputHCount = hCount;
  p.a2fOutputWCount = wCount;

  p.a2fRegularTileH = outputTileHeight;
  p.a2fLastTileH = outputHeight - (hCount - 1) * p.a2fRegularTileH;
  p.a2fRegularTileW = outputTileWidth;
  p.a2fLastTileW = outputWidth - (wCount - 1) * p.a2fRegularTileW;

  p.qdmaStartAddress = thresholdAddress;
  p.bnqEnable = enable_bnq ? 1 : 0;

  return p;
}

void RunTCA(unsigned long input_addr, unsigned long output_addr, unsigned long kernel_addr,
  unsigned long thresholds_addr, unsigned in_w, unsigned in_h, unsigned in_c, unsigned nbits_in_data,
  unsigned out_w, unsigned out_h, unsigned out_c, unsigned k_w, unsigned k_h, unsigned pad, unsigned stride) {

  unsigned use_threshold = (thresholds_addr != 0) ? 1 : 0;

  static volatile uint32_t* csr = nullptr;
  if (csr == nullptr) {
    csr = reinterpret_cast<uint32_t*>(mapPhysicalMemory(HPS_TO_FPGA_LW_BASE, 0xFF));
  }
    auto tileWidth = 32u;
    auto tileHeight = 32u;
    auto p = calcParameters(in_h, in_w, in_c, tileWidth, tileHeight, out_c, k_h, k_w, input_addr, kernel_addr, thresholds_addr, output_addr, use_threshold == 1);

    csr[Csr::admaInputAddress] = p.admaInputAddress;
    csr[Csr::admaInputHCount] = p.admaInputHCount;
    csr[Csr::admaInputWCount] = p.admaInputWCount;
    csr[Csr::admaInputCCount] = p.admaInputCCount;
    csr[Csr::admaTopTileH] = p.admaTopTileH;
    csr[Csr::admaMiddleTileH] = p.admaMiddleTileH;
    csr[Csr::admaBottomTileH] = p.admaBottomTileH;
    csr[Csr::admaLeftTileW] = p.admaLeftTileW;
    csr[Csr::admaMiddleTileW] = p.admaMiddleTileW;
    csr[Csr::admaRightTileW] = p.admaRightTileW;
    csr[Csr::admaLeftRowToRowDistance] = p.admaLeftRowToRowDistance;
    csr[Csr::admaMiddleRowToRowDistance] = p.admaMiddleRowToRowDistance;
    csr[Csr::admaRightRowToRowDistance] = p.admaRightRowToRowDistance;
    csr[Csr::admaLeftStep] = p.admaLeftStep;
    csr[Csr::admaMiddleStep] = p.admaMiddleStep;
    csr[Csr::admaTopRowDistance] = p.admaTopRowDistance;
    csr[Csr::admaMidRowDistance] = p.admaMidRowDistance;
    csr[Csr::admaInputSpace] = p.admaInputSpace;
    csr[Csr::admaTopBottomLeftPad] = p.admaTopBottomLeftPad;
    csr[Csr::admaTopBottomMiddlePad] = p.admaTopBottomMiddlePad;
    csr[Csr::admaTopBottomRightPad] = p.admaTopBottomRightPad;
    csr[Csr::admaSidePad] = p.admaSidePad;
    csr[Csr::wdmaStartAddress] = p.wdmaStartAddress;
    csr[Csr::wdmaOutputHCount] = p.wdmaOutputHCount;
    csr[Csr::wdmaOutputWCount] = p.wdmaOutputWCount;
    csr[Csr::wdmaKernelBlockCount] = p.wdmaKernelBlockCount;
    csr[Csr::fdmaOutputAddress] = p.fdmaOutputAddress;
    csr[Csr::fdmaOutputHCount] = p.fdmaOutputHCount;
    csr[Csr::fdmaOutputWCount] = p.fdmaOutputWCount;
    csr[Csr::fdmaOutputCCount] = p.fdmaOutputCCount;
    csr[Csr::fdmaRegularTileH] = p.fdmaRegularTileH;
    csr[Csr::fdmaLastTileH] = p.fdmaLastTileH;
    csr[Csr::fdmaRegularTileW] = p.fdmaRegularTileW;
    csr[Csr::fdmaLastTileW] = p.fdmaLastTileW;
    csr[Csr::fdmaRegularRowToRowDistance] = p.fdmaRegularRowToRowDistance;
    csr[Csr::fdmaLastRowToRowDistance] = p.fdmaLastRowToRowDistance;
    csr[Csr::fdmaOutputSpace] = p.fdmaOutputSpace;
    csr[Csr::fdmaRowDistance] = p.fdmaRowDistance;
    csr[Csr::a2fInputCCount] = p.a2fInputCCount;
    csr[Csr::a2fKernelVCount] = p.a2fKernelVCount;
    csr[Csr::a2fKernelHCount] = p.a2fKernelHCount;
    csr[Csr::a2fTileStep] = p.a2fTileStep;
    csr[Csr::a2fTileGap] = p.a2fTileGap;
    csr[Csr::a2fOutputHCount] = p.a2fOutputHCount;
    csr[Csr::a2fOutputWCount] = p.a2fOutputWCount;
    csr[Csr::a2fRegularTileH] = p.a2fRegularTileH;
    csr[Csr::a2fLastTileH] = p.a2fLastTileH;
    csr[Csr::a2fRegularTileW] = p.a2fRegularTileW;
    csr[Csr::a2fLastTileW] = p.a2fLastTileW;
    csr[Csr::qdmaStartAddress] = p.qdmaStartAddress;
    csr[Csr::bnqEnable] = p.bnqEnable;

    // std::cout << "Status " << csr[Csr::statusRegister] << std::endl;
    csr[Csr::start] = 1;

    // std::cout << "Status " << csr[Csr::statusRegister] << std::endl;
    while (csr[Csr::statusRegister] != 127) {
        // std::cout << "Status " << csr[Csr::statusRegister] << std::endl;
        continue;
    }
}

} // namespace de10_nano
