

class Lighthouse : public Scene
{
public:

	Lighthouse(const char* name) : Scene(name) {}

	virtual void Initialize()
	{
		float radius = 0.15f;
		float restDistance = radius*0.6f;

		NvFlexDistanceFieldId sdf = CreateSDF(GetFilePathByPlatform("../../data/lighthouse.ply").c_str(), 128);
		AddSDF(sdf, Vec3(4.0f, 0.0f, 0.0f), Quat(), 10.f);

		CreateParticleGrid(Vec3(0.0f, 0.3f, 0.0f), 48, 48, 128, restDistance, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid), 0.005f);

		g_sceneLower = 0.0f;
		g_sceneUpper = Vec3(12, 0.0f, 0.0f);

		g_numSubsteps = 2;

		g_params.radius = radius;
		g_params.dynamicFriction = 0.f;
		g_params.fluid = true;
		g_params.viscosity = 0.01f;
		g_params.numIterations = 3;
		g_params.vorticityConfinement = 50.0f;
		g_params.anisotropyScale = 20.0f;
		g_params.fluidRestDistance = restDistance;
		g_params.gravity[1] *= 0.5f;
		g_params.cohesion *= 0.5f;

		g_fluidColor = Vec4(0.413f, 0.725f, 0.85f, 0.7f);

		g_maxDiffuseParticles = 1024 * 1024;
		g_diffuseScale = 0.3f;
		g_diffuseShadow = true;
		g_diffuseColor = 1.0f;
		g_diffuseMotionScale = 1.0f;
		g_params.diffuseThreshold *= 10.f;
		g_params.diffuseBallistic = 4;
		g_params.diffuseBuoyancy = 2.0f;
		g_params.diffuseDrag = 1.0f;

		g_params.numPlanes = 5;

		g_waveFrequency = 1.2f;
		g_waveAmplitude = 2.2f;
		g_waveFloorTilt = 0.1f;


		// draw options		
		g_drawPoints = false;
		g_drawEllipsoids = true;
		g_drawDiffuse = true;
	}
};