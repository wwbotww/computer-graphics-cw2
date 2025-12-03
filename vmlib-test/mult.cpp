#include <catch2/catch_amalgamated.hpp>

#include "../vmlib/mat44.hpp"

TEST_CASE( "Mat44 multiplication behaves as expected", "[mat44][multiply]" )
{
	using Catch::Matchers::WithinAbs;
	static constexpr float kEps = 1e-6f;

	SECTION( "Multiplying by identity returns the original matrix" )
	{
		Mat44f base{};
		float value = 1.f;
		for( std::size_t i = 0; i < 4; ++i )
		{
			for( std::size_t j = 0; j < 4; ++j )
				base[i,j] = value++;
		}

		auto const resultLeft = base * kIdentity44f;
		auto const resultRight = kIdentity44f * base;

		for( std::size_t i = 0; i < 4; ++i )
		{
			for( std::size_t j = 0; j < 4; ++j )
			{
				REQUIRE_THAT( (resultLeft[i,j]), WithinAbs( base[i,j], kEps ) );
				REQUIRE_THAT( (resultRight[i,j]), WithinAbs( base[i,j], kEps ) );
			}
		}
	}

	SECTION( "General multiplication yields known-good values" )
	{
		Mat44f left{};
		Mat44f right{};

		float value = 1.f;
		for( std::size_t i = 0; i < 4; ++i )
		{
			for( std::size_t j = 0; j < 4; ++j )
			{
				left[i,j] = value;
				right[i,j] = value + 16.f;
				value += 1.f;
			}
		}

		auto const result = left * right;
		float const expected[4][4] = {
			{ 250.f, 260.f, 270.f, 280.f },
			{ 618.f, 644.f, 670.f, 696.f },
			{ 986.f, 1028.f, 1070.f, 1112.f },
			{ 1354.f, 1412.f, 1470.f, 1528.f }
		};

		for( std::size_t i = 0; i < 4; ++i )
		{
			for( std::size_t j = 0; j < 4; ++j )
				REQUIRE_THAT( (result[i,j]), WithinAbs( expected[i][j], kEps ) );
		}
	}
}

TEST_CASE( "Mat44 times Vec4", "[mat44][matvec]" )
{
	using Catch::Matchers::WithinAbs;
	static constexpr float kEps = 1e-6f;

	Mat44f transform = kIdentity44f;
	transform[0,0] = 2.f;
	transform[1,1] = 3.f;
	transform[2,2] = 4.f;
	transform[0,3] = 5.f;
	transform[1,3] = -1.f;
	transform[2,3] = 2.f;

	SECTION( "Affine transform affects points" )
	{
		Vec4f point{ 1.f, 2.f, 3.f, 1.f };
		auto const result = transform * point;

		REQUIRE_THAT( result.x, WithinAbs( 7.f, kEps ) );
		REQUIRE_THAT( result.y, WithinAbs( 5.f, kEps ) );
		REQUIRE_THAT( result.z, WithinAbs( 14.f, kEps ) );
		REQUIRE_THAT( result.w, WithinAbs( 1.f, kEps ) );
	}

	SECTION( "Affine transform preserves direction w component" )
	{
		Vec4f dir{ 1.f, -1.f, 0.5f, 0.f };
		auto const result = transform * dir;

		REQUIRE_THAT( result.x, WithinAbs( 2.f, kEps ) );
		REQUIRE_THAT( result.y, WithinAbs( -3.f, kEps ) );
		REQUIRE_THAT( result.z, WithinAbs( 2.f, kEps ) );
		REQUIRE_THAT( result.w, WithinAbs( 0.f, kEps ) );
	}
}

