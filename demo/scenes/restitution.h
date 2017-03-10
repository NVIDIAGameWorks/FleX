


class Restitution : public Scene
{
public:

	Restitution(const char* name) : Scene(name) {}

	void Initialize()
	{
		float radius = 0.05f;
		CreateParticleGrid(Vec3(0.0f, 1.0f, 0.0f), 1, 1, 1, radius, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide), 0.0f);

		g_params.radius = radius;
		g_params.dynamicFriction = 0.025f;
		g_params.dissipation = 0.0f;
		g_params.restitution = 1.0;
		g_params.numIterations = 4;

		g_numSubsteps = 4;

		// draw options		
		g_drawPoints = true;
	}
	
};
