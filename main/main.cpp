#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <print>
#include <numbers>
#include <typeinfo>
#include <stdexcept>
#include <filesystem>
#include <vector>
#include <array>
#include <memory>
#include <limits>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <type_traits>

#include <cstdlib>

#include "../support/error.hpp"
#include "../support/program.hpp"
#include "../support/checkpoint.hpp"
#include "../support/debug_output.hpp"

#include "../vmlib/vec4.hpp"
#include "../vmlib/vec2.hpp"
#include "../vmlib/mat44.hpp"
#include "../vmlib/vec3.hpp"

#include "defaults.hpp"

#include "../third_party/rapidobj/include/rapidobj/rapidobj.hpp"
#include "../third_party/stb/include/stb_image.h"

namespace task5
{
	struct VehicleGeometry
	{
		GLuint vao = 0;
		GLuint vbo = 0;
		GLsizei vertexCount = 0;
	};

	VehicleGeometry create_vehicle_geometry();
    void destroy_geometry(VehicleGeometry&);
    void render_vehicle(const VehicleGeometry&, const Mat44f& modelMatrix, GLint uModelLocation);
}

namespace task6
{
		struct PointLight
	{
		Vec3f position;
		Vec3f color;
		bool enabled = true;
	};

	struct LightState
	{
		bool dirLightEnabled = true;
		bool pointEnabled[3] = { true, true, true };
	};

	void upload_lights_to_shader(GLuint programId,
                             LightState const& lightState,
                             std::array<PointLight,3> const& lights,
                             Vec3f const& dirLightDir);
}

namespace task7
{
    struct AnimationState
    {
        bool active   = false;
        bool paused   = false;
        float time    = 0.f;

        Vec3f startPos{};
        Vec3f lastPos{};

        Mat44f baseModel = kIdentity44f;
        Mat44f currentModel = kIdentity44f;

        std::array<Vec3f,3> lightOffsets{};
    };

    void initialise(AnimationState& anim,
                    Mat44f const& baseModel,
                    std::array<task6::PointLight,3>& lights);

    void toggle_play(AnimationState& anim);

    void reset(AnimationState& anim);

    void update(AnimationState& anim,
                float deltaSeconds,
                Mat44f& vehicleModelMatrix,
                std::array<task6::PointLight,3>& lights);
}

namespace 
{
	struct Camera
	{
		Vec3f position{ 0.f, 0.f, 5.f };
		float yaw = 0.f;
		float pitch = 0.f;
	};
}

namespace task8
{
    enum class CameraMode
    {
        Free,
        Follow,
        Ground
    };

    struct TrackingCamera
    {
        CameraMode mode = CameraMode::Free;

        Vec3f groundPos{ -30.f, 0.f, 20.f };

        Vec3f followOffset{ 0.f, 5.f, -15.f };

        void next_mode();
    };

    CameraMode next_mode( CameraMode mode );

	
    Mat44f compute_camera_view(
        TrackingCamera const& cam,
        Camera const& freeCam,
        Mat44f const& rocketModel
    );
}

// =============================================================================
// Internal (file-local) helpers, state, and forward declarations
// =============================================================================
namespace
{
	// --- Window / constants ---
	constexpr char const* kWindowTitle = "COMP3811 - CW2";
	constexpr float kPi = std::numbers::pi_v<float>;

	struct GLFWCleanupHelper
	{
		~GLFWCleanupHelper();
	};
	struct GLFWWindowDeleter
	{
		~GLFWWindowDeleter();
		GLFWwindow* window;
	};

	// === Mesh vertex formats ===
	struct VertexPNT
	{
		Vec3f position;
		Vec3f normal;
		Vec2f texCoord;
	};

	struct VertexPNC
	{
		Vec3f position;
		Vec3f normal;
		Vec3f color;
	};

	// === View / viewport containers ===
	struct ViewportRect
	{
		GLint x = 0;
		GLint y = 0;
		GLsizei width = 1;
		GLsizei height = 1;
	};

	struct RenderView
	{
		Mat44f view;
		Mat44f proj;
		ViewportRect viewport;
	};

	// === Particle system data ===
	struct Particle
	{
		Vec3f position{};
		Vec3f velocity{};
		float age = 0.f;
		float lifetime = 0.f;
		float size = 1.f;
		bool alive = false;
	};

	struct ParticlePipeline
	{
		std::unique_ptr<ShaderProgram> program;
		GLint uView = -1;
		GLint uProj = -1;
		GLint uViewportHeight = -1;
		GLint uTanHalfFov = -1;
		GLint uTexture = -1;
		GLint uColor = -1;
	};

	struct ParticleGpu
	{
		Vec3f position;
		float size;
		float alpha;
	};

	struct ParticleSystem
	{
		GLuint vao = 0;
		GLuint vbo = 0;
		GLuint textureId = 0;
		std::vector<Particle> pool;
		std::size_t head = 0;
		std::size_t aliveCount = 0;
		float emitAccumulator = 0.f;
	};

	// === Input / scene state ===
	struct InputState
	{
		bool forward = false;
		bool backward = false;
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
		bool fast = false;
		bool slow = false;
	};

	struct SceneGeometry
	{
		GLuint vao = 0;
		GLuint vbo = 0;
		GLsizei vertexCount = 0;
		Vec3f minBounds{ 0.f, 0.f, 0.f };
		Vec3f maxBounds{ 0.f, 0.f, 0.f };
		Vec3f center{ 0.f, 0.f, 0.f };
		float radius = 1.f;
	};

	struct LandingPadGeometry
	{
		GLuint vao = 0;
		GLuint vbo = 0;
		GLsizei vertexCount = 0;
	};

	struct TerrainPipeline
	{
		std::unique_ptr<ShaderProgram> program;
		GLint uModel = -1;
		GLint uView = -1;
		GLint uProj = -1;
		GLint uLightDir = -1;
		GLint uAmbient = -1;
		GLint uDiffuse = -1;
		GLint uTexture = -1;
		GLuint textureId = 0;
	};

	struct LandingPadPipeline
	{
		std::unique_ptr<ShaderProgram> program;
		GLint uModel = -1;
		GLint uView = -1;
		GLint uProj = -1;
		GLint uLightDir = -1;
		GLint uAmbient = -1;
		GLint uDiffuse = -1;
	};

	// === Feature toggles ===
	struct SplitScreenState
	{
		bool enabled = false;
		task8::CameraMode primaryMode = task8::CameraMode::Free;
		task8::CameraMode secondaryMode = task8::CameraMode::Follow;
	};

	struct AppState
	{
		Camera camera;
		InputState input;
		bool mouseLookActive = false;
		bool lastCursorValid = false;
		double lastCursorX = 0.0;
		double lastCursorY = 0.0;
		float mouseSensitivity = 0.0025f; // radians per pixel
		float baseSpeed = 35.f;           // meters per second
		float fastMultiplier = 6.f;
		float slowMultiplier = 0.2f;
		Vec3f worldUp{ 0.f, 1.f, 0.f };
		float fovRadians = kPi / 3.f;
		float nearPlane = 0.5f;
		float farPlane = 4000.f;
		Mat44f projection = kIdentity44f;
		GLsizei framebufferWidth = 1280;
		GLsizei framebufferHeight = 720;
		Clock::time_point previousFrameTime = Clock::now();
		task6::LightState lights;
		task7::AnimationState animation;
		task8::TrackingCamera trackingCam;
		SplitScreenState splitScreen;
		ParticleSystem particles;
	};

	void glfw_callback_error_( int, char const* );
	void glfw_callback_key_( GLFWwindow*, int, int, int, int );
	void glfw_callback_cursor_( GLFWwindow*, double, double );
	void glfw_callback_mouse_button_( GLFWwindow*, int, int, int );
	void glfw_callback_framebuffer_( GLFWwindow*, int, int );

	// --- Loading / resources ---
	SceneGeometry load_parlahti_mesh( std::filesystem::path const& objPath );
	void destroy_geometry( SceneGeometry& geometry );
	LandingPadGeometry load_landingpad_mesh( std::filesystem::path const& objPath );

	void destroy_geometry( LandingPadGeometry& geometry );
	GLuint load_texture_2d( std::filesystem::path const& imagePath );
	GLuint create_particle_texture();

	void init_particle_system( ParticleSystem& system );
	void destroy_particle_system( ParticleSystem& system );
	void emit_particles( ParticleSystem& system, Vec3f const& emitterPos, Vec3f const& emitterDir, float rate, float dt );
	void update_particles( ParticleSystem& system, float dt );
	void upload_particles( ParticleSystem& system );
	void render_particles( ParticlePipeline const& pipeline, ParticleSystem const& system, Mat44f const& view, Mat44f const& proj, ViewportRect const& viewport, float fovRadians );

