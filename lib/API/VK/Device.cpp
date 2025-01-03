//===- VX/Device.cpp - Vulkan Device API ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/Device.h"
#include "Support/Pipeline.h"
#include "llvm/Support/Error.h"

#include <memory>
#include <vulkan/vulkan.h>

using namespace offloadtest;

#define VKFormats(FMT)                                                         \
  if (Channels == 1)                                                           \
    return VK_FORMAT_R32_##FMT;                                                \
  if (Channels == 2)                                                           \
    return VK_FORMAT_R32G32_##FMT;                                             \
  if (Channels == 3)                                                           \
    return VK_FORMAT_R32G32B32_##FMT;                                          \
  if (Channels == 4)                                                           \
    return VK_FORMAT_R32G32B32A32_##FMT;

static VkFormat getVKFormat(DataFormat Format, int Channels) {
  switch (Format) {
  case DataFormat::Int32:
    VKFormats(SINT) break;
  case DataFormat::Float32:
    VKFormats(SFLOAT) break;
  default:
    llvm_unreachable("Unsupported Resource format specified");
  }
  return VK_FORMAT_UNDEFINED;
}
namespace {

class VKDevice : public offloadtest::Device {
private:
  VkPhysicalDevice Device;
  VkPhysicalDeviceProperties Props;
  Capabilities Caps;
  using LayerVector = std::vector<VkLayerProperties>;
  LayerVector Layers;

  struct BufferRef {
    VkBuffer Buffer;
    VkDeviceMemory Memory;
  };

  struct UAVRef {
    BufferRef Host;
    BufferRef Device;
    uint64_t Size;
  };

  struct InvocationState {
    VkDevice Device;
    VkQueue Queue;
    VkCommandPool CmdPool;
    VkCommandBuffer CmdBuffer;
    VkPipelineLayout PipelineLayout;
    VkDescriptorPool Pool;
    VkPipelineCache PipelineCache;
    VkShaderModule Shader;
    VkPipeline Pipeline;

