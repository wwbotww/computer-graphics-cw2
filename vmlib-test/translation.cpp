#include <catch2/catch_amalgamated.hpp>

#include "../vmlib/mat44.hpp"

TEST_CASE( "Translation matrix offsets positions", "[mat44][translation]" )
{
	using Catch::Matchers::WithinAbs;
	static constexpr float kEps = 1e-6f;

	Vec3f const offset{ 4.f, -2.f, 0.5f };
	auto const translation = make_translation( offset );

	SECTION( "Points receive the offset" )
	{
		Vec4f point{ 1.f, 2.f, 3.f, 1.f };
		auto const result = translation * point;

		REQUIRE_THAT( result.x, WithinAbs( 5.f, kEps ) );
		REQUIRE_THAT( result.y, WithinAbs( 0.f, kEps ) );
		REQUIRE_THAT( result.z, WithinAbs( 3.5f, kEps ) );
		REQUIRE_THAT( result.w, WithinAbs( 1.f, kEps ) );
	}

	SECTION( "Direction vectors remain unchanged" )
	{
		Vec4f direction{ -3.f, 0.5f, 2.f, 0.f };
		auto const result = translation * direction;

		REQUIRE_THAT( result.x, WithinAbs( direction.x, kEps ) );
		REQUIRE_THAT( result.y, WithinAbs( direction.y, kEps ) );
		REQUIRE_THAT( result.z, WithinAbs( direction.z, kEps ) );
		REQUIRE_THAT( result.w, WithinAbs( 0.f, kEps ) );
	}
}