	Vec3f compute_forward_vector( Camera const& camera );
	Mat44f make_view_matrix( Camera const& camera, Vec3f const& worldUp );
	void update_projection( AppState& app );
	void update_camera( AppState& app, float deltaSeconds );
	std::array<float,16> to_gl_matrix( Mat44f const& mat );

	// === Inline math helpers ===
	inline Vec3f cross( Vec3f const& a, Vec3f const& b ) noexcept
	{
		return Vec3f{
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		};
	}
	inline Vec3f safe_normalize( Vec3f vec, Vec3f fallback = Vec3f{ 0.f, 1.f, 0.f } ) noexcept
	{
		float const len = length( vec );
		if( len <= 1e-6f )
			return fallback;
		return vec / len;
	}
}

int main() try
{
	// === Init window/GL context ===
	if( GLFW_TRUE != glfwInit() )
	{
		char const* msg = nullptr;
		int ecode = glfwGetError( &msg );
		throw Error( "glfwInit() failed with '{}' ({})", msg, ecode );
	}

	GLFWCleanupHelper cleanupHelper;

	glfwSetErrorCallback( &glfw_callback_error_ );
	glfwWindowHint( GLFW_SRGB_CAPABLE, GLFW_TRUE );
	glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
	glfwWindowHint( GLFW_DEPTH_BITS, 24 );
#	if !defined(NDEBUG)
	glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );
#	endif

	GLFWwindow* window = glfwCreateWindow( 1280, 720, kWindowTitle, nullptr, nullptr );
	if( !window )
	{
		char const* msg = nullptr;
		int ecode = glfwGetError( &msg );
		throw Error( "glfwCreateWindow() failed with '{}' ({})", msg, ecode );
	}

	GLFWWindowDeleter windowDeleter{ window };

	glfwMakeContextCurrent( window );
	glfwSwapInterval( 1 );

	if( !gladLoadGLLoader( (GLADloadproc)&glfwGetProcAddress ) )
		throw Error( "gladLoadGLLoader() failed - cannot load GL API!" );
	glEnable( GL_FRAMEBUFFER_SRGB );

	std::print( "RENDERER {}\n", (char const*)glGetString( GL_RENDERER ) );
	std::print( "VENDOR {}\n", (char const*)glGetString( GL_VENDOR ) );
	std::print( "VERSION {}\n", (char const*)glGetString( GL_VERSION ) );
	std::print( "SHADING_LANGUAGE_VERSION {}\n", (char const*)glGetString( GL_SHADING_LANGUAGE_VERSION ) );

#	if !defined(NDEBUG)
	setup_gl_debug_output();
