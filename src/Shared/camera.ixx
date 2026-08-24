export module shared:camera;
import std;
import :win32;
import :d3dutil;
import :mathhelper;

export class Camera
{
public:
	struct Position
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};
	struct Lens
	{
		float fovY = 0.0f;
		float aspect = 0.0f;
		float zn = 0.0f;
		float zf = 0.0f;
	};
	Camera()
	{
		SetLens(0.25f * MathHelper::Pi, 1.0f, 1.0f, 1000.0f);
	}

	Camera(Position pos)
	{
		SetPosition(pos.x, pos.y, pos.z);
	}

	Camera(Lens lens)
	{
		SetLens(lens.fovY, lens.aspect, lens.zn, lens.zf);
	}

	Camera(Position pos, Lens lens)
	{
		SetPosition(pos.x, pos.y, pos.z);
		SetLens(lens.fovY, lens.aspect, lens.zn, lens.zf);
	}

	// Get/Set world camera position.
	auto GetPositionXM()const -> DirectX::XMVECTOR
	{
		return XMLoadFloat3(&mPosition);
	}

	auto GetPosition()const -> DirectX::XMFLOAT3
	{
		return mPosition;
	}

	void SetPosition(float x, float y, float z)
	{
		mPosition = DirectX::XMFLOAT3(x, y, z);
	}

	void SetPosition(const DirectX::XMFLOAT3& v)
	{
		mPosition = v;
	}

	// Get camera basis vectors.
	auto GetRightXM()const -> DirectX::XMVECTOR
	{
		return DirectX::XMLoadFloat3(&mRight);
	}

	auto GetRight()const -> DirectX::XMFLOAT3
	{
		return mRight;
	}

	auto GetUpXM()const -> DirectX::XMVECTOR
	{
		return XMLoadFloat3(&mUp);
	}

	auto GetUp()const -> DirectX::XMFLOAT3
	{
		return mUp;
	}

	auto GetLookXM()const -> DirectX::XMVECTOR
	{
		return XMLoadFloat3(&mLook);
	}

	auto GetLook()const -> DirectX::XMFLOAT3
	{
		return mLook;
	}

	// Get frustum properties.
	auto GetNearZ()const -> float
	{
		return mNearZ;
	}

	auto GetFarZ()const -> float
	{
		return mFarZ;
	}

	auto GetAspect()const -> float
	{
		return mAspect;
	}

	auto GetFovY()const -> float
	{
		return mFovY;
	}

	auto GetFovX()const -> float
	{
		float halfWidth = 0.5f * GetNearWindowWidth();
		return 2.0f * atan(halfWidth / mNearZ);
	}

	// Get near and far plane dimensions in view space coordinates.
	auto GetNearWindowWidth()const -> float
	{
		return mAspect * mNearWindowHeight;
	}

	auto GetNearWindowHeight()const -> float
	{
		return mNearWindowHeight;
	}

	auto GetFarWindowWidth()const -> float
	{
		return mAspect * mFarWindowHeight;
	}

	auto GetFarWindowHeight()const -> float
	{
		return mFarWindowHeight;
	}

	// Set frustum.
	void SetLens(float fovY, float aspect, float zn, float zf)
	{
		// cache properties
		mFovY = fovY;
		mAspect = aspect;
		mNearZ = zn;
		mFarZ = zf;

		mNearWindowHeight = 2.0f * mNearZ * std::tanf(0.5f * mFovY);
		mFarWindowHeight = 2.0f * mFarZ * std::tanf(0.5f * mFovY);

		auto P = DirectX::XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
		DirectX::XMStoreFloat4x4(&mProj, P);
	}

	// Define camera space via LookAt parameters.
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp)
	{
		auto L = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, pos));
		auto R = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(worldUp, L));
		auto U = DirectX::XMVector3Cross(L, R);

		DirectX::XMStoreFloat3(&mPosition, pos);
		DirectX::XMStoreFloat3(&mLook, L);
		DirectX::XMStoreFloat3(&mRight, R);
		DirectX::XMStoreFloat3(&mUp, U);
	}

	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up)
	{
		auto P = DirectX::XMLoadFloat3(&pos);
		auto T = DirectX::XMLoadFloat3(&target);
		auto U = DirectX::XMLoadFloat3(&up);

		LookAt(P, T, U);
	}

	// Get View/Proj matrices.
	auto View()const -> DirectX::XMMATRIX
	{
		return DirectX::XMLoadFloat4x4(&mView);
	}
	auto Proj()const -> DirectX::XMMATRIX
	{
		return DirectX::XMLoadFloat4x4(&mProj);
	}
	auto ViewProj()const -> DirectX::XMMATRIX
	{
		return DirectX::XMMatrixMultiply(View(), Proj());
	}

	// Strafe/Walk the camera a distance d.
	void Strafe(float d)
	{
		// mPosition += d*mRight
		auto s = DirectX::XMVectorReplicate(d);
		auto r = DirectX::XMLoadFloat3(&mRight);
		auto p = DirectX::XMLoadFloat3(&mPosition);
		DirectX::XMStoreFloat3(&mPosition, DirectX::XMVectorMultiplyAdd(s, r, p));
	}

	void Walk(float d)
	{
		// mPosition += d*mLook
		auto s = DirectX::XMVectorReplicate(d);
		auto l = DirectX::XMLoadFloat3(&mLook);
		auto p = DirectX::XMLoadFloat3(&mPosition);
		DirectX::XMStoreFloat3(&mPosition, DirectX::XMVectorMultiplyAdd(s, l, p));
	}

	// Rotate the camera.
	void Pitch(float angle)
	{
		// Rotate up and look vector about the right vector.
		auto R = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&mRight), angle);

		DirectX::XMStoreFloat3(&mUp, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mUp), R));
		DirectX::XMStoreFloat3(&mLook, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mLook), R));
	}

	void RotateY(float angle)
	{
		// Rotate the basis vectors about the world y-axis.
		auto R = DirectX::XMMatrixRotationY(angle);

		DirectX::XMStoreFloat3(&mRight, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mRight), R));
		DirectX::XMStoreFloat3(&mUp, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mUp), R));
		DirectX::XMStoreFloat3(&mLook, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mLook), R));
	}


	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix()
	{
		auto R = DirectX::XMLoadFloat3(&mRight);
		auto U = DirectX::XMLoadFloat3(&mUp);
		auto L = DirectX::XMLoadFloat3(&mLook);
		auto P = DirectX::XMLoadFloat3(&mPosition);

		// Keep camera's axes orthogonal to each other and of unit length.
		L = DirectX::XMVector3Normalize(L);
		U = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(L, R));

		// U, L already ortho-normal, so no need to normalize cross product.
		R = DirectX::XMVector3Cross(U, L);

		// Fill in the view matrix entries.
		auto x = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, R));
		auto y = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, U));
		auto z = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, L));

		DirectX::XMStoreFloat3(&mRight, R);
		DirectX::XMStoreFloat3(&mUp, U);
		DirectX::XMStoreFloat3(&mLook, L);

		mView(0, 0) = mRight.x;
		mView(1, 0) = mRight.y;
		mView(2, 0) = mRight.z;
		mView(3, 0) = x;

		mView(0, 1) = mUp.x;
		mView(1, 1) = mUp.y;
		mView(2, 1) = mUp.z;
		mView(3, 1) = y;

		mView(0, 2) = mLook.x;
		mView(1, 2) = mLook.y;
		mView(2, 2) = mLook.z;
		mView(3, 2) = z;

		mView(0, 3) = 0.0f;
		mView(1, 3) = 0.0f;
		mView(2, 3) = 0.0f;
		mView(3, 3) = 1.0f;
	}

private:
	// Camera coordinate system with coordinates relative to world space.
	DirectX::XMFLOAT3 mPosition = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 mRight = {1.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 mUp = {0.0f, 1.0f, 0.0f};
	DirectX::XMFLOAT3 mLook = {0.0f, 0.0f, 1.0f};

	// Cache frustum properties.
	float mNearZ;
	float mFarZ;
	float mAspect;
	float mFovY;
	float mNearWindowHeight;
	float mFarWindowHeight;

	// Cache View/Proj matrices.
	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;
};