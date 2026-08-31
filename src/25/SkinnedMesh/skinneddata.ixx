export module skinnedmeshdemo:skinneddata;
import std;
import shared;
import :animationhelper;

///<summary>
/// An animation clip contains one bone animation for every bone.
///</summary>
struct AnimationClip
{
	auto GetClipStartTime() const -> float
	{
		auto startTime = MathHelper::Infinity;
		for (const auto& boneAnimation : BoneAnimations)
		{
			startTime =
				std::min(startTime, boneAnimation.GetStartTime());
		}
		return startTime;
	}

	auto GetClipEndTime() const -> float
	{
		auto endTime = 0.0f;
		for (const auto& boneAnimation : BoneAnimations)
		{
			endTime = std::max(endTime, boneAnimation.GetEndTime());
		}
		return endTime;
	}

	void Interpolate(
		float timePosition,
		std::vector<DirectX::XMFLOAT4X4>& boneTransforms) const
	{
		boneTransforms.resize(BoneAnimations.size());
		for (auto i = 0u; i < BoneAnimations.size(); ++i)
		{
			BoneAnimations[i].Interpolate(timePosition, boneTransforms[i]);
		}
	}

	std::vector<BoneAnimation> BoneAnimations;
};

class SkinnedData
{
public:
	auto BoneCount() const -> std::uint32_t
	{
		return static_cast<std::uint32_t>(mBoneHierarchy.size());
	}

	auto GetClipStartTime(const std::string& clipName) const -> float
	{
		return mAnimations.at(clipName).GetClipStartTime();
	}

	auto GetClipEndTime(const std::string& clipName) const -> float
	{
		return mAnimations.at(clipName).GetClipEndTime();
	}

	void Set(
		std::vector<int> boneHierarchy,
		std::vector<DirectX::XMFLOAT4X4> boneOffsets,
		std::map<std::string, AnimationClip> animations)
	{
		if (boneHierarchy.size() != boneOffsets.size())
		{
			throw std::invalid_argument(
				"Bone hierarchy and offset counts do not match.");
		}

		for (auto i = 1u; i < boneHierarchy.size(); ++i)
		{
			if (boneHierarchy[i] < 0 ||
				static_cast<std::size_t>(boneHierarchy[i]) >= i)
			{
				throw std::invalid_argument(
					"Bone hierarchy contains an invalid parent index.");
			}
		}

		for (const auto& [clipName, clip] : animations)
		{
			if (clip.BoneAnimations.size() != boneOffsets.size())
			{
				throw std::invalid_argument(
					"Animation clip bone count does not match the skeleton: " +
					clipName);
			}
		}

		mBoneHierarchy = std::move(boneHierarchy);
		mBoneOffsets = std::move(boneOffsets);
		mAnimations = std::move(animations);
	}

	void GetFinalTransforms(
		const std::string& clipName,
		float timePosition,
		std::vector<DirectX::XMFLOAT4X4>& finalTransforms) const
	{
		const auto boneCount = mBoneOffsets.size();
		finalTransforms.resize(boneCount);
		if (boneCount == 0)
		{
			return;
		}

		auto toParentTransforms =
			std::vector<DirectX::XMFLOAT4X4>(boneCount);
		mAnimations.at(clipName).Interpolate(
			timePosition, toParentTransforms);

		auto toRootTransforms =
			std::vector<DirectX::XMFLOAT4X4>(boneCount);
		toRootTransforms[0] = toParentTransforms[0];

		for (auto i = 1u; i < boneCount; ++i)
		{
			const auto toParent =
				DirectX::XMLoadFloat4x4(&toParentTransforms[i]);
			const auto parentIndex =
				static_cast<std::size_t>(mBoneHierarchy.at(i));
			const auto parentToRoot =
				DirectX::XMLoadFloat4x4(
					&toRootTransforms.at(parentIndex));
			const auto toRoot =
				DirectX::XMMatrixMultiply(toParent, parentToRoot);
			DirectX::XMStoreFloat4x4(&toRootTransforms[i], toRoot);
		}

		for (auto i = 0u; i < boneCount; ++i)
		{
			const auto offset =
				DirectX::XMLoadFloat4x4(&mBoneOffsets[i]);
			const auto toRoot =
				DirectX::XMLoadFloat4x4(&toRootTransforms[i]);
			DirectX::XMStoreFloat4x4(
				&finalTransforms[i],
				DirectX::XMMatrixMultiply(offset, toRoot));
		}
	}

private:
	std::vector<int> mBoneHierarchy;
	std::vector<DirectX::XMFLOAT4X4> mBoneOffsets;
	std::map<std::string, AnimationClip> mAnimations;
};
