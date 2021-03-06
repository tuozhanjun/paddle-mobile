/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "../test_helper.h"
#include "../test_include.h"
#include "operators/quantize_op.h"

namespace paddle_mobile {

// static float g_test_data[50] = {
//   -5.55, -5.5, -5.45, -5.0, -4.55, -4.5, -4.45, -4.0, -3.55, -3.5,
//   -3.45, -3.01, -2.75, -2.5, -2.501, -2.49, -2.01, -1.75, -1.5, -1.25,
//   -1.0, -0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75, 1.0, 1.25,
//   1.5, 1.75, 2.01, 2.49, 2.501, 2.5, 2.75, 3.01, 3.45, 3.5,
//   3.55, 4.0, 4.45, 4.5, 4.55, 5.0, 5.45, 5.5, 5.55, 6.0,
// };

static float find_abs_max(const Tensor *input) {
  float max_abs = 0.f;
  const float *x = input->data<const float>();
  size_t size = input->numel();
  for (size_t i = 0; i < size; ++i) {
    float value = std::abs(x[i]);
    if (value > max_abs) {
      max_abs = value;
    }
  }
  return max_abs;
}

static void quantize_round_to_even(const Tensor *input, const float scale,
                                   Tensor *output) {
  const float *x = input->data<const float>();
  int8_t *y = output->mutable_data<int8_t>();
  size_t size = input->numel();
  for (size_t i = 0; i < size; ++i) {
    float value = x[i] * scale;
    float v = round(value);
    int32_t q = (int32_t)v;
    if (abs(abs(q - value) - 0.5) > 0) {
      y[i] = q;
    } else {
      if (abs(q) % 2 == 0) {
        y[i] = q;
      } else {
        y[i] = q + ((q > 0) ? -1 : 1);
      }
    }
  }
}

int TestQuqntizeOp() {
  framework::DDim dim = framework::make_ddim({1, 3, 224, 224});

  VariableNameMap inputs;
  VariableNameMap outputs;
  auto scope = std::make_shared<framework::Scope>();
  inputs["X"] = std::vector<std::string>({"input"});
  outputs["Out"] = std::vector<std::string>({"output"});
  outputs["OutScale"] = std::vector<std::string>({"output_scale"});

  auto input_var = scope.get()->Var("input");
  auto input = input_var->template GetMutable<framework::LoDTensor>();
  SetupTensor<float>(input, dim, -100.f, 100.f);

  auto output_var = scope.get()->Var("output");
  auto output_scale_var = scope.get()->Var("output_scale");

  framework::AttributeMap attrs;
  auto *op = new operators::QuantizeOp<CPU, float>("quantize", inputs, outputs,
                                                   attrs, scope);
  op->InferShape();
  op->Run();

  auto output = output_var->template Get<framework::LoDTensor>();
  const int8_t *output_data = output->data<int8_t>();
  auto output_scale = output_scale_var->template Get<framework::LoDTensor>();
  const float *output_scale_data = output_scale->data<float>();

  float max_abs = find_abs_max(input);
  float output_scale_cmp = 127 / max_abs;
  PADDLE_MOBILE_ENFORCE(output_scale_cmp == output_scale_data[0],
                        "output_scale = %.6f, output_scale_cmp = %.6f",
                        output_scale_cmp, output_scale_data[0]);

  framework::Tensor output_cmp;
  output_cmp.Resize(dim);
  quantize_round_to_even(input, output_scale_cmp, &output_cmp);
  int8_t *output_cmp_data = output_cmp.data<int8_t>();
  for (int i = 0; i < output->numel(); ++i) {
    PADDLE_MOBILE_ENFORCE(output_data[i] == output_cmp_data[i],
                          "output[%d] = %d, output_cmp[%d] = %d", i,
                          static_cast<int>(output_data[i]), i,
                          static_cast<int>(output_cmp_data[i]));
  }
  delete op;
  return 0;
}

}  // namespace paddle_mobile

int main() { return paddle_mobile::TestQuqntizeOp(); }