#	endif

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );
	glFrontFace( GL_CCW );

	AppState app{};
	int fbWidth = 0;
	int fbHeight = 0;
	glfwGetFramebufferSize( window, &fbWidth, &fbHeight );
	app.framebufferWidth = fbWidth;
	app.framebufferHeight = fbHeight;
	update_projection( app );
	app.previousFrameTime = Clock::now();

	glfwSetWindowUserPointer( window, &app );
	glfwSetKeyCallback( window, &glfw_callback_key_ );
	glfwSetCursorPosCallback( window, &glfw_callback_cursor_ );
	glfwSetMouseButtonCallback( window, &glfw_callback_mouse_button_ );
	glfwSetFramebufferSizeCallback( window, &glfw_callback_framebuffer_ );

	// === Load terrain / setup camera ===
	std::filesystem::path const objPath = std::filesystem::path( "assets/cw2/parlahti.obj" );
	auto geometry = load_parlahti_mesh( objPath );

	app.camera.position = Vec3f{
		geometry.center.x,
		geometry.center.y + geometry.radius * 0.45f,
		geometry.center.z + geometry.radius * 1.1f
	};

	Vec3f lookDir = safe_normalize( geometry.center - app.camera.position );
	app.camera.yaw = std::atan2( lookDir.z, lookDir.x );
	app.camera.pitch = std::asin( std::clamp( lookDir.y, -1.f, 1.f ) );

	std::filesystem::path const shaderRoot = std::filesystem::path( "assets/cw2" );
	TerrainPipeline terrain{};
	terrain.program = std::make_unique<ShaderProgram>( std::vector<ShaderProgram::ShaderSource>{
		{ GL_VERTEX_SHADER, (shaderRoot / "terrain.vert").string() },
		{ GL_FRAGMENT_SHADER, (shaderRoot / "terrain.frag").string() }
	} );
	terrain.uModel = glGetUniformLocation( terrain.program->programId(), "uModel" );
	terrain.uView = glGetUniformLocation( terrain.program->programId(), "uView" );
	terrain.uProj = glGetUniformLocation( terrain.program->programId(), "uProj" );
	terrain.uLightDir = glGetUniformLocation( terrain.program->programId(), "uLightDir" );
	terrain.uAmbient = glGetUniformLocation( terrain.program->programId(), "uAmbientColor" );
	terrain.uDiffuse = glGetUniformLocation( terrain.program->programId(), "uDiffuseColor" );
	terrain.uTexture = glGetUniformLocation( terrain.program->programId(), "uTerrainTexture" );

	std::filesystem::path const texturePath = shaderRoot / "L4343A-4k.jpeg";
	terrain.textureId = load_texture_2d( texturePath );

	auto landingPadGeometry = load_landingpad_mesh( shaderRoot / "landingpad.obj" );
	LandingPadPipeline landingPad{};
	landingPad.program = std::make_unique<ShaderProgram>( std::vector<ShaderProgram::ShaderSource>{
		{ GL_VERTEX_SHADER, (shaderRoot / "landingpad.vert").string() },
		{ GL_FRAGMENT_SHADER, (shaderRoot / "landingpad.frag").string() }
	} );
	landingPad.uModel = glGetUniformLocation( landingPad.program->programId(), "uModel" );
	landingPad.uView = glGetUniformLocation( landingPad.program->programId(), "uView" );
	landingPad.uProj = glGetUniformLocation( landingPad.program->programId(), "uProj" );
	landingPad.uLightDir = glGetUniformLocation( landingPad.program->programId(), "uLightDir" );
	landingPad.uAmbient = glGetUniformLocation( landingPad.program->programId(), "uAmbientColor" );
	landingPad.uDiffuse = glGetUniformLocation( landingPad.program->programId(), "uDiffuseColor" );

	Mat44f modelMatrix = kIdentity44f;
	Vec3f lightDirection = safe_normalize( Vec3f{ 0.f, 1.f, -1.f } );
	Vec3f ambientColor{ 0.25f, 0.25f, 0.25f };
	Vec3f diffuseColor{ 0.75f, 0.75f, 0.75f };

	float const waterLevel = geometry.minBounds.y;
	std::array<Vec3f, 2> landingPadAnchors{
		Vec3f{ -20.f, 0.f, 12.f },
		Vec3f{ -10.f, 0.f, 23.f }
	};
	float const landingPadScale = 25.f;
	Mat44f const landingPadScaleMatrix = make_scaling( landingPadScale, landingPadScale, landingPadScale );
	std::array<Mat44f, 2> landingPadModels{};
	for( std::size_t i = 0; i < landingPadAnchors.size(); ++i )
	{
		Vec3f position = landingPadAnchors[i];
		position.y = waterLevel + 0.1f;
		landingPadModels[i] = make_translation( position ) * landingPadScaleMatrix;
	}

	glViewport( 0, 0, fbWidth, fbHeight );

	//task5: create vehicle geometry
	task5::VehicleGeometry vehicleGeometry = task5::create_vehicle_geometry();
	Mat44f vehicleModelMatrix =
		landingPadModels[0] * make_translation( Vec3f{ 0.f, 0.2f, 0.f } );

	//task6: setup point lights
	// Vehicle position
	Vec3f pad0Pos = landingPadAnchors[0];
	pad0Pos.y = waterLevel + 5.f;
	float radius = 6.f;
	float const kSqrt3Over2 = 0.87f;

	std::array<task6::PointLight, 3> pointLights;

	// R
	pointLights[0].position = pad0Pos + Vec3f{ radius, 3.f, 0.f };
	pointLights[0].color    = Vec3f{ 100.f, 0.f, 0.f };

	// G
	pointLights[1].position = pad0Pos + Vec3f{ -radius * 0.5f, 3.f, radius * kSqrt3Over2 };
	pointLights[1].color    = Vec3f{ 0.f, 100.f, 0.f };

	// B
	pointLights[2].position = pad0Pos + Vec3f{ -radius * 0.5f, 3.f, -radius * kSqrt3Over2 };
	pointLights[2].color    = Vec3f{ 0.f, 0.f, 100.f };

	//task7: initialise animation
	task7::initialise(app.animation, vehicleModelMatrix, pointLights);

	// task10: particle system (exhaust)
	ParticlePipeline particlePipeline{};
	particlePipeline.program = std::make_unique<ShaderProgram>( std::vector<ShaderProgram::ShaderSource>{
		{ GL_VERTEX_SHADER, (shaderRoot / "particles.vert").string() },
		{ GL_FRAGMENT_SHADER, (shaderRoot / "particles.frag").string() }
	} );
	particlePipeline.uView = glGetUniformLocation( particlePipeline.program->programId(), "uView" );
	particlePipeline.uProj = glGetUniformLocation( particlePipeline.program->programId(), "uProj" );
	particlePipeline.uViewportHeight = glGetUniformLocation( particlePipeline.program->programId(), "uViewportHeight" );
	particlePipeline.uTanHalfFov = glGetUniformLocation( particlePipeline.program->programId(), "uTanHalfFov" );
	particlePipeline.uTexture = glGetUniformLocation( particlePipeline.program->programId(), "uParticleTex" );
	particlePipeline.uColor = glGetUniformLocation( particlePipeline.program->programId(), "uParticleColor" );
	init_particle_system( app.particles );
	app.particles.textureId = create_particle_texture();

	while( !glfwWindowShouldClose( window ) )
	{
		glfwPollEvents();

		// === Per-frame timing & input ===
		auto const now = Clock::now();
		Secondsf const elapsed = now - app.previousFrameTime;
		app.previousFrameTime = now;
		update_camera( app, elapsed.count() );

		// === Simulation: animation & particles (frozen when paused) ===
		task7::update(app.animation,
                      elapsed.count(),
                      vehicleModelMatrix,
                      pointLights);

		// task10: update particles (freeze when动画暂停)
		bool const animPaused = app.animation.paused;
		float const simDt = animPaused ? 0.f : elapsed.count();
		if( simDt > 0.f )
		{
			// 取得火箭世界位置与朝向，作为喷口基准
			Vec3f rocketPos{ vehicleModelMatrix[0,3], vehicleModelMatrix[1,3], vehicleModelMatrix[2,3] };
			Vec3f forward{
				vehicleModelMatrix[1,0],
				vehicleModelMatrix[1,1],
				vehicleModelMatrix[1,2]
			};
			forward = safe_normalize( forward, Vec3f{ 0.f, 0.f, 1.f } );
			Vec3f exhaustDir = -forward;

			Vec3f emitterPos = rocketPos - forward * 2.0f + Vec3f{ 0.f, -0.3f, 0.f };

			float const emitRate = 280.f; // 粒子/秒
			emit_particles( app.particles, emitterPos, exhaustDir, emitRate, simDt );
			update_particles( app.particles, simDt );
			upload_particles( app.particles );
		}

		glClearColor( 0.15f, 0.17f, 0.22f, 1.f );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		// === Build per-view viewport/projection list (split-screen aware) ===
		auto compute_projection_for = [&]( ViewportRect const& viewport ) -> Mat44f
		{
			int const safeWidth = std::max( 1, static_cast<int>( viewport.width ) );
			int const safeHeight = std::max( 1, static_cast<int>( viewport.height ) );
			float const aspect = safeWidth / static_cast<float>( safeHeight );
			return make_perspective_projection( app.fovRadians, aspect, app.nearPlane, app.farPlane );
		};

		std::array<RenderView, 2> views{};
		std::size_t viewCount = 0;
		auto add_view = [&]( Mat44f const& view, Mat44f const& proj, ViewportRect viewport )
		{
			views[viewCount++] = RenderView{ view, proj, viewport };
		};

		ViewportRect const fullViewport{
			0,
			0,
			app.framebufferWidth,
			app.framebufferHeight
		};

		app.trackingCam.mode = app.splitScreen.primaryMode;

		if( app.splitScreen.enabled )
		{
			GLsizei const leftWidth = std::max<GLsizei>( 1, app.framebufferWidth / 2 );
			GLsizei const rightWidth = std::max<GLsizei>( 1, app.framebufferWidth - leftWidth );
			ViewportRect const leftViewport{ 0, 0, leftWidth, app.framebufferHeight };
			ViewportRect const rightViewport{ static_cast<GLint>( leftWidth ), 0, rightWidth, app.framebufferHeight };

			Mat44f const leftView = task8::compute_camera_view(
				app.trackingCam,
				app.camera,
				vehicleModelMatrix
			);
			add_view( leftView, compute_projection_for( leftViewport ), leftViewport );

			task8::TrackingCamera secondaryCam = app.trackingCam;
			secondaryCam.mode = app.splitScreen.secondaryMode;
			Mat44f const rightView = task8::compute_camera_view(
				secondaryCam,
				app.camera,
				vehicleModelMatrix
			);
			add_view( rightView, compute_projection_for( rightViewport ), rightViewport );
		}
		else
		{
			Mat44f const viewMatrix = task8::compute_camera_view(
				app.trackingCam,
				app.camera,
				vehicleModelMatrix
			);
			add_view( viewMatrix, app.projection, fullViewport );
		}

		auto const modelGl = to_gl_matrix( modelMatrix );

		// === Render a single view (shared for split and non-split) ===
		auto render_view = [&]( RenderView const& renderView )
		{
			glViewport( renderView.viewport.x, renderView.viewport.y, renderView.viewport.width, renderView.viewport.height );

			auto const viewGl = to_gl_matrix( renderView.view );
			auto const projGl = to_gl_matrix( renderView.proj );

			glUseProgram( terrain.program->programId() );
			task6::upload_lights_to_shader(
				terrain.program->programId(),
				app.lights,
				pointLights,
				lightDirection
			);
			glUniformMatrix4fv( terrain.uModel, 1, GL_FALSE, modelGl.data() );
			glUniformMatrix4fv( terrain.uView, 1, GL_FALSE, viewGl.data() );
			glUniformMatrix4fv( terrain.uProj, 1, GL_FALSE, projGl.data() );

			glUniform3f( terrain.uLightDir, lightDirection.x, lightDirection.y, lightDirection.z );
			glUniform3f( terrain.uAmbient, ambientColor.x, ambientColor.y, ambientColor.z );
			glUniform3f( terrain.uDiffuse, diffuseColor.x, diffuseColor.y, diffuseColor.z );
			glUniform1i( terrain.uTexture, 0 );

			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, terrain.textureId );

			glBindVertexArray( geometry.vao );
			glDrawArrays( GL_TRIANGLES, 0, geometry.vertexCount );
			glBindVertexArray( 0 );

			glUseProgram( landingPad.program->programId() );
			task6::upload_lights_to_shader(
				landingPad.program->programId(),
				app.lights,
				pointLights,
				lightDirection
			);
			glUniformMatrix4fv( landingPad.uView, 1, GL_FALSE, viewGl.data() );
			glUniformMatrix4fv( landingPad.uProj, 1, GL_FALSE, projGl.data() );
			glUniform3f( landingPad.uLightDir, lightDirection.x, lightDirection.y, lightDirection.z );
			glUniform3f( landingPad.uAmbient, ambientColor.x, ambientColor.y, ambientColor.z );
			glUniform3f( landingPad.uDiffuse, diffuseColor.x, diffuseColor.y, diffuseColor.z );

			glBindVertexArray( landingPadGeometry.vao );
			for( auto const& padModel : landingPadModels )
			{
				auto const padModelGl = to_gl_matrix( padModel );
				glUniformMatrix4fv( landingPad.uModel, 1, GL_FALSE, padModelGl.data() );
				glDrawArrays( GL_TRIANGLES, 0, landingPadGeometry.vertexCount );
			}
			glBindVertexArray( 0 );

			task5::render_vehicle( vehicleGeometry, vehicleModelMatrix, landingPad.uModel );

			// task10: particles
			render_particles( particlePipeline, app.particles, renderView.view, renderView.proj, renderView.viewport, app.fovRadians );
		};

		for( std::size_t viewIndex = 0; viewIndex < viewCount; ++viewIndex )
			render_view( views[viewIndex] );

		glfwSwapBuffers( window );
	}

	destroy_geometry( geometry );
	destroy_geometry( landingPadGeometry );
	task5::destroy_geometry( vehicleGeometry );
	destroy_particle_system( app.particles );
	if( terrain.textureId )
	{
		glDeleteTextures( 1, &terrain.textureId );
		terrain.textureId = 0;
	}

	return 0;
}

