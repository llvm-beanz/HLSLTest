//===- VX/Device.cpp - HLSL API Vulkan Device API -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "HLSLTest/API/Device.h"
#include "HLSLTest/API/Pipeline.h"
#include "llvm/Support/Error.h"

#include <memory>
#include <vulkan/vulkan.h>

using namespace hlsltest;

namespace {

class VKDevice : public hlsltest::Device {
private:
  VkPhysicalDevice Device;
  VkPhysicalDeviceProperties Props;
  Capabilities Caps;

  struct BufferRef {
    VkBuffer Buffer;
    VkDeviceMemory Memory;
  };

  struct UAVRef {
    BufferRef Host;
    BufferRef Device;
  };

  struct InvocationState {
    VkDevice Device;
    VkQueue Queue;
    VkCommandPool CmdPool;
    VkCommandBuffer CmdBuffer;

    llvm::SmallVector<VkDescriptorPool> DescriptorPools;
    llvm::SmallVector<VkDescriptorSetLayout> DescriptorSetLayouts;
    llvm::SmallVector<UAVRef> UAVs;
  };

public:
  VKDevice(VkPhysicalDevice D) : Device(D) {
    vkGetPhysicalDeviceProperties(Device, &Props);
    Description =
        llvm::StringRef(Props.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
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

  void queryCapabilities() {
    VkPhysicalDeviceFeatures Features;
    vkGetPhysicalDeviceFeatures(Device, &Features);

#define VULKAN_FEATURE_BOOL(Name)                                              \
  Caps.insert(                                                                 \
      std::make_pair(#Name, make_capability<bool>(#Name, Features.Name)));
#include "VKFeatures.def"
  }

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
    QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
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
                                         VkMemoryPropertyFlags Flags,
                                         size_t Size, void *Data = nullptr) {
    VkBuffer Buffer;
    VkDeviceMemory Memory;
    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.flags = Flags;
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
          ((MemProperties.memoryTypes[MemIdx].propertyFlags & Flags) ==
           Flags)) {
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
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, R.Size);
    if (!ExDeviceBuf)
      return ExDeviceBuf.takeError();

    VkBufferCopy Copy = {};
    Copy.size = R.Size;
    vkCmdCopyBuffer(IS.CmdBuffer, ExHostBuf->Buffer, ExDeviceBuf->Buffer, 1,
                    &Copy);

    IS.UAVs.push_back(UAVRef{*ExHostBuf, *ExDeviceBuf});

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

  llvm::Error executeCommandBuffer(InvocationState &IS) {
    if (vkEndCommandBuffer(IS.CmdBuffer))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Could not end command buffer.");

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &IS.CmdBuffer;
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

  llvm::Error createPipeline(Pipeline &P, InvocationState &IS) {
    for (const auto &S : P.Sets) {
      VkDescriptorPoolSize PoolSize = {};
      PoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      PoolSize.descriptorCount = S.Resources.size();
      VkDescriptorPoolCreateInfo PoolCreateInfo;
      PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      PoolCreateInfo.poolSizeCount = 1;
      PoolCreateInfo.pPoolSizes = &PoolSize;
      VkDescriptorPool Pool;
      if (vkCreateDescriptorPool(IS.Device, &PoolCreateInfo, nullptr, &Pool))
        return llvm::createStringError(std::errc::device_or_resource_busy,
                                       "Failed to create descriptor pool.");
      IS.DescriptorPools.push_back(Pool);

      std::vector<VkDescriptorSetLayoutBinding> Bindings;
      uint32_t BindingIdx = 0;
      for (const auto &R : S.Resources) {
        (void)R; // Todo: set this correctly for the data type.
        VkDescriptorSetLayoutBinding Binding = {};
        Binding.binding = BindingIdx++;
        Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        Binding.descriptorCount = 1;
        Binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        Bindings.push_back(Binding);
      }
      VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = {};
      LayoutCreateInfo.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      LayoutCreateInfo.bindingCount = Bindings.size();
      LayoutCreateInfo.pBindings = Bindings.data();
      VkDescriptorSetLayout Layout;
      if (vkCreateDescriptorSetLayout(IS.Device, &LayoutCreateInfo, nullptr,
                                      &Layout))
        return llvm::createStringError(
            std::errc::device_or_resource_busy,
            "Failed to create descriptor set layout.");
      IS.DescriptorSetLayouts.push_back(Layout);
    }
    return llvm::Error::success();
  }

  llvm::Error cleanup(InvocationState &IS) {
    for (auto &R : IS.UAVs) {
      vkDestroyBuffer(IS.Device, R.Device.Buffer, nullptr);
      vkFreeMemory(IS.Device, R.Device.Memory, nullptr);
      vkDestroyBuffer(IS.Device, R.Host.Buffer, nullptr);
      vkFreeMemory(IS.Device, R.Host.Memory, nullptr);
    }

    for (auto &L : IS.DescriptorSetLayouts)
      vkDestroyDescriptorSetLayout(IS.Device, L, nullptr);

    for (auto &P : IS.DescriptorPools)
      vkDestroyDescriptorPool(IS.Device, P, nullptr);

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
    if (auto Err = createPipeline(P, State))
      return Err;
    llvm::outs() << "Compute pipeline created.\n";

    if (auto Err = cleanup(State))
      return Err;
    llvm::outs() << "Cleanup complete.\n";
    return llvm::createStringError(std::errc::not_supported,
                                   "VKDevice::executeProgram not supported.");
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
    AppInfo.pApplicationName = "HLSLTest";
    AppInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CreateInfo.pApplicationInfo = &AppInfo;

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
