
class Viscosity : public Scene
{
public:

	Viscosity(const char* name, float viscosity = 1.0f, float dissipation = 0.0f) : Scene(name), viscosity(viscosity), dissipation(dissipation) {}

	virtual void Initialize()
	{
		float radius = 0.1f;
		float restDistance = radius*0.5f;

		g_solverDesc.featureMode = eNvFlexFeatureModeSimpleFluids;

		g_params.radius = radius;

		g_params.numIterations = 3;
		g_params.vorticityConfinement = 0.0f;
		g_params.fluidRestDistance = restDistance;
		g_params.smoothing = 0.35f;
		g_params.relaxationFactor = 1.f;
		g_params.restitution = 0.0f;
		g_params.collisionDistance = 0.00125f;
		g_params.shapeCollisionMargin = g_params.collisionDistance*0.25f;
		g_params.dissipation = dissipation;

		g_params.gravity[1] *= 2.0f;

		g_fluidColor = Vec4(1.0f, 1.0f, 1.0f, 0.0f);
		g_meshColor = Vec3(0.7f, 0.8f, 0.9f)*0.7f;

		g_params.dynamicFriction = 1.0f;
		g_params.staticFriction = 0.0f;
		g_params.viscosity = 20.0f + 20.0f*viscosity;
		g_params.adhesion = 0.1f*viscosity;
		g_params.cohesion = 0.05f*viscosity;
		g_params.surfaceTension = 0.0f;

		const float shapeSize = 2.0f;
		const Vec3 shapeLower = Vec3(-shapeSize*0.5f, 0.0f, -shapeSize*0.5f);
		const Vec3 shapeUpper = shapeLower + Vec3(shapeSize);
		const Vec3 shapeCenter = (shapeLower + shapeUpper)*0.5f;

		NvFlexDistanceFieldId sdf = CreateSDF(GetFilePathByPlatform("../../data/bunny.ply").c_str(), 128);
		AddSDF(sdf, shapeLower, Quat(), shapeSize);

		float emitterSize = 1.f;

		Emitter e;
		e.mEnabled = true;
		e.mWidth = int(emitterSize / restDistance);
		e.mPos = Vec3(shapeCenter.x - 0.2f, shapeUpper.y + 0.75f, shapeCenter.z);
		e.mDir = Vec3(0.0f, -1.0f, 0.0f);
		e.mRight = Vec3(1.0f, 0.0f, 0.0f);
		e.mSpeed = (restDistance*2.f / g_dt);

		g_sceneUpper.z = 5.0f;

		g_emitters.push_back(e);

		g_numExtraParticles = 64 * 1024;

		g_lightDistance *= 2.5f;

		// draw options		
		g_drawEllipsoids = true;

		g_emit = true;
		g_pause = false;
	}

	virtual void DoGui()
	{
		imguiSlider("Emitter Pos", &g_emitters.back().mPos.x, -1.0f, 1.0f, 0.001f);
	}

	float viscosity;
	float dissipation;
};