catch( std::exception const& eErr )
{
	std::print( stderr, "Top-level Exception ({}):\n", typeid(eErr).name() );
	std::print( stderr, "{}\n", eErr.what() );
	std::print( stderr, "Bye.\n" );
	return 1;
}


namespace
{
	// === GLFW callbacks ===
	void glfw_callback_error_( int aErrNum, char const* aErrDesc )
	{
		std::print( stderr, "GLFW error: {} ({})\n", aErrDesc, aErrNum );
	}

	void glfw_callback_key_( GLFWwindow* aWindow, int aKey, int, int aAction, int aMods )
	{
		auto* app = static_cast<AppState*>( glfwGetWindowUserPointer( aWindow ) );
		if( !app )
			return;

		bool const isPress = (aAction == GLFW_PRESS) || (aAction == GLFW_REPEAT);
		bool const isRelease = (aAction == GLFW_RELEASE);

		if( aKey == GLFW_KEY_ESCAPE && isPress )
		{
			glfwSetWindowShouldClose( aWindow, GLFW_TRUE );
			return;
		}

		auto apply_state = [&]( bool& field )
		{
			if( aAction == GLFW_PRESS )
				field = true;
			else if( aAction == GLFW_RELEASE )
				field = false;
		};

		switch( aKey )
		{
			case GLFW_KEY_W: apply_state( app->input.forward ); break;
			case GLFW_KEY_S: apply_state( app->input.backward ); break;
			case GLFW_KEY_A: apply_state( app->input.left ); break;
			case GLFW_KEY_D: apply_state( app->input.right ); break;
			case GLFW_KEY_E: apply_state( app->input.up ); break;
			case GLFW_KEY_Q: apply_state( app->input.down ); break;
			case GLFW_KEY_LEFT_SHIFT:
			case GLFW_KEY_RIGHT_SHIFT: apply_state( app->input.fast ); break;
			case GLFW_KEY_LEFT_CONTROL:
			case GLFW_KEY_RIGHT_CONTROL: apply_state( app->input.slow ); break;
			//task6: light toggles
			case GLFW_KEY_1:
				if (aAction == GLFW_PRESS)
					app->lights.pointEnabled[0] = !app->lights.pointEnabled[0];
				break;

			case GLFW_KEY_2:
				if (aAction == GLFW_PRESS)
					app->lights.pointEnabled[1] = !app->lights.pointEnabled[1];
				break;

			case GLFW_KEY_3:
				if (aAction == GLFW_PRESS)
					app->lights.pointEnabled[2] = !app->lights.pointEnabled[2];
				break;

			case GLFW_KEY_4:
				if (aAction == GLFW_PRESS)
					app->lights.dirLightEnabled = !app->lights.dirLightEnabled;
				break;

			case GLFW_KEY_F:
                if (aAction == GLFW_PRESS)
                    task7::toggle_play(app->animation);
                break;
            case GLFW_KEY_R:
                if (aAction == GLFW_PRESS)
                    task7::reset(app->animation);
                break;
			case GLFW_KEY_C:
				if( aAction == GLFW_PRESS )
				{
					if( (aMods & GLFW_MOD_SHIFT) != 0 )
					{
						app->splitScreen.secondaryMode = task8::next_mode( app->splitScreen.secondaryMode );
					}
					else
					{
						app->splitScreen.primaryMode = task8::next_mode( app->splitScreen.primaryMode );
						app->trackingCam.mode = app->splitScreen.primaryMode;
					}
				}
				break;
			case GLFW_KEY_V:
				if( aAction == GLFW_PRESS )
					app->splitScreen.enabled = !app->splitScreen.enabled;
				break;
			default:
				break;
		}

		(void)isPress;
		(void)isRelease;
	}

	void glfw_callback_cursor_( GLFWwindow* aWindow, double aXPos, double aYPos )
	{
		auto* app = static_cast<AppState*>( glfwGetWindowUserPointer( aWindow ) );
		if( !app || !app->mouseLookActive )
		{
			if( app )
				app->lastCursorValid = false;
			return;
		}

		if( !app->lastCursorValid )
		{
			app->lastCursorX = aXPos;
			app->lastCursorY = aYPos;
			app->lastCursorValid = true;
			return;
		}

		double const deltaX = aXPos - app->lastCursorX;
		double const deltaY = aYPos - app->lastCursorY;
		app->lastCursorX = aXPos;
		app->lastCursorY = aYPos;

		app->camera.yaw += static_cast<float>( deltaX ) * app->mouseSensitivity;
		app->camera.pitch -= static_cast<float>( deltaY ) * app->mouseSensitivity;

		float constexpr kPitchLimit = std::numbers::pi_v<float> / 2.f - 0.01f;
		app->camera.pitch = std::clamp( app->camera.pitch, -kPitchLimit, kPitchLimit );
	}

	void glfw_callback_mouse_button_( GLFWwindow* aWindow, int aButton, int aAction, int )
	{
		auto* app = static_cast<AppState*>( glfwGetWindowUserPointer( aWindow ) );
		if( !app )
			return;

		if( aButton == GLFW_MOUSE_BUTTON_RIGHT && aAction == GLFW_PRESS )
		{
			app->mouseLookActive = !app->mouseLookActive;
			app->lastCursorValid = false;
			glfwSetInputMode( aWindow, GLFW_CURSOR, app->mouseLookActive ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL );
		}
	}

	void glfw_callback_framebuffer_( GLFWwindow* aWindow, int aWidth, int aHeight )
	{
		glViewport( 0, 0, aWidth, aHeight );
		auto* app = static_cast<AppState*>( glfwGetWindowUserPointer( aWindow ) );
		if( !app )
			return;
		app->framebufferWidth = std::max( 1, aWidth );
		app->framebufferHeight = std::max( 1, aHeight );
		update_projection( *app );
	}

