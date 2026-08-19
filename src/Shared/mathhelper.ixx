export module shared:mathhelper;
import std;
import :win32;

export using bool32 = std::uint32_t;

export class MathHelper
{
public:
	// Returns random float in [0, 1).
	static auto RandF() -> float
	{
		auto rd = std::random_device{};
		auto gen = std::mt19937{rd()};
		auto dis = std::uniform_real_distribution<float>{0.0f, 1.0f};
		return dis(gen);
	}

	// Returns random float in [a, b).
	static auto RandF(float a, float b) -> float
	{
		auto rd = std::random_device{};
		auto gen = std::mt19937{ rd() };
		auto dis = std::uniform_real_distribution<float>{ a, b };
		return dis(gen);
	}

	template<typename T>
	static auto Lerp(const T& a, const T& b, float t) -> T
	{
		return a + (b - a) * t;
	}

	// Returns the polar angle of the point (x,y) in [0, 2*PI).
	static auto AngleFromXY(float x, float y) -> float
	{
		float theta = 0.0f;

		// Quadrant I or IV
		if (x >= 0.0f)
		{
			// If x = 0, then atanf(y/x) = +pi/2 if y > 0
			//                atanf(y/x) = -pi/2 if y < 0
			theta = std::atanf(y / x); // in [-pi/2, +pi/2]

			if (theta < 0.0f)
				theta += 2.0f * Pi; // in [0, 2*pi).
		}

		// Quadrant II or III
		else
			theta = std::atanf(y / x) + Pi; // in [0, 2*pi).

		return theta;
	}

	static auto InverseTranspose(DirectX::CXMMATRIX M) -> DirectX::XMMATRIX
	{
		// Inverse-transpose is just applied to normals.  So zero out 
		// translation row so that it doesn't get into our inverse-transpose
		// calculation--we don't want the inverse-transpose of the translation.
		auto A = DirectX::XMMATRIX{ M };
		A.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(A);
		return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, A));
	}

	static auto RandUnitVec3() -> DirectX::XMVECTOR
	{
		auto One = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
		auto Zero = DirectX::XMVectorZero();

		// Keep trying until we get a point on/in the hemisphere.
		while (true)
		{
			// Generate random point in the cube [-1,1]^3.
			DirectX::XMVECTOR v = DirectX::XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

			// Ignore points outside the unit sphere in order to get an even distribution 
			// over the unit sphere.  Otherwise points will clump more on the sphere near 
			// the corners of the cube.

			if (DirectX::XMVector3Greater(DirectX::XMVector3LengthSq(v), One))
				continue;

			return DirectX::XMVector3Normalize(v);
		}
	}
	static auto RandHemisphereUnitVec3(DirectX::XMVECTOR n) -> DirectX::XMVECTOR
	{
		auto One = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
		auto Zero = DirectX::XMVectorZero();

		// Keep trying until we get a point on/in the hemisphere.
		while (true)
		{
			// Generate random point in the cube [-1,1]^3.
			DirectX::XMVECTOR v = DirectX::XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

			// Ignore points outside the unit sphere in order to get an even distribution 
			// over the unit sphere.  Otherwise points will clump more on the sphere near 
			// the corners of the cube.

			if (DirectX::XMVector3Greater(DirectX::XMVector3LengthSq(v), One))
				continue;

			// Ignore points in the bottom hemisphere.
			if (DirectX::XMVector3Less(DirectX::XMVector3Dot(n, v), Zero))
				continue;

			return DirectX::XMVector3Normalize(v);
		}
	}

	static inline constexpr auto Infinity = std::numeric_limits<float>::infinity();
	static inline constexpr auto Pi = 3.1415926535f;
};