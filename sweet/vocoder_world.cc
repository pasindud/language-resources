// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2017 Google, Inc.
// Author: mjansche@google.com (Martin Jansche)
//
// \file
// Analysis and synthesis with the World vocoder.

#include <cmath>
#include <iostream>
#include <vector>

#include <tools/audioio.h>
#include <world/dio.h>
#include <world/stonemask.h>

#include "sweet/voice_data.pb.h"

namespace {

class Analysis {
 public:
  Analysis() = delete;

  Analysis(const char *path, double frame_shift_ms)
      : frame_shift_ms_(frame_shift_ms),
        num_samples_(GetAudioLength(path)) {
    if (num_samples_ <= 0) return;
    samples_.resize(num_samples_);
    wavread(path, &sample_rate_, &bit_depth_, samples_.data());
    num_frames_ = GetSamplesForDIO(sample_rate_, num_samples_, frame_shift_ms_);
  }

  double FrameShiftInSeconds() const { return frame_shift_ms_ * 1e-3; }

  int num_samples() const { return num_samples_; }

  int sample_rate() const { return sample_rate_; }

  int num_frames() const { return num_frames_; }

  std::ostream &PrintSummary(std::ostream &strm) const {
    strm << "Audio length: " << num_samples_ << " samples" << std::endl;
    strm << "Sample rate: " << sample_rate_ << " Hz" << std::endl;
    strm << "Bit depth: " << bit_depth_ << " bits" << std::endl;
    return strm;
  }

  void GetF0(sweet::WorldData *world_data) const {
    DioOption dio_option;
    InitializeDioOption(&dio_option);
    dio_option.frame_period = frame_shift_ms_;
    std::vector<double> temporal_positions(num_samples_);
    std::vector<double> f0(num_frames_);
    std::vector<double> rf0(num_frames_);
    Dio(samples_.data(), samples_.size(), sample_rate_, &dio_option,
        temporal_positions.data(), f0.data());
    StoneMask(samples_.data(), samples_.size(), sample_rate_,
              temporal_positions.data(), f0.data(), f0.size(), rf0.data());
    for (size_t f = 0; f < num_frames_; ++f) {
      double x = rf0[f];
      float y = -1e10f;
      if (x > 0) {
        y = std::log(x);
      }
      world_data->mutable_frame(f)->set_lf0(y);
    }
  }

 private:
  const double frame_shift_ms_;
  const int num_samples_;
  int sample_rate_;
  int bit_depth_;
  std::vector<double> samples_;
  size_t num_frames_;
};

}  // namespace

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Input file not provided" << std::endl;
    return 1;
  }
  const char *const path = argv[1];

  Analysis analysis(path, 5.0);
  analysis.PrintSummary(std::cerr);

  sweet::WorldData world_data;
  world_data.set_frame_shift_s(analysis.FrameShiftInSeconds());
  world_data.set_num_samples(analysis.num_samples());
  world_data.set_sample_rate_hz(analysis.sample_rate());

  auto *frames = world_data.mutable_frame();
  frames->Reserve(analysis.num_frames());
  for (int i = 0; i < analysis.num_frames(); ++i) {
    frames->Add();
  }

  analysis.GetF0(&world_data);

  // Generate debug output.
  const double duration_s = world_data.num_samples() /
      static_cast<double>(world_data.sample_rate_hz());
  for (int i = 0; i < world_data.frame_size(); ++i) {
    float start = i * world_data.frame_shift_s();
    float end = (i + 1) * world_data.frame_shift_s();
    if (end > duration_s) end = duration_s;
    auto *frame = world_data.mutable_frame(i);
    frame->set_start(start);
    frame->set_end(end);
  }
  std::cout << world_data.Utf8DebugString();

  return 0;
}