	// === Geometry loading / destruction (terrain & landing pad) ===
	SceneGeometry load_parlahti_mesh( std::filesystem::path const& objPath )
	{
		auto const resultPath = objPath.lexically_normal();
		auto result = rapidobj::ParseFile( resultPath );
		if( result.error )
			throw Error( "Failed to load '{}': {} at line {}", resultPath.string(), result.error.code.message(), result.error.line_num );
		if( !rapidobj::Triangulate( result ) )
			throw Error( "Triangulation failed for '{}'", resultPath.string() );

		SceneGeometry geometry{};
		Vec3f minBounds{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		Vec3f maxBounds{ std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

		std::size_t totalIndices = 0;
		for( auto const& shape : result.shapes )
			totalIndices += shape.mesh.indices.size();

		std::vector<VertexPNT> vertices;
		vertices.reserve( totalIndices );

		auto const fetch_position = [&]( int index ) -> Vec3f
		{
			std::size_t const base = static_cast<std::size_t>( index ) * 3;
			return Vec3f{
				result.attributes.positions[base + 0],
				result.attributes.positions[base + 1],
				result.attributes.positions[base + 2]
			};
		};
		auto const fetch_normal = [&]( int index ) -> Vec3f
		{
			std::size_t const base = static_cast<std::size_t>( index ) * 3;
			return Vec3f{
				result.attributes.normals[base + 0],
				result.attributes.normals[base + 1],
				result.attributes.normals[base + 2]
			};
		};

		auto const fetch_texcoord = [&]( int index ) -> Vec2f
		{
			std::size_t const base = static_cast<std::size_t>( index ) * 2;
			return Vec2f{
				result.attributes.texcoords[base + 0],
				result.attributes.texcoords[base + 1]
			};
		};

		for( auto const& shape : result.shapes )
		{
			auto const& mesh = shape.mesh;
			std::size_t indexOffset = 0;

			for( auto const faceVertices : mesh.num_face_vertices )
			{
				if( faceVertices != 3 )
				{
					indexOffset += faceVertices;
					continue;
				}

				std::array<rapidobj::Index, 3> faceIndices{};
				std::array<Vec3f, 3> positions{};
				std::array<Vec3f, 3> normals{};
				bool hasPerVertexNormals = !result.attributes.normals.empty();
				bool hasTexCoordData = !result.attributes.texcoords.empty();
				std::array<Vec2f, 3> texcoords{};

				for( std::size_t v = 0; v < 3; ++v )
				{
					faceIndices[v] = mesh.indices[indexOffset++];
					positions[v] = fetch_position( faceIndices[v].position_index );
					if( hasPerVertexNormals && faceIndices[v].normal_index >= 0 )
						normals[v] = fetch_normal( faceIndices[v].normal_index );
					else
						hasPerVertexNormals = false;

					if( hasTexCoordData && faceIndices[v].texcoord_index >= 0 )
						texcoords[v] = fetch_texcoord( faceIndices[v].texcoord_index );
				}

				Vec3f const edgeA = positions[1] - positions[0];
				Vec3f const edgeB = positions[2] - positions[0];
				Vec3f faceNormal = safe_normalize( cross( edgeA, edgeB ) );

				for( std::size_t v = 0; v < 3; ++v )
				{
					VertexPNT vertex{};
					vertex.position = positions[v];
					vertex.normal = hasPerVertexNormals ? safe_normalize( normals[v], faceNormal ) : faceNormal;
					vertex.texCoord = (hasTexCoordData && faceIndices[v].texcoord_index >= 0) ? texcoords[v] : Vec2f{ 0.f, 0.f };
					vertices.emplace_back( vertex );

					minBounds.x = std::min( minBounds.x, vertex.position.x );
					minBounds.y = std::min( minBounds.y, vertex.position.y );
					minBounds.z = std::min( minBounds.z, vertex.position.z );
					maxBounds.x = std::max( maxBounds.x, vertex.position.x );
					maxBounds.y = std::max( maxBounds.y, vertex.position.y );
					maxBounds.z = std::max( maxBounds.z, vertex.position.z );
				}
			}
		}

		if( vertices.empty() )
			throw Error( "OBJ '{}' did not contain triangles", resultPath.string() );

		geometry.minBounds = minBounds;
		geometry.maxBounds = maxBounds;
		geometry.center = Vec3f{
			(minBounds.x + maxBounds.x) * 0.5f,
			(minBounds.y + maxBounds.y) * 0.5f,
			(minBounds.z + maxBounds.z) * 0.5f
		};
		Vec3f const diagonal = maxBounds - minBounds;
		geometry.radius = 0.5f * length( diagonal );

		glGenVertexArrays( 1, &geometry.vao );
		glGenBuffers( 1, &geometry.vbo );

		glBindVertexArray( geometry.vao );
		glBindBuffer( GL_ARRAY_BUFFER, geometry.vbo );
		glBufferData( GL_ARRAY_BUFFER, static_cast<GLsizeiptr>( vertices.size() * sizeof( VertexPNT ) ), vertices.data(), GL_STATIC_DRAW );

		glEnableVertexAttribArray( 0 );
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPNT ), reinterpret_cast<void*>( offsetof( VertexPNT, position ) ) );
		glEnableVertexAttribArray( 1 );
		glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPNT ), reinterpret_cast<void*>( offsetof( VertexPNT, normal ) ) );
		glEnableVertexAttribArray( 2 );
		glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( VertexPNT ), reinterpret_cast<void*>( offsetof( VertexPNT, texCoord ) ) );

		glBindVertexArray( 0 );
		glBindBuffer( GL_ARRAY_BUFFER, 0 );

		geometry.vertexCount = static_cast<GLsizei>( vertices.size() );
		return geometry;
	}

	void destroy_geometry( SceneGeometry& geometry )
	{
		if( geometry.vbo )
		{
			glDeleteBuffers( 1, &geometry.vbo );
			geometry.vbo = 0;
		}
		if( geometry.vao )
		{
			glDeleteVertexArrays( 1, &geometry.vao );
			geometry.vao = 0;
		}
		geometry.vertexCount = 0;
	}

	LandingPadGeometry load_landingpad_mesh( std::filesystem::path const& objPath )
	{
		auto const resultPath = objPath.lexically_normal();
		auto result = rapidobj::ParseFile( resultPath );
		if( result.error )
			throw Error( "Failed to load '{}': {} at line {}", resultPath.string(), result.error.code.message(), result.error.line_num );
		if( !rapidobj::Triangulate( result ) )
			throw Error( "Triangulation failed for '{}'", resultPath.string() );

		LandingPadGeometry geometry{};

		std::size_t totalIndices = 0;
		for( auto const& shape : result.shapes )
			totalIndices += shape.mesh.indices.size();

		std::vector<VertexPNC> vertices;
		vertices.reserve( totalIndices );

		auto const fetch_position = [&]( int index ) -> Vec3f
		{
			std::size_t const base = static_cast<std::size_t>( index ) * 3;
			return Vec3f{
				result.attributes.positions[base + 0],
				result.attributes.positions[base + 1],
				result.attributes.positions[base + 2]
			};
		};
		auto const fetch_normal = [&]( int index ) -> Vec3f
		{
			std::size_t const base = static_cast<std::size_t>( index ) * 3;
			return Vec3f{
				result.attributes.normals[base + 0],
				result.attributes.normals[base + 1],
				result.attributes.normals[base + 2]
			};
		};
		auto const fetch_color = [&]( int materialIndex ) -> Vec3f
		{
			if( materialIndex >= 0 && static_cast<std::size_t>( materialIndex ) < result.materials.size() )
			{
				auto const& mat = result.materials[static_cast<std::size_t>( materialIndex )];
				return Vec3f{ mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] };
			}
			return Vec3f{ 0.7f, 0.7f, 0.7f };
		};

		for( auto const& shape : result.shapes )
		{
			auto const& mesh = shape.mesh;
			std::size_t indexOffset = 0;
			std::size_t faceIndex = 0;

			for( auto const faceVertices : mesh.num_face_vertices )
			{
				if( faceVertices != 3 )
				{
					indexOffset += faceVertices;
					++faceIndex;
					continue;
				}

				std::array<rapidobj::Index, 3> faceIndices{};
				std::array<Vec3f, 3> positions{};
				std::array<Vec3f, 3> normals{};
				bool hasPerVertexNormals = !result.attributes.normals.empty();

				for( std::size_t v = 0; v < 3; ++v )
				{
					faceIndices[v] = mesh.indices[indexOffset++];
					positions[v] = fetch_position( faceIndices[v].position_index );
					if( hasPerVertexNormals && faceIndices[v].normal_index >= 0 )
						normals[v] = fetch_normal( faceIndices[v].normal_index );
					else
						hasPerVertexNormals = false;
				}

				Vec3f const edgeA = positions[1] - positions[0];
				Vec3f const edgeB = positions[2] - positions[0];
				Vec3f faceNormal = safe_normalize( cross( edgeA, edgeB ) );
				Vec3f const diffuseColor = (!mesh.material_ids.empty() && faceIndex < mesh.material_ids.size())
					? fetch_color( mesh.material_ids[faceIndex] )
					: fetch_color( -1 );

				for( std::size_t v = 0; v < 3; ++v )
				{
					VertexPNC vertex{};
					vertex.position = positions[v];
					vertex.normal = hasPerVertexNormals ? safe_normalize( normals[v], faceNormal ) : faceNormal;
					vertex.color = diffuseColor;
					vertices.emplace_back( vertex );
				}

				++faceIndex;
			}
		}

		if( vertices.empty() )
			throw Error( "OBJ '{}' did not contain triangles", resultPath.string() );

		glGenVertexArrays( 1, &geometry.vao );
		glGenBuffers( 1, &geometry.vbo );

		glBindVertexArray( geometry.vao );
		glBindBuffer( GL_ARRAY_BUFFER, geometry.vbo );
		glBufferData( GL_ARRAY_BUFFER, static_cast<GLsizeiptr>( vertices.size() * sizeof( VertexPNC ) ), vertices.data(), GL_STATIC_DRAW );

		glEnableVertexAttribArray( 0 );
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPNC ), reinterpret_cast<void*>( offsetof( VertexPNC, position ) ) );
		glEnableVertexAttribArray( 1 );
		glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPNC ), reinterpret_cast<void*>( offsetof( VertexPNC, normal ) ) );
		glEnableVertexAttribArray( 2 );
		glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPNC ), reinterpret_cast<void*>( offsetof( VertexPNC, color ) ) );

		glBindVertexArray( 0 );
		glBindBuffer( GL_ARRAY_BUFFER, 0 );

		geometry.vertexCount = static_cast<GLsizei>( vertices.size() );
		return geometry;
	}

	void destroy_geometry( LandingPadGeometry& geometry )
	{
		if( geometry.vbo )
		{
			glDeleteBuffers( 1, &geometry.vbo );
			geometry.vbo = 0;
		}
		if( geometry.vao )
		{
			glDeleteVertexArrays( 1, &geometry.vao );
			geometry.vao = 0;
		}
		geometry.vertexCount = 0;
	}

	GLuint load_texture_2d( std::filesystem::path const& imagePath )
	{
		auto const normalizedPath = imagePath.lexically_normal();
		int width = 0;
		int height = 0;
		int channels = 0;

		stbi_set_flip_vertically_on_load( true );
		stbi_uc* pixels = stbi_load( normalizedPath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha );
		if( !pixels )
		{
			char const* reason = stbi_failure_reason();
			throw Error( "Failed to load texture '{}': {}", normalizedPath.string(), reason ? reason : "unknown error" );
		}

		if( width <= 0 || height <= 0 )
		{
			stbi_image_free( pixels );
			throw Error( "Texture '{}' reported invalid size {}x{}", normalizedPath.string(), width, height );
		}

		GLuint texture = 0;
		glGenTextures( 1, &texture );
		if( texture == 0 )
		{
			stbi_image_free( pixels );
			throw Error( "glGenTextures() failed for '{}'", normalizedPath.string() );
		}

		glBindTexture( GL_TEXTURE_2D, texture );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_SRGB8_ALPHA8,
			width,
			height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			pixels
		);
		glGenerateMipmap( GL_TEXTURE_2D );

		glBindTexture( GL_TEXTURE_2D, 0 );
		stbi_image_free( pixels );
		return texture;
	}

	// === Particle helpers (texture/pool/render) ===
	GLuint create_particle_texture()
	{
		// 程序生成一张带 alpha 的圆形软边纹理
		int const size = 64;
		std::vector<unsigned char> data( size * size * 4 );
		for( int y = 0; y < size; ++y )
		{
			for( int x = 0; x < size; ++x )
			{
				float nx = (x + 0.5f) / size * 2.f - 1.f;
				float ny = (y + 0.5f) / size * 2.f - 1.f;
				float r = std::sqrt( nx * nx + ny * ny );
				float alpha = std::clamp( 1.f - r, 0.f, 1.f );
				alpha = std::pow( alpha, 2.0f );
				std::size_t idx = static_cast<std::size_t>( y * size + x ) * 4;
				data[idx + 0] = 255;
				data[idx + 1] = 255;
				data[idx + 2] = 255;
				data[idx + 3] = static_cast<unsigned char>( alpha * 255 );
			}
		}

		GLuint tex = 0;
		glGenTextures( 1, &tex );
		glBindTexture( GL_TEXTURE_2D, tex );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data() );
		glGenerateMipmap( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, 0 );
		return tex;
	}

	void init_particle_system( ParticleSystem& system )
	{
		constexpr std::size_t kMaxParticles = 4000;
		system.pool.clear();
		system.pool.resize( kMaxParticles );
		system.head = 0;
		system.aliveCount = 0;
		system.emitAccumulator = 0.f;

		glGenVertexArrays( 1, &system.vao );
		glGenBuffers( 1, &system.vbo );

		glBindVertexArray( system.vao );
		glBindBuffer( GL_ARRAY_BUFFER, system.vbo );
		glBufferData( GL_ARRAY_BUFFER, static_cast<GLsizeiptr>( kMaxParticles * sizeof( ParticleGpu ) ), nullptr, GL_DYNAMIC_DRAW );

		// position
		glEnableVertexAttribArray( 0 );
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( ParticleGpu ), reinterpret_cast<void*>( offsetof( ParticleGpu, position ) ) );
		// size + alpha
		glEnableVertexAttribArray( 1 );
		glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( ParticleGpu ), reinterpret_cast<void*>( offsetof( ParticleGpu, size ) ) );

		glBindVertexArray( 0 );
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
	}

	void destroy_particle_system( ParticleSystem& system )
	{
		if( system.vbo )
		{
			glDeleteBuffers( 1, &system.vbo );
			system.vbo = 0;
		}
		if( system.vao )
		{
			glDeleteVertexArrays( 1, &system.vao );
			system.vao = 0;
		}
		if( system.textureId )
		{
			glDeleteTextures( 1, &system.textureId );
			system.textureId = 0;
		}
		system.pool.clear();
		system.head = 0;
		system.aliveCount = 0;
		system.emitAccumulator = 0.f;
	}

	static float rand01()
	{
		return static_cast<float>( std::rand() ) / static_cast<float>( RAND_MAX );
	}

	void emit_particles( ParticleSystem& system, Vec3f const& emitterPos, Vec3f const& emitterDir, float rate, float dt )
	{
		system.emitAccumulator += rate * dt;
		int toEmit = static_cast<int>( std::floor( system.emitAccumulator ) );
		system.emitAccumulator -= toEmit;
		if( toEmit <= 0 )
			return;

		Vec3f dir = safe_normalize( emitterDir, Vec3f{ 0.f, 0.f, -1.f } );

		for( int i = 0; i < toEmit; ++i )
		{
			std::size_t idx = system.head;
			system.head = (system.head + 1) % system.pool.size();
			Particle& p = system.pool[idx];
			p.alive = true;
			p.age = 0.f;
			p.lifetime = 0.6f + rand01() * 0.6f;
			p.size = 0.8f + rand01() * 0.6f;

			Vec3f tangent = safe_normalize( cross( dir, Vec3f{ 0.f, 1.f, 0.f } ), Vec3f{ 1.f, 0.f, 0.f } );
			Vec3f bitangent = cross( tangent, dir );
			float spread = 0.4f;
			float u = rand01() * 2.f * kPi;
			float r = rand01() * spread;
			Vec3f jitter = tangent * ( std::cos( u ) * r ) + bitangent * ( std::sin( u ) * r );

			float speed = 25.f + rand01() * 10.f;
			p.velocity = ( dir + jitter * 0.2f ) * speed;
			p.position = emitterPos + dir * 0.2f;
		}
	}

	void update_particles( ParticleSystem& system, float dt )
	{
		if( dt <= 0.f || system.pool.empty() )
			return;

		std::size_t alive = 0;
		for( auto& p : system.pool )
		{
			if( !p.alive )
				continue;
			p.age += dt;
			if( p.age >= p.lifetime )
			{
				p.alive = false;
				continue;
			}
			p.position += p.velocity * dt;
			++alive;
		}
		system.aliveCount = alive;
	}

	void upload_particles( ParticleSystem& system )
	{
		if( system.vbo == 0 )
			return;
		std::vector<ParticleGpu> gpuData;
		gpuData.reserve( system.aliveCount );
		for( auto const& p : system.pool )
		{
			if( !p.alive )
				continue;
			float alpha = 1.f - ( p.age / p.lifetime );
			ParticleGpu g{};
			g.position = p.position;
			g.size = p.size;
			g.alpha = alpha;
			gpuData.emplace_back( g );
		}
		system.aliveCount = gpuData.size();

		glBindBuffer( GL_ARRAY_BUFFER, system.vbo );
		glBufferSubData( GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>( gpuData.size() * sizeof( ParticleGpu ) ), gpuData.data() );
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
	}

	void render_particles( ParticlePipeline const& pipeline, ParticleSystem const& system, Mat44f const& view, Mat44f const& proj, ViewportRect const& viewport, float fovRadians )
	{
		if( system.aliveCount == 0 || system.vao == 0 )
			return;

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE );
		glDepthMask( GL_FALSE );
		glEnable( GL_PROGRAM_POINT_SIZE );

		glUseProgram( pipeline.program->programId() );

		auto const viewGl = to_gl_matrix( view );
		auto const projGl = to_gl_matrix( proj );

		glUniformMatrix4fv( pipeline.uView, 1, GL_FALSE, viewGl.data() );
		glUniformMatrix4fv( pipeline.uProj, 1, GL_FALSE, projGl.data() );
		glUniform1f( pipeline.uViewportHeight, static_cast<float>( std::max<GLsizei>( 1, viewport.height ) ) );
		glUniform1f( pipeline.uTanHalfFov, std::tan( fovRadians * 0.5f ) );
		glUniform3f( pipeline.uColor, 1.0f, 0.8f, 0.5f );
		glUniform1i( pipeline.uTexture, 0 );

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, system.textureId );

		glBindVertexArray( system.vao );
		glDrawArrays( GL_POINTS, 0, static_cast<GLsizei>( system.aliveCount ) );
		glBindVertexArray( 0 );

		glBindTexture( GL_TEXTURE_2D, 0 );
		glDepthMask( GL_TRUE );
		glDisable( GL_BLEND );
	}

	Vec3f compute_forward_vector( Camera const& camera )
	{
		float const cosPitch = std::cos( camera.pitch );
		return safe_normalize( Vec3f{
			std::cos( camera.yaw ) * cosPitch,
			std::sin( camera.pitch ),
			std::sin( camera.yaw ) * cosPitch
		} );
	}

	Mat44f make_view_matrix( Camera const& camera, Vec3f const& worldUp )
	{
		Vec3f const forward = compute_forward_vector( camera );
		Vec3f const right = safe_normalize( cross( forward, worldUp ), Vec3f{ 1.f, 0.f, 0.f } );
		Vec3f const up = cross( right, forward );

		Mat44f view = kIdentity44f;
		view[0,0] = right.x;
		view[0,1] = right.y;
		view[0,2] = right.z;
		view[0,3] = -dot( right, camera.position );

		view[1,0] = up.x;
		view[1,1] = up.y;
		view[1,2] = up.z;
		view[1,3] = -dot( up, camera.position );

		view[2,0] = -forward.x;
		view[2,1] = -forward.y;
		view[2,2] = -forward.z;
		view[2,3] = dot( forward, camera.position );

		return view;
	}

	void update_projection( AppState& app )
	{
		float const aspect = std::max( 1, static_cast<int>( app.framebufferWidth ) ) / float( std::max( 1, static_cast<int>( app.framebufferHeight ) ) );
		app.projection = make_perspective_projection( app.fovRadians, aspect, app.nearPlane, app.farPlane );
	}

	void update_camera( AppState& app, float deltaSeconds )
	{
		if( deltaSeconds <= 0.f )
			return;

		Vec3f const forward = compute_forward_vector( app.camera );
		Vec3f const right = safe_normalize( cross( forward, app.worldUp ), Vec3f{ 1.f, 0.f, 0.f } );
		Vec3f const up = app.worldUp;

		Vec3f movement{ 0.f, 0.f, 0.f };
		if( app.input.forward ) movement += forward;
		if( app.input.backward ) movement -= forward;
		if( app.input.right ) movement += right;
		if( app.input.left ) movement -= right;
		if( app.input.up ) movement += up;
		if( app.input.down ) movement -= up;

		if( length( movement ) > 0.f )
			movement = safe_normalize( movement );

		float speed = app.baseSpeed;
		if( app.input.fast )
			speed *= app.fastMultiplier;
		if( app.input.slow )
			speed *= app.slowMultiplier;

		app.camera.position += movement * ( speed * deltaSeconds );
	}

	std::array<float,16> to_gl_matrix( Mat44f const& mat )
	{
		std::array<float,16> glMat{};
		for( std::size_t row = 0; row < 4; ++row )
		{
			for( std::size_t col = 0; col < 4; ++col )
				glMat[col * 4 + row] = mat[row, col];
		}
		return glMat;
	}

	GLFWCleanupHelper::~GLFWCleanupHelper()
	{
		glfwTerminate();
	}

	GLFWWindowDeleter::~GLFWWindowDeleter()
	{
		if( window )
			glfwDestroyWindow( window );
	}
}

