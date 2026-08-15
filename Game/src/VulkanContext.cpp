#include "VulkanContext.h"

#include "Utils/FileUtils.h"
#include "Utils/Log.h"
#include "Config.h"
#include "vulkan/vulkan_core.h"

#include "Platform.h"
#include <array>

constexpr static VkFormat ImageFormat = VK_FORMAT_B8G8R8A8_SRGB;

constexpr static std::array ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

constexpr static std::array RequiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static bool VulkanContext_CreateInstance(VulkanContext& context);
static bool VulkanContext_CreateDevice(VulkanContext& context);
static bool VulkanContext_CreateSwapchain(VulkanContext& context, Platform& platform);
static bool VulkanContext_CreateCommandBuffers(VulkanContext& context);
static bool VulkanContext_CreateShadersAndGraphicsPipeline(VulkanContext& context);

static int VulkanContext_GetDeviceScore(VkPhysicalDevice device);

bool VulkanContext::Init(Platform& platform) {
	if (!VulkanContext_CreateInstance(*this)) {
		LOG_ERROR("Failed to create Vulkan instance.");
		return false;
	}

	if constexpr (Config::EnableValidationLayers) {
		constexpr auto debugCreateInfo = VkUtils::CreateDebugMessengerCreateInfo();
		if (vkCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			LOG_ERROR("Failed to create debug utils messenger");
			return false;
		}
	}

	if (!VulkanContext_CreateDevice(*this)) {
		LOG_ERROR("Failed to create Vulkan device.");
		return false;
	}

	if (!VulkanContext_CreateSwapchain(*this, platform)) {
		LOG_ERROR("Failed to create Vulkan swapchain.");
		return false;
	}

	if (!VulkanContext_CreateCommandBuffers(*this)) {
		LOG_ERROR("Failed to create Vulkan command buffers.");
		return false;
	}

	if (!VulkanContext_CreateShadersAndGraphicsPipeline(*this)) {
		LOG_ERROR("Failed to create Vulkan shaders and graphics pipeline.");
		return false;
	}

	return true;
}

void VulkanContext::Shutdown() {
	if (device != VK_NULL_HANDLE) {
		if (vkDeviceWaitIdle(device) != VK_SUCCESS) {
			LOG_ERROR("Failed to wait for Vulkan device to become idle.");
		}
	}

	for (uint32_t i = 0; i < MaxFramesInFlight; ++i) {
		commandBuffers[i] = VK_NULL_HANDLE;
	}

	if (depthImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(device, depthImageView, nullptr);
		depthImageView = VK_NULL_HANDLE;
	}

	if (depthImage != VK_NULL_HANDLE) {
		vmaDestroyImage(vmaAllocator, depthImage, depthImageAllocation);
		depthImage = VK_NULL_HANDLE;
		depthImageAllocation = VK_NULL_HANDLE;
	}

	for (VkImageView imageView : swapchainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}
	swapchainImageViews.clear();
	swapchainImages.clear();

	if (graphicsPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		graphicsPipeline = VK_NULL_HANDLE;
	}

	if (pipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		pipelineLayout = VK_NULL_HANDLE;
	}

	if (swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		swapchain = VK_NULL_HANDLE;
	}

	if (surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(instance, surface, nullptr);
		surface = VK_NULL_HANDLE;
	}

	if (commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(device, commandPool, nullptr);
		commandPool = VK_NULL_HANDLE;
	}

	if (vmaAllocator != VK_NULL_HANDLE) {
		vmaDestroyAllocator(vmaAllocator);
		vmaAllocator = VK_NULL_HANDLE;
	}

	if (device != VK_NULL_HANDLE) {
		vkDestroyDevice(device, nullptr);
		device = VK_NULL_HANDLE;
	}

	if constexpr (Config::EnableValidationLayers) {
		if (debugMessenger != VK_NULL_HANDLE) {
			vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
			debugMessenger = VK_NULL_HANDLE;
		}
	}

	if (instance != VK_NULL_HANDLE) {
		vkDestroyInstance(instance, nullptr);
		instance = VK_NULL_HANDLE;
	}

	graphicsQueue = VK_NULL_HANDLE;
	physicalDevice = VK_NULL_HANDLE;
	swapchainExtent = {};
	graphicsQueueFamilyIndex = VkUtils::InvalidQueueFamilyIndex;
	depthFormat = VK_FORMAT_UNDEFINED;
}

