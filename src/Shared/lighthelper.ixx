export module shared:lighthelper;
import std;
import :win32;

export
{
	// Note: Make sure structure alignment agrees with HLSL structure padding rules. 
	//   Elements are packed into 4D vectors with the restriction that an element
	//   cannot straddle a 4D vector boundary.
	struct DirectionalLight
	{
		DirectX::XMFLOAT4 Ambient = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT4 Diffuse = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT4 Specular = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT3 Direction = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
		[[maybe_unused]]float Pad = 0.f; // Pad the last float so we can set an array of lights if we wanted.
	};

	struct PointLight
	{
		DirectX::XMFLOAT4 Ambient = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT4 Diffuse = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT4 Specular = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};

		// Packed into 4D vector: (Position, Range)
		DirectX::XMFLOAT3 Position = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
		float Range = 0.0f;

		// Packed into 4D vector: (A0, A1, A2, Pad)
		DirectX::XMFLOAT3 Att = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
		[[maybe_unused]] float Pad = 0.0f; // Pad the last float so we can set an array of lights if we wanted.
	};

	struct SpotLight
	{
		DirectX::XMFLOAT4 Ambient = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT4 Diffuse = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT4 Specular = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};

		// Packed into 4D vector: (Position, Range)
		DirectX::XMFLOAT3 Position = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
		float Range = 0.0f;

		// Packed into 4D vector: (Direction, Spot)
		DirectX::XMFLOAT3 Direction = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
		float Spot = 0.0f;

		// Packed into 4D vector: (Att, Pad)
		DirectX::XMFLOAT3 Att = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
		[[maybe_unused]] float Pad = 0.0f; // Pad the last float so we can set an array of lights if we wanted.
	};

	struct Material
	{
		DirectX::XMFLOAT4 Ambient = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT4 Diffuse = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT4 Specular = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f}; // w = SpecPower
		DirectX::XMFLOAT4 Reflect = DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};
	};
}
