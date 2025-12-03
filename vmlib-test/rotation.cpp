#include <catch2/catch_amalgamated.hpp>

#include <numbers>

#include "../vmlib/mat44.hpp"

namespace
{
	constexpr float kPiOver2 = std::numbers::pi_v<float> * 0.5f;
}

TEST_CASE( "Rotation matrices preserve identity at zero angle", "[mat44][rotation]" )
{
	using Catch::Matchers::WithinAbs;
	static constexpr float kEps = 1e-6f;

	Vec4f vec{ 1.f, 2.f, 3.f, 1.f };

	auto const rx = make_rotation_x( 0.f ) * vec;
	auto const ry = make_rotation_y( 0.f ) * vec;
	auto const rz = make_rotation_z( 0.f ) * vec;

	REQUIRE_THAT( rx.x, WithinAbs( vec.x, kEps ) );
	REQUIRE_THAT( rx.y, WithinAbs( vec.y, kEps ) );
	REQUIRE_THAT( rx.z, WithinAbs( vec.z, kEps ) );

	REQUIRE_THAT( ry.x, WithinAbs( vec.x, kEps ) );
	REQUIRE_THAT( ry.y, WithinAbs( vec.y, kEps ) );
	REQUIRE_THAT( ry.z, WithinAbs( vec.z, kEps ) );

	REQUIRE_THAT( rz.x, WithinAbs( vec.x, kEps ) );
	REQUIRE_THAT( rz.y, WithinAbs( vec.y, kEps ) );
	REQUIRE_THAT( rz.z, WithinAbs( vec.z, kEps ) );
}

TEST_CASE( "Rotation matrices rotate around the proper axis", "[mat44][rotation]" )
{
	using Catch::Matchers::WithinAbs;
	static constexpr float kEps = 1e-5f;

	SECTION( "X axis rotation swaps Y/Z with handedness" )
	{
		Vec4f vec{ 0.f, 1.f, 0.f, 1.f };
		auto const rotated = make_rotation_x( kPiOver2 ) * vec;

		REQUIRE_THAT( rotated.x, WithinAbs( 0.f, kEps ) );
		REQUIRE_THAT( rotated.y, WithinAbs( 0.f, kEps ) );
		REQUIRE_THAT( rotated.z, WithinAbs( 1.f, kEps ) );

		Vec4f vecNeg{ 0.f, 0.f, 1.f, 1.f };
		auto const rotatedNeg = make_rotation_x( -kPiOver2 ) * vecNeg;

		REQUIRE_THAT( rotatedNeg.y, WithinAbs( 1.f, kEps ) );
		REQUIRE_THAT( rotatedNeg.z, WithinAbs( 0.f, kEps ) );
	}

	SECTION( "Y axis rotation swaps X/Z with handedness" )
	{
		Vec4f vec{ 0.f, 0.f, 1.f, 1.f };
		auto const rotated = make_rotation_y( kPiOver2 ) * vec;

		REQUIRE_THAT( rotated.x, WithinAbs( 1.f, kEps ) );
		REQUIRE_THAT( rotated.y, WithinAbs( 0.f, kEps ) );
		REQUIRE_THAT( rotated.z, WithinAbs( 0.f, kEps ) );

		Vec4f vecNeg{ 1.f, 0.f, 0.f, 1.f };
		auto const rotatedNeg = make_rotation_y( -kPiOver2 ) * vecNeg;

		REQUIRE_THAT( rotatedNeg.x, WithinAbs( 0.f, kEps ) );
		REQUIRE_THAT( rotatedNeg.z, WithinAbs( 1.f, kEps ) );
	}

	SECTION( "Z axis rotation swaps X/Y with handedness" )
	{
		Vec4f vec{ 1.f, 0.f, 0.f, 1.f };
		auto const rotated = make_rotation_z( kPiOver2 ) * vec;

		REQUIRE_THAT( rotated.x, WithinAbs( 0.f, kEps ) );
		REQUIRE_THAT( rotated.y, WithinAbs( 1.f, kEps ) );
		REQUIRE_THAT( rotated.z, WithinAbs( 0.f, kEps ) );

		Vec4f vecNeg{ 0.f, 1.f, 0.f, 1.f };
		auto const rotatedNeg = make_rotation_z( -kPiOver2 ) * vecNeg;

		REQUIRE_THAT( rotatedNeg.x, WithinAbs( 1.f, kEps ) );
		REQUIRE_THAT( rotatedNeg.y, WithinAbs( 0.f, kEps ) );
	}
}