    llvm::SmallVector<VkDescriptorSetLayout> DescriptorSetLayouts;
    llvm::SmallVector<UAVRef> UAVs;
    llvm::SmallVector<VkDescriptorSet> DescriptorSets;
    llvm::SmallVector<VkBufferView> BufferViews;
  };

public:
  VKDevice(VkPhysicalDevice D) : Device(D) {
    vkGetPhysicalDeviceProperties(Device, &Props);
    uint64_t StrSz =
        strnlen(Props.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
    Description = std::string(Props.deviceName, StrSz);
  }
  VKDevice(const VKDevice &) = default;

  ~VKDevice() override = default;

  llvm::StringRef getAPIName() const override { return "Vulkan"; }
  GPUAPI getAPI() const override { return GPUAPI::Vulkan; }

  const Capabilities &getCapabilities() override {
    if (Caps.empty())
      queryCapabilities();
    return Caps;
  }

  const LayerVector &getLayers() {
    if (Layers.empty())
      queryLayers();
    return Layers;
  }

  bool isLayerSupported(llvm::StringRef QueryName) {
    for (auto Layer : getLayers()) {
      if (Layer.layerName == QueryName)
        return true;
    }
    return false;
  }

  void printExtra(llvm::raw_ostream &OS) override {
    OS << "  Layers:\n";
    for (auto Layer : getLayers()) {
      uint64_t Sz = strnlen(Layer.layerName, VK_MAX_EXTENSION_NAME_SIZE);
      OS << "  - LayerName: " << llvm::StringRef(Layer.layerName, Sz) << "\n";
      OS << "    SpecVersion: " << Layer.specVersion << "\n";
      OS << "    ImplVersion: " << Layer.implementationVersion << "\n";
      Sz = strnlen(Layer.description, VK_MAX_DESCRIPTION_SIZE);
      OS << "    LayerDesc: " << llvm::StringRef(Layer.description, Sz) << "\n";
    }
  }

private:
  void queryCapabilities() {
    VkPhysicalDeviceFeatures Features;
    vkGetPhysicalDeviceFeatures(Device, &Features);

    Caps.insert(std::make_pair(
        "APIMajorVersion",
        make_capability<uint32_t>("APIMajorVersion",
                                  VK_API_VERSION_MAJOR(Props.apiVersion))));

    Caps.insert(std::make_pair(
        "APIMinorVersion",
        make_capability<uint32_t>("APIMinorVersion",
                                  VK_API_VERSION_MINOR(Props.apiVersion))));

#define VULKAN_FEATURE_BOOL(Name)                                              \
  Caps.insert(                                                                 \
      std::make_pair(#Name, make_capability<bool>(#Name, Features.Name)));
#include "VKFeatures.def"
  }

  void queryLayers() {
    assert(Layers.empty() && "Should not be called twice!");
    uint32_t LayerCount;
    vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

    if (LayerCount == 0)
      return;

    Layers.insert(Layers.begin(), LayerCount, VkLayerProperties());
    vkEnumerateInstanceLayerProperties(&LayerCount, Layers.data());
  }

public:
  llvm::Error createDevice(InvocationState &IS) {

    // Find a queue that supports compute
    uint32_t QueueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueCount, 0);
    std::unique_ptr<VkQueueFamilyProperties[]> QueueFamilyProps =
        std::unique_ptr<VkQueueFamilyProperties[]>(
            new VkQueueFamilyProperties[QueueCount]);
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueCount,
                                             QueueFamilyProps.get());
    uint32_t QueueIdx = 0;
    for (; QueueIdx < QueueCount; ++QueueIdx)
      if (QueueFamilyProps.get()[QueueIdx].queueFlags & VK_QUEUE_COMPUTE_BIT)
        break;
    if (QueueIdx >= QueueCount)
      return llvm::createStringError(std::errc::no_such_device,
                                     "No compute queue found.");

    VkDeviceQueueCreateInfo QueueInfo = {};
    float QueuePriority = 0.0f;
    QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    QueueInfo.queueFamilyIndex = QueueIdx;
    QueueInfo.queueCount = 1;
    QueueInfo.pQueuePriorities = &QueuePriority;

    VkDeviceCreateInfo DeviceInfo = {};
    DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceInfo.queueCreateInfoCount = 1;
    DeviceInfo.pQueueCreateInfos = &QueueInfo;

    if (vkCreateDevice(Device, &DeviceInfo, nullptr, &IS.Device))
      return llvm::createStringError(std::errc::no_such_device,
                                     "Could not create Vulkan logical device.");
    vkGetDeviceQueue(IS.Device, QueueIdx, 0, &IS.Queue);

    VkCommandPoolCreateInfo CmdPoolInfo = {};
    CmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CmdPoolInfo.queueFamilyIndex = QueueIdx;
    CmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(IS.Device, &CmdPoolInfo, nullptr, &IS.CmdPool))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Could not create command pool.");
    return llvm::Error::success();
  }

