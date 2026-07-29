export module lighting:waves;
import std;
import shared;

class Waves
{
public:
	auto RowCount()const -> std::uint32_t
	{
		return mNumRows;
	}

	auto ColumnCount()const -> std::uint32_t
	{
		return mNumCols;
	}

	auto VertexCount()const -> std::uint32_t
	{
		return mVertexCount;
	}

	auto TriangleCount()const -> std::uint32_t
	{
		return mTriangleCount;
	}

	auto Width()const -> float
	{
		return mNumCols * mSpatialStep;
	}

	auto Depth()const -> float
	{
		return mNumRows * mSpatialStep;
	}

	// Returns the solution at the ith grid point.
	auto operator[](int i)const -> const DirectX::XMFLOAT3& 
	{ 
		return mCurrSolution[i]; 
	}

	// Returns the solution normal at the ith grid point.
	auto Normal(int i)const -> const DirectX::XMFLOAT3& 
	{ 
		return mNormals[i]; 
	}

	// Returns the unit tangent vector at the ith grid point in the local x-axis direction.
	auto TangentX(int i)const -> const DirectX::XMFLOAT3& 
	{ 
		return mTangentX[i]; 
	}

	void Init(std::uint32_t m, std::uint32_t n, float dx, float dt, float speed, float damping)
	{
		// In case Init() called again.
		if (m != mNumRows or n != mNumCols or not mCurrSolution or not mPrevSolution or not mNormals or not mTangentX)
		{
			mNumRows = m;
			mNumCols = n;
			mPrevSolution = std::unique_ptr<DirectX::XMFLOAT3[]>{ new DirectX::XMFLOAT3[m * n] };
			mCurrSolution = std::unique_ptr<DirectX::XMFLOAT3[]>{ new DirectX::XMFLOAT3[m * n] };
			mNormals = std::unique_ptr<DirectX::XMFLOAT3[]>{ new DirectX::XMFLOAT3[m * n] };
			mTangentX = std::unique_ptr<DirectX::XMFLOAT3[]>{ new DirectX::XMFLOAT3[m * n] };
		}

		mVertexCount = m * n;
		mTriangleCount = (m - 1) * (n - 1) * 2;

		mTimeStep = dt;
		mSpatialStep = dx;

		auto d = damping * dt + 2.0f;
		auto e = (speed * speed) * (dt * dt) / (dx * dx);
		mK1 = (damping * dt - 2.0f) / d;
		mK2 = (4.0f - 8.0f * e) / d;
		mK3 = (2.0f * e) / d;

		// Generate grid vertices in system memory.
		auto halfWidth = (n - 1) * dx * 0.5f;
		auto halfDepth = (m - 1) * dx * 0.5f;
		for (auto i = 0u; i < m; ++i)
		{
			auto z = halfDepth - i * dx;
			for (auto j = 0u; j < n; ++j)
			{
				auto x = -halfWidth + j * dx;

				mPrevSolution[i * n + j] = DirectX::XMFLOAT3(x, 0.0f, z);
				mCurrSolution[i * n + j] = DirectX::XMFLOAT3(x, 0.0f, z);
				mNormals[i * n + j] = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
				mTangentX[i * n + j] = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
			}
		}
	}

	void Update(float dt)
	{
		static auto t = 0.0f;
		// Accumulate time.
		t += dt;
		// Only update the simulation at the specified time step.
		if (t < mTimeStep)
			return;

		// Only update interior points; we use zero boundary conditions.
		for (auto i = 1u; i < mNumRows - 1; ++i)
		{
			for (auto j = 1u; j < mNumCols - 1; ++j)
			{
				// After this update we will be discarding the old previous
				// buffer, so overwrite that buffer with the new update.
				// Note how we can do this inplace (read/write to same element) 
				// because we won't need prev_ij again and the assignment happens last.

				// Note j indexes x and i indexes z: h(x_j, z_i, t_k)
				// Moreover, our +z axis goes "down"; this is just to 
				// keep consistent with our row indices going down.

				mPrevSolution[i * mNumCols + j].y =
					mK1 * mPrevSolution[i * mNumCols + j].y +
					mK2 * mCurrSolution[i * mNumCols + j].y +
					mK3 * (mCurrSolution[(i + 1) * mNumCols + j].y +
						mCurrSolution[(i - 1) * mNumCols + j].y +
						mCurrSolution[i * mNumCols + j + 1].y +
						mCurrSolution[i * mNumCols + j - 1].y);
			}
		}

		// We just overwrote the previous buffer with the new data, so
		// this data needs to become the current solution and the old
		// current solution becomes the new previous solution.
		std::swap(mPrevSolution, mCurrSolution);

		t = 0.0f; // reset time

		//
		// Compute normals using finite difference scheme.
		//
		for (auto i = 1u; i < mNumRows - 1; ++i)
		{
			for (auto j = 1u; j < mNumCols - 1; ++j)
			{
				auto l = mCurrSolution[i * mNumCols + j - 1].y;
				auto r = mCurrSolution[i * mNumCols + j + 1].y;
				auto t = mCurrSolution[(i - 1) * mNumCols + j].y;
				auto b = mCurrSolution[(i + 1) * mNumCols + j].y;
				mNormals[i * mNumCols + j].x = -r + l;
				mNormals[i * mNumCols + j].y = 2.0f * mSpatialStep;
				mNormals[i * mNumCols + j].z = b - t;

				auto n = DirectX::XMVECTOR{DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&mNormals[i * mNumCols + j]))};
				DirectX::XMStoreFloat3(&mNormals[i * mNumCols + j], n);

				mTangentX[i * mNumCols + j] = DirectX::XMFLOAT3(2.0f * mSpatialStep, r - l, 0.0f);
				auto T = DirectX::XMVECTOR{DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&mTangentX[i * mNumCols + j]))};
				DirectX::XMStoreFloat3(&mTangentX[i * mNumCols + j], T);
			}
		}
	}

	void Disturb(std::uint32_t i, std::uint32_t j, float magnitude)
	{
		// Don't disturb boundaries.
		//assert(i > 1 && i < mNumRows - 2);
		//assert(j > 1 && j < mNumCols - 2);

		auto halfMag = 0.5f * magnitude;

		// Disturb the ijth vertex height and its neighbors.
		mCurrSolution[i * mNumCols + j].y += magnitude;
		mCurrSolution[i * mNumCols + j + 1].y += halfMag;
		mCurrSolution[i * mNumCols + j - 1].y += halfMag;
		mCurrSolution[(i + 1) * mNumCols + j].y += halfMag;
		mCurrSolution[(i - 1) * mNumCols + j].y += halfMag;
	}

private:
	std::uint32_t mNumRows = 0;
	std::uint32_t mNumCols = 0;

	std::uint32_t mVertexCount = 0;
	std::uint32_t mTriangleCount = 0;

	// Simulation constants we can precompute.
	float mK1 = 0.0f;
	float mK2 = 0.0f;
	float mK3 = 0.0f;

	float mTimeStep = 0.0f;
	float mSpatialStep = 0.0f;

	std::unique_ptr<DirectX::XMFLOAT3[]> mPrevSolution;
	std::unique_ptr<DirectX::XMFLOAT3[]> mCurrSolution;
	std::unique_ptr<DirectX::XMFLOAT3[]> mNormals;
	std::unique_ptr<DirectX::XMFLOAT3[]> mTangentX;
};