namespace task5
{
    struct Task5VertexPNC
    {
        Vec3f position;
        Vec3f normal;
        Vec3f color;
    };

    void append_box(
        std::vector<Task5VertexPNC>& vertices,
        Vec3f const& center,
        Vec3f const& halfSize,
        Vec3f const& color
    )
    {
        // define 8 corners of the box
        Vec3f p000 = center + Vec3f{ -halfSize.x, -halfSize.y, -halfSize.z };
        Vec3f p001 = center + Vec3f{ -halfSize.x, -halfSize.y,  halfSize.z };
        Vec3f p010 = center + Vec3f{ -halfSize.x,  halfSize.y, -halfSize.z };
        Vec3f p011 = center + Vec3f{ -halfSize.x,  halfSize.y,  halfSize.z };
        Vec3f p100 = center + Vec3f{  halfSize.x, -halfSize.y, -halfSize.z };
        Vec3f p101 = center + Vec3f{  halfSize.x, -halfSize.y,  halfSize.z };
        Vec3f p110 = center + Vec3f{  halfSize.x,  halfSize.y, -halfSize.z };
        Vec3f p111 = center + Vec3f{  halfSize.x,  halfSize.y,  halfSize.z };

        auto emit_tri = [&](Vec3f const& a, Vec3f const& b, Vec3f const& c)
        {
            Vec3f n = safe_normalize(cross(b - a, c - a));
            vertices.push_back({ a, n, color });
            vertices.push_back({ b, n, color });
            vertices.push_back({ c, n, color });
        };

        // six faces, two triangles each
        emit_tri(p100, p110, p111); emit_tri(p100, p111, p101); // +X
        emit_tri(p000, p011, p010); emit_tri(p000, p001, p011); // -X
        emit_tri(p010, p111, p110); emit_tri(p010, p011, p111); // +Y
        emit_tri(p000, p101, p001); emit_tri(p000, p100, p101); // -Y
        emit_tri(p001, p101, p111); emit_tri(p001, p111, p011); // +Z
        emit_tri(p000, p110, p100); emit_tri(p000, p010, p110); // -Z
    }