  llvm::Error createCommandBuffer(InvocationState &IS) {
    VkCommandBufferAllocateInfo CBufAllocInfo = {};
    CBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CBufAllocInfo.commandPool = IS.CmdPool;
    CBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CBufAllocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(IS.Device, &CBufAllocInfo, &IS.CmdBuffer))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Could not create command buffer.");
    VkCommandBufferBeginInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(IS.CmdBuffer, &BufferInfo))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Could not begin command buffer.");
    return llvm::Error::success();
  }

  llvm::Expected<BufferRef> createBuffer(InvocationState &IS,
                                         VkBufferUsageFlags Usage,
                                         VkMemoryPropertyFlags MemoryFlags,
                                         size_t Size, void *Data = nullptr) {
    VkBuffer Buffer;
    VkDeviceMemory Memory;
    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size = Size;
    BufferInfo.usage = Usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(IS.Device, &BufferInfo, nullptr, &Buffer))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Could not create buffer.");

    VkMemoryRequirements MemReqs;
    vkGetBufferMemoryRequirements(IS.Device, Buffer, &MemReqs);
    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemReqs.size;

    VkPhysicalDeviceMemoryProperties MemProperties;
    vkGetPhysicalDeviceMemoryProperties(Device, &MemProperties);
    uint32_t MemIdx = 0;
    for (; MemIdx < MemProperties.memoryTypeCount;
         ++MemIdx, MemReqs.memoryTypeBits >>= 1) {
      if ((MemReqs.memoryTypeBits & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
          ((MemProperties.memoryTypes[MemIdx].propertyFlags & MemoryFlags) ==
           MemoryFlags)) {
        break;
      }
    }
    if (MemIdx >= MemProperties.memoryTypeCount)
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Could not identify appropriate memory.");

    AllocInfo.memoryTypeIndex = MemIdx;

    if (vkAllocateMemory(IS.Device, &AllocInfo, nullptr, &Memory))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Memory allocation failed.");
    if (Data) {
      void *Dst = nullptr;
      if (vkMapMemory(IS.Device, Memory, 0, Size, 0, &Dst))
        return llvm::createStringError(std::errc::not_enough_memory,
                                       "Failed to map memory.");
      memcpy(Dst, Data, Size);

      VkMappedMemoryRange Range = {};
      Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      Range.memory = Memory;
      Range.offset = 0;
      Range.size = VK_WHOLE_SIZE;
      vkFlushMappedMemoryRanges(IS.Device, 1, &Range);

      vkUnmapMemory(IS.Device, Memory);
    }

    if (vkBindBufferMemory(IS.Device, Buffer, Memory, 0))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Failed to bind buffer to memory.");

    return BufferRef{Buffer, Memory};
  }

  llvm::Error createUAV(Resource &R, InvocationState &IS,
                        const uint32_t HeapIdx) {
    auto ExHostBuf = createBuffer(
        IS, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, R.Size, R.Data.get());
    if (!ExHostBuf)
      return ExHostBuf.takeError();

    auto ExDeviceBuf = createBuffer(
        IS,
        (R.isRaw() ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                   : VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, R.Size);
    if (!ExDeviceBuf)
      return ExDeviceBuf.takeError();

    VkBufferCopy Copy = {};
    Copy.size = R.Size;
    vkCmdCopyBuffer(IS.CmdBuffer, ExHostBuf->Buffer, ExDeviceBuf->Buffer, 1,
                    &Copy);

    IS.UAVs.push_back(UAVRef{*ExHostBuf, *ExDeviceBuf, R.Size});

    return llvm::Error::success();
  }

  llvm::Error createSRV(Resource &R, InvocationState &IS,
                        const uint32_t HeapIdx) {
    return llvm::createStringError(std::errc::not_supported,
                                   "VXDevice::createSRV not supported.");
  }

  llvm::Error createCBV(Resource &R, InvocationState &IS,
                        const uint32_t HeapIdx) {
    return llvm::createStringError(std::errc::not_supported,
                                   "VXDevice::createCBV not supported.");
  }

  llvm::Error createBuffers(Pipeline &P, InvocationState &IS) {
    uint32_t HeapIndex = 0;
    for (auto &D : P.Sets) {
      for (auto &R : D.Resources) {
        switch (R.Access) {
        case DataAccess::ReadOnly:
          if (auto Err = createSRV(R, IS, HeapIndex++))
            return Err;
          break;
        case DataAccess::ReadWrite:
          if (auto Err = createUAV(R, IS, HeapIndex++))
            return Err;
          break;
        case DataAccess::Constant:
          if (auto Err = createCBV(R, IS, HeapIndex++))
            return Err;
          break;
        }
      }
    }
    return llvm::Error::success();
  }

  llvm::Error executeCommandBuffer(InvocationState &IS,
                                   VkPipelineStageFlags WaitMask = 0) {
    if (vkEndCommandBuffer(IS.CmdBuffer))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Could not end command buffer.");

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &IS.CmdBuffer;
    SubmitInfo.pWaitDstStageMask = &WaitMask;
    VkFenceCreateInfo FenceInfo = {};
    FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence Fence;
    if (vkCreateFence(IS.Device, &FenceInfo, nullptr, &Fence))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Could not create fence.");

    // Submit to the queue
    if (vkQueueSubmit(IS.Queue, 1, &SubmitInfo, Fence))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to submit to queue.");
    if (vkWaitForFences(IS.Device, 1, &Fence, VK_TRUE, UINT64_MAX))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed waiting for fence.");

    vkDestroyFence(IS.Device, Fence, nullptr);
    vkFreeCommandBuffers(IS.Device, IS.CmdPool, 1, &IS.CmdBuffer);
    return llvm::Error::success();
  }

  llvm::Error createDescriptorPool(Pipeline &P, InvocationState &IS) {
    uint32_t TexelBufferCount = 0;
    uint32_t StorageBufferCount = 0;
    for (const auto &S : P.Sets) {
      for (const auto &R : S.Resources) {
        if (R.Access == DataAccess::ReadWrite) {
          if (R.isRaw())
            StorageBufferCount += 1;
          else
            TexelBufferCount += 1;
        }
      }
    }
    assert(TexelBufferCount + StorageBufferCount == P.getDescriptorCount() &&
           "Mismatch in descriptor type identification.");
    llvm::SmallVector<VkDescriptorPoolSize> PoolSizes;
    if (TexelBufferCount > 0) {
      VkDescriptorPoolSize PoolSize = {};
      PoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
      PoolSize.descriptorCount = TexelBufferCount;
      PoolSizes.push_back(PoolSize);
    }

    if (StorageBufferCount > 0) {
      VkDescriptorPoolSize PoolSize = {};
      PoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      PoolSize.descriptorCount = StorageBufferCount;
      PoolSizes.push_back(PoolSize);
    }

    VkDescriptorPoolCreateInfo PoolCreateInfo = {};
    PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolCreateInfo.poolSizeCount = PoolSizes.size();
    PoolCreateInfo.pPoolSizes = PoolSizes.data();
    PoolCreateInfo.maxSets = P.Sets.size();
    if (vkCreateDescriptorPool(IS.Device, &PoolCreateInfo, nullptr, &IS.Pool))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create descriptor pool.");
    return llvm::Error::success();
  }

  llvm::Error createDescriptorSets(Pipeline &P, InvocationState &IS) {
    for (const auto &S : P.Sets) {
      std::vector<VkDescriptorSetLayoutBinding> Bindings;
      uint32_t BindingIdx = 0;
      for (const auto &R : S.Resources) {
        (void)R; // Todo: set this correctly for the data type.
        VkDescriptorSetLayoutBinding Binding = {};
        Binding.binding = BindingIdx++;
        Binding.descriptorType = R.isRaw()
                                     ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                                     : VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        Binding.descriptorCount = 1;
        Binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        Bindings.push_back(Binding);
      }
      VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = {};
      LayoutCreateInfo.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      LayoutCreateInfo.bindingCount = Bindings.size();
      LayoutCreateInfo.pBindings = Bindings.data();
      llvm::outs() << "Binding " << Bindings.size() << " descriptors.\n";
      VkDescriptorSetLayout Layout;
      if (vkCreateDescriptorSetLayout(IS.Device, &LayoutCreateInfo, nullptr,
                                      &Layout))
        return llvm::createStringError(
            std::errc::device_or_resource_busy,
            "Failed to create descriptor set layout.");
      IS.DescriptorSetLayouts.push_back(Layout);
    }

    VkPipelineLayoutCreateInfo PipelineCreateInfo = {};
    PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineCreateInfo.setLayoutCount = IS.DescriptorSetLayouts.size();
    PipelineCreateInfo.pSetLayouts = IS.DescriptorSetLayouts.data();
    if (vkCreatePipelineLayout(IS.Device, &PipelineCreateInfo, nullptr,
                               &IS.PipelineLayout))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create pipeline layout.");

    VkDescriptorSetAllocateInfo DSAllocInfo = {};
    DSAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DSAllocInfo.descriptorPool = IS.Pool;
    DSAllocInfo.descriptorSetCount = IS.DescriptorSetLayouts.size();
    DSAllocInfo.pSetLayouts = IS.DescriptorSetLayouts.data();
    assert(IS.DescriptorSets.empty());
    IS.DescriptorSets.insert(IS.DescriptorSets.begin(),
                             IS.DescriptorSetLayouts.size(), VkDescriptorSet());
    llvm::outs() << "Num Descriptor sets: " << IS.DescriptorSetLayouts.size()
                 << "\n";
    if (vkAllocateDescriptorSets(IS.Device, &DSAllocInfo,
                                 IS.DescriptorSets.data()))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to allocate descriptor sets.");
    llvm::SmallVector<VkWriteDescriptorSet> WriteDescriptors;
    assert(IS.BufferViews.empty());
    llvm::SmallVector<VkDescriptorBufferInfo> RawBufferInfos;

    uint32_t UAVIdx = 0;
    for (uint32_t SetIdx = 0; SetIdx < P.Sets.size(); ++SetIdx) {
      for (uint32_t RIdx = 0; RIdx < P.Sets[SetIdx].Resources.size();
           ++RIdx, ++UAVIdx) {
        // This is a hack... need a better way to do this.
        VkBufferViewCreateInfo ViewCreateInfo = {};
        bool IsRaw = P.Sets[SetIdx].Resources[RIdx].isRaw();
        VkFormat Format =
            IsRaw ? VK_FORMAT_UNDEFINED
                  : getVKFormat(P.Sets[SetIdx].Resources[RIdx].Format,
                                P.Sets[SetIdx].Resources[RIdx].Channels);
        ViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        ViewCreateInfo.buffer = IS.UAVs[UAVIdx].Device.Buffer;
        ViewCreateInfo.format = Format;
        ViewCreateInfo.range = VK_WHOLE_SIZE;
        if (IsRaw) {
          VkDescriptorBufferInfo BI = {IS.UAVs[UAVIdx].Device.Buffer, 0,
                                       VK_WHOLE_SIZE};
          RawBufferInfos.push_back(BI);
        } else {
          IS.BufferViews.push_back(VkBufferView{0});
          if (vkCreateBufferView(IS.Device, &ViewCreateInfo, nullptr,
                                 &IS.BufferViews.back()))
            return llvm::createStringError(std::errc::device_or_resource_busy,
                                           "Failed to create buffer view.");
        }
        VkWriteDescriptorSet WDS = {};
        WDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        WDS.dstSet = IS.DescriptorSets[SetIdx];
        WDS.dstBinding = RIdx;
        WDS.descriptorCount = 1;
        if (IsRaw) {
          WDS.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
          WDS.pBufferInfo = &RawBufferInfos.back();
        } else {
          WDS.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
          WDS.pTexelBufferView = &IS.BufferViews.back();
        }
        llvm::outs() << "Updating Descriptor [" << UAVIdx << "] { " << SetIdx
                     << ", " << RIdx << " }\n";
        WriteDescriptors.push_back(WDS);
      }
    }
    llvm::outs() << "WriteDescriptors: " << WriteDescriptors.size() << "\n";
    vkUpdateDescriptorSets(IS.Device, WriteDescriptors.size(),
                           WriteDescriptors.data(), 0, nullptr);
    return llvm::Error::success();
  }

  llvm::Error createShaderModule(llvm::StringRef Program, InvocationState &IS) {
    VkShaderModuleCreateInfo ShaderCreateInfo = {};
    ShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderCreateInfo.codeSize = Program.size();
    ShaderCreateInfo.pCode = reinterpret_cast<const uint32_t *>(Program.data());
    if (vkCreateShaderModule(IS.Device, &ShaderCreateInfo, nullptr, &IS.Shader))
      return llvm::createStringError(std::errc::not_supported,
                                     "Failed to create shader module.");
    return llvm::Error::success();
  }

  llvm::Error createPipeline(Pipeline &P, InvocationState &IS) {
    VkPipelineCacheCreateInfo CacheCreateInfo = {};
    CacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    if (vkCreatePipelineCache(IS.Device, &CacheCreateInfo, nullptr,
                              &IS.PipelineCache))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create pipeline cache.");

    VkPipelineShaderStageCreateInfo StageInfo = {};
    StageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    StageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    StageInfo.module = IS.Shader;
    StageInfo.pName = "main";

    VkComputePipelineCreateInfo PipelineCreateInfo = {};
    PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.stage = StageInfo;
    PipelineCreateInfo.layout = IS.PipelineLayout;
    if (vkCreateComputePipelines(IS.Device, IS.PipelineCache, 1,
                                 &PipelineCreateInfo, nullptr, &IS.Pipeline))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create pipeline.");

    return llvm::Error::success();
  }

  llvm::Error createComputeCommands(Pipeline &P, InvocationState &IS) {
    for (auto &UAV : IS.UAVs) {
      VkBufferMemoryBarrier Barrier = {};
      Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      Barrier.buffer = UAV.Device.Buffer;
      Barrier.size = VK_WHOLE_SIZE;
      Barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
      Barrier.dstAccessMask =
          VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr,
                           1, &Barrier, 0, nullptr);
    }
    vkCmdBindPipeline(IS.CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      IS.Pipeline);
    vkCmdBindDescriptorSets(IS.CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            IS.PipelineLayout, 0, IS.DescriptorSets.size(),
                            IS.DescriptorSets.data(), 0, 0);
    vkCmdDispatch(IS.CmdBuffer, P.DispatchSize[0], P.DispatchSize[1],
                  P.DispatchSize[2]);

    for (auto &UAV : IS.UAVs) {
      VkBufferMemoryBarrier Barrier = {};
      Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      Barrier.buffer = UAV.Device.Buffer;
      Barrier.size = VK_WHOLE_SIZE;
      Barrier.srcAccessMask =
          VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
                           &Barrier, 0, nullptr);
      VkBufferCopy CopyRegion = {};
      CopyRegion.size = UAV.Size;
      vkCmdCopyBuffer(IS.CmdBuffer, UAV.Device.Buffer, UAV.Host.Buffer, 1,
                      &CopyRegion);

      Barrier.buffer = UAV.Host.Buffer;
      Barrier.size = VK_WHOLE_SIZE;
      Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      Barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1,
                           &Barrier, 0, nullptr);
    }
    return llvm::Error::success();
  }

  llvm::Error readBackData(Pipeline &P, InvocationState &IS) {
    uint32_t UAVIdx = 0;
    for (auto &S : P.Sets) {
      for (auto &R : S.Resources) {
        if (R.Access != DataAccess::ReadWrite)
          continue;
        void *Mapped = nullptr;
        vkMapMemory(IS.Device, IS.UAVs[UAVIdx].Host.Memory, 0, VK_WHOLE_SIZE, 0,
                    &Mapped);
        VkMappedMemoryRange Range = {};
        Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        Range.memory = IS.UAVs[UAVIdx].Host.Memory;
        Range.offset = 0;
        Range.size = VK_WHOLE_SIZE;
        vkInvalidateMappedMemoryRanges(IS.Device, 1, &Range);
        memcpy(R.Data.get(), Mapped, R.Size);
        vkUnmapMemory(IS.Device, IS.UAVs[UAVIdx].Host.Memory);
        UAVIdx++;
      }
    }
    return llvm::Error::success();
  }

  llvm::Error cleanup(InvocationState &IS) {
    vkQueueWaitIdle(IS.Queue);
    for (auto &V : IS.BufferViews)
      vkDestroyBufferView(IS.Device, V, nullptr);

    for (auto &R : IS.UAVs) {
      vkDestroyBuffer(IS.Device, R.Device.Buffer, nullptr);
      vkFreeMemory(IS.Device, R.Device.Memory, nullptr);
      vkDestroyBuffer(IS.Device, R.Host.Buffer, nullptr);
      vkFreeMemory(IS.Device, R.Host.Memory, nullptr);
    }

    vkDestroyPipeline(IS.Device, IS.Pipeline, nullptr);

    vkDestroyShaderModule(IS.Device, IS.Shader, nullptr);

    vkDestroyPipelineCache(IS.Device, IS.PipelineCache, nullptr);

    vkDestroyPipelineLayout(IS.Device, IS.PipelineLayout, nullptr);

    for (auto &L : IS.DescriptorSetLayouts)
      vkDestroyDescriptorSetLayout(IS.Device, L, nullptr);

    vkDestroyDescriptorPool(IS.Device, IS.Pool, nullptr);

    vkDestroyCommandPool(IS.Device, IS.CmdPool, nullptr);
    vkDestroyDevice(IS.Device, nullptr);
    return llvm::Error::success();
  }

  llvm::Error executeProgram(llvm::StringRef Program, Pipeline &P) override {
    InvocationState State;
    if (auto Err = createDevice(State))
      return Err;
    llvm::outs() << "Physical device created.\n";
    if (auto Err = createCommandBuffer(State))
      return Err;
    llvm::outs() << "Copy command buffer created.\n";
    if (auto Err = createBuffers(P, State))
      return Err;
    llvm::outs() << "Memory buffers created.\n";
    if (auto Err = executeCommandBuffer(State))
      return Err;
    llvm::outs() << "Executed copy command buffer.\n";
    if (auto Err = createCommandBuffer(State))
      return Err;
    llvm::outs() << "Execute command buffer created.\n";
    if (auto Err = createDescriptorPool(P, State))
      return Err;
    llvm::outs() << "Descriptor pool created.\n";
    if (auto Err = createDescriptorSets(P, State))
      return Err;
    llvm::outs() << "Descriptor sets created.\n";
    if (auto Err = createShaderModule(Program, State))
      return Err;
    llvm::outs() << "Shader module created.\n";
    if (auto Err = createPipeline(P, State))
      return Err;
    llvm::outs() << "Compute pipeline created.\n";
    if (auto Err = createComputeCommands(P, State))
      return Err;
    llvm::outs() << "Compute commands created.\n";
    if (auto Err = executeCommandBuffer(State, VK_PIPELINE_STAGE_TRANSFER_BIT))
      return Err;
    llvm::outs() << "Executed compute command buffer.\n";
    if (auto Err = readBackData(P, State))
      return Err;
    llvm::outs() << "Compute pipeline created.\n";

    if (auto Err = cleanup(State))
      return Err;
    llvm::outs() << "Cleanup complete.\n";
    return llvm::Error::success();
  }
};

