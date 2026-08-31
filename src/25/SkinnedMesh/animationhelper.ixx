export module skinnedmeshdemo:animationhelper;
import std;
import shared;

///<summary>
/// A Keyframe defines the bone transformation at an instant in time.
///</summary>
struct Keyframe
{
	float TimePos = 0.0f;
	DirectX::XMFLOAT3 Translation = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 Scale = {1.0f, 1.0f, 1.0f};
	DirectX::XMFLOAT4 RotationQuat = {0.0f, 0.0f, 0.0f, 1.0f};
};

///<summary>
/// A BoneAnimation is defined by a list of keyframes.  For time
/// values inbetween two keyframes, we interpolate between the
/// two nearest keyframes that bound the time.  
///
/// We assume an animation always has two keyframes.
///</summary>
struct BoneAnimation
{
	auto GetStartTime()const -> float
	{
		// Keyframes are sorted by time, so first keyframe gives start time.
		return Keyframes.front().TimePos;
	}

	auto GetEndTime()const -> float
	{
		// Keyframes are sorted by time, so last keyframe gives end time.
		return Keyframes.back().TimePos;
	}

	void Interpolate(float t, DirectX::XMFLOAT4X4& M)const
	{
		if (t <= Keyframes.front().TimePos)
		{
			DirectX::XMVECTOR S = DirectX::XMLoadFloat3(&Keyframes.front().Scale);
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&Keyframes.front().Translation);
			DirectX::XMVECTOR Q = DirectX::XMLoadFloat4(&Keyframes.front().RotationQuat);

			DirectX::XMVECTOR zero = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			DirectX::XMStoreFloat4x4(&M, DirectX::XMMatrixAffineTransformation(S, zero, Q, P));
		}
		else if (t >= Keyframes.back().TimePos)
		{
			DirectX::XMVECTOR S = DirectX::XMLoadFloat3(&Keyframes.back().Scale);
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&Keyframes.back().Translation);
			DirectX::XMVECTOR Q = DirectX::XMLoadFloat4(&Keyframes.back().RotationQuat);

			DirectX::XMVECTOR zero = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			DirectX::XMStoreFloat4x4(&M, DirectX::XMMatrixAffineTransformation(S, zero, Q, P));
		}
		else
		{
			for (auto i = 0u; i < Keyframes.size() - 1; ++i)
			{
				if (t >= Keyframes[i].TimePos && t <= Keyframes[i + 1].TimePos)
				{
					float lerpPercent = (t - Keyframes[i].TimePos) / (Keyframes[i + 1].TimePos - Keyframes[i].TimePos);

					DirectX::XMVECTOR s0 = DirectX::XMLoadFloat3(&Keyframes[i].Scale);
					DirectX::XMVECTOR s1 = DirectX::XMLoadFloat3(&Keyframes[i + 1].Scale);

					DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&Keyframes[i].Translation);
					DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&Keyframes[i + 1].Translation);

					DirectX::XMVECTOR q0 = DirectX::XMLoadFloat4(&Keyframes[i].RotationQuat);
					DirectX::XMVECTOR q1 = DirectX::XMLoadFloat4(&Keyframes[i + 1].RotationQuat);

					DirectX::XMVECTOR S = DirectX::XMVectorLerp(s0, s1, lerpPercent);
					DirectX::XMVECTOR P = DirectX::XMVectorLerp(p0, p1, lerpPercent);
					DirectX::XMVECTOR Q = DirectX::XMQuaternionSlerp(q0, q1, lerpPercent);

					DirectX::XMVECTOR zero = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
					DirectX::XMStoreFloat4x4(&M, DirectX::XMMatrixAffineTransformation(S, zero, Q, P));

					break;
				}
			}
		}
	}

	std::vector<Keyframe> Keyframes;
};