static bool VulkanContext_CreateInstance(VulkanContext& context) {
	if (volkInitialize() != VK_SUCCESS) {
		LOG_ERROR("Failed to initialize volk.");
		return false;
	}

	constexpr VkApplicationInfo appInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Khronos Vulkan Tutorial",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VulkanContext::VkApiVersion,
	};

	const auto requiredExtensions = VkUtils::GetRequiredVulkanExtensions();

	VkInstanceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = (uint32_t)std::size(requiredExtensions),
		.ppEnabledExtensionNames = std::data(requiredExtensions),
	};

	constexpr auto debugMessengerCreateInfo = VkUtils::CreateDebugMessengerCreateInfo();
	if constexpr (Config::EnableValidationLayers) {
		if (!VkUtils::CheckInstanceLayerSupport(ValidationLayers)) {
			LOG_ERROR("Validation layers requested, but not available.");
			return false;
		}

		createInfo.pNext = &debugMessengerCreateInfo;
		createInfo.enabledLayerCount = (uint32_t)std::size(ValidationLayers);
		createInfo.ppEnabledLayerNames = std::data(ValidationLayers);
	}

	const VkResult result = vkCreateInstance(&createInfo, nullptr, &context.instance);
	if (result != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan instance: {}", (uint32_t)result);
		return false;
	}

	volkLoadInstance(context.instance);

	return true;
}

static bool VulkanContext_CreateDevice(VulkanContext& context) {
	const auto physicalDevices = VkUtils::GetPhysicalDevices(context.instance);
	if (std::empty(physicalDevices)) {
		LOG_ERROR("Failed to find any Vulkan physical devices.");
		return false;
	}

	int bestScore = 0;
	VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
	for (VkPhysicalDevice device : physicalDevices) {
		const int score = VulkanContext_GetDeviceScore(device);
		if (score > bestScore) {
			bestScore = score;
			bestDevice = device;
		}
	}

	if (bestDevice == VK_NULL_HANDLE) {
		LOG_ERROR("Failed to find a suitable Vulkan physical device.");
		return false;
	}

	uint32_t queueFamilyIndex = VkUtils::FindGraphicsPresentQueueFamilyIndex(context.instance, bestDevice);
	if (queueFamilyIndex == VkUtils::InvalidQueueFamilyIndex) {
		LOG_ERROR("Failed to find a suitable Vulkan queue family index.");
		return false;
	}

	constexpr float queuePriority = 1.0f;
	const VkDeviceQueueCreateInfo queueCreateInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = queueFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,
	};

	VkPhysicalDeviceVulkan11Features features11 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.shaderDrawParameters = VK_TRUE,
	};

	VkPhysicalDeviceVulkan12Features features12 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &features11,
		.descriptorIndexing = VK_TRUE,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE,
	};

	VkPhysicalDeviceVulkan13Features features13 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &features12,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE,
	};

	constexpr VkPhysicalDeviceFeatures enabledFeatures {
		.samplerAnisotropy = VK_TRUE,
	};

	const VkDeviceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &features13,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = (uint32_t)std::size(RequiredDeviceExtensions),
		.ppEnabledExtensionNames = std::data(RequiredDeviceExtensions),
		.pEnabledFeatures = &enabledFeatures,
	};

	if (vkCreateDevice(bestDevice, &createInfo, nullptr, &context.device) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan device.");
		return false;
	}

	vkGetDeviceQueue(context.device, queueFamilyIndex, 0, &context.graphicsQueue);

	volkLoadDevice(context.device);

	context.physicalDevice = bestDevice;
	context.graphicsQueueFamilyIndex = queueFamilyIndex;

	VmaVulkanFunctions vmaVulkanFunctions {};
	const VmaAllocatorCreateInfo vmaAllocatorCreateInfo {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = context.physicalDevice,
		.device = context.device,
		.pVulkanFunctions = &vmaVulkanFunctions,
		.instance = context.instance,
		.vulkanApiVersion = VulkanContext::VkApiVersion,
	};

	vmaImportVulkanFunctionsFromVolk(&vmaAllocatorCreateInfo, &vmaVulkanFunctions);

	if (vmaCreateAllocator(&vmaAllocatorCreateInfo, &context.vmaAllocator) != VK_SUCCESS) {
		LOG_ERROR("Failed to create VMA allocator.");
		return false;
	}

	return true;
}