class VKContext {
private:
  VkInstance Instance;
  llvm::SmallVector<std::shared_ptr<VKDevice>> Devices;

  VKContext() = default;
  ~VKContext() { vkDestroyInstance(Instance, NULL); }
  VKContext(const VKContext &) = delete;

public:
  static VKContext &instance() {
    static VKContext Ctx;
    return Ctx;
  }

  llvm::Error initialize() {
    VkApplicationInfo AppInfo = {};
    AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName = "OffloadTest";
    AppInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CreateInfo.pApplicationInfo = &AppInfo;

// TODO: This is a bit hacky but matches what I did in DX.
#ifndef NDEBUG
    const char *ValidationLayer = "VK_LAYER_KHRONOS_validation";
    CreateInfo.ppEnabledLayerNames = &ValidationLayer;
    CreateInfo.enabledLayerCount = 1;
#endif

    VkResult Res = vkCreateInstance(&CreateInfo, NULL, &Instance);
    if (Res == VK_ERROR_INCOMPATIBLE_DRIVER)
      return llvm::createStringError(std::errc::no_such_device,
                                     "Cannot find a compatible Vulkan device");
    else if (Res)
      return llvm::createStringError(std::errc::no_such_device,
                                     "Unkown Vulkan initialization error");

    uint32_t DeviceCount = 0;
    if (vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr))
      return llvm::createStringError(std::errc::no_such_device,
                                     "Failed to get device count");
    std::vector<VkPhysicalDevice> PhysicalDevices(DeviceCount);
    if (vkEnumeratePhysicalDevices(Instance, &DeviceCount,
                                   PhysicalDevices.data()))
      return llvm::createStringError(std::errc::no_such_device,
                                     "Failed to enumerate devices");
    for (const auto &Dev : PhysicalDevices) {
      auto NewDev = std::make_shared<VKDevice>(Dev);
      Devices.push_back(NewDev);
      Device::registerDevice(std::static_pointer_cast<Device>(NewDev));
    }

    return llvm::Error::success();
  }
};
} // namespace

llvm::Error InitializeVXDevices() { return VKContext::instance().initialize(); }
