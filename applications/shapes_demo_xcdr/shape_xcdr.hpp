/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <holoscan/core/domain/dlpack.hpp>
#include <holoscan/core/domain/tensor.hpp>
#include <holoscan/core/tensor_contract.hpp>
#include <holoscan/memory/memory_storage_type.hpp>
#include <holoscan/memory/tensor_storage.hpp>

#include "ShapeType.hpp"
#include "ShapeTypePlugin.hpp"

namespace rti::holoscan::example {

inline constexpr DLDataType kXcdrByteType{kDLUInt, 8U, 1U};
inline constexpr std::uint64_t kMaxShapeXcdrBytes = 512U;
inline constexpr ::holoscan::TensorRepresentation kXcdrTensorRepresentation{
    .memory_kind = ::holoscan::MemoryKind::kHost,
    .dtype = kXcdrByteType,
    .rank = 1U,
};

class XcdrBufferReference final : public ::holoscan::TensorMemoryReference {
 public:
  explicit XcdrBufferReference(std::vector<std::uint8_t> bytes)
      : bytes_(std::move(bytes)) {}

  [[nodiscard]] void* data() noexcept { return bytes_.data(); }
  [[nodiscard]] std::size_t size() const noexcept { return bytes_.size(); }

 private:
  std::vector<std::uint8_t> bytes_;
};

struct ShapeXcdrAdapter {
  using holoscan_type = ::holoscan::Tensor;

  [[nodiscard]] static ::holoscan::TensorInputSpec tensor_input_spec() {
    return {
        .representation = kXcdrTensorRepresentation,
        .bounds = ::holoscan::tensor_bounds(kMaxShapeXcdrBytes),
    };
  }

  [[nodiscard]] static ::holoscan::TensorOutputSpec tensor_output_spec() {
    return {
        .representation = kXcdrTensorRepresentation,
        .bounds = ::holoscan::tensor_bounds(kMaxShapeXcdrBytes),
        .storage = ::holoscan::TensorOutputStorage::kPassThrough,
        .transfer = ::holoscan::TensorTransferPolicy{
            .foreign_emit_policy =
                ::holoscan::InterfaceTensorEmitPolicy::kInlineSmallHostThenCopyIfNeeded,
            .max_inline_bytes = kMaxShapeXcdrBytes,
            .max_staging_byte_span = kMaxShapeXcdrBytes,
        },
    };
  }

  [[nodiscard]] static ::holoscan::Tensor encode(const ::ShapeTypeExtended& sample) {
    unsigned int size = 0U;
    if (!::ShapeTypeExtendedPlugin_serialize_to_cdr_buffer(
            nullptr, &size, &sample, ::dds::core::policy::DataRepresentation::xcdr()) ||
        size == 0U || size > kMaxShapeXcdrBytes) {
      throw std::runtime_error{"Unable to determine bounded ShapeTypeExtended XCDR size"};
    }

    std::vector<std::uint8_t> bytes(size);
    unsigned int written = size;
    if (!::ShapeTypeExtendedPlugin_serialize_to_cdr_buffer(
            reinterpret_cast<char*>(bytes.data()), &written, &sample,
            ::dds::core::policy::DataRepresentation::xcdr()) ||
        written == 0U || written > size) {
      throw std::runtime_error{"Unable to serialize ShapeTypeExtended as XCDR"};
    }
    bytes.resize(written);

    auto storage = std::make_shared<XcdrBufferReference>(std::move(bytes));
    const std::array<std::int64_t, 1U> shape{static_cast<std::int64_t>(storage->size())};
    auto tensor = ::holoscan::make_tensor(
        shape, kXcdrByteType, std::nullopt, ::holoscan::MemoryStorageType::kSystem,
        storage->data(), storage);
    if (!tensor) {
      throw std::runtime_error{"Unable to wrap the XCDR buffer in a Holoscan Tensor"};
    }
    return std::move(*tensor);
  }

  [[nodiscard]] static ::ShapeTypeExtended decode(const ::holoscan::Tensor& tensor) {
    if (tensor.data() == nullptr || tensor.device().device_type != kDLCPU || tensor.ndim() != 1 ||
        tensor.dtype().code != kXcdrByteType.code ||
        tensor.dtype().bits != kXcdrByteType.bits ||
        tensor.dtype().lanes != kXcdrByteType.lanes || !tensor.is_contiguous() ||
        tensor.nbytes() <= 0 ||
        tensor.nbytes() > static_cast<std::int64_t>(kMaxShapeXcdrBytes) ||
        tensor.nbytes() > static_cast<std::int64_t>(std::numeric_limits<unsigned int>::max())) {
      throw std::invalid_argument{"Expected a bounded contiguous host XCDR byte tensor"};
    }

    const auto* bytes = tensor.data_as<std::uint8_t>();
    if (bytes == nullptr) {
      throw std::invalid_argument{"XCDR tensor data is unavailable"};
    }

    ::ShapeTypeExtended sample;
    if (!::ShapeTypeExtendedPlugin_deserialize_from_cdr_buffer(
            &sample, reinterpret_cast<const char*>(bytes),
            static_cast<unsigned int>(tensor.nbytes()))) {
      throw std::runtime_error{"Unable to deserialize ShapeTypeExtended from XCDR"};
    }
    return sample;
  }

  [[nodiscard]] static ::ShapeTypeExtended to_dds(const holoscan_type& tensor) {
    return decode(tensor);
  }

  [[nodiscard]] static holoscan_type from_dds(const ::ShapeTypeExtended& sample) {
    return encode(sample);
  }
};

}  // namespace rti::holoscan::example