static bool VulkanContext_CreateSwapchain(VulkanContext& context, Platform& platform) {
	context.surface = platform.window.CreateVulkanSurface(context.instance);
	if (context.surface == VK_NULL_HANDLE) {
		LOG_ERROR("Failed to create Vulkan surface.");
		return false;
	}

	VkSurfaceCapabilitiesKHR surfaceCapabilities {};
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface, &surfaceCapabilities) != VK_SUCCESS) {
		LOG_ERROR("Failed to get Vulkan surface capabilities.");
		return false;
	}

	context.swapchainExtent = surfaceCapabilities.currentExtent;
	if (context.swapchainExtent.width == 0xFFFFFFFF) {
		context.swapchainExtent = {
			.width = std::clamp(platform.window.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
			.height = std::clamp(platform.window.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height),
		};
	}

	constexpr VkFormat desiredFormat = ImageFormat;
	const VkSwapchainCreateInfoKHR swapchainCreateInfo {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = context.surface,
		.minImageCount = surfaceCapabilities.minImageCount + 1,
		.imageFormat = desiredFormat,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = context.swapchainExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
	};

	if (vkCreateSwapchainKHR(context.device, &swapchainCreateInfo, nullptr, &context.swapchain) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan swapchain.");
		return false;
	}

	uint32_t imageCount = 0;
	if (vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageCount, nullptr) != VK_SUCCESS || imageCount == 0) {
		LOG_ERROR("Failed to get Vulkan swapchain image count.");
		return false;
	}

	context.swapchainImages.resize(imageCount);
	if (vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageCount, std::data(context.swapchainImages)) != VK_SUCCESS) {
		LOG_ERROR("Failed to get Vulkan swapchain images.");
		return false;
	}

	context.swapchainImageViews.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i) {
		const VkImageViewCreateInfo imageViewCreateInfo {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = context.swapchainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = desiredFormat,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		};

		if (vkCreateImageView(context.device, &imageViewCreateInfo, nullptr, &context.swapchainImageViews[i]) != VK_SUCCESS) {
			LOG_ERROR("Failed to create Vulkan swapchain image view.");
			return false;
		}
	}

	constexpr std::array<VkFormat, 2> depthFormats = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
	};

	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	for (VkFormat format : depthFormats) {
		VkFormatProperties2 props {
			.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
		};
		vkGetPhysicalDeviceFormatProperties2(context.physicalDevice, format, &props);
		if (props.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			depthFormat = format;
			break;
		}
	}

	if (depthFormat == VK_FORMAT_UNDEFINED) {
		LOG_ERROR("Failed to find a suitable Vulkan depth format.");
		return false;
	}

	const VkImageCreateInfo depthImageCreateInfo {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = depthFormat,
		.extent = {
			.width = context.swapchainExtent.width,
			.height = context.swapchainExtent.height,
			.depth = 1,
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	constexpr VmaAllocationCreateInfo depthImageAllocationCreateInfo {
		.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO,
	};

	if (vmaCreateImage(context.vmaAllocator, &depthImageCreateInfo, &depthImageAllocationCreateInfo, &context.depthImage, &context.depthImageAllocation, nullptr) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan depth image.");
		return false;
	}

	const VkImageViewCreateInfo depthImageViewCreateInfo {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = context.depthImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = depthFormat,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	if (vkCreateImageView(context.device, &depthImageViewCreateInfo, nullptr, &context.depthImageView) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan depth image view.");
		return false;
	}

	context.depthFormat = depthFormat;

	return true;
}

static bool VulkanContext_CreateCommandBuffers(VulkanContext& context) {
	const VkCommandPoolCreateInfo commandPoolCreateInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = context.graphicsQueueFamilyIndex,
	};

	if (vkCreateCommandPool(context.device, &commandPoolCreateInfo, nullptr, &context.commandPool) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan command pool.");
		return false;
	}

	const VkCommandBufferAllocateInfo commandBufferAllocateInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = context.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = VulkanContext::MaxFramesInFlight,
	};

	if (vkAllocateCommandBuffers(context.device, &commandBufferAllocateInfo, std::data(context.commandBuffers)) != VK_SUCCESS) {
		LOG_ERROR("Failed to allocate Vulkan command buffers.");
		return false;
	}

	return true;
}

