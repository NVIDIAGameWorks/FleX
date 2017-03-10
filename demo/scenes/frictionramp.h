

class FrictionRamp : public Scene
{
public:

	FrictionRamp(const char* name) : Scene(name) {}

	virtual void Initialize()
	{
		float radius = 0.1f;

		g_params.radius = radius;
		g_params.dynamicFriction = 0.35f;
		g_params.dissipation = 0.0f;
		g_params.numIterations = 8;
		g_params.viscosity = 0.0f;
		g_params.drag = 0.0f;
		g_params.lift = 0.0f;
		g_params.collisionDistance = radius*0.5f;

		g_windStrength = 0.0f;

		g_numSubsteps = 1;

		// draw options
		g_drawPoints = false;
		g_wireframe = false;
		g_drawSprings = false;

		for (int i = 0; i < 3; ++i)
		{
			// box
			CreateParticleShape(GetFilePathByPlatform("../../data/box.ply").c_str(), Vec3(0.0f, 3.5f, -i*2.0f), 0.5f, 0.0f, radius, 0.0f, 1.0f, true, 1.0f, NvFlexMakePhase(i, 0), true, 0.0f);

			// ramp
			AddBox(Vec3(5.0f, 0.25f, 1.f), Vec3(3.0f, 1.0f, -i*2.0f), QuatFromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), DegToRad(-11.25f*(i + 1))));
		}
	}
};