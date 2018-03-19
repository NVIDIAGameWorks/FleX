
class Adhesion : public Scene
{
public:

	Adhesion(const char* name) : Scene(name) {}

	virtual void Initialize()
	{
		float radius = 0.1f;

		g_params.radius = radius;

		g_params.numIterations = 3;
		g_params.vorticityConfinement = 0.0f;
		g_params.fluidRestDistance = g_params.radius*0.55f;
		g_params.smoothing = 0.5f;
		g_params.relaxationFactor = 1.f;
		g_params.restitution = 0.0f;
		g_params.collisionDistance = 0.01f;

		g_fluidColor = Vec4(0.0f, 0.8f, 0.2f, 1.0f);
		//g_fluidColor = Vec4(0.7f, 0.6f, 0.6f, 0.2f);

		g_params.dynamicFriction = 0.5f;
		g_params.viscosity = 50.0f;
		g_params.adhesion = 0.5f;
		g_params.cohesion = 0.08f;
		g_params.surfaceTension = 0.0f;

		g_numExtraParticles = 64 * 1024;

		AddBox(Vec3(1.0f, 1.5f, 0.1f), Vec3(0.0f, 1.5f, 0.0f));
		AddBox(Vec3(1.0f, 0.1f, 6.0f), Vec3(-1.0f, 3.0f, 0.0f));

		g_emitters[0].mEnabled = true;
		g_emitters[0].mSpeed = (g_params.fluidRestDistance*2.f / g_dt);

		g_params.numPlanes = 3;

		// draw options		
		g_drawEllipsoids = true;

		g_pause = false;
	}

	bool mViscous;
};