static bool VulkanContext_CreateShadersAndGraphicsPipeline(VulkanContext& context) {
	const auto shaderCode = FileUtils::ReadBytes("res/shaders/shader.spv");
	if (std::empty(shaderCode)) {
		LOG_ERROR("Failed to read shader code from file: res/shaders/shader.spv");
		return false;
	}

	const VkShaderModuleCreateInfo shaderModuleCreateInfo {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = std::size(shaderCode),
		.pCode = reinterpret_cast<const uint32_t*>(std::data(shaderCode)),
	};

	VkShaderModule shaderModule {};
	if (vkCreateShaderModule(context.device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan shader module.");
		return false;
	}

	const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	};

	if (vkCreatePipelineLayout(context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelineLayout) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan pipeline layout.");
		vkDestroyShaderModule(context.device, shaderModule, nullptr);
		return false;
	}

	const VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	};

	constexpr VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	};

	const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
		VkPipelineShaderStageCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = shaderModule,
			.pName = "vertMain",
		},
		VkPipelineShaderStageCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = shaderModule,
			.pName = "fragMain",
		},
	};

	constexpr VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};

	constexpr std::array<VkDynamicState, 2> dynamicStates {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = (uint32_t)std::size(dynamicStates),
		.pDynamicStates = std::data(dynamicStates),
	};

	constexpr VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
	};

	const VkPipelineRenderingCreateInfo renderingCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &ImageFormat,
		.depthAttachmentFormat = context.depthFormat,
	};

	constexpr VkPipelineColorBlendAttachmentState colorBlendAttachmentState {
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	const VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
	};

	constexpr VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.lineWidth = 1.0f,
	};

	constexpr VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};

	const VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &renderingCreateInfo,
		.stageCount = (uint32_t)std::size(shaderStages),
		.pStages = std::data(shaderStages),
		.pVertexInputState = &vertexInputStateCreateInfo,
		.pInputAssemblyState = &inputAssemblyStateCreateInfo,
		.pViewportState = &viewportStateCreateInfo,
		.pRasterizationState = &rasterizationStateCreateInfo,
		.pMultisampleState = &multisampleStateCreateInfo,
		.pDepthStencilState = &depthStencilStateCreateInfo,
		.pColorBlendState = &colorBlendStateCreateInfo,
		.pDynamicState = &dynamicStateCreateInfo,
		.layout = context.pipelineLayout,
	};

	if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.graphicsPipeline) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan graphics pipeline.");
		vkDestroyShaderModule(context.device, shaderModule, nullptr);
		return false;
	}

	vkDestroyShaderModule(context.device, shaderModule, nullptr);

	return true;
}

static int VulkanContext_GetDeviceScore(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	if (deviceProperties.apiVersion < VulkanContext::VkApiVersion) {
		LOG_WARN("Vulkan physical device does not support required API version: {}", deviceProperties.deviceName);
		return -1;
	}

	int score = 0;
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	} else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
		score += 500;
	} else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
		score += 100;
	} else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
		score += 50;
	} else {
		LOG_WARN("Vulkan physical device is an unknown type: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!VkUtils::CheckDeviceExtensionSupport(device, RequiredDeviceExtensions)) {
		LOG_WARN("Vulkan physical device does not support required extensions: {}", deviceProperties.deviceName);
		return -1;
	}

	VkPhysicalDeviceVulkan11Features features11 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
	};

	VkPhysicalDeviceVulkan12Features features12 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &features11,
	};

	VkPhysicalDeviceVulkan13Features features13 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &features12,
	};

	VkPhysicalDeviceFeatures2 features2 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &features13,
	};
	vkGetPhysicalDeviceFeatures2(device, &features2);

	if (!features2.features.samplerAnisotropy) {
		LOG_WARN("Vulkan physical device does not support sampler anisotropy: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features11.shaderDrawParameters) {
		LOG_WARN("Vulkan physical device does not support shader draw parameters: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features12.bufferDeviceAddress) {
		LOG_WARN("Vulkan physical device does not support buffer device address: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features12.descriptorIndexing) {
		LOG_WARN("Vulkan physical device does not support descriptor indexing: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features12.shaderSampledImageArrayNonUniformIndexing) {
		LOG_WARN("Vulkan physical device does not support shader sampled image array non uniform indexing: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features12.descriptorBindingVariableDescriptorCount) {
		LOG_WARN("Vulkan physical device does not support descriptor binding variable descriptor count: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features12.runtimeDescriptorArray) {
		LOG_WARN("Vulkan physical device does not support runtime descriptor array: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features13.synchronization2) {
		LOG_WARN("Vulkan physical device does not support synchronization2: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features13.dynamicRendering) {
		LOG_WARN("Vulkan physical device does not support dynamic rendering: {}", deviceProperties.deviceName);
		return -1;
	}

	return score;
}