    VehicleGeometry create_vehicle_geometry()
    {
        VehicleGeometry geom{};
        std::vector<Task5VertexPNC> verts;

        verts.reserve(2000);

		float const scale = 0.2f;
		// Vehicle 
        // Body
        append_box(
            verts,
            Vec3f{0.f, 2.5f, 0.f} * scale,
            Vec3f{0.5f, 2.5f, 0.5f} * scale,
            Vec3f{0.85f, 0.85f, 0.95f}
        );

        // bottom
        append_box(
            verts,
            Vec3f{0.f, 0.5f, 0.f} * scale,
            Vec3f{0.7f, 0.5f, 0.7f} * scale,
            Vec3f{0.7f, 0.7f, 0.8f}
        );

        // Wings
		append_box(
			verts,
			Vec3f{ 0.9f, 0.f, 0.f } * scale,
			Vec3f{0.3f, 0.7f, 0.05f} * scale,
			Vec3f{1.f,0.2f,0.2f}
		);
		append_box(
			verts,
			Vec3f{-0.9f, 0.f, 0.f } * scale,
			Vec3f{0.3f, 0.7f, 0.05f} * scale,
			Vec3f{1.f,0.2f,0.2f}
		);
		append_box(
			verts,
			Vec3f{0.f, 0.f,  0.9f} * scale,
			Vec3f{0.05f,0.7f,0.3f} * scale,
			Vec3f{1.f,0.2f,0.2f}
		);
		append_box(
			verts,
			Vec3f{0.f, 0.f, -0.9f} * scale,
			Vec3f{0.05f,0.7f,0.3f} * scale,
			Vec3f{1.f,0.2f,0.2f}
		);

        // Cockpit
        append_box(
			verts,
			Vec3f{0.f, 5.2f, 0.f} * scale,
			Vec3f{0.3f, 0.3f, 0.3f} * scale,
			Vec3f{0.9f, 0.9f, 1.f}
		);

		// create VAO / VBO
        glGenVertexArrays(1, &geom.vao);
        glGenBuffers(1, &geom.vbo);

        glBindVertexArray(geom.vao);
        glBindBuffer(GL_ARRAY_BUFFER, geom.vbo);

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(verts.size() * sizeof(Task5VertexPNC)),
            verts.data(),
            GL_STATIC_DRAW
        );

        // layout(location = 0) position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE,
            sizeof(Task5VertexPNC),
            reinterpret_cast<void*>(offsetof(Task5VertexPNC, position))
        );

        // layout(location = 1) normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE,
            sizeof(Task5VertexPNC),
            reinterpret_cast<void*>(offsetof(Task5VertexPNC, normal))
        );

        // layout(location = 2) color
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(
            2, 3, GL_FLOAT, GL_FALSE,
            sizeof(Task5VertexPNC),
            reinterpret_cast<void*>(offsetof(Task5VertexPNC, color))
        );

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        geom.vertexCount = static_cast<GLsizei>(verts.size());
        return geom;
    }

    void destroy_geometry(VehicleGeometry& g)
    {
        if (g.vbo)
        {
            glDeleteBuffers(1, &g.vbo);
            g.vbo = 0;
        }
        if (g.vao)
        {
            glDeleteVertexArrays(1, &g.vao);
            g.vao = 0;
        }
        g.vertexCount = 0;
    }

    void render_vehicle(
        VehicleGeometry const& g,
        Mat44f const& modelMatrix,
        GLint uModel
    )
    {
        if (g.vao == 0 || g.vertexCount == 0)
            return;

        auto modelGl = to_gl_matrix(modelMatrix);
        glUniformMatrix4fv(uModel, 1, GL_FALSE, modelGl.data());

        glBindVertexArray(g.vao);
        glDrawArrays(GL_TRIANGLES, 0, g.vertexCount);
        glBindVertexArray(0);
    }

}

