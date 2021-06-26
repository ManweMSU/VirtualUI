#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Media
	{
		enum MediaEncoderOptions : uint {
			MediaEncoderChannelLayout = 0x0001,
			MediaEncoderSuggestedBytesPerSecond = 0x0002,
			MediaEncoderFramesPerPacket = 0x0003,
			MediaEncoderAACProfile = 0x0004,
			MediaEncoderH264Profile = 0x0005,
			MediaEncoderMaxKeyframePeriod = 0x0006,
		};
		enum AACProfile : uint {
			AACProfileAAC_L2 = 0x29,
			AACProfileAAC_L4 = 0x2A,
			AACProfileAAC_L5 = 0x2B,
			AACProfileV1_HE_AAC_L2 = 0x2C,
			AACProfileV1_HE_AAC_L4 = 0x2E,
			AACProfileV1_HE_AAC_L5 = 0x2F,
			AACProfileV2_HE_AAC_L2 = 0x30,
			AACProfileV2_HE_AAC_L3 = 0x31,
			AACProfileV2_HE_AAC_L4 = 0x32,
			AACProfileV2_HE_AAC_L5 = 0x33,
		};
		enum H264Profile : uint {
			H264ProfileBase = 0x0001,
			H264ProfileMain = 0x0002,
			H264ProfileHigh = 0x0003,
		};
	}
}