namespace task6 
{
	void upload_lights_to_shader(GLuint programId,
                             LightState const& lightState,
                             std::array<PointLight,3> const& lights,
                             Vec3f const& dirLightDir)
	{
		glUniform1i(glGetUniformLocation(programId, "uDirLightEnabled"),
					lightState.dirLightEnabled ? 1 : 0);

		glUniform3f(glGetUniformLocation(programId, "uLightDir"),
					dirLightDir.x, dirLightDir.y, dirLightDir.z);

		// point lights
		for (int i = 0; i < 3; ++i)
		{
			std::string base = "uPointLights[" + std::to_string(i) + "]";

			glUniform1i(glGetUniformLocation(programId,
					(base + ".enabled").c_str()),
					lightState.pointEnabled[i] ? 1 : 0);

			glUniform3f(glGetUniformLocation(programId,
					(base + ".position").c_str()),
					lights[i].position.x,
					lights[i].position.y,
					lights[i].position.z);

			glUniform3f(glGetUniformLocation(programId,
					(base + ".color").c_str()),
					lights[i].color.x,
					lights[i].color.y,
					lights[i].color.z);
		}
	}
}

namespace task7
{
    static Vec3f extract_translation(Mat44f const& m)
    {
        return Vec3f{ m[0,3], m[1,3], m[2,3] };
    }


    void initialise(AnimationState& anim,
                    Mat44f const& baseModel,
                    std::array<task6::PointLight,3>& lights)
    {
        anim.baseModel   = baseModel;
        anim.currentModel = baseModel;

        anim.startPos = extract_translation(baseModel);
        anim.lastPos  = anim.startPos;

        for (int i = 0; i < 3; ++i)
        {
            anim.lightOffsets[i] = lights[i].position - anim.startPos;
        }

        anim.active = false;
        anim.paused = false;
        anim.time   = 0.f;
    }

    void toggle_play(AnimationState& anim)
    {
        if (!anim.active)
        {
            anim.active = true;
            anim.paused = false;
            anim.time   = 0.f;
            anim.lastPos = anim.startPos;
        }
        else
        {
            anim.paused = !anim.paused;
        }
    }

    void reset(AnimationState& anim)
    {
        anim.active = false;
        anim.paused = false;
        anim.time   = 0.f;
        anim.currentModel = anim.baseModel;
        anim.lastPos      = anim.startPos;
    }

    void update(AnimationState& anim,
            float deltaSeconds,
            Mat44f& vehicleModelMatrix,
            std::array<task6::PointLight,3>& lights)
	{
		if (!anim.active || deltaSeconds <= 0.f)
		{
			anim.currentModel = anim.baseModel;
			vehicleModelMatrix = anim.currentModel;

			Vec3f pos = extract_translation(vehicleModelMatrix);
			for (int i = 0; i < 3; ++i)
				lights[i].position = pos + anim.lightOffsets[i];

			return;
		}

		if (anim.paused)
		{
			vehicleModelMatrix = anim.currentModel;
			Vec3f pos = extract_translation(vehicleModelMatrix);
			for (int i = 0; i < 3; ++i)
				lights[i].position = pos + anim.lightOffsets[i];
			return;
		}

		anim.time += deltaSeconds;
		float const totalDuration = 8.f;
		float u = std::clamp(anim.time / totalDuration, 0.f, 1.f);

		float s2 = u * u;
		float s3 = s2 * u;

		float xRange = 60.f;
		float yRange = 40.f;
		float zRange = 20.f;

		Vec3f offset{
			xRange * s3,
			yRange * s2,
			zRange * s3
		};

		Vec3f currentPos = anim.startPos + offset;

		Vec3f velocity = (currentPos - anim.lastPos) / deltaSeconds;
		anim.lastPos = currentPos;

		Vec3f forward = velocity;
		if (length(forward) < 1e-4f)
			forward = Vec3f{0.f, 1.f, 0.f};
		else
			forward = forward / length(forward);

		Vec3f worldSide{0.f, 0.f, 1.f};
		if (std::abs(dot(worldSide, forward)) > 0.9f)
			worldSide = Vec3f{1.f, 0.f, 0.f};

		Vec3f right = safe_normalize(cross(worldSide, forward), Vec3f{1.f,0.f,0.f});
		Vec3f up    = cross(forward, right);

		Mat44f R = kIdentity44f;
		R[0,0] = right.x;   R[0,1] = right.y;   R[0,2] = right.z;
		R[1,0] = forward.x; R[1,1] = forward.y; R[1,2] = forward.z;
		R[2,0] = up.x;      R[2,1] = up.y;      R[2,2] = up.z;

		Mat44f T = make_translation(currentPos);

		anim.currentModel = T * R;
		vehicleModelMatrix = anim.currentModel;

		for (int i = 0; i < 3; ++i)
		{
			lights[i].position = currentPos + anim.lightOffsets[i];
		}

		if (u >= 1.f)
		{
			anim.active = false;
			anim.paused = false;
		}
	}
}

namespace task8
{
    CameraMode next_mode( CameraMode mode )
    {
        switch( mode )
        {
            case CameraMode::Free:   return CameraMode::Follow;
            case CameraMode::Follow: return CameraMode::Ground;
            case CameraMode::Ground: return CameraMode::Free;
        }
        return CameraMode::Free;
    }

    void TrackingCamera::next_mode()
    {
        mode = task8::next_mode( mode );
    }

    static Vec3f rocket_world_pos(Mat44f const& model)
    {
        return Vec3f{ model[0,3], model[1,3], model[2,3] };
    }

    Mat44f make_follow_camera(Vec3f const& rocketPos, Mat44f const& rocketModel, Vec3f offset)
    {
        Vec3f worldOffset =
        {
            rocketModel[0,0] * offset.x + rocketModel[0,1] * offset.y + rocketModel[0,2] * offset.z,
            rocketModel[1,0] * offset.x + rocketModel[1,1] * offset.y + rocketModel[1,2] * offset.z,
            rocketModel[2,0] * offset.x + rocketModel[2,1] * offset.y + rocketModel[2,2] * offset.z
        };

        Vec3f camPos = rocketPos + worldOffset;
        Vec3f forward = safe_normalize(rocketPos - camPos);
        Vec3f right = safe_normalize(cross(forward, Vec3f{0.f,1.f,0.f}), Vec3f{1.f,0.f,0.f});
        Vec3f up = cross(right, forward);

        Mat44f V = kIdentity44f;
        V[0,0] = right.x; V[0,1] = right.y; V[0,2] = right.z;
        V[1,0] = up.x;    V[1,1] = up.y;    V[1,2] = up.z;
        V[2,0] = -forward.x; V[2,1] = -forward.y; V[2,2] = -forward.z;

        V[0,3] = -dot(right, camPos);
        V[1,3] = -dot(up, camPos);
        V[2,3] = dot(forward, camPos);
        return V;
    }

    Mat44f make_ground_camera(Vec3f const& rocketPos, Vec3f const& groundPos)
    {
        Vec3f camPos = groundPos;
        Vec3f forward = safe_normalize(rocketPos - camPos);
        Vec3f right = safe_normalize(cross(forward, Vec3f{0.f,1.f,0.f}), Vec3f{1.f,0.f,0.f});
        Vec3f up = cross(right, forward);

        Mat44f V = kIdentity44f;
        V[0,0] = right.x; V[0,1] = right.y; V[0,2] = right.z;
        V[1,0] = up.x;    V[1,1] = up.y;    V[1,2] = up.z;
        V[2,0] = -forward.x; V[2,1] = -forward.y; V[2,2] = -forward.z;

        V[0,3] = -dot(right, camPos);
        V[1,3] = -dot(up, camPos);
        V[2,3] = dot(forward, camPos);
        return V;
    }

    Mat44f compute_camera_view(
        TrackingCamera const& cam,
        Camera const& freeCam,
        Mat44f const& rocketModel
    )
    {
        Vec3f rocketPos = rocket_world_pos(rocketModel);

        switch (cam.mode)
        {
            case CameraMode::Free:
                return make_view_matrix(freeCam, Vec3f{0.f,1.f,0.f});

            case CameraMode::Follow:
                return make_follow_camera(rocketPos, rocketModel, cam.followOffset);

            case CameraMode::Ground:
                return make_ground_camera(rocketPos, cam.groundPos);
        }

        return make_view_matrix(freeCam, Vec3f{0.f,1.f,0.f});
    }